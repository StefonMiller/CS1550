# CS/COE 1550 Lab 3 - Priority Scheduling For Xv6

(_Based on http://www.cs.fsu.edu/~zwang/files/cop4610/Spring2014/project2.pdf_)

In this lab, you will implement a priority-based scheduler for xv6. To get started, download a new copy of the xv6 source code per the instructions in Part 0 below. You’ll do two things in this lab:
1.	You’ll replace xv6’s current round-robin scheduler with a priority-based scheduler.
2.	You’ll add a new syscall for a process to set its own priority.

## PART 0: Setting Up the XV6 Development Environment (Same as in Lab 1 and 2)

We will use the Thoth server (thoth.cs.pitt.edu) or the CS Department's Linux cluster (linux.cs.pitt.edu) or Pitt's Linux cluster (linux-ts.it.pitt.edu) as backup in case you have a problem accessing Thoth. 
You can connect to Thoth without having to connect to Pitt VPN. However, to access the Linux clusters, you need to connect to [Pitt VPN](https://www.technology.pitt.edu/services/pittnet-vpn-pulse-secure). 

1. These instructions will help you clone the Xv6 code of the lab from your Github Classroom private repository into your home folder on Thoth or the Linux clusters.
2. Whenever the amount of AFS free space in your account is less than 500MB, you can increase your disk quota from the [Accounts Self-Service](https://accounts.pitt.edu/Unix/) page.
3. Log in to the server using your Pitt account. From a UNIX box, you can type: ssh ksm73@thoth.cs.pitt.edu assuming ksm73 is your Pitt ID. From Windows, you may use PuTTY or the PowerShell ssh client.
4. Make sure that you are in a private directory (e.g., your private folder in your AFS space).
5. From the server's command prompt, run the following command to download the Xv6 source code for the lab: 
`git clone https://github.com/cs1550-2217/cs1550-lab3-USERNAME.git` where USERNAME is your Github username that you have used when accepting the Github Classroom assignment.
6. Enter your Github username and password. An alternative that is more secure is to [generate and use personal access tokens](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token) instead of passwords.
7. Change directory into the xv6 folder using `cd cs1550-lab3-USERNAME` where again USERNAME is your Github username and `cd xv6` then type `make qemu-nox` to compile and run Xv6. To exit, press `CTRL+a` _then_ `x`.
8. **It is important for all labs and project of this class that you commit frequently into your Github Classroom repository.**


## PART 1: PRIORITY-BASED SCHEDULER FOR XV6

In the first part, you will replace the round-robin scheduler for xv6 with a priority-based scheduler. The valid priority for a process is in the range of 0 to 200, inclusive. **The smaller value represents the higher priority**. For example, a process with a priority of 0 has the highest priority, while a process with a priority of 200 has the lowest priority. The default priority for a process is 50. A priority-based scheduler always selects the process with the highest priority for execution.  If there are multiple processes with the same highest priority, the scheduler uses round-robin to execute them in turn. For example, if processes A, B, C, D, E have the priority of 30, 30, 30, 40, 50, respectively, the scheduler should execute A, B, and C first in a round-robin fashion until they exit, then execute D, and execute E at last.

For this part, you will need to modify `proc.h` and `proc.c`. The change to `proc.h` is simple: just add
an integer field called `priority` to `struct proc`. The changes to `proc.c` are more complicated. You
first need to add a line of code in the `allocproc` function to set the default priority for a process to 50. 

Xv6’s scheduler is implemented in the `scheduler` function in `proc.c`. The `scheduler` function is called by the `mpmain` function in `main.c` as the last step of initialization. The `scheduler` function will never return. It loops forever to schedule the next available process for execution. If you are curious about how it works, read Chapter 5 of the Xv6 book available on Canvas. 

In this part, you need to replace the `scheduler` function with your implementation of a priority-based scheduler. The major difference between your scheduler and the original one lies in how the next process is selected. Your scheduler should loop through all the processes to find a process with the highest priority (instead of locating the next runnable process). If there are multiple processes with the same priority, it schedules them in turn (round-robin). One way to do that is to save the
last scheduled process and start from it to loop through all the processes.

## PART 2: ADD A SYSCALL TO SET PRIORITY OF A PROCESS

The first part adds support for the priority-based scheduling. However, all the processes still have the same priority (50, the default priority). In the second part, you will add a new syscall (`setpriority`) for a process to change its priority. The syscall changes the current process’s priority and returns the old priority. If the new priority is lower than the old priority (i.e., the value of new priority is larger), the syscall will call `yield` to reschedule.

In this part, you will need to change `user.h`, `usys.S`, `syscall.h`, `syscall.c`, and `sysproc.c`. Review
Lab 1 to refresh the steps to add a new syscall. Here is a summary of what to do in each file:

-	`syscall.h`: add a new definition for `SYS_setpriority`.
-	`user.h`: declare the function for user-space applications to access the syscall by adding: `int setpriority(int);`
-	`usys.S`: implement the `setpriority` function by making a syscall to the kernel.
-	`syscall.c`: add the handler for `SYS_setpriority` to the syscalls table using this declaration:
`extern int sys_setpriority(void);`
-	`sysproc.c`: implement the syscall handler `sys_setpriority`. In this function, you need to check that the new priority is valid (in the range of \[0, 200\]), update the process’s priority, and, if the new priority is larger than the old priority, call `yield` to reschedule. You can use the `proc` pointer to access the process control block of the current process.

## TESTING YOUR CODE

Here is an example test program, in which the child process should run before its parent since it has higher priority.

```c
#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
  //create a child process.
  //The fork system call returns the child process's id
  //to the parent process and 0 to the child process.
  int pid = fork();
  if(pid < 0) {
     printf(1, "Error forking\n");
  } else if (pid == 0) {
    setpriority(0);
    int i = 0;
    for(; i < 100000; i++); //Loop a bit
    printf(1, "Child done\n");
  } else {
    setpriority(100);
    int i = 0;
    for(; i < 10; i++); //Loop a bit
    printf(1, "Parent done\n");
  
    //The parent process waits until the child exits.
    wait();
    printf(1, "Child completed\n");
    printf(1, "Parent exiting\n");
  }
  exit();
}
```

##	BONUS (2 POINTS)

If the scheduler preempts a process and there are no other processes of the same or higher priority, it'd be wasteful to preempt and then rselect the same process.  To optimize the scheduler, the scheduler avoids "yield"ing and continues to execute the running process.

**If you do the bonus please let the TA know.**

##	DELIVERABLES

Submit to GradeScope the files that you have modified within the source code of xv6. You should modify the following files only:
- `syscall.h`
- `syscall.c`
- `user.h`
- `usys.S`
- `proc.h`
- `proc.c`
- `sysproc.c`
- `trap.c` (if you do the bonus part).
