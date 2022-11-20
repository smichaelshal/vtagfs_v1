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

#include "../../vtagfs.h"


static struct dentry *__lock_parent(struct dentry *dentry)
{
	struct dentry *parent;
	rcu_read_lock();
	spin_unlock(&dentry->d_lock);
again:
	parent = READ_ONCE(dentry->d_parent);
	spin_lock(&parent->d_lock);
	/*
	 * We can't blindly lock dentry until we are sure
	 * that we won't violate the locking order.
	 * Any changes of dentry->d_parent must have
	 * been done with parent->d_lock held, so
	 * spin_lock() above is enough of a barrier
	 * for checking if it's still our child.
	 */
	if (unlikely(parent != dentry->d_parent)) {
		spin_unlock(&parent->d_lock);
		goto again;
	}
	rcu_read_unlock();
	if (parent != dentry)
		spin_lock_nested(&dentry->d_lock, DENTRY_D_LOCK_NESTED);
	else
		parent = NULL;
	return parent;
}

static inline struct dentry *lock_parent(struct dentry *dentry)
{
	struct dentry *parent = dentry->d_parent;
	if (IS_ROOT(dentry))
		return NULL;
	if (likely(spin_trylock(&parent->d_lock)))
		return parent;
	return __lock_parent(dentry);
}

struct dentry *dentry_kill(struct dentry *dentry)
	__releases(dentry->d_lock)
{
	struct inode *inode = dentry->d_inode;
	struct dentry *parent = NULL;

	if (inode && unlikely(!spin_trylock(&inode->i_lock)))
		goto slow_positive;

	if (!IS_ROOT(dentry)) {
		parent = dentry->d_parent;
		if (unlikely(!spin_trylock(&parent->d_lock))) {
			parent = __lock_parent(dentry);
			if (likely(inode || !dentry->d_inode))
				goto got_locks;
			/* negative that became positive */
			if (parent)
				spin_unlock(&parent->d_lock);
			inode = dentry->d_inode;
			goto slow_positive;
		}
	}
	__dentry_kill(dentry);
	return parent;

slow_positive:
	spin_unlock(&dentry->d_lock);
	spin_lock(&inode->i_lock);
	spin_lock(&dentry->d_lock);
	parent = lock_parent(dentry);
got_locks:
	if (unlikely(dentry->d_lockref.count != 1)) {
		dentry->d_lockref.count--;
	}
	// else if (likely(!retain_dentry(dentry))) { // ???
	// 	__dentry_kill(dentry);
	// 	return parent;
	// }
	/* we are keeping it, after all */
	if (inode)
		spin_unlock(&inode->i_lock);
	if (parent)
		spin_unlock(&parent->d_lock);
	spin_unlock(&dentry->d_lock);
	return NULL;
}


struct external_name {
	union {
		atomic_t count;
		struct rcu_head head;
	} u;
	unsigned char name[];
};

static inline struct external_name *external_name(struct dentry *dentry)
{
	return container_of(dentry->d_name.name, struct external_name, name[0]);
}

static void __d_free(struct rcu_head *head)
{
	struct dentry *dentry = container_of(head, struct dentry, d_u.d_rcu);

	// kmem_cache_free(dentry_cache, dentry); // ???
}

static void __d_free_external(struct rcu_head *head)
{
	struct dentry *dentry = container_of(head, struct dentry, d_u.d_rcu);
	kfree(external_name(dentry));
	// kmem_cache_free(dentry_cache, dentry); // ???
}


static inline int dname_external(const struct dentry *dentry)
{
	return dentry->d_name.name != dentry->d_iname;
}

static void dentry_free(struct dentry *dentry)
{
	WARN_ON(!hlist_unhashed(&dentry->d_u.d_alias));
	if (unlikely(dname_external(dentry))) {
		struct external_name *p = external_name(dentry);
		if (likely(atomic_dec_and_test(&p->u.count))) {
			call_rcu(&dentry->d_u.d_rcu, __d_free_external);
			return;
		}
	}
	/* if dentry was never visible to RCU, immediate free is OK */
	if (dentry->d_flags & DCACHE_NORCU)
		__d_free(&dentry->d_u.d_rcu);
	else
		call_rcu(&dentry->d_u.d_rcu, __d_free);
}

static inline void __d_clear_type_and_inode(struct dentry *dentry)
{
	unsigned flags = READ_ONCE(dentry->d_flags);

	flags &= ~(DCACHE_ENTRY_TYPE | DCACHE_FALLTHRU);
	WRITE_ONCE(dentry->d_flags, flags);
	dentry->d_inode = NULL;
	// if (dentry->d_flags & DCACHE_LRU_LIST)
	// 	this_cpu_inc(nr_dentry_negative); // ???
}

