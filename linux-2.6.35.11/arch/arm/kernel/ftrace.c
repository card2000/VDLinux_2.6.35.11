/*
 * Dynamic function tracing support.
 *
 * Copyright (C) 2008 Abhishek Sagar <sagar.abhishek@gmail.com>
 *
 * For licencing details, see COPYING.
 *
 * Defines low-level handling of mcount calls when the kernel
 * is compiled with the -pg flag. When using dynamic ftrace, the
 * mcount call-sites get patched lazily with NOP till they are
 * enabled. All code mutation routines here take effect atomically.
 */

#include <linux/ftrace.h>
#ifdef CONFIG_KDEBUGD_FTRACE
#include <linux/uaccess.h>
#endif /* CONFIG_KDEBUGD_FTRACE */

#include <asm/cacheflush.h>
#include <asm/ftrace.h>

#ifndef CONFIG_KDEBUGD_FTRACE
#define PC_OFFSET      8
#define BL_OPCODE      0xeb000000
#define BL_OFFSET_MASK 0x00ffffff

static unsigned long bl_insn;
static const unsigned long NOP = 0xe1a00000; /* mov r0, r0 */

unsigned char *ftrace_nop_replace(void)
{
	return (char *)&NOP;
}

/* construct a branch (BL) instruction to addr */
unsigned char *ftrace_call_replace(unsigned long pc, unsigned long addr)
{
	long offset;

	offset = (long)addr - (long)(pc + PC_OFFSET);
	if (unlikely(offset < -33554432 || offset > 33554428)) {
		/* Can't generate branches that far (from ARM ARM). Ftrace
		 * doesn't generate branches outside of kernel text.
		 */
		WARN_ON_ONCE(1);
		return NULL;
	}
	offset = (offset >> 2) & BL_OFFSET_MASK;
	bl_insn = BL_OPCODE | offset;
	return (unsigned char *)&bl_insn;
}

int ftrace_modify_code(unsigned long pc, unsigned char *old_code,
		       unsigned char *new_code)
{
	unsigned long err = 0, replaced = 0, old, new;

	old = *(unsigned long *)old_code;
	new = *(unsigned long *)new_code;

	__asm__ __volatile__ (
		"1:  ldr    %1, [%2]  \n"
		"    cmp    %1, %4    \n"
		"2:  streq  %3, [%2]  \n"
		"    cmpne  %1, %3    \n"
		"    movne  %0, #2    \n"
		"3:\n"

		".pushsection .fixup, \"ax\"\n"
		"4:  mov  %0, #1  \n"
		"    b    3b      \n"
		".popsection\n"

		".pushsection __ex_table, \"a\"\n"
		"    .long 1b, 4b \n"
		"    .long 2b, 4b \n"
		".popsection\n"

		: "=r"(err), "=r"(replaced)
		: "r"(pc), "r"(new), "r"(old), "0"(err), "1"(replaced)
		: "memory");

	if (!err && (replaced == old))
		flush_icache_range(pc, pc + MCOUNT_INSN_SIZE);

	return err;
}

int ftrace_update_ftrace_func(ftrace_func_t func)
{
	int ret;
	unsigned long pc, old;
	unsigned char *new;

	pc = (unsigned long)&ftrace_call;
	memcpy(&old, &ftrace_call, MCOUNT_INSN_SIZE);
	new = ftrace_call_replace(pc, (unsigned long)func);
	ret = ftrace_modify_code(pc, (unsigned char *)&old, new);
	return ret;
}
#else /* CONFIG_KDEBUGD_FTRACE */
#define    NOP     0xe8bd4000  /* pop {lr} */

#ifdef CONFIG_OLD_MCOUNT
#define OLD_MCOUNT_ADDR    ((unsigned long) mcount)
#define OLD_FTRACE_ADDR ((unsigned long) ftrace_caller_old
#define    OLD_NOP     0xe1a00000  /* mov r0, r0 */

static unsigned long ftrace_nop_replace(struct dyn_ftrace *rec)
{
   return rec->arch.old_mcount ? OLD_NOP : NOP;
}

static unsigned long adjust_address(struct dyn_ftrace *rec, unsigned long addr)
{
   if (!rec->arch.old_mcount)
		return addr;

   if (addr == MCOUNT_ADDR)
		addr = OLD_MCOUNT_ADDR;
   else if (addr == FTRACE_ADDR)
		addr = OLD_FTRACE_ADDR;

   return addr;
}
#else
static unsigned long ftrace_nop_replace(struct dyn_ftrace *rec)
{
   return NOP;
}

static unsigned long adjust_address(struct dyn_ftrace *rec, unsigned long addr)
{
	return addr;
}
#endif

