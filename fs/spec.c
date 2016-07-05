
#include <list.h>
#include <fs/vnode.h>
#include <fs/specdev.h>
#include <buf.h>
#include <vmm.h>
#include <errno.h>
#include <aim/device.h>
#include <sys/types.h>
#include <sys/param.h>
#include <panic.h>

static struct list_head specinfo_list = EMPTY_LIST(specinfo_list);
static struct vops spec_vops;	/* forward declaration */

int
spec_inactive(struct vnode *vp, struct proc *p)
{
	/* For a special vnode we just unlock it */
	vunlock(vp);
	return 0;
}

int
spec_open(struct vnode *vp, int mode, struct ucred *cred, struct proc *p)
{
	struct driver *drv;

	switch (vp->type) {
	case VCHR:
		/* TODO, currently fallthru */
	case VBLK:
		drv = devsw[major(vdev(vp))];
		return (drv->open)(vdev(vp), mode, p);
	default:
		return -ENODEV;
	}
	return -EINVAL;
}

int
spec_close(struct vnode *vp, int mode, struct ucred *cred, struct proc *p)
{
	bool lock = !!(vp->flags & VXLOCK);
	int err;
	struct driver *drv;

        kpdebug("closing spec vnode %p, lock: %d\n", vp, lock);
	switch (vp->type) {
	case VCHR:
		panic("spec_close: VCHR NYI\n");
		break;
	case VBLK:
		/* Invalidate all buffers, which requires that the vnode is
		 * locked. */
		if (!lock)
			vlock(vp);
		err = vinvalbuf(vp, cred, p);
		if (err) {
			if (!lock)
				vunlock(vp);
			return err;
		}
		/* We really close the device only if (1) we are on last close
		 * (the ref count is 1), or (2) we are forcibly closing (the
		 * vnode is locked). */
		if (vp->refs > 1 && !lock) {
			vunlock(vp);
			return 0;
		}
		drv = devsw[major(vdev(vp))];
		err = (drv->close)(vdev(vp), mode, p);
		if (!lock)
			vunlock(vp);
		return err;
	default:
		panic("spec_close: not spec\n");
	}
	/* NOTREACHED */
	return 0;
}

int
spec_strategy(struct buf *bp)
{
	struct driver *drv;

	assert(bp->devno == vdev(bp->vnode));
	drv = devsw[major(bp->devno)];
	assert(drv != NULL);
	if (bp->blkno == BLKNO_INVALID)
		bp->blkno = bp->lblkno;

	return (drv->strategy)(bp);
}

/*
 * Get or create the vnode corresponding to the device @devno.
 * Currently we require that each device is matched exactly by one vnode.
 */
int
getdevvp(dev_t devno, struct vnode **vpp, enum vtype type)
{
	struct specinfo *si;
	struct vnode *vp;
	int err;

	for_each_entry (si, &specinfo_list, spec_node) {
		if (si->devno == devno) {
			*vpp = si->vnode;
			return 0;
		}
	}

	err = getnewvnode(NULL, &spec_vops, &vp);
	if (err != 0) {
		*vpp = NULL;
		return err;
	}

	vp->type = type;
	/* Allocate and fill in a specinfo structure */
	si = kmalloc(sizeof(*si), 0);
	si->devno = devno;
	si->vnode = vp;
	list_add_tail(&(si->spec_node), &specinfo_list);
	vp->specinfo = si;

	*vpp = vp;
	return 0;
}

/* Get or create a block device vnode */
int
bdevvp(dev_t devno, struct vnode **vpp)
{
	return getdevvp(devno, vpp, VBLK);
}

dev_t
vdev(struct vnode *vp)
{
	if (vp->type != VCHR && vp->type != VBLK)
		return NODEV;
	else
		return vp->specinfo->devno;
}

static struct vops spec_vops = {
	.open = spec_open,
	.close = spec_close,
	.inactive = spec_inactive,
	.strategy = spec_strategy,
};
