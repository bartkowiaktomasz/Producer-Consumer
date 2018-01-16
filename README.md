# Producer-Consumer
Popular synchronisation problem solved using semaphores and POSIX threads. Each producer and consumer runs in a different thread. The shared data structure is a circular queue (array).
The project was done as a part of Operating Systems course at Imperial and was graded A*.

The program reads in four command line arguments:
size of the queue, number of jobs to generate for each producer (each producer will generate the same number of jobs), number of producers and number of consumers.

Usage:
compile with `make` and run `./main size jobs producers consumers`.

Example use:
`./main 5 10 2 3`

[More on POSIX Threads Programming](https://computing.llnl.gov/tutorials/pthreads/)
