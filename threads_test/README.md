# threads test

测试线程调度的性能，用于高负载下的线程性能测试。

输入：thread-yields 每个请求执行“lock/yield/unlock”循环的次数（默认为1000）；thread-locks 每个线程的互斥锁（默认为8）。

输出：CPU speed: 总消耗时间；所有线程完成的event个数；所有线程平均每秒完成event的个数；平均每个event的运行时间。 threads: 平均每个线程完成envet的个数；平均每个线程完成envet的个数。
