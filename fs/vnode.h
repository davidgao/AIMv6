
#ifndef _FS_VNODE_H
#define _FS_VNODE_H

#include <sys/types.h>
#include <aim/sync.h>
#include <list.h>
#include <buf.h>

struct specinfo;	/* fs/specdev.h */
struct mount;		/* fs/mount.h */
struct ucred;		/* include/ucred.h */
struct proc;		/* include/proc.h */

enum vtype {
	VNON,	/* no-type */
	VREG,	/* regular file */
	VDIR,	/* directory */
	VBLK,	/* block device */
	VCHR,	/* character device */
	VLNK,	/* symlink */
	VBAD	/* ??? */
};

struct vnode {
	enum vtype	type;
	struct vops	*ops;
	atomic_t	refs;
	lock_t		lock;
	uint32_t	flags;
#define VXLOCK		0x1
	/* members for mount */
	struct mount	*mount;
	struct list_head mount_node;

	/*
	 * Buffer queue for the vnode.
	 * To simplify buf management, we ensure that the lock is held
	 * (VXLOCK is set in flags) whenever we are manipulating the
	 * buf queue.
	 */
	struct list_head buf_head;
	int		noutputs;	/* # of writing buf's */

	union {
		struct specinfo *specinfo;
		void	*typedata;
	};
	/* FS-specific data.  For UFS-like filesystems this points to inode. */
	void		*data;
};

struct vops {
	/*
	 * open:
	 */
	int (*open)(struct vnode *, int, struct ucred *, struct proc *);
	/*
	 * close:
	 */
	int (*close)(struct vnode *, int, struct ucred *, struct proc *);
	/*
	 * inactive:
	 * Truncate, update, unlock, and clean up the data.  Usually called when
	 * a kernel is no longer using the vnode.
	 * xv6 equivalent is iput().
	 */
	int (*inactive)(struct vnode *, struct proc *);
	/*
	 * strategy:
	 * Initiate a block I/O.
	 */
	int (*strategy)(struct buf *);
};

int getnewvnode(struct mount *, struct vops *, struct vnode **);
void vlock(struct vnode *);
void vunlock(struct vnode *);
int vrele(struct vnode *);
int vwaitforio(struct vnode *);
int vinvalbuf(struct vnode *, struct ucred *, struct proc *);

extern dev_t rootdev;	/* initialized in mach_init() or arch_init() */
extern struct vnode *rootvp;

#endif