# Mutex performance test

The mutex test simulates all threads running concurrently at the same time


Input：

--num-threads=N             number of threads to use [1]

--mutex-num=N               number of mutexes [1024]

--mutex-locks=N             control the number of mutex_loops [100]

--mutex-loops=N             number of loopz [1000]

·

Output：

mutex speed: 

total time；all events；events per m-second；The average running time per event。

threads:  

num_threads；events per thread；time per thread。