static void dentry_unlink_inode(struct dentry * dentry)
	__releases(dentry->d_lock)
	__releases(dentry->d_inode->i_lock)
{
	struct inode *inode = dentry->d_inode;

	raw_write_seqcount_begin(&dentry->d_seq);
	__d_clear_type_and_inode(dentry);
	hlist_del_init(&dentry->d_u.d_alias);
	raw_write_seqcount_end(&dentry->d_seq);
	spin_unlock(&dentry->d_lock);
	spin_unlock(&inode->i_lock);
	// if (!inode->i_nlink)
	// 	fsnotify_inoderemove(inode); // ???
	if (dentry->d_op && dentry->d_op->d_iput)
		dentry->d_op->d_iput(dentry, inode);
	else
		iput(inode);
}
#define D_FLAG_VERIFY(dentry,x) WARN_ON_ONCE(((dentry)->d_flags & (DCACHE_LRU_LIST | DCACHE_SHRINK_LIST)) != (x))


static void d_lru_del(struct dentry *dentry)
{
	D_FLAG_VERIFY(dentry, DCACHE_LRU_LIST);
	dentry->d_flags &= ~DCACHE_LRU_LIST;
	// this_cpu_dec(nr_dentry_unused);
	// if (d_is_negative(dentry))
	// 	this_cpu_dec(nr_dentry_negative); // ???
	WARN_ON_ONCE(!list_lru_del(&dentry->d_sb->s_dentry_lru, &dentry->d_lru));
}

static inline void dentry_unlist(struct dentry *dentry, struct dentry *parent)
{
	struct dentry *next;
	/*
	 * Inform d_walk() and shrink_dentry_list() that we are no longer
	 * attached to the dentry tree
	 */
	dentry->d_flags |= DCACHE_DENTRY_KILLED;
	if (unlikely(list_empty(&dentry->d_child)))
		return;
	__list_del_entry(&dentry->d_child);
	/*
	 * Cursors can move around the list of children.  While we'd been
	 * a normal list member, it didn't matter - ->d_child.next would've
	 * been updated.  However, from now on it won't be and for the
	 * things like d_walk() it might end up with a nasty surprise.
	 * Normally d_walk() doesn't care about cursors moving around -
	 * ->d_lock on parent prevents that and since a cursor has no children
	 * of its own, we get through it without ever unlocking the parent.
	 * There is one exception, though - if we ascend from a child that
	 * gets killed as soon as we unlock it, the next sibling is found
	 * using the value left in its ->d_child.next.  And if _that_
	 * pointed to a cursor, and cursor got moved (e.g. by lseek())
	 * before d_walk() regains parent->d_lock, we'll end up skipping
	 * everything the cursor had been moved past.
	 *
	 * Solution: make sure that the pointer left behind in ->d_child.next
	 * points to something that won't be moving around.  I.e. skip the
	 * cursors.
	 */
	while (dentry->d_child.next != &parent->d_subdirs) {
		next = list_entry(dentry->d_child.next, struct dentry, d_child);
		if (likely(!(next->d_flags & DCACHE_DENTRY_CURSOR)))
			break;
		dentry->d_child.next = next->d_child.next;
	}
}

void __dentry_kill(struct dentry *dentry)
{
	struct dentry *parent = NULL;
	bool can_free = true;
	if (!IS_ROOT(dentry))
		parent = dentry->d_parent;

	/*
	 * The dentry is now unrecoverably dead to the world.
	 */
	lockref_mark_dead(&dentry->d_lockref);

	/*
	 * inform the fs via d_prune that this dentry is about to be
	 * unhashed and destroyed.
	 */
	if (dentry->d_flags & DCACHE_OP_PRUNE)
		dentry->d_op->d_prune(dentry);

	if (dentry->d_flags & DCACHE_LRU_LIST) {
		if (!(dentry->d_flags & DCACHE_SHRINK_LIST))
			d_lru_del(dentry);
	}
	/* if it was on the hash then remove it */
	__d_drop(dentry);
	dentry_unlist(dentry, parent);
	if (parent)
		spin_unlock(&parent->d_lock);
	if (dentry->d_inode)
		dentry_unlink_inode(dentry);
	else
		spin_unlock(&dentry->d_lock);
	// this_cpu_dec(nr_dentry); // ???
	if (dentry->d_op && dentry->d_op->d_release)
		dentry->d_op->d_release(dentry);

	spin_lock(&dentry->d_lock);
	if (dentry->d_flags & DCACHE_SHRINK_LIST) {
		dentry->d_flags |= DCACHE_MAY_FREE;
		can_free = false;
	}
	spin_unlock(&dentry->d_lock);
	// if (likely(can_free))
	// 	dentry_free(dentry);
	cond_resched();
}