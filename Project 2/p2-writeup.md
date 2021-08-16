My solution for project 2 is deadlock free. This is because my implementation protects access to shared variables and ensures no 2 processes are stuck waiting for each other. All accesses to shared data, be it reading or writing, are protected with a mutex lock which prevents race conditions and potential deadlocks. Additionally, any time a thread exits or returns, the mutex is unlocked which prevents a non-running process from holding the mutex. Since my shared data is protected and the implementation is correct, deadlocks never occur.

My solution is also starvation-free because my ticket system ensures all visitors with a ticket will be admitted eventually(As long as there are enough guides to give them tours). If there are no guides left for a visitor to tour, they will leave the queue which prevents starvation. Additionally, my algorithm for admitting visitors is fair because a once a visitor claims a ticket number they cannot be cut in front of by another thread. My code uses a global ticket number that decrements every time a visitor is admitted. Visitors also claim ticket numbers starting from the max ticket number, decrementing each time. This ensures when a visitor claims a ticket number, they will be seen before a process that claimed a ticket after them, thus preventing starvation. This is similar to the FCFS algorithm discussed in class.