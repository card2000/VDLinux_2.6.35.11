/*
 * linux/kernel/irq/proc.c
 *
 * Copyright (C) 1992, 1998-2004 Linus Torvalds, Ingo Molnar
 *
 * This file contains the /proc/irq/ handling code.
 */

#include <linux/irq.h>
#include <linux/gfp.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>

#ifdef CONFIG_MSTAR_CHIP
#include <linux/poll.h>
#include "chip_int.h"
#endif  /* End of CONFIG_MSTAR_CHIP */

#include "internals.h"
static struct proc_dir_entry *root_irq_dir;

#ifdef CONFIG_SMP

static int irq_affinity_proc_show(struct seq_file *m, void *v)
{
	struct irq_desc *desc = irq_to_desc((long)m->private);
	const struct cpumask *mask = desc->affinity;

#ifdef CONFIG_GENERIC_PENDING_IRQ
	if (desc->status & IRQ_MOVE_PENDING)
		mask = desc->pending_mask;
#endif
	seq_cpumask(m, mask);
	seq_putc(m, '\n');
	return 0;
}

static int irq_affinity_hint_proc_show(struct seq_file *m, void *v)
{
	struct irq_desc *desc = irq_to_desc((long)m->private);
	unsigned long flags;
	cpumask_var_t mask;

	if (!zalloc_cpumask_var(&mask, GFP_KERNEL))
		return -ENOMEM;

	raw_spin_lock_irqsave(&desc->lock, flags);
	if (desc->affinity_hint)
		cpumask_copy(mask, desc->affinity_hint);
	raw_spin_unlock_irqrestore(&desc->lock, flags);

	seq_cpumask(m, mask);
	seq_putc(m, '\n');
	free_cpumask_var(mask);

	return 0;
}

#ifndef is_affinity_mask_valid
#define is_affinity_mask_valid(val) 1
#endif

int no_irq_affinity;
static ssize_t irq_affinity_proc_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *pos)
{
	unsigned int irq = (int)(long)PDE(file->f_path.dentry->d_inode)->data;
	cpumask_var_t new_value;
	int err;

	if (!irq_to_desc(irq)->chip->set_affinity || no_irq_affinity ||
	    irq_balancing_disabled(irq))
		return -EIO;

	if (!alloc_cpumask_var(&new_value, GFP_KERNEL))
		return -ENOMEM;

	err = cpumask_parse_user(buffer, count, new_value);
	if (err)
		goto free_cpumask;

	if (!is_affinity_mask_valid(new_value)) {
		err = -EINVAL;
		goto free_cpumask;
	}

	/*
	 * Do not allow disabling IRQs completely - it's a too easy
	 * way to make the system unusable accidentally :-) At least
	 * one online CPU still has to be targeted.
	 */
	if (!cpumask_intersects(new_value, cpu_online_mask)) {
		/* Special case for empty set - allow the architecture
		   code to set default SMP affinity. */
		err = irq_select_affinity_usr(irq) ? -EINVAL : count;
	} else {
		irq_set_affinity(irq, new_value);
		err = count;
	}

free_cpumask:
	free_cpumask_var(new_value);
	return err;
}

static int irq_affinity_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, irq_affinity_proc_show, PDE(inode)->data);
}

static int irq_affinity_hint_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, irq_affinity_hint_proc_show, PDE(inode)->data);
}

static const struct file_operations irq_affinity_proc_fops = {
	.open		= irq_affinity_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= irq_affinity_proc_write,
};

static const struct file_operations irq_affinity_hint_proc_fops = {
	.open		= irq_affinity_hint_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int default_affinity_show(struct seq_file *m, void *v)
{
	seq_cpumask(m, irq_default_affinity);
	seq_putc(m, '\n');
	return 0;
}

static ssize_t default_affinity_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	cpumask_var_t new_value;
	int err;

	if (!alloc_cpumask_var(&new_value, GFP_KERNEL))
		return -ENOMEM;

	err = cpumask_parse_user(buffer, count, new_value);
	if (err)
		goto out;

	if (!is_affinity_mask_valid(new_value)) {
		err = -EINVAL;
		goto out;
	}

	/*
	 * Do not allow disabling IRQs completely - it's a too easy
	 * way to make the system unusable accidentally :-) At least
	 * one online CPU still has to be targeted.
	 */
	if (!cpumask_intersects(new_value, cpu_online_mask)) {
		err = -EINVAL;
		goto out;
	}

	cpumask_copy(irq_default_affinity, new_value);
	err = count;

out:
	free_cpumask_var(new_value);
	return err;
}

static int default_affinity_open(struct inode *inode, struct file *file)
{
	return single_open(file, default_affinity_show, PDE(inode)->data);
}

static const struct file_operations default_affinity_proc_fops = {
	.open		= default_affinity_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= default_affinity_write,
};

static int irq_node_proc_show(struct seq_file *m, void *v)
{
	struct irq_desc *desc = irq_to_desc((long) m->private);

	seq_printf(m, "%d\n", desc->node);
	return 0;
}