/* construct a branch (BL) instruction to addr */
static unsigned long ftrace_gen_branch(unsigned long pc, unsigned long addr,
				       bool link)
{
	unsigned long opcode = 0xea000000;
	long offset;

	if (link)
		opcode |= 1 << 24;

	offset = (long)addr - (long)(pc + 8);
	if (unlikely(offset < -33554432 || offset > 33554428)) {
		/* Can't generate branches that far (from ARM ARM). Ftrace
		 * doesn't generate branches outside of kernel text.
		 */
		WARN_ON_ONCE(1);
		return 0;
	}
	offset = (offset >> 2) & 0x00ffffff;
	return opcode | offset;
}

static unsigned long ftrace_call_replace(unsigned long pc, unsigned long addr)
{
	return ftrace_gen_branch(pc, addr, true);
}

static int ftrace_modify_code(unsigned long pc, unsigned long old,
			unsigned long new)
{
	unsigned long replaced;

	if (probe_kernel_read(&replaced, (void *)pc, MCOUNT_INSN_SIZE))
		return -EFAULT;

	if (replaced != old)
		return -EINVAL;

	if (probe_kernel_write((void *)pc, &new, MCOUNT_INSN_SIZE))
		return -EPERM;

	flush_icache_range(pc, pc + MCOUNT_INSN_SIZE);

	return 0;
}

int ftrace_update_ftrace_func(ftrace_func_t func)
{
	unsigned long pc, old;
	unsigned long new;
	int ret;

	pc = (unsigned long)&ftrace_call;
	memcpy(&old, &ftrace_call, MCOUNT_INSN_SIZE);
	new = ftrace_call_replace(pc, (unsigned long)func);

   ret = ftrace_modify_code(pc, old, new);

#ifdef CONFIG_OLD_MCOUNT
   if (!ret) {
		pc = (unsigned long)&ftrace_call_old;
       memcpy(&old, &ftrace_call_old, MCOUNT_INSN_SIZE);
       new = ftrace_call_replace(pc, (unsigned long)func);

       ret = ftrace_modify_code(pc, old, new);
   }
#endif

   return ret;
}

int ftrace_make_call(struct dyn_ftrace *rec, unsigned long addr)
{
   unsigned long new, old;
   unsigned long ip = rec->ip;

   old = ftrace_nop_replace(rec);
   new = ftrace_call_replace(ip, adjust_address(rec, addr));

   return ftrace_modify_code(rec->ip, old, new);
}

int ftrace_make_nop(struct module *mod,
			struct dyn_ftrace *rec, unsigned long addr)
{
   unsigned long ip = rec->ip;
   unsigned long old;
   unsigned long new;
   int ret;

   old = ftrace_call_replace(ip, adjust_address(rec, addr));
   new = ftrace_nop_replace(rec);
   ret = ftrace_modify_code(ip, old, new);

#ifdef CONFIG_OLD_MCOUNT
   if (ret == -EINVAL && addr == MCOUNT_ADDR) {
		rec->arch.old_mcount = true;

       old = ftrace_call_replace(ip, adjust_address(rec, addr));
       new = ftrace_nop_replace(rec);
       ret = ftrace_modify_code(ip, old, new);
   }
#endif

   return ret;
}


#if defined(CONFIG_DYNAMIC_FTRACE) && defined(CONFIG_FUNCTION_GRAPH_TRACER)
extern unsigned long ftrace_graph_call;
extern unsigned long ftrace_graph_call_old;
extern void ftrace_graph_caller_old(void);

static int __ftrace_modify_caller(unsigned long *callsite,
				  void (*func) (void), bool enable)
{
	unsigned long caller_fn = (unsigned long) func;
	unsigned long pc = (unsigned long) callsite;
	unsigned long branch = ftrace_gen_branch(pc, caller_fn, false);
	unsigned long nop = 0xe1a00000;	/* mov r0, r0 */
	unsigned long old = enable ? nop : branch;
	unsigned long new = enable ? branch : nop;

	return ftrace_modify_code(pc, old, new);
}

static int ftrace_modify_graph_caller(bool enable)
{
	int ret;

	ret = __ftrace_modify_caller(&ftrace_graph_call,
				     ftrace_graph_caller,
				     enable);

#ifdef CONFIG_OLD_MCOUNT
	if (!ret)
		ret = __ftrace_modify_caller(&ftrace_graph_call_old,
					     ftrace_graph_caller_old,
					     enable);
#endif

	return ret;
}

int ftrace_enable_ftrace_graph_caller(void)
{
	return ftrace_modify_graph_caller(true);
}

int ftrace_disable_ftrace_graph_caller(void)
{
	return ftrace_modify_graph_caller(false);
}
#endif /* CONFIG_DYNAMIC_FTRACE */
#endif /* CONFIG_KDEBUGD_FTRACE */



/* run from ftrace_init with irqs disabled */
int __init ftrace_dyn_arch_init(void *data)
{
#ifndef CONFIG_KDEBUGD_FTRACE
	ftrace_mcount_set(data);
#else /* CONFIG_KDEBUGD_FTRACE */
	*(unsigned long *)data = 0;
#endif /* CONFIG_KDEBUGD_FTRACE */

	return 0;
}
