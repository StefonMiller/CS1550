# CS/COE 1550 Lab 2 - Process Synchronization in Xv6

In this lab, you will implement a synchronization solution using locks and condition variables to guarantee a specific execution ordering among two processes. Make sure that you start from a fresh copy of the Xv6 code.

## PART 0: Setting Up the XV6 Development Environment (Same as in Lab 1)

We will use the Thoth server (thoth.cs.pitt.edu) or the CS Department's Linux cluster (linux.cs.pitt.edu) or Pitt's Linux cluster (linux-ts.it.pitt.edu) as backup in case you have a problem accessing Thoth. 
You can connect to Thoth without having to connect to Pitt VPN. However, to access the Linux clusters, you need to connect to [Pitt VPN](https://www.technology.pitt.edu/services/pittnet-vpn-pulse-secure). 

1. These instructions will help you clone the Xv6 code of the lab from your Github Classroom private repository into your home folder on Thoth or the Linux clusters.
2. Whenever the amount of AFS free space in your account is less than 500MB, you can increase your disk quota from the [Accounts Self-Service](https://accounts.pitt.edu/Unix/) page.
3. Log in to the server using your Pitt account. From a UNIX box, you can type: ssh ksm73@thoth.cs.pitt.edu assuming ksm73 is your Pitt ID. From Windows, you may use PuTTY or the PowerShell ssh client.
4. Make sure that you are in a private directory (e.g., your private folder in your AFS space).
5. From the server's command prompt, run the following command to download the Xv6 source code for the lab: 
`git clone https://github.com/cs1550-2214/cs1550-lab2-USERNAME.git` where USERNAME is your Github username that you have used when accepting the Github Classroom assignment.
6. Enter your Github username and password. An alternative that is more secure is to [generate and use personal access tokens](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token) instead of passwords.
7. Change directory into the xv6 folder using `cd cs1550-lab2-USERNAME` where again USERNAME is your Github username and `cd xv6` then type `make qemu-nox` to compile and run Xv6. To exit, press `CTRL+a` _then_ `x`.
8. **It is important for all labs and project of this class that you commit frequently into your Github Classroom repository.**

## PART 1: THREE PROCESSES - A PARENT AND TWO CHILDREN
In the first part you will write a small user-land program that has a parent process forking two child processes, resulting in a total of three processes: the parent process and the two children.
```c
#include "types.h" 
#include "stat.h" 
#include "user.h"

//We want Child 1 to execute first, then Child 2, and finally Parent.
int main() {
  int pid = fork(); //fork the first child
  if(pid < 0) {
    printf(1, "Error forking first child.\n");
  } else if (pid == 0) {
    printf(1, "Child 1 Executing\n");
  } else {
    pid = fork(); //fork the second child
    if(pid < 0) {
      printf(1, "Error forking second child.\n");
    } else if(pid == 0) {
      printf(1, "Child 2 Executing\n");
    } else {
      printf(1, "Parent Waiting\n"); 
      int i;
      for(i=0; i< 2; i++) 
        wait();
      printf(1, "Children completed\n"); 
      printf(1, "Parent Executing\n"); 
      printf(1, "Parent exiting.\n");
    }
  }
  exit();
 }
```
•	Think of the `printf(1, "<X> executing\n")` statements as placeholders for the code that each process runs.
•	What is the effect of the parent process calling wait() two times?

1.	Put the previous code into a file named race.c inside your Xv6 source code folder. Add `_race\` to the `UPROGS` variable inside your `Makefile`. Compile and run Xv6 by typing `make qemu-nox`.
2.	Run the user-land program inside Xv6 by typing `race` at the Xv6 prompt. Notice the order of execution of the three processes. Run the program multiple times.
•	Do you always get the same order of execution?
•	Does Child 1 always execute (print Child 1 Executing) before Child 2?
3.	Add a `sleep(5);` line before the line where Child 2 prints "Child Executing".
•	What do you notice?
•	Can we guarantee that Child 1 always execute before Child 2?

## PART 2: SPIN LOCKS AND CONDITON VARIABLES

We will start by defining a spinlock that we can use in our user-land program. Xv6 already has a spinlock (see `spinlock.c`) that it uses inside its kernel and is coded in somehow a complex way to handle concurrency caused by interrupts. We don't need most of that complexity, so will write our own light-weight version of spinlocks and make it available to the program via syscalls.

We will then use condition variables to be able to make Child 2 sleep (block) until Child 1 finishes
execution. We will use condition variables to ensure that Child 1 always executes before Child 2. We will add six system calls to Xv6: 

. `init_lock`, `lock`, and `unlock` to initialize, lock, and unlock a spinlock.
. `init_cv`, `cv_wait()`, and `cv_broadcast()` to initialize, wait (sleep) on a condition variable, and to wakeup all sleeping processes on a condition variable.

Recall that both waiting and signaling a condition variable has to be called after acquiring a lock (that's why we define a spinlock). `cv_wait` releases the lock before sleeping and reacquires it after waking up.

4.	First, define the condition variable structure in a new file `condvar.h` as follows.

```c
#include "spinlock.h"
struct condvar {
  struct spinlock *lk;
};
```
A condition variable has an associated spin lock.
Let's then add the system calls. This process is the same process that we followed in Lab 1 to add the `getcount` syscall.

5.	Inside `syscall.h`, add the following lines:

```c
#define SYS_init_cv 22
#define SYS_cv_broadcast 23
#define SYS_cv_wait	24
#define SYS_init_lock 25
#define SYS_lock 26
#define SYS_unlock 27
```
Inside `usys.S`, add:

```c
SYSCALL(init_cv)
SYSCALL(cv_broadcast) 
SYSCALL(cv_wait)
SYSCALL(init_lock)
SYSCALL(lock)
SYSCALL(unlock)
```
 
Inside `syscall.c`, add:
```c
extern int sys_init_cv(void);
extern int sys_cv_broadcast(void);
extern int sys_cv_wait(void);
extern int sys_init_lock(void);
extern int sys_lock(void);
extern int sys_unlock(void);
```
and
```c
[SYS_init_cv] sys_init_cv,
[SYS_cv_wait] sys_cv_wait, 
[SYS_cv_broadcast] sys_cv_broadcast,
[SYS_init_lock] sys_init_lock,
[SYS_lock] sys_lock,
[SYS_unlock] sys_unlock,
```
in the corresponding places in the file.

Inside `user.h`, add
```c
int init_cv(void);
int cv_wait(void); 
int cv_broadcast(void);
int init_lock(void);
int lock(void);
int unlock(void);
```
to the end of the system calls section of the file.

Our condition variable implementation depends heavily on the sleep/wakeup mechanism implemented inside Xv6 (Please read Sleep and Wakeup on Page 65 of the Xv6 book). We will again define a more light-weight version of the sleep function to use our light-weight spinlocks instead of Xv6's spinlocks.

6.	Inside `proc.c`, add the following function definition:

```c
void
sleep1(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0) panic("sleep");

  if(lk == 0)
  panic("sleep without lk");


  acquire(&ptable.lock); 
  lk->locked = 0;
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING; sched();
  // Tidy up.
  p->chan = 0;

  release(&ptable.lock);
  while(xchg(&lk->locked, 1) != 0)
    ;
}
```

After a couple of sanity checks, the function acquires the process table lock `ptable.lock` to be able to call `sched()`, which works on the process table. Then, it releases the spinlock (by setting `locked` to 0) and goes to sleep by setting the process state to `SLEEPING`, setting the channel that the process sleeps on, and switching into the scheduler by calling `sched()`. After the process wakes up, it releases the `ptable lock` and reacquires the spinlock (using the `xchg` instruction).

7.	Inside `defs.h`, add the following function prototype in the `//proc.c` section:

```c
void	sleep1(void*, struct spinlock*);
```

8.	Inside `sysproc.c` add
```c
#include "condvar.h"
struct spinlock lk;
struct condvar cv;
```
after the line:
```c
#include "types.h"
```
and add the following syscall implementations:
```c
int sys_init_cv(void)
{
  cv.lk = &lk;
  return 0;
}

int sys_cv_broadcast(void)
{
  wakeup(&cv);
  return 0;
}

int sys_cv_wait(void)
{
  sleep1(&cv, cv.lk); 
  return 0;
}

void sys_init_lock() { 
  lk.locked = 0;
}

void sys_lock() {
  while(xchg(&lk.locked, 1) != 0)
    ;
}

void sys_unlock() { 
  xchg(&lk.locked, 0);
}
```

We will use the same `struct spinlock` defined in `spinlock.h` but we will only use its `locked`
field. Initializing the lock and unlocking it both work by setting locked to 0. Locking uses the 
atomic `xchg` instruction, which sets the contents of its first parameter (a memory address) to the second parameter and returns the old value of the contents of the first parameter.

The address of the condition variable is used as the channel passed over to the `sleep1` function defined in Step 8. The address of the condition variable is unique and this is all what we need: a unique channel number to sleep and to get waked up on.

This implementation uses **one condition variable and one spinlock for the entire system**. This is not a general solution by any means but is enough for our small experiment.
 
_After seeing what the two system calls do, why do you think we had to add system calls for the operations on condition variables? Why not just have these operations as functions in ulib.c as we did for the spinlock?_


## PART 3: USING THE CONDITION VARIABLES

We can then modify `race.c` to use a condition variable to guarantee process ordering.

```c
#include "types.h" 
#include "stat.h" 
#include "user.h" 
include "condvar.h"

//We want Child 1 to execute first, then Child 2, and finally Parent.
int main() {
  init_lock();
  init_cv();
  int pid = fork(); //fork the first child
  if(pid < 0) {
    printf(1, "Error forking first child.\n");
  } else if (pid == 0) { 
    sleep(5);
    lock();
    printf(1, "Child 1 Executing\n");
    cv_broadcast();	
    unlock();	
  } else {
    pid = fork(); //fork the second
    if(pid < 0) {
      printf(1, "Error forking second child.\n");
    } else if(pid == 0) {
      lock();
      cv_wait();
      printf(1, "Child 2 Executing\n");
      unlock();
    } else {
      lock();
      printf(1, "Parent Waiting\n");
      unlock();      
      int i;
      for(i=0; i< 2; i++) 
        wait();
      printf(1, "Children completed\n"); 
      printf(1, "Parent Executing\n"); 
      printf(1, "Parent exiting.\n");
    }
  }
  exit(); 
}
```
The system-wide spinlock and condition variable are initialized. Then Child 1 signals the condition variable after acquiring the spinlock. Child 2 sleeps on the condition variable after acquiring the spinlock as well. The `printf`s are put inside the critical sections to avoid fragmented output.

Compile and run the modified race program.
•	Does Child 1 always execute before Child 2?

## PART 4: LOST WAKEUPS

Does it happen that the program gets stuck? This is called a deadlock. If Child 2 gets to sleep after Child 1 signals, the wakeup signal is lost (i.e., never received by Child 2). In this case, Child 2 has no way of being awaked.
To solve this problem, we need to enclose the `cv_wait()` call inside a while loop. We need some form of a flag that gets set by Child 1 when it is done executing. Child 2 will then do
```c
while(flag is not set) 
  cv_wait();
```
This way, even if Child 1 sets the flag and signals before Child 2 executes the while loop, Child 2 will avoid going to sleep because the flag will be set. The flag has to be shared between the two processes. We will use a file for that. Other methods for sharing are shared memory and pipes.

To create a file,
```c
int fd = open("flag", O_RDWR | O_CREATE);
```
To write into the file,
```c
write(fd, "done", 4);
```
Checking the flag has to be non-blocking. The `read` system call is blocking. Reading the size of the file is not. So, we will check the flag by reading the file size. To read the size of a file,

```c
struct stat stats; 
fstat(fd, &stats);
printf(1, "file size = %d\n", stats.size);
```
Now, we are ready to write the while loop inside Child 2. It will loop until the file size is greater than zero, which happens when Child 1 writes "done" into the file after it finishes execution.
 
```c
lock(); 
struct stat stats; 
fstat(fd, &stats);
printf(1, "file size = %d\n", stats.size);
while(stats.size <= 0){ 
  cv_wait(); 
  fstat(fd, &stats);
  printf(1, "file size = %d\n", stats.size);
}
unlock();
```


The new race.c is:
```c
#include "types.h" 
#include "stat.h" 
#include "user.h" 
#include "fcntl.h"

//We want Child 1 to execute first, then Child 2, and finally Parent.
int main() {
  init_lock();
  init_cv();
  int fd = open("flag", O_RDWR | O_CREATE); 
  int pid = fork(); //fork the first child
  if(pid < 0) {
    printf(1, "Error forking first child.\n");
  } else if (pid == 0) { 
    sleep(5);
    lock();
    printf(1, "Child 1 Executing\n"); 
    write(fd, "done", 4); 
    cv_broadcast(); 
    unlock();
  } else {
    pid = fork(); //fork the second
    if(pid < 0) {
      printf(1, "Error forking second child.\n");
     } else if(pid == 0) { 
      lock(); 
      struct stat stats; 
      fstat(fd, &stats);
      printf(1, "file size = %d\n", stats.size);
      while(stats.size <= 0){ 
        cv_wait(); 
        fstat(fd, &stats);
        printf(1, "file size = %d\n", stats.size);
      }
      printf(1, "Child 2 Executing\n");
      unlock();
    } else {
      lock(); 
      printf(1, "Parent Waiting\n");
      unlock();
      int i;
      for(i=0; i< 2; i++) 
        wait();
      printf(1, "Children completed\n"); 
      printf(1, "Parent Executing\n"); 
      printf(1, "Parent exiting.\n");
    }
  }
  close(fd);
  unlink("flag"); 
  exit();
}
```
Note that we are closing the file and deleting it before the parent exits. This is to start afresh the next time we run the program.

Compile and run `race.c` many times.
•	Is it always the case that Child 1 executes before Child 2?
•	Do you observe deadlocks?

Of course, synchronization bugs cannot be ruled out by running a program many times. Formal proof is typically the preferred way especially for safety- and mission-critical systems. There are tools that help with this kind of formal proofs.


## SUBMISSION INSTRUCTIONS
Submit your Github Classroom repository to GradeScope. You should modify the following files only:

- syscall.h
- syscall.c
- user.h
- usys.S
- proc.c
- sysproc.c
- Makefile
- condvar.h
- defs.h

Your submission will be graded by compiling and running it by the autograder.