static int irq_node_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, irq_node_proc_show, PDE(inode)->data);
}

static const struct file_operations irq_node_proc_fops = {
	.open		= irq_node_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static int irq_spurious_proc_show(struct seq_file *m, void *v)
{
	struct irq_desc *desc = irq_to_desc((long) m->private);

	seq_printf(m, "count %u\n" "unhandled %u\n" "last_unhandled %u ms\n",
		   desc->irq_count, desc->irqs_unhandled,
		   jiffies_to_msecs(desc->last_unhandled));
	return 0;
}

static int irq_spurious_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, irq_spurious_proc_show, PDE(inode)->data);
}

static const struct file_operations irq_spurious_proc_fops = {
	.open		= irq_spurious_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

#ifdef CONFIG_MSTAR_CHIP
struct irq_proc {
	int irq;
	wait_queue_head_t q;
	atomic_t count;
	char devname[sizeof ((struct task_struct *) 0)->comm];
};

irqreturn_t irq_proc_irq_handler(int irq, void *vidp)
{
	struct irq_proc *idp = (struct irq_proc *)vidp;

	BUG_ON(idp->irq != irq);
	disable_irq_nosync(irq);
	atomic_inc(&idp->count);
	wake_up(&idp->q);
	return IRQ_HANDLED;
}

/*
 * Signal to userspace an interrupt has occured.
 * Note: no data is ever transferred to/from user space!
 */
ssize_t irq_proc_read(struct file *fp, char *bufp, size_t len, loff_t *where)
{
	struct irq_proc *ip = (struct irq_proc *)fp->private_data;
	irq_desc_t *idp = irq_desc + ip->irq;
	int i;
	int err;

	DEFINE_WAIT(wait);

	if (len < sizeof(int))
		return -EINVAL;

	if ((i = atomic_read(&ip->count)) == 0) {
		if (idp->status & IRQ_DISABLED)
			enable_irq(ip->irq);
		if (fp->f_flags & O_NONBLOCK)
			return -EWOULDBLOCK;
	}

	while (i == 0) {
		prepare_to_wait(&ip->q, &wait, TASK_INTERRUPTIBLE);
		if ((i = atomic_read(&ip->count)) == 0)
			schedule();
		finish_wait(&ip->q, &wait);
		if (signal_pending(current))
		{
			return -ERESTARTSYS;
	    }
	}

	if ((err = copy_to_user(bufp, &i, sizeof i)))
		return err;
	*where += sizeof i;

	atomic_sub(i, &ip->count);
	return sizeof i;
}

ssize_t irq_proc_write(struct file *fp, const char *bufp, size_t len, loff_t *where)
{
	struct irq_proc *ip = (struct irq_proc *)fp->private_data;
	int enable;
	int err;

	if (len < sizeof(int))
		return -EINVAL;

	if ((err = copy_from_user(&enable, bufp, sizeof enable)))
		return err;

	if (enable)
	{
		// if irq has been enabled, does not need eable again 
		if ((irq_desc[ip->irq].status & IRQ_DISABLED)) {
			enable_irq(ip->irq);}
		}
	else
	{
	    disable_irq_nosync(ip->irq);
	}

	*where += sizeof enable;
	return sizeof enable;
}

int irq_proc_open(struct inode *inop, struct file *fp)
{
	struct irq_proc *ip;
	struct proc_dir_entry *ent = PDE(inop);
	int error;
	unsigned long  irqflags;

	ip = kmalloc(sizeof *ip, GFP_KERNEL);
	if (ip == NULL)
		return -ENOMEM;

	memset(ip, 0, sizeof(*ip));
	strcpy(ip->devname, current->comm);
	init_waitqueue_head(&ip->q);
	atomic_set(&ip->count, 0);
	ip->irq = (int)(unsigned long)ent->data;

	if(ip->irq == E_IRQ_DISP)
		irqflags = IRQF_SHARED;
	else
		irqflags = SA_INTERRUPT;

	if ((error = request_irq(ip->irq,
					irq_proc_irq_handler,
					irqflags,
					ip->devname,
					ip)) < 0) {
		kfree(ip);
		printk(KERN_EMERG"[%s][%d] %d\n", __FUNCTION__, __LINE__, error);
		return error;
	}
	fp->private_data = (void *)ip;
	if (!(irq_desc[ip->irq].status & IRQ_DISABLED))
		disable_irq_nosync(ip->irq);
	return 0;
}

int irq_proc_release(struct inode *inop, struct file *fp)
{

	if(fp->private_data != NULL)
	{
	    struct irq_proc *ip = (struct irq_proc *)fp->private_data;
	    free_irq(ip->irq, ip);
	    kfree(ip);
	    fp->private_data = NULL;
	}
	return 0;
}

unsigned int irq_proc_poll(struct file *fp, struct poll_table_struct *wait)
{
    int i;
    struct irq_proc *ip = (struct irq_proc *)fp->private_data;

    poll_wait(fp, &ip->q, wait);

#if 0 //let user mode driver take interrupt enable responsibility
    /* if interrupts disabled and we don't have one to process */
    if (idp->status & IRQ_DISABLED && atomic_read(&ip->count) == 0)
        enable_irq(ip->irq);
#endif

    if ((i = atomic_read(&ip->count)) > 0)
    {
        //atomic_sub(i, &ip->count);//clear counter
        atomic_sub(1, &ip->count);//only take one of it
        return POLLIN | POLLRDNORM; /* readable */
    }
    return 0;
}

long irq_proc_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        case 137://special command to free_irq immediately
        {
            struct irq_proc *ip = (struct irq_proc *)fp->private_data;
            free_irq(ip->irq, ip);
            kfree(ip);
            fp->private_data = NULL;
        }
            break;
        default:
            return -1;
    }
        return 0;
}

