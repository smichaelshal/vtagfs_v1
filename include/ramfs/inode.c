#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/ramfs.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs_context.h>
#include <linux/fs_parser.h>
#include <linux/namei.h>
// #include "internal.h"

#define FILENAME "/home/john/a1"

#include "../../vtagfs.h"



// void test_lookup_fake(struct inode *dir, struct dentry *dentry, unsigned int flags){
// 	struct path path;
// 	int error;
// 	struct dentry *my_dentry;
// 	struct inode *my_inode = NULL;

// 	error = kern_path(FILENAME, 0, &path);
// 	if (error)
// 		return error;

	

// 	// if (dentry->d_op && dentry->d_op->d_release)
// 	// 	dentry->d_op->d_release(dentry);

// 	// path_put(&path);
	

// 	// my_inode = my_dentry->d_inode;
// 	// parent_dentry = path.dentry->d_parent
// }


struct dentry *my_simple_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags){
	struct path path;
	int error;

	printk("my_lookup t3: %s %ld, flags: %d\n", dentry->d_name.name, strlen(dentry->d_name.name), flags);

	if(IS_ROOT(dentry) || (dentry && dentry->d_inode && S_ISDIR(dentry->d_inode->i_mode)))
		return simple_lookup(dir, dentry, flags);
	
	// dput(dentry);
	error = kern_path(FILENAME, 0, &path);
	if (error)
		return ERR_PTR(-ENAMETOOLONG);

	printk("dentry_src: %ld\n", sizeof(*(dentry)));
	printk("dentry_path: %ld\n", sizeof(*(path.dentry)));

	// dput(path.dentry);
	// mntput(path.mnt);


	// return simple_lookup(dir, dentry, flags);

	// dentry_kill(dentry);

	memcpy(dentry, path.dentry, sizeof(*dentry));
	mntput(path.mnt);


	// mntput(path.mnt);

	

	// printk("my_lookup l_1\n");

	// if (dentry->d_op && dentry->d_op->d_iput){
	// 	printk("my_lookup l_2\n");
	// 	dentry->d_op->d_iput(dentry, dentry->d_inode);
	// }
	// else
	// {
	// 	printk("my_lookup l_3\n");
	// 	iput(dentry->d_inode);
	// }

	// printk("my_lookup l_4\n");
	// dentry->d_inode = path.dentry->d_inode;
	// dentry->d_sb = path.dentry->d_sb;

	
	
	// printk("my_lookup l_5\n");



	// if (!dentry->d_sb->s_d_op)
	// 	d_set_d_op(dentry, &simple_dentry_operations);
	
	d_add(dentry, NULL);
	return NULL;

	

	// if(strlen(dentry->d_name.name) == 3){
		
		
	// } else {

	// }


	// if (dentry->d_name.len > NAME_MAX)
	// 	return ERR_PTR(-ENAMETOOLONG);
	
	// if (!dentry->d_sb->s_d_op)
	// 	d_set_d_op(dentry, &simple_dentry_operations);
	// d_add(dentry, NULL);
	// return NULL;

	// return simple_lookup(dir, dentry, flags);

}

int my_dcache_dir_open(struct inode *inode, struct file *file){
	printk("my_dcache_dir_open s_v2\n");
	return dcache_dir_open(inode, file);
}


int my_dcache_readdir(struct file *file, struct dir_context *ctx){
	printk("my_dcache_readdir s_v2\n");
	return dcache_readdir(file, ctx);
}


int my_dcache_dir_close(struct inode *inode, struct file *file){
	printk("my_dcache_dir_close s_v2\n");
	return dcache_dir_close(inode, file);
}


// ssize_t my_generic_read_dir(struct file *filp, char __user *buf, size_t siz, loff_t *ppos)
// {
// 	printk("my_generic_read_dir s_v2\n");
// 	return generic_read_dir(filp, buf, siz, ppos);
// }


