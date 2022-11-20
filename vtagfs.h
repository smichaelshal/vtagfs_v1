#ifndef VTAGFS_H_
#define VTAGFS_H_

#include <linux/fs.h>
#include <linux/module.h>

// generic.h
#define FS_NAME "vtagfs"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple module");
MODULE_AUTHOR("Kernel Hacker");

#define MY_MAGIC 0x817358f2




// libfs.c

extern const struct address_space_operations ram_aops;

int simple_write_end(struct file *file, struct address_space *mapping,
			loff_t pos, unsigned len, unsigned copied,
			struct page *page, void *fsdata);

int simple_readpage(struct file *file, struct page *page);


// internal.h

extern const struct inode_operations ramfs_file_inode_operations;

// file-mmu.c

extern const struct file_operations ramfs_file_operations;

extern const struct inode_operations ramfs_file_inode_operations;

unsigned long ramfs_mmu_get_unmapped_area(struct file *file,
		unsigned long addr, unsigned long len, unsigned long pgoff,
		unsigned long flags);

// inode.c 

extern struct file_system_type ramfs_fs_type;


// dcache.c
extern void __dentry_kill(struct dentry *dentry);
extern struct dentry *dentry_kill(struct dentry *dentry);

#endif /* VTAGFS_H_ */
