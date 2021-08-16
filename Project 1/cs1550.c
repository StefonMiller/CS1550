#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/stddef.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/cs1550.h>

//Macros to define global semaphore list and rwlock. Initialization of global semaphore ID
static LIST_HEAD(sem_list);
static DEFINE_RWLOCK(sem_rwlock);
int semaphore_id = 0;

/**
 * Creates a new semaphore. The long integer value is used to
 * initialize the semaphore's value.
 *
 * The initial `value` must be greater than or equal to zero.
 *
 * On success, returns the identifier of the created
 * semaphore, which can be used with up() and down().
 *
 * On failure, returns -EINVAL or -ENOMEM, depending on the
 * failure condition.
 */
SYSCALL_DEFINE1(cs1550_create, long, value)
{
	//Allocate memory for semaphore
    struct cs1550_sem *sem = kmalloc(sizeof(struct cs1550_sem), GFP_ATOMIC);

	//Check if malloc returned null for the semaphore, also check if value is >= 0
	if(sem == NULL)
	{
		return -ENOMEM;
	}
	else if(value < 0)
	{
		return -EINVAL;
	}

	//Initialize semaphore ID, spinlock, value, and global semaphore list
	sem->sem_id = semaphore_id;
    spin_lock_init(&sem->lock);
	INIT_LIST_HEAD(&sem->list);
	sem->value = value;
	INIT_LIST_HEAD(&sem->waiting_tasks);

	//Lock data with RWlock before initializing and adding to the global semaphore list, as well as incrementing the semaphore ID
	write_lock(&sem_rwlock);
	list_add(&sem->list, &sem_list);
	semaphore_id++;
	write_unlock(&sem_rwlock);
	return sem->sem_id;
}

/**
 * Performs the down() operation on an existing semaphore
 * using the semaphore identifier obtained from a previous call
 * to cs1550_create().
 *
 * This decrements the value of the semaphore, and *may cause* the
 * calling process to sleep (if the semaphore's value goes below 0)
 * until up() is called on the semaphore by another process.
 *
 * Returns 0 when successful, or -EINVAL or -ENOMEM if an error
 * occurred.
 */
SYSCALL_DEFINE1(cs1550_down, long, sem_id)
{
	struct cs1550_sem *sem = NULL;
	//Create read lock to protect access to global semaphore list
	read_lock(&sem_rwlock);
	list_for_each_entry(sem, &sem_list, list)
	{
		//Loop through global semaphore list until we find one that matches the sem_id argument
		if(sem->sem_id == sem_id)
		{
			//Lock the spinlock before decrementing the value
			spin_lock(&sem->lock);
			sem->value--;
			//If the value goes below 0, then the process needs to sleep
			if(sem->value < 0)
			{
				//Allocate memory for a new task
				struct cs1550_task *task_node = kmalloc(sizeof(struct cs1550_task), GFP_ATOMIC);
				//If task node is still null then we are out of memory
				if(task_node == NULL)
				{
					//Free locks and return -enomem
					spin_unlock(&sem->lock);
					read_unlock(&sem_rwlock);
					return -ENOMEM;
				}
				//If allocation for the new task was successful, initialize the previous and next pointers
				INIT_LIST_HEAD(&task_node->list);
				//Add new task to the end of the queue
				list_add_tail(&task_node->list, &sem->waiting_tasks);
				//Initialize task's task struct pointer to the current task
				task_node->task = current;
				//Unlock the spinlock
				spin_unlock(&sem->lock);
				//Set the task state to no longer being ready and invoke the scheduler
				set_current_state(TASK_INTERRUPTIBLE);
				schedule();
				//Once the scheduler has finished, unlock the read lock and return 0(success)
				read_unlock(&sem_rwlock);
				return 0;
			}
			else
			{
				//If the value is 0, then the process can run after unlocking the read and spin locks
				spin_unlock(&sem->lock);
				read_unlock(&sem_rwlock);
				return 0;
			}
		}
	}
	//No semaphore found matching the sem_id argument. Return -einval
	read_unlock(&sem_rwlock);
	return -EINVAL;
}

/**
 * Performs the up() operation on an existing semaphore
 * using the semaphore identifier obtained from a previous call
 * to cs1550_create().
 *
 * This increments the value of the semaphore, and *may cause* the
 * calling process to wake up a process waiting on the semaphore,
 * if such a process exists in the queue.
 *
 * Returns 0 when successful, or -EINVAL if the semaphore ID is
 * invalid.
 */
SYSCALL_DEFINE1(cs1550_up, long, sem_id)
{
	struct cs1550_sem *sem = NULL;
	//Create read lock to protect access to global semaphore list
	read_lock(&sem_rwlock);
	list_for_each_entry(sem, &sem_list, list)
	{
		//Loop through global semaphore list until we find one that matches the sem_id argument
		if(sem->sem_id == sem_id)
		{
			//Lock the spinlock on the semaphore and increment it
			spin_lock(&sem->lock);
			sem->value++;
			//If the value is <= 0, remove the first process from the queue and wake it up
			if(sem->value <= 0)
			{
				//Get first entry in list
				struct cs1550_task *temp_task = list_first_entry(&sem->waiting_tasks, struct cs1550_task, list);
				//Delete entry from the semaphore task list
				list_del(&temp_task->list);
				//Wake up the task
				wake_up_process(temp_task->task);
				//Free memory after task has finished
				kfree(temp_task);
			}
			spin_unlock(&sem->lock);
			read_unlock(&sem_rwlock);
			return 0;
		}
	}
	//No semaphore found matching the sem_id argument. Return -einval
	read_unlock(&sem_rwlock);
	return -EINVAL;
}

/**
 * Removes an already-created semaphore from the system-wide
 * semaphore list using the identifier obtained from a previous
 * call to cs1550_create().
 *
 * Returns 0 when successful or -EINVAL if the semaphore ID is
 * invalid or the semaphore's process queue is not empty.
 */
SYSCALL_DEFINE1(cs1550_close, long, sem_id)
{
	struct cs1550_sem *sem = NULL;

	//Lock global semaphore list before traversal
	write_lock(&sem_rwlock);
	//Loop through global semaphore list
	list_for_each_entry(sem, &sem_list, list) 
	{
		//Attempt to remove semaphore if the id matches the id argument
		if(sem->sem_id == sem_id)
		{
			//Protect access to semaphore's queue before checking if it is empty
			spin_lock(&sem->lock);
			//Once the semaphore is found, check if it has any pending processes waiting on it
			if(!list_empty(&sem->waiting_tasks))
			{
				//If waiting processes are found, we cannot close the semaphore
				spin_unlock(&sem->lock);
				write_unlock(&sem_rwlock);
				return -EINVAL;
			}

			//Remove semaphore from global list if there are no pending processes
			list_del(&sem->list);
			spin_unlock(&sem->lock);
			//Once removed from the list, free the memory allocated to the semaphore
			kfree(sem);
			write_unlock(&sem_rwlock);
			return 0;
		}
	}
	write_unlock(&sem_rwlock);
	//Semaphore not found in list
	return -EINVAL;
}
