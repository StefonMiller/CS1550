On average, a hash table is superior to a queue for searching, insertion, and deletion with a time complexity of Θ(1). A Queue has similar time complexity for insertions and deletions but is worse when it comes to searching with a time complexity of Θ(n). This is not ideal as we are often searching the global semaphore list for the up(), down(), and close() functions. However, in the worst case a hash table has a time complexity of Θ(n) for searching, insertion, and deletion whereas a queue maintains the Θ(1) performance for insertion and deletion. With hash tables, achieving a worst case scenario is extremely rare and would only happen with a terrible hash function, but it can happen. This means that in most cases a hash table will be more performant in terms of speed than a queue. However, this puts additional overhead of creating a good hash function on the programmer whereas a queue is much easier to implement. 

Additionally, a queue maintains an order of insertion whereas a hash table is unordered. This is useful for each semaphore's task list where we want the first task in the queue to be removed, but doesn't have any real advantages in the global semaphore list.