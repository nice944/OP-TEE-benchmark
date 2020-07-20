# Memory functions speed test

Memory allocation tests include four forms: sequential write, sequential read, random write, and random read.

Input：

--num-threads=N             number of threads to use [1]

--memory-block-size=SIZE    size of memory block for test [8K]

--memory-total-size=SIZE    total size of data to transfer [100M]

--memory-oper=STRING        type of memory operations {read, write} [write]

--memory-access-mode=STRING memory access mode {seq, rnd} [seq]

·

Output：

memory speed: 

total time；all events；events per m-second；The average running time per event。

threads:  

num_threads；events per thread；time per thread。