struct file_operations irq_proc_file_operations = {
	.read = irq_proc_read,
	.write = irq_proc_write,
	.open = irq_proc_open,
	.release = irq_proc_release,
	.poll = irq_proc_poll,
	.unlocked_ioctl= irq_proc_ioctl,  //Procfs ioctl handlers must use unlocked_ioctl.
};

#endif /* End of CONFIG_MSTAR_CHIP */

#define MAX_NAMELEN 128

static int name_unique(unsigned int irq, struct irqaction *new_action)
{
	struct irq_desc *desc = irq_to_desc(irq);
	struct irqaction *action;
	unsigned long flags;
	int ret = 1;

	raw_spin_lock_irqsave(&desc->lock, flags);
	for (action = desc->action ; action; action = action->next) {
		if ((action != new_action) && action->name &&
				!strcmp(new_action->name, action->name)) {
			ret = 0;
			break;
		}
	}
	raw_spin_unlock_irqrestore(&desc->lock, flags);
	return ret;
}

void register_handler_proc(unsigned int irq, struct irqaction *action)
{
	char name [MAX_NAMELEN];
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc->dir || action->dir || !action->name ||
					!name_unique(irq, action))
		return;

	memset(name, 0, MAX_NAMELEN);
	snprintf(name, MAX_NAMELEN, "%s", action->name);

	/* create /proc/irq/1234/handler/ */
	action->dir = proc_mkdir(name, desc->dir);
}

#undef MAX_NAMELEN

#define MAX_NAMELEN 10

void register_irq_proc(unsigned int irq, struct irq_desc *desc)
{
	char name [MAX_NAMELEN];

#ifdef CONFIG_MSTAR_CHIP
	struct proc_dir_entry *entry;
#endif  /* End of CONFIG_MSTAR_CHIP */

	if (!root_irq_dir || (desc->chip == &no_irq_chip) || desc->dir)
		return;

	memset(name, 0, MAX_NAMELEN);
	sprintf(name, "%d", irq);

	/* create /proc/irq/1234 */
	desc->dir = proc_mkdir(name, root_irq_dir);
	if (!desc->dir)
		return;

#ifdef CONFIG_SMP
	/* create /proc/irq/<irq>/smp_affinity */
	proc_create_data("smp_affinity", 0600, desc->dir,
			 &irq_affinity_proc_fops, (void *)(long)irq);

	/* create /proc/irq/<irq>/affinity_hint */
	proc_create_data("affinity_hint", 0400, desc->dir,
			 &irq_affinity_hint_proc_fops, (void *)(long)irq);

	proc_create_data("node", 0444, desc->dir,
			 &irq_node_proc_fops, (void *)(long)irq);
#endif

	proc_create_data("spurious", 0444, desc->dir,
			 &irq_spurious_proc_fops, (void *)(long)irq);

#ifdef CONFIG_MSTAR_CHIP
	entry = create_proc_entry("irq", 0600, desc->dir);
	if (entry) {
		entry->data = (void *)(long)irq;
		entry->read_proc = NULL;
        entry->write_proc = NULL;
        entry->proc_fops = &irq_proc_file_operations;
	}
#endif  /* End of CONFIG_MSTAR_CHIP */
}

#undef MAX_NAMELEN

void unregister_handler_proc(unsigned int irq, struct irqaction *action)
{
	if (action->dir) {
		struct irq_desc *desc = irq_to_desc(irq);

		remove_proc_entry(action->dir->name, desc->dir);
	}
}

static void register_default_affinity_proc(void)
{
#ifdef CONFIG_SMP
	proc_create("irq/default_smp_affinity", 0600, NULL,
		    &default_affinity_proc_fops);
#endif
}

void init_irq_proc(void)
{
	unsigned int irq;
	struct irq_desc *desc;

	/* create /proc/irq */
	root_irq_dir = proc_mkdir("irq", NULL);
	if (!root_irq_dir)
		return;

	register_default_affinity_proc();

	/*
	 * Create entries for all existing IRQs.
	 */
	for_each_irq_desc(irq, desc) {
		if (!desc)
			continue;

		register_irq_proc(irq, desc);
	}
}

