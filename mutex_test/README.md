# mutex test

互斥锁测试模拟所有线程在同一时刻并发运行

输入：mutex_num 互斥锁数量（默认为1024）；mutex_loops "lock/ta/unlock"循环的次数，lock时用的锁是从mutex_num中随机选择一个；mutex_locks 控制mutex_loop的次数。

输出：mutex speed: 总消耗时间；所有线程完成的event个数；平均每个event的运行时间。 threads: 平均每个线程完成envet的个数；平均每个线程完成envet的个数。
