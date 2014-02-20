#ifndef __PVR_EDIT_H_
#define __PVR_EDIT_H_


#if (defined(CONFIG_XFS_FS_TRUNCATE_RANGE) || defined (CONFIG_XFS_FS_SPLIT) \
     || defined(CONFIG_EXT3_FS_FILE_FUNC) || defined (CONFIG_EXT3_FS_TRUNCATE_RANGE))

typedef struct {
        unsigned long long offset;
        unsigned long long end;
        char filename[64];
}PVR_INFO;

#define FTRUNCATERANGE	0x9001
#define FSPLIT		0x9002
#endif



#endif