// loff_t my_dcache_dir_lseek(struct file *file, loff_t offset, int whence){
// 	printk("my_dcache_dir_lseek s_v2\n");
// 	return dcache_dir_lseek(file, offset, whence);
// }

// int my_noop_fsync(struct file *file, loff_t start, loff_t end, int datasync){
// 	printk("my_noop_fsync s_v2\n");
// 	return noop_fsync(file, start, end, datasync);
// }

const struct file_operations my_simple_dir_operations = {
	.open		= my_dcache_dir_open, // <<
	.release	= my_dcache_dir_close, // <<
	.llseek		= dcache_dir_lseek,
	.read		= generic_read_dir,
	.iterate_shared	= my_dcache_readdir, // <<
	.fsync		= noop_fsync,
};



struct ramfs_mount_opts {
	umode_t mode;
};

struct ramfs_fs_info {
	struct ramfs_mount_opts mount_opts;
};

#define RAMFS_DEFAULT_MODE	0755

static const struct super_operations ramfs_ops;
static const struct inode_operations ramfs_dir_inode_operations;

struct inode *ramfs_get_inode(struct super_block *sb,
				const struct inode *dir, umode_t mode, dev_t dev)
{
	struct inode * inode = new_inode(sb);

	if (inode) {
		inode->i_ino = get_next_ino();
		inode_init_owner(&init_user_ns, inode, dir, mode);
		inode->i_mapping->a_ops = &ram_aops;
		mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
		mapping_set_unevictable(inode->i_mapping);
		inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
		switch (mode & S_IFMT) {
		default:
			init_special_inode(inode, mode, dev);
			break;
		case S_IFREG:
			inode->i_op = &ramfs_file_inode_operations;
			inode->i_fop = &ramfs_file_operations;
			break;
		case S_IFDIR:
			inode->i_op = &ramfs_dir_inode_operations;
			inode->i_fop = &my_simple_dir_operations;

			/* directory inodes start off with i_nlink == 2 (for "." entry) */
			inc_nlink(inode);
			break;
		case S_IFLNK:
			inode->i_op = &page_symlink_inode_operations;
			inode_nohighmem(inode);
			break;
		}
	}
	return inode;
}

/*
 * File creation. Allocate an inode, and we're done..
 */
/* SMP-safe */
static int
ramfs_mknod(struct user_namespace *mnt_userns, struct inode *dir,
	    struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode * inode = ramfs_get_inode(dir->i_sb, dir, mode, dev);
	int error = -ENOSPC;

	if (inode) {
		d_instantiate(dentry, inode);
		dget(dentry);	/* Extra count - pin the dentry in core */
		error = 0;
		dir->i_mtime = dir->i_ctime = current_time(dir);
	}
	return error;
}

static int ramfs_mkdir(struct user_namespace *mnt_userns, struct inode *dir,
		       struct dentry *dentry, umode_t mode)
{
	int retval = ramfs_mknod(&init_user_ns, dir, dentry, mode | S_IFDIR, 0);
	if (!retval)
		inc_nlink(dir);
	return retval;
}

static int ramfs_create(struct user_namespace *mnt_userns, struct inode *dir,
			struct dentry *dentry, umode_t mode, bool excl)
{
	return ramfs_mknod(&init_user_ns, dir, dentry, mode | S_IFREG, 0);
}

static int ramfs_symlink(struct user_namespace *mnt_userns, struct inode *dir,
			 struct dentry *dentry, const char *symname)
{
	struct inode *inode;
	int error = -ENOSPC;

	inode = ramfs_get_inode(dir->i_sb, dir, S_IFLNK|S_IRWXUGO, 0);
	if (inode) {
		int l = strlen(symname)+1;
		error = page_symlink(inode, symname, l);
		if (!error) {
			d_instantiate(dentry, inode);
			dget(dentry);
			dir->i_mtime = dir->i_ctime = current_time(dir);
		} else
			iput(inode);
	}
	return error;
}

