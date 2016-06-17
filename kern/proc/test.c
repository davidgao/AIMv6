
#include <console.h>
#include <proc.h>
#include <sched.h>
#include <libc/unistd.h>
#include <mp.h>

/*
 * Temporary test
 */

void kthread(void *arg)
{
	int j;
	int id = (int)arg;
	kprintf("KTHREAD%d: congratulations!\n", id);
	for (;;) {
		/* There is no unified timer interface so I implemented
		 * suspension as busy-wait.  Change the limit if things
		 * happen too fast for you. */
		for (j = 0; j < 100000; ++j)
			/* nothing */;
		kprintf("KTHREAD%d: running on CPU %d\n", id, cpuid());
#if 0
		/* TODO: create user threads instead of kernel threads.
		 * Here I'm just checking whether system call framework
		 * works. */
		schedule();
#endif
	}
}

void userinit(void)
{
	/*
	 * XXX
	 * replace the following assembly with your arch code to quickly test
	 * (1) system call
	 * (2) sched_yield
	 * (3) user space
	 * (4) scheduler
	 */
#if 1
	asm volatile (
		"1:	li	$3, 100000;"
		"2:	subu	$3, 1;"
		"	bnez	$3, 2b;"
		"	li	$2, 6;"
		"	syscall;"
#if 0
		"	li	$2, 5;"
		"	syscall;"
#endif
		"	b	1b;"
	);
#else
	for (;;)
		/* nothing */;
#endif
}

void proc_test(void)
{
	struct proc *kthreads[5];
	struct proc *uthreads[5];
	int i;

	/*
	 * DEVELOPER NOTE:
	 * Make sure you have tested cases where
	 * (1) # of runnable processes is less than # of CPUs.
	 * (2) # of runnable processes is greater than # of CPUs.
	 * (3) transitions between (1) and (2) (not tested since fork() is
	 *     unavailable yet)
	 */
	for (i = 0; i < 5; ++i) {
		kthreads[i] = proc_new(NULL);
		proc_ksetup(kthreads[i], kthread, (void *)i);
		kthreads[i]->state = PS_RUNNABLE;
		proc_add(kthreads[i]);
	}
	for (i = 0; i < 5; ++i) {
		uthreads[i] = proc_new(NULL);
		uthreads[i]->mm = mm_new();
		assert(uthreads[i]->mm != NULL);
		assert(create_uvm(uthreads[i]->mm, (void *)PAGE_SIZE, PAGE_SIZE, VMA_READ | VMA_EXEC) == 0);
		assert(create_uvm(uthreads[i]->mm, (void *)(3 * PAGE_SIZE), PAGE_SIZE, VMA_READ | VMA_WRITE) == 0);
		assert(copy_to_uvm(uthreads[i]->mm, (void *)PAGE_SIZE, userinit, PAGE_SIZE) == 0);

		proc_usetup(uthreads[i], (void *)PAGE_SIZE, (void *)(4 * PAGE_SIZE - 32), NULL);
		uthreads[i]->state = PS_RUNNABLE;
		proc_add(uthreads[i]);
	}
}

