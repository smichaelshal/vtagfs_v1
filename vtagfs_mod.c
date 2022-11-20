#include <linux/module.h>
#include <linux/fs.h>
#include "vtagfs.h"


int myfs_init(void)
{
	int err;
	printk("register_filesystem vtagfs start v2: %s\n", FS_NAME);


	/* TODO 1/1: register */
	err = register_filesystem(&ramfs_fs_type);
	if (err) {
		printk("register_filesystem vtagfs failed v2\n");
		return err;
	}

	printk("register_filesystem vtagfs sucsess v2: %d\n", err);
	return 0;
}

static void myfs_exit(void)
{
	/* TODO 1/1: unregister */
	unregister_filesystem(&ramfs_fs_type);
	printk("register_filesystem vtagfs exit v2\n");

	
}

module_init(myfs_init);
module_exit(myfs_exit);