#include "diskusage.h"
#include "mountlist.h"
#include "fsusage.h"

#include <stdio.h>
#include <unistd.h>

#include <assert.h>

/* #define DU_DEBUG */

/* Many systems reserve some space on each filesystem that only the superuser
 * can use. Set this to true if you want count this space towards the among of
 * free space. This would be the opposite of what 'df' does. */
#undef ADD_RESERVED_SPACE

void
diskusage_read (DiskusageInfo *ps)
{
	unsigned i = 0;
	struct mount_entry *mount_list, *me;
	struct fs_usage fsu;
	
	/* Get list of currently mounted filesystems. */
	
	mount_list = read_filesystem_list (0, 0);
	
	assert (mount_list != NULL);

	for (me = mount_list; me; me = me->me_next) {
#ifdef DU_DEBUG
		printf("Filesystem: %p - %s - %s - %c\n",
		       me->me_devname, me->me_mountdir, me->me_type);
#endif
		
		assert (get_fs_usage (me->me_mountdir, me->me_devname, &fsu) == 0);
		
		fsu.fsu_blocks /= 2;
		fsu.fsu_bfree /= 2;
		fsu.fsu_bavail /= 2;
		
		if (fsu.fsu_blocks == 0)
			continue;

		ps->filesystems[i].sizeinfo [DU_FS_TOTAL] =
			fsu.fsu_blocks;
#ifdef ADD_RESERVED_SPACE
		ps->filesystems[i].sizeinfo [DU_FS_FREE] =
			fsu.fsu_bfree;
#else
		ps->filesystems[i].sizeinfo [DU_FS_FREE] =
			fsu.fsu_bavail;
#endif
		ps->filesystems[i].sizeinfo [DU_FS_USED] =
			fsu.fsu_blocks - fsu.fsu_bfree;

#ifdef DU_DEBUG
		printf("Usage: %ld, %ld, %ld\n",
		       ps->filesystems[i].sizeinfo [DU_FS_TOTAL],
		       ps->filesystems[i].sizeinfo [DU_FS_FREE],
		       ps->filesystems[i].sizeinfo [DU_FS_USED]);
#endif
		
		i++;
	}

	ps->n_filesystems = i;
	
#ifdef DU_DEBUG
	printf("n_filesystems = %u \n", i);
#endif
}
