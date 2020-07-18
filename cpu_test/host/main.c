#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <cl_cpu_test_ta.h>

// 定义全局变量
int num_threads = 1;
int max_prime = 10000;
double max_time = 0.0;
int max_request = 1;
int request_num = 0;
double Total_time = 0.0;
double thread_time = 0.0;

pthread_mutex_t mut; //互斥锁类型

int * getCPUusage(){
	FILE *fp = NULL;

	int user = 0,nice = 0,system = 0,idle = 0,iowait = 0,irq = 0,softirq = 0,steal = 0;
	char read1[100];

	int *time;
	time = (int *)malloc(8*sizeof(int));

	fp = fopen("/proc/stat", "r");
	fgets(read1,sizeof(read1),fp);

	int i = 5;
	while(read1[i] != ' '){
		user *= 10;
		user += read1[i] -'0';
		i ++;
	}

	*(time + 0) = user;
	i ++;
	while(read1[i] != ' '){
		nice *= 10;
		nice += read1[i] -'0';
		i ++;
	}
	*(time + 1) = nice;
	i ++;

	while(read1[i] != ' '){
		system *= 10;
		system += read1[i] -'0';
		i ++;
	}
	*(time + 2) = system;
	i ++;

	while(read1[i] != ' '){
		idle *= 10;
		idle += read1[i] -'0';
		i ++;
	}

	*(time + 3) = idle;
	i ++;

	while(read1[i] != ' '){
		iowait *= 10;
		iowait += read1[i] -'0';
		i ++;
	}

	*(time + 4) = iowait;
	i ++;

	while(read1[i] != ' '){
		irq *= 10;
		irq += read1[i] -'0';
		i ++;
	}

	*(time + 5) = irq;
	i ++;

	while(read1[i] != ' '){
		softirq *= 10;
		softirq += read1[i] -'0';
		i ++;
	}

	*(time + 6) = softirq;
	i ++;

	while(read1[i] != ' '){
		steal *= 10;
		steal += read1[i] -'0';
		i ++;
	}

	*(time + 7) = steal;
	fclose(fp);

	return time;
}

float calUsage(int * time1,int * time2){
	int idle1 = *(time1 + 3) + *(time1 + 4);
	int idle2 = *(time2 + 3) + *(time2 + 4);
        int total1 = *(time1 + 0) + *(time1 + 1) + *(time1 + 2)+ *(time1 + 3) + *(time1 + 4)+ *(time1 + 5) + *(time1 + 6)+ *(time1 + 7);
	int total2 = *(time2 + 0) + *(time2 + 1) + *(time2 + 2)+ *(time2 + 3) + *(time2 + 4)+ *(time2 + 5) + *(time2 + 6)+ *(time2 + 7);
        float usage = ((float)((total2 - total1) - (idle2 - idle1)) / (total2 - total1)) * 100;
	printf("===cpu usage is : %.2f% \n",usage);
	return usage;

}

void getRAM(){
	char buff[80];
	FILE *fp=popen("free", "r");
	fgets(buff,sizeof(buff),(FILE*)fp);
	fgets(buff,sizeof(buff),(FILE*)fp);
	printf("buff: %s\n",buff);
	pclose(fp);
	int i = 4;
	int total = 0;
	int used = 0;
	int free = 0;
	while(buff[i] == ' '){
		i ++;
	}
	while(buff[i] != ' '){
		total *= 10;
		total += buff[i] -'0';
		i ++;
	}

	printf("---RAM Total is %.2f MB\n",total / 1024.0);
	while(buff[i] == ' '){
		i ++;
	}
	while(buff[i] != ' '){
		used *= 10;
		used += buff[i] -'0';
		i ++;
	}
	printf("---RAM used is %.2f MB\n",used / 1024.0);
	while(buff[i] == ' '){
		i ++;
	}
	while(buff[i] != ' '){
		free *= 10;
		free += buff[i] -'0';
		i ++;
	}
	printf("---RAM free is %.2f MB\n",free / 1024.0d);
	printf("---RAM Used Percentage is %.2f%\n",used / (float)(total/100.0));
}


void getDisk(){
	char buff[80];
	FILE *fp=popen("df -h", "r");
	fgets(buff,sizeof(buff),(FILE*)fp);
	fgets(buff,sizeof(buff),(FILE*)fp);
	fgets(buff,sizeof(buff),(FILE*)fp);
	pclose(fp);
	int i = 0;

	while(buff[i] == ' '){
		i ++;
	}
	printf("\n....DISK Total Space is: ");
	while(buff[i] != ' '){
		printf("%c",buff[i]);
		i ++;
	}
	printf("\n");
	while(buff[i] == ' '){
		i ++;
	}
	printf("....DISK Used Space is: ");
	while(buff[i] != ' '){
		printf("%c",buff[i]);
		i ++;
	}
	printf("\n");
	while(buff[i] == ' '){
		i ++;
	}
	while(buff[i] != ' '){
		i ++;
	}
	while(buff[i] == ' '){
		i ++;
	}
	printf("....DISK Used Percentage = ");
	while(buff[i] != ' '){
		printf("%c",buff[i]);
		i ++;
	}
	printf("\n");

}


