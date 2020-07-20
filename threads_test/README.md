# Threads subsystem performance test

Test the performance of thread scheduling for thread performance testing under high load.

Input：

--num-threads=N             number of threads to use [1]

--thread-yields=N           number of times the "lock/yield/unlock" loop is executed [1000]

--thread-locks=N            mutex for each thread [8]

·

Output：

Thread test: 

total time；all events；events per m-second；The average running time per event。

threads:  

num_threads；events per thread；time per thread。
