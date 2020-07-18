#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <cl_threads_test_ta.h>

#define YIELD sched_yield

// 定义全局变量
int num_threads = 1;
int thread_yields = 1000;
int thread_locks = 8;
double Total_time;
static pthread_mutex_t *test_mutexes;
static unsigned int req_performed;
unsigned int max_requests = 6;
int lock_num;
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

//线程函数
void *myThread(){

	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_CL_THREADS_TEST_UUID;
	uint32_t err_origin;


	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = 1;

	pthread_mutex_lock(&mut); //加锁，用于对共享变量操作
	int n;
	for(n = 0; n < thread_yields; n ++){
            pthread_mutex_lock(&test_mutexes[lock_num]);

	res = TEEC_InvokeCommand(&sess, TA_CL_THREADS_TEST_CMD_RUN_TEST, &op,
				 &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);
            YIELD();
            pthread_mutex_unlock(&test_mutexes[lock_num]);
        }

	pthread_mutex_unlock(&mut); //解锁

	TEEC_CloseSession(&sess);

	TEEC_FinalizeContext(&ctx);
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

int main(void)
{
	int *time1;
	int *time2;

	double Total_time;
	char input[50];

	printf("initialization parameter: \n");
	printf("number of yields to do per request: %d\n",thread_yields);
	printf("number of locks per thread: %d\n",thread_locks);
	printf("max_requests: %d\n",max_requests);
	printf("If you don't want to change it, just Enter.\n");
	printf("Input parameters are separated by Spaces. eg.thread_yields 1500 thread_locks 9\n");
	gets(input);

	time1 = getCPUusage();

	char *ptr, *retptr, *arr[50] = {"0"};
	int i;

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
			if(strcmp(arr[j], "thread_yields") == 0){
				thread_yields = atoi(arr[j+1]);
			} else if(strcmp(arr[j], "thread_locks") == 0){
				thread_locks = atoi(arr[j+1]);
			} else if(strcmp(arr[j], "max_requests") == 0){
				max_requests = atoi(arr[j+1]);
			} else if(strcmp(arr[j], "num_threads") == 0){
				num_threads = atoi(arr[j+1]);
			}
		}
	}
	printf("\nnumber of yields to do per request: %d\n",thread_yields);
	printf("number of locks per thread: %d\n",thread_locks);
	printf("max_requests: %d\n",max_requests);

	struct timeval start, end;  // 定义第一次调用CPU时钟单位的时间，可以理解为定义一个计数器
	gettimeofday( &start, NULL );   // 获取进入要测试执行时间代码段之前的CPU时间占用值

	// 初始化test_mutexes
	test_mutexes = (pthread_mutex_t *)malloc(thread_locks * sizeof(pthread_mutex_t));
	 if (test_mutexes == NULL){
	     printf("Memory allocation failure!");
	     return 1;
	 }
	// 用默认属性初始化互斥锁
	int k;
	for(k = 0; k < thread_locks; k ++)
		pthread_mutex_init(test_mutexes + k, NULL);

	pthread_mutex_lock(&mut);
	if (req_performed >= max_requests){
		pthread_mutex_unlock(&mut);
		return 1;
	}
	lock_num = req_performed % thread_locks;
	req_performed++;
	pthread_mutex_unlock(&mut);

	thread_create();
	thread_wait();

	gettimeofday( &end, NULL );  // 获取执行完后的CPU时间占用值
	Total_time = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;

	time2 = getCPUusage();

	calUsage(time1, time2);

	getRAM();

	getDisk();

	printf("\nThread test: \n");
	printf("all events: %d \n", max_requests); // 所有线程完成的event个数
	printf("events per m-second: %.1f \n", max_requests / Total_time); // 所有线程平均每微秒完成event的个数
	printf("The average running time per event: %0.4fms\n", Total_time / max_requests); // 平均每个event的运行时间
	printf("total time: %0.4fms\n", Total_time); // 总消耗时间

	printf("\nthreads:\n");
	printf("num_threads: %d\n",num_threads);
	printf("events per thread: %0.4f\n", max_requests / (float)num_threads); // 平均每个线程完成envet的个数
	printf("time per thread: %0.4fms\n", Total_time / num_threads); // 平均每个线程平均耗时

	return 0;
}