static int ramfs_tmpfile(struct user_namespace *mnt_userns,
			 struct inode *dir, struct dentry *dentry, umode_t mode)
{
	struct inode *inode;

	inode = ramfs_get_inode(dir->i_sb, dir, mode, 0);
	if (!inode)
		return -ENOSPC;
	d_tmpfile(dentry, inode);
	return 0;
}

static const struct inode_operations ramfs_dir_inode_operations = {
	.create		= ramfs_create,
	.lookup		= my_simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.symlink	= ramfs_symlink,
	.mkdir		= ramfs_mkdir,
	.rmdir		= simple_rmdir,
	.mknod		= ramfs_mknod,
	.rename		= simple_rename,
	.tmpfile	= ramfs_tmpfile,
};

/*
 * Display the mount options in /proc/mounts.
 */
static int ramfs_show_options(struct seq_file *m, struct dentry *root)
{
	struct ramfs_fs_info *fsi = root->d_sb->s_fs_info;

	if (fsi->mount_opts.mode != RAMFS_DEFAULT_MODE)
		seq_printf(m, ",mode=%o", fsi->mount_opts.mode);
	return 0;
}

static const struct super_operations ramfs_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
	.show_options	= ramfs_show_options,
};

enum ramfs_param {
	Opt_mode,
};

const struct fs_parameter_spec ramfs_fs_parameters[] = {
	fsparam_u32oct("mode",	Opt_mode),
	{}
};

static int ramfs_parse_param(struct fs_context *fc, struct fs_parameter *param)
{
	struct fs_parse_result result;
	struct ramfs_fs_info *fsi = fc->s_fs_info;
	int opt;

	opt = fs_parse(fc, ramfs_fs_parameters, param, &result);
	if (opt < 0) {
		/*
		 * We might like to report bad mount options here;
		 * but traditionally ramfs has ignored all mount options,
		 * and as it is used as a !CONFIG_SHMEM simple substitute
		 * for tmpfs, better continue to ignore other mount options.
		 */
		if (opt == -ENOPARAM)
			opt = 0;
		return opt;
	}

	switch (opt) {
	case Opt_mode:
		fsi->mount_opts.mode = result.uint_32 & S_IALLUGO;
		break;
	}

	return 0;
}

static int ramfs_fill_super(struct super_block *sb, struct fs_context *fc)
{
	struct ramfs_fs_info *fsi = sb->s_fs_info;
	struct inode *inode;

	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_SIZE;
	sb->s_blocksize_bits	= PAGE_SHIFT;
	sb->s_magic		= MY_MAGIC;
	sb->s_op		= &ramfs_ops;
	sb->s_time_gran		= 1;

	inode = ramfs_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

static int ramfs_get_tree(struct fs_context *fc)
{
	return get_tree_nodev(fc, ramfs_fill_super);
}

static void ramfs_free_fc(struct fs_context *fc)
{
	kfree(fc->s_fs_info);
}

static const struct fs_context_operations ramfs_context_ops = {
	.free		= ramfs_free_fc,
	.parse_param	= ramfs_parse_param,
	.get_tree	= ramfs_get_tree,
};

int ramfs_init_fs_context(struct fs_context *fc)
{
	struct ramfs_fs_info *fsi;

	fsi = kzalloc(sizeof(*fsi), GFP_KERNEL);
	if (!fsi)
		return -ENOMEM;

	fsi->mount_opts.mode = RAMFS_DEFAULT_MODE;
	fc->s_fs_info = fsi;
	fc->ops = &ramfs_context_ops;
	return 0;
}

static void ramfs_kill_sb(struct super_block *sb)
{
	kfree(sb->s_fs_info);
	kill_litter_super(sb);
}

struct file_system_type ramfs_fs_type = {
	.name		= FS_NAME,
	.init_fs_context = ramfs_init_fs_context,
	.parameters	= ramfs_fs_parameters,
	.kill_sb	= ramfs_kill_sb,
	.fs_flags	= FS_USERNS_MOUNT,
};