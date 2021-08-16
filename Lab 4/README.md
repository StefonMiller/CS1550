# CS/COE 1550 Lab 4 - Priority Scheduling For Xv6

One of the many neat tricks an OS can play with page table hardware is lazy allocation of heap memory. Xv6 applications ask the kernel for heap memory using the `sbrk()` system call, which allocates physical memory and maps it into the process's virtual address space. There are programs that allocate memory but never use it, for example to implement large sparse arrays. Sophisticated kernels delay allocation of each page of heap memory until the application tries to use that page -- as signaled by a page fault. You'll add this lazy allocation feature to xv6 in this lab.

## PART 0: Setting Up the XV6 Development Environment (Same as in previous labs)

We will use the Thoth server (thoth.cs.pitt.edu) or the CS Department's Linux cluster (linux.cs.pitt.edu) or Pitt's Linux cluster (linux-ts.it.pitt.edu) as backup in case you have a problem accessing Thoth. 
You can connect to Thoth without having to connect to Pitt VPN. However, to access the Linux clusters, you need to connect to [Pitt VPN](https://www.technology.pitt.edu/services/pittnet-vpn-pulse-secure). 

1. These instructions will help you clone the Xv6 code of the lab from your Github Classroom private repository into your home folder on Thoth or the Linux clusters.
2. Whenever the amount of AFS free space in your account is less than 500MB, you can increase your disk quota from the [Accounts Self-Service](https://accounts.pitt.edu/Unix/) page.
3. Log in to the server using your Pitt account. From a UNIX box, you can type: ssh ksm73@thoth.cs.pitt.edu assuming ksm73 is your Pitt ID. From Windows, you may use PuTTY or the PowerShell ssh client.
4. Make sure that you are in a private directory (e.g., your private folder in your AFS space).
5. From the server's command prompt, run the following command to download the Xv6 source code for the lab: 
`git clone https://github.com/cs1550-2217/cs1550-lab4-USERNAME.git` where USERNAME is your Github username that you have used when accepting the Github Classroom assignment.
6. Enter your Github username and password. An alternative that is more secure is to [generate and use personal access tokens](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token) instead of passwords.
7. Change directory into the xv6 folder using `cd cs1550-lab4-USERNAME` where again USERNAME is your Github username and `cd xv6` then type `make qemu-nox` to compile and run Xv6. To exit, press `CTRL+a` _then_ `x`.
8. **It is important for all labs and project of this class that you commit frequently into your Github Classroom repository.**

##	PART 1: ELIMINATE ALLOCATION FROM SBRK()

Your first task is to delete page allocation from the `sbrk(n)` system call implementation, which is function `sys_sbrk()` in `sysproc.c`. The `sbrk(n)` system call grows the process's memory size by `n` bytes, and then returns the start of the newly allocated region (i.e., the old size). Your new `sbrk(n)` should just increment the process's size (`proc->sz`) by `n` and return the old size. It should not allocate memory -- so you should delete the call to `growproc()`.

_Try to guess what the result of this modification will be: what will break?_

Make this modification, boot xv6, and type `echo hi` to the shell. You should see something like this:

```
init: starting sh
$ echo hi
pid 3 sh: trap 14 err 6 on cpu 0 eip 0x12f1 addr 0x4004--kill proc
$ 
```

The `"pid 3 sh: trap..."` message is from the kernel trap handler in `trap.c`; it has caught a page fault (trap 14, or `T_PGFLT`), which the xv6 kernel does not know how to handle. Make sure you understand why this page fault occurs. The `"addr 0x4004"` indicates that the virtual address that caused the page fault is `0x4004`.

##	PART 2: LAZY ALLOCATION

Modify the code in `trap.c` to respond to a page fault from user space by mapping a newly-allocated page of physical memory at the faulting address, and then returning back to user space to let the process continue executing. You should add your code just before the `cprintf` call that produced the `"pid 3 sh: trap 14"` message. Your code is not required to cover all corner cases and error situations; it just needs to be good enough to let sh run simple commands like echo and ls.

**Hint**: look at the cprintf arguments to see how to find the virtual address that caused the page fault.

**Hint**: steal code from `allocuvm()` in `vm.c`, which is what `sbrk()` calls (via `growproc()`).

**Hint**: use `PGROUNDDOWN(va)` to round the faulting virtual address down to a page boundary.

**Hint**: `break` or `return` in order to avoid the `cprintf` and the `proc->killed = 1`.

**Hint**: you'll need to call `mappages()`. In order to do this you'll need to delete the `static` keyword in the declaration of `mappages()` in `vm.c`, and you'll need to declare `mappages()` in `trap.c`. Add this declaration to `trap.c` before any call to `mappages()`:
```c
      int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);
```

**Hint**: you can check whether a fault is a page fault by checking if `tf->trapno` is equal to `T_PGFLT` in `trap()`.

If all goes well, your lazy allocation code should result in `echo hi` working. You should get at least one page fault (and thus lazy allocation) in the shell, and perhaps two.

By the way, this is not a fully correct implementation. See the challenges below for a list of problems. Don't worry about these for this lab.

Challenges: 

-	Handle negative `sbrk()` arguments. 
-	Handle error cases such as `sbrk()` arguments that are too large. 
-	Verify that `fork()` and `exit()` work even if some `sbrk()`'d addresses have no memory allocated for them. 
-	Correctly handle faults on the invalid page below the stack. 
-	Make sure that kernel use of not-yet-allocated user addresses works -- for example, if a program passes an `sbrk()`-allocated address to `read()`.

##	DELIVERABLES

Submit to GradeScope the files that you have modified within the source code of xv6. You should modify the following files only:

-	`sysproc.c`
-	`trap.c`
-	`vm.c`

