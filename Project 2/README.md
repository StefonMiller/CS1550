Project 2: Synchronization
==========================

To submit your project code, submit your GitHub Classroom repository to Gradescope. **Only the following files will be used for grading**:

- `museumsim.c`
- `p2-writeup.md` (the writeup in [GitHub Markdown](https://guides.github.com/features/mastering-markdown/))

Don't forget to name your writeup `p2-writeup.md`.

- Due date: Friday 7/9/2021 at 11:59pm EST
- Late due date: Sunday 7/11/2021 at 11:59pm EST

---


Project overview
----------------

Imagine that the **CS1550 museum** has been established to showcase old operating systems from all over the history of computing. Imagine also that a number of CS1550 students have volunteered to work as tour guides to answer the questions of the museum visitors and help them run the showcased operating systems.

The museum is hosted in a room that can hold up to 20 visitors and 2 tour guides at a time (with COVID-19 social distancing restrictions in place). In order not to overwhelm the volunteer tour guides, who already have lots of work to do in the CS1550 class, each volunteer guide is limited to supervise at most 10 visitors each day, and is free to leave afterwards. However, being such nice students, if there is more than one guide inside the museum, all guides will wait for each other before leaving, and leave together.

To provide good quality of service to the visitors, it should never be the case that the number of visitors inside the museum exceeds 10 times the number of tour guides who are inside the museum.

The museum has an associated ticket booth. Each morning, a number of tickets is made available based on the number of visitors which will arrive and the number of tour guides that volunteered to show up on that day. Visitors claim the tickets in no particular order. Once there are no more tickets, any arriving visitors are turned away. Once no more visitors will arrive, any remaining guides can leave.


Synchronization
---------------

The CS1550 museum involves multiple independently acting entities &ndash; namely, the visitors and the tour guides. We will use our synchronization skills to simulate the operation of the CS1550 museum. In particular, you will use locks, condition variables, and/or barriers to simulate the **safe museum tour problem**, whereby visitors and guides are modeled as threads that need to synchronize in such a way that all of the following constraints are met:

- visitors cannot tour the museum without a ticket; if there are no more tickets, the visitor leaves
- visitors cannot tour the museum without a guide; if there is no guide inside the museum, the visitor waits outside; if all guides inside the museum have admitted ten visitors each, the visitor waits outside; otherwise, the visitor proceeds into the musuem
- guides cannot leave until all visitors inside the museum leave
- guides inside the museum must leave together
- _at most_ **two** guides can be in the museum at a time
- each guide serves _at most_ **ten** visitors

To enforce the above conditions, you need to use shared variables between the visitor and guide **threads**. Recall from Project 1 that any time we share data between two or more processes or threads, we run the risk of having race conditions and deadlocks. In race conditions, the shared data could become corrupted. In deadlocks, the threads end up waiting indefinitely on each other. In order to avoid race conditions, we have discussed various mechanisms to ensure that critical regions are guarded.

Your job is to write a multi-threaded program that:

- always satisfies the above constraints
- under no conditions busy waits
- under no conditions causes a deadlock to occur
- under no conditions causes a race condition to occur

A deadlock happens, for example, when a guide and a visitor arrive to an empty museum, and they stay waiting outside forever.

You are not allowed to use busy waiting or timed sleeping to implement this project. This means spinlocks and looping until a memory value changes are **not allowed**, and the use of any sleep call is **not allowed**. Instead, you must use the POSIX threading primitives discussed below and present in the respective document.


POSIX threading API
-------------------

The POSIX threading library (`libpthread`) allows implementing multi-threaded applications and synchronization primitives on a wide array of platforms, such as Windows (via `winpthread`), Linux and MacOS. A discussion on how to use its various features follows. You may use any or all of the allowed features depending on how you choose to implement your project, but not all will be required to produce a working implementation.

Please see the [README for POSIX threads](README.libpthread.md) for information on the functions you may use.


Project details
---------------

You are to write C code for four functions:
- `museum_init`
- `museum_destroy`
- `visitor`
- `guide`

### Constants

Some of the constants you will use while implementing the program are defined as macros, and will be referenced using these names in the following code.

```c
#define VISITORS_PER_GUIDE 10
#define GUIDES_ALLOWED_INSIDE 2
```

### Shared data

Because we need variables to be shared across multiple threads, we must have a safe place to put the synchronization constructs and the variables we will be synchronizing. This safe place is provided for you in a global structure which can be accessed from any thread.

```c
struct shared_data {
	// Add any relevant synchronization constructs and shared state here.
	// For example:
	//     pthread_mutex_t ticket_mutex;
	//     int tickets;
}

static struct shared_data shared;
```


### `museum_init` and `museum_destroy`

To set up and tear down the shared variables for your implementation, `museum_init` will be called before any threads are created, and `museum_destroy` will be called after all threads are done executing.

In `museum_init`, you must initialize the synchronization constructs in your shared data section. As an example:

```c
void museum_init(int num_guides, int num_visitors)
{
	pthread_mutex_init(&shared.ticket_mutex, NULL);

	shared.tickets = MIN(VISITORS_PER_GUIDE * num_guides, num_visitors);
}
```

In `museum_destroy`, you will do the reverse, destroying all the synchronization constructs you made, and potentially freeing any memory you have allocated (e.g., using `malloc()`).
```c
void museum_destroy()
{
	pthread_mutex_destroy(&shared.ticket_mutex);
}
```

### `visitor`

In `visitor`, you will implement the visitor arrival, touring, and leaving sequence.

```c
void visitor(int id)
{
}
```

The visitor thread will indicate that it has arrived at the museum as soon as it has started. However, the visitor should leave without touring if there are no tickets remaining.

After acquiring a ticket, the thread must get in line and **block** (i.e., wait) until there is a tour guide available to let the visitor in.

Unlike in a real museum, in the CS1550 museum, visitors who arrive may enter the museum in any order after they arrive, as long as there are no more than `GUIDES_ALLOWED_INSIDE * VISITORS_PER_GUIDE` inside the museum at any given time.

Once the visitor thread has entered the museum, it should indicate that it is touring by calling the `visitor_tours` mentioned below. Once a visitor thread is done touring, it should indicate that it is leaving by calling `visitor_leaves`, and after that, it may do anything else necessary to leave. Each visitor's departure should **not** depend on any other visitor leaving, or on a tour guide leaving.

Three functions are provided by the driver program to automatically test your code.

```c
void visitor_arrives(int id);
void visitor_tours(int id);
void visitor_leaves(int id);
```

You must call these functions in the correct order and at the correct times to receive full credit. The respective functions will print the following messages to the console:

```
Visitor V arrives at time T.
Visitor V tours the museum at time T.
Visitor V leaves the museum at time T.
```

To simulate touring, `visitor_tours` will also sleep for a configurable amount of time. The default when debugging is two seconds, but you can adjust this to help you test your implementation.

The driver program will automatically measure the elapsed time for you; you do not need to measure it in your code.

### `guide`

In `guide`, you will implement the tour guide arrival, admitting, and leaving sequence.

```c
void guide(int id)
{
}
```

Like the visitor thread, the tour guide thread will indicate that it has arrived as soon as it has started. If there are already `GUIDES_ALLOWED_INSIDE` tour guides inside the museum, it must wait until all guides inside leave before entering.

Once the tour guide is inside the museum, it should indicate that it has entered. Then, it must then begin waiting for and admitting visitors until it has either served a maximum of `VISITORS_PER_GUIDE` visitors, or there are no visitors waiting and no tickets remaining.

After the tour guide has admitted all the visitors it can, it must leave as soon as possible given the following constraints: it must wait for all visitors inside to leave, and for any other tour guide inside to finish before it can leave.

Multiple tour guides inside must indicate they are leaving together before any new tour guides enter. The order in which multiple tour guides leave does not matter. If there are tour guides remaining but no visitors left to serve, the tour guides should enter and then immediately leave.

If another tour guide enters after a guide inside is done but before it leaves, both most wait until the one that entered is done serving visitors.

As with the visitor, four functions are provided by the driver program to automatically test your code.

```c
void guide_arrives(int id);
void guide_enters(int id);
void guide_admits(int id);
void guide_leaves(int id);
```

You must call these functions in the correct order and at the correct times to receive full credit. The respective functions will print the following messages to the console:

```
Guide G arrives at time T.
Guide G admits a visitor at time T.
Guide G leaves the museum at time T.
```
You should be able to use **one mutex lock** to solve this problem.

Testing your code
-----------------

As you implement your project, you are going to want a way to automatically build and debug your code. To do this, run `make debug` from the project directory. If all goes well, your output could be similar to the following:

```
INFO: Starting run

Guide 0 arrives at time 0.
Guide 0 enters at time 0.
Visitor 0 arrives at time 0.
Guide 0 admits a visitor at time 0.
Visitor 0 tours the museum at time 0.
Visitor 1 arrives at time 0.
Guide 0 admits a visitor at time 0.
Visitor 1 tours the museum at time 0.
Visitor 2 arrives at time 0.
Visitor 3 arrives at time 0.
Visitor 4 arrives at time 0.
Visitor 5 arrives at time 0.
Guide 0 admits a visitor at time 0.
Visitor 6 arrives at time 0.
Guide 0 admits a visitor at time 0.
Visitor 2 tours the museum at time 0.
Visitor 7 arrives at time 0.
Visitor 8 arrives at time 0.
Visitor 9 arrives at time 0.
Visitor 3 tours the museum at time 0.
Guide 0 admits a visitor at time 0.
Visitor 7 tours the museum at time 0.
Guide 0 admits a visitor at time 0.
Visitor 5 tours the museum at time 0.
Guide 0 admits a visitor at time 0.
Visitor 6 tours the museum at time 0.
Guide 0 admits a visitor at time 0.
Visitor 4 tours the museum at time 0.
Guide 0 admits a visitor at time 0.
Visitor 8 tours the museum at time 0.
Guide 0 admits a visitor at time 0.
Visitor 9 tours the museum at time 0.
Visitor 0 leaves the museum at time 2.
Visitor 1 leaves the museum at time 2.
Visitor 2 leaves the museum at time 2.
Visitor 3 leaves the museum at time 2.
Visitor 5 leaves the museum at time 2.
Visitor 7 leaves the museum at time 2.
Visitor 6 leaves the museum at time 2.
Visitor 4 leaves the museum at time 2.
Visitor 8 leaves the museum at time 2.
Visitor 9 leaves the museum at time 2.
Guide 0 leaves the museum at time 2.
INFO: Finishing run

```

Several variables can be set on the command line to influence the behavior of the debug program. The defaults are given below. Fractional values are not allowed.

```bash
# The total number of visitors which will arrive.
visitors=10

# The total number of guides which will arrive.
guides=1

# The % chance that a visitor will arrive immediately after the previous one.
visitor_cluster_probability=100

# The % chance that a guide will arrive immediately after the previous one.
guide_cluster_probability=100

# The delay (in microseconds) between visitors if they do not arrive immediately
# after each other.
visitor_arrival_delay=1000000

# The delay (in microseconds) between guides if they do not arrive immediately
# after each other.
guide_arrival_delay=1000000

# The delay (in microseconds) representing the duration of a visitor tour.
visitor_tour_delay=2000000
```

For example:
```bash
visitor_tour_delay=1000000 make debug
```

To automatically test your program, run `make test`. The test program will automatically test your code as fast as possible and generate a lot of output. To stress your implementation, the delays are small and randomized or skipped altogether. Synchronization is left to the mercy of your program.

The test program depends on your correct implementation of `museum_init` and `museum_destroy` to reset the state of the simulation after each run. If you are encountering strange errors, you should check to see that you have properly cleaned up after the run.

The test program runs these cases with randomized delays:
|Visitors|Guides|
|--------|------|
|1|1|
|2|1|
|3|1|
|4|1|
|5|1|
|6|1|
|7|1|
|8|1|
|9|1|
|10|1|
|11|1|
|11|2|
|22|3|
|32|4|
|42|4|
|50|5|


Setting up the sources (to do in recitation)
--------------------------------------------

These instructions will help you clone the starter code for the project from your GitHub Classroom private repository into Thoth.
1. Click on the GitHub Classroom link for the project on Canvas. After you link your GitHub username to the course and accept the assignment, a private repository with the name `cs1550-2217/cs1550-project2-GITHUB_USERNAME.git`, where `GITHUB_USERNAME` is your GitHub username that you have used when accepting the GitHub Classroom assignment, will be created for you.
2. Log in to Thoth using your Pitt account. From a UNIX box, you can type: `ssh PITT_USERNAME@thoth.cs.pitt.edu`, assuming `PITT_USERNAME` is your Pitt ID. From Windows, you may use PuTTY or the PowerShell ssh client.
3. Make sure that you are in a private directory (e.g., `/u/OSLab/PITT_USERNAME` on Thoth).
4. From the server's command prompt, run the following command to download the starter code of the project:
`git clone https://github.com/cs1550-2217/cs1550-project2-GITHUB_USERNAME.git` where `GITHUB_USERNAME` is your GitHub username.
5. Enter your GitHub username and password. An alternative that is more secure is to [generate and use personal access tokens](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token) instead of passwords.
6. Change directory into the project folder using `cd cs1550-project2-GITHUB_USERNAME/project2`.


Debugging tips
--------------

### Using `gdb`

To debug your program using `gdb`, first run `make` to build it, then run `gdb`:

```
$ make
make: Nothing to be done for 'all'.

$ gdb ./museumsim
GNU gdb (Ubuntu 9.2-0ubuntu1~20.04) 9.2
Copyright (C) 2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from museumsim...
(gdb)
```

To influence the program execution, you can set its environment variables before the program starts:
```
(gdb) set env visitor_tour_delay=1000000
(gdb) r
Starting program: /u/OSLab/skhattab/cs1550-project2/museumsim
```

### Using Valgrind

Valgrind includes two tools which may be useful for you in debugging the project.

The first tool is Memcheck, a memory error detector. You can read more about Memcheck [here](https://www.valgrind.org/docs/manual/mc-manual.html).
```
$ make
make: Nothing to be done for 'all'.

$ valgrind ./museumsim
==2369201== Memcheck, a memory error detector
==2369201== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==2369201== Using Valgrind-3.15.0 and LibVEX; rerun with -h for copyright info
==2369201== Command: ./museumsim
==2369201==

...

==2369201==
==2369201== HEAP SUMMARY:
==2369201==     in use at exit: 0 bytes in 0 blocks
==2369201==   total heap usage: 11,060 allocs, 11,060 frees, 191,312 bytes allocated
==2369201==
==2369201== All heap blocks were freed -- no leaks are possible
==2369201==
==2369201== For lists of detected and suppressed errors, rerun with: -s
==2369201== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

The second tool is Helgrind, a thread error detector. You can read more about Helgrind [here](https://valgrind.org/docs/manual/hg-manual.html).

```
$ make
make: Nothing to be done for 'all'.

$ valgrind --tool=helgrind ./museumsim
==1485705== Helgrind, a thread error detector
==1485705== Copyright (C) 2007-2017, and GNU GPL'd, by OpenWorks LLP et al.
==1485705== Using Valgrind-3.15.0 and LibVEX; rerun with -h for copyright info
==1485705== Command: ./test
==1485705==

...

==1485705==
==1485705== Use --history-level=approx or =none to gain increased speed, at
==1485705== the cost of reduced accuracy of conflicting-access information
==1485705== For lists of detected and suppressed errors, rerun with: -s
==1485705== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

File Backups
------------

The `/u/OSLab/` partition and your home folder on thoth are **not** part of AFS space. Thus, any files you modify under these folders are not backed up. If there is a catastrophic disk failure, all of your work will be irrecoverably lost. As such, it is our recommendation that you:

**Commit and push all the files you change to your GitHub repository frequently!**

**BE FOREWARNED:** Loss of work not backed up is not grounds for an extension.


Submission
----------

We will use an automatic grader for Project 2. It runs the same cases as `make test`. You can test your code on the autograder before the deadline. You get unlimited attempts until the deadline. You need to submit your GitHub repository into Gradescope. **You may not change the folder structure and file locations**.

- Your repository should also contain a short report (200-300 words) in [GitHub Markdown](https://guides.github.com/features/mastering-markdown/) named `p2-writeup.md` at the root of the repository that answers the following question:
__Indicate if your solution is deadlock- and starvation-free and explain why.__


Grading rubric
--------------

The rubric items can be found on the project submission page on Gradescope. Non-compiling code gets **zero** points.

|Item|Grade|
|----|-----|
|Test cases on the autograder|80%|
|Comments and style|10%|
|Report|10%|

The following penalty points, among others, may be deducted using manual grading:

|Item|Grade|
|----|-----|
|Use of sleep calls for synchronization|-25%|
|Use of spinlocks|-25%|
|Use of busy waiting|-25%|
|Unprotected shared variable access|-20%|

**If the autograder score is below 40 points (out of 80), a partial grade will be assigned using manual grading of your code. The maximum points that you can get in this manual grading is 50 points. Again, non-compiling code gets **zero** points.**

Please note that the score that you get from the autograder is not your final score. We still do manual grading. We may discover bugs and mistakes that were not caught by the test scripts and take penalty points off. Please use the autograder (or the local test harness `make test`) only as a tool to get immediate feedback and discover bugs in your program. Please note that certain bugs (e.g, deadlocks) in your program may or may not manifest themselves when the test cases are run on your program. It may be the case that the same exact code fails in some tests then the same code passes the same tests when resubmitted. The fact that a test once fails should make you debug your code, not simply resubmit and hope that the situation that caused the bug won't happen the next time around.