void primeCalculate(){

	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_CL_CPU_TEST_UUID;
	uint32_t err_origin;

	double ta_time, temp1, temp2;
	struct timeval start1, end1;
	gettimeofday( &start1, NULL );

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);
	gettimeofday( &end1, NULL );
	temp1 = 1000000 * ( end1.tv_sec - start1.tv_sec ) + end1.tv_usec - start1.tv_usec;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = max_prime;

	res = TEEC_InvokeCommand(&sess, TA_CL_CPU_TEST_CMD_RUN_TEST, &op,
				 &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	printf("return!!!%d\n", op.params[0].value.a);

	struct timeval start2, end2;
	gettimeofday( &start2, NULL );

	TEEC_CloseSession(&sess);

	TEEC_FinalizeContext(&ctx);

	gettimeofday( &end2, NULL );
	temp2 = 1000000 * ( end2.tv_sec - start2.tv_sec ) + end2.tv_usec - start2.tv_usec;

	ta_time = temp1 + temp2;
	printf("------------------ta_time: %.1fμs\n", ta_time);

}

void test(){
    struct timeval start, end;  // 定义第一次调用CPU时钟单位的时间，可以理解为定义一个计数器
	gettimeofday( &start, NULL );  // 获取进入要测试执行时间代码段之前的CPU时间占用值
	int n;
	for(n = 0; n < max_request; n ++){
        primeCalculate();
        request_num ++;
	}

    gettimeofday( &end, NULL ); // 获取执行完后的CPU时间占用值
    Total_time = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;

    // 如果同时指定了max_request和max_time，则以max_request为主
    if(max_request == 1 && Total_time < max_time){
        Total_time = setTimer(Total_time);
    }
}

double setTimer(double Total_time){
    struct timeval start, end;
    double Total_time2, sum_time;
	gettimeofday( &start, NULL );

	primeCalculate();
	request_num ++;

	gettimeofday( &end, NULL );
	Total_time2 = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
	// 再次比较时间
	sum_time = Total_time + Total_time2;
	if(sum_time < max_time){
        setTimer(sum_time);
    } else {
        return sum_time;
    }
}

//线程函数
void *myThread(){
    double temp;
    pthread_mutex_lock(&mut); //加锁，用于对共享变量操作

    struct timeval start, end;
    gettimeofday( &start, NULL );

    test();

    gettimeofday( &end, NULL );
    temp = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
    pthread_mutex_unlock(&mut); //解锁

    thread_time += temp;
}

void thread_create(void){
    // 创建线程
    pthread_t thread[num_threads];
    int i = 0;
    for (i = 0;i < num_threads; i ++){
        pthread_create(&thread[i], NULL, myThread, NULL);
    }
}

void thread_wait(void){
    pthread_t thread[num_threads];
    int i = 0;
    for (i = 0; i < num_threads; i ++){
        pthread_join(thread[i], NULL);
    }
}

int main(void) {

	int * time1;
	int * time2;

	char input[50];

	printf("initialization parameter: num_threads = 1, max_prime = 10000, max_time = 0μs, max_request = 1\n");
    printf("If you don't want to change it, just Enter.\n");
    printf("Input parameters are separated by Spaces. eg. num_threads 1 max_prime 20\n");
    gets(input);

	time1 = getCPUusage();

    char *ptr, *retptr, *arr[50] = {"0"};
    int i;
    long l;

    ptr = input;

    // 把输入参数以空格为分隔符分割开，存入arr
    while ((retptr = strtok(ptr, " ")) != NULL) {
        arr[i] = retptr;
        ptr = NULL;
        i ++;
    }

	int j;
   	for(j = 0; j < 50; j ++){
   	    if(arr[j] != NULL){
   	        if(strcmp(arr[j], "num_threads") == 0){
   	            num_threads = atoi(arr[j+1]);
            } else if(strcmp(arr[j], "max_prime") == 0){
                max_prime = atoi(arr[j+1]);
            } else if(strcmp(arr[j], "max_time") == 0){
                max_time = atof(arr[j+1]);
            } else if(strcmp(arr[j], "max_request") == 0){
                max_request = atoi(arr[j+1]);
            }
        }
    }

    printf("\nmax_prime: %d\n",max_prime);
    printf("max_time / μs: %.3f\n",max_time);
    printf("max_request: %d\n",max_request);
    printf("(Note: when specifying both 'max_time' and 'max_request', the 'max_request' is the main)\n\n");

    // 用默认属性初始化互斥锁
    pthread_mutex_init(&mut, NULL);
    thread_create();
    thread_wait();

	time2 = getCPUusage();

	calUsage(time1, time2);

	getRAM();

	getDisk();

    printf("\nCPU speed:\n");
    printf("all events: %d\n", request_num); // 所有线程完成的event个数
    printf("events per m-second: %.3f \n", request_num / Total_time); // 所有线程平均每秒完成event的个数
    printf("The average running time per event: %0.3fms\n", Total_time / request_num); // 平均每个event的运行时间
    printf("total time: %.1fms\n", Total_time); // 总消耗时间

    printf("\nthreads:\n");
	printf("num_threads: %d\n",num_threads);
    printf("events per thread: %0.3f \n", request_num / (float)num_threads); // 平均每个线程完成envet的个数
    printf("time per thread: %0.3fms\n", thread_time / num_threads); // 平均每个线程平均耗时

	return 0;
}
