#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <cl_memory_test_ta.h>

// 定义全局变量
int num_threads = 1;
long int memory_block_size = 8 * 1024;
long long memory_total_size = 100 * 1024 * 1024;
char *memory_oper = "read";
int memory_oper_num = 2;
char *memory_access_mode = "rnd";
double thread_time = 0.0;
double ta_time = 0.0;

TEEC_Result res;
TEEC_Context ctx;
TEEC_Session sess;
TEEC_Operation op;
TEEC_UUID uuid = TA_CL_MEMORY_TEST_UUID;
uint32_t err_origin;

double Total_time;

/* Statistics */
static unsigned int total_ops;
static long long    total_bytes;
static long long    last_bytes;

/* Array of per-thread buffers */

/* Global buffer */
static int *buffer;
static int **buffers;

#define SB_MAX_RND 0x3fffffffu
#define LARGE_PAGE_SIZE (4UL * 1024 * 1024)

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


void test(){
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_CL_MEMORY_TEST_UUID;
	uint32_t err_origin;

	static int k = -1;
	k ++;
	int *buf;
	buf = buffers[k];
	struct timeval ta_start1, ta_end1;
    	gettimeofday( &ta_start1, NULL );

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);


	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);

	gettimeofday( &ta_end1, NULL );
	ta_time = 1000000 * ( ta_end1.tv_sec - ta_start1.tv_sec ) + ta_end1.tv_usec - ta_start1.tv_usec;

	memset(&op, 0, sizeof(op));

	int rands;
	if(strcmp(memory_access_mode, "rnd") == 0){
	    printf("---");
            srand(time(NULL));
            rands = (rand() % SB_MAX_RND);

            switch (memory_oper_num) {
              case 1:
                while (total_bytes < memory_total_size){
                    printf("case1");
                    total_ops++;
                    total_bytes += memory_block_size;

                    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT, TEEC_NONE);
                    op.params[0].value.a = memory_block_size;
                    op.params[1].value.a = rands;
                    op.params[2].tmpref.buffer = buf;
                    op.params[2].tmpref.size = sizeof(buf);
                    res = TEEC_InvokeCommand(&sess, TA_CL_MEMORY_TEST_CMD_RND_WRITE, &op, &err_origin);
                    if (res != TEEC_SUCCESS)
                        errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
                            res, err_origin);

                    printf("rndw ");
                }
                total_bytes = 0;
                break;
              case 2:
                while (total_bytes < memory_total_size){
                   	total_ops++;
                    total_bytes += memory_block_size;

                    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT, TEEC_NONE);
                    op.params[0].value.a = memory_block_size;
                    op.params[1].value.a = rands;
                    op.params[2].tmpref.buffer = buf;
                    op.params[2].tmpref.size = sizeof(buf);

                    res = TEEC_InvokeCommand(&sess, TA_CL_MEMORY_TEST_CMD_RND_READ, &op, &err_origin);
                    if (res != TEEC_SUCCESS)
                        errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);

                    printf("rndr ");
                }
                total_bytes = 0;
                break;
              default:
                printf("Unknown memory request type:%d. Aborting...\n");
                return 1;
            }
        } else{
	    printf("+++");
            switch (memory_oper_num) {
              case 1:
                while (total_bytes < memory_total_size){
                    total_ops++;
                    total_bytes += memory_block_size;

                    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE,
                             TEEC_NONE, TEEC_NONE);
                    //op.params[0].value.a = memory_block_size;
                    op.params[0].tmpref.buffer = buf;
                    op.params[0].tmpref.size = sizeof(buf);

                    res = TEEC_InvokeCommand(&sess, TA_CL_MEMORY_TEST_CMD_SEQ_WRITE, &op, &err_origin);
                    if (res != TEEC_SUCCESS)
                        errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
                    printf("seqw ");
                }
                total_bytes = 0;
                break;
              case 2:
                while (total_bytes < memory_total_size){
                    total_ops++;
                    total_bytes += memory_block_size;

                    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE,
                             TEEC_NONE, TEEC_NONE);
                    //op.params[0].value.a = memory_block_size;
                    op.params[0].tmpref.buffer = buf;
                    op.params[0].tmpref.size = sizeof(buf);

                    res = TEEC_InvokeCommand(&sess, TA_CL_MEMORY_TEST_CMD_SEQ_READ, &op, &err_origin);
                    if (res != TEEC_SUCCESS)
                        errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
                    printf("seqr ");
                }
                total_bytes = 0;
                break;
              default:
                printf("Unknown memory request type:%d. Aborting...\n");
                return 1;
            }
        }
	struct timeval ta_start2, ta_end2;
    gettimeofday( &ta_start2, NULL );

	TEEC_CloseSession(&sess);

	TEEC_FinalizeContext(&ctx);

	gettimeofday( &ta_end2, NULL );
	ta_time = ta_time + 1000000 * ( ta_end2.tv_sec - ta_start2.tv_sec ) + ta_end2.tv_usec - ta_start2.tv_usec;
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

int memory_init(void){
        buffers = (int **)malloc(num_threads * sizeof(char *));
        if (buffers == NULL){
            printf("Failed to allocate buffers array!");
            return 1;
        }
        for (int i = 0; i < num_threads; i++){
            buffers[i] = (int *)malloc(memory_block_size);
            if (buffers[i] == NULL){
                printf("Failed to allocate buffer for thread #%d!", i);
                return 1;
            }
            memset(buffers[i], 0, memory_block_size);
        }


    return 0;
}

int main(void)
{
	int *time1;
	int *time2;

	char input[50];

	printf("initialization parameter:\n");
    printf("memory_block_size: %dK\n",memory_block_size / 1024);
    printf("memory_total_size: %dG\n",memory_total_size / 1024 / 1024);
    printf("memory_oper: %s\n",memory_oper);
    printf("memory_access_mode: %s\n\n",memory_access_mode);
    printf("If you don't want to change it, just Enter.\n");
    printf("Input parameters are separated by Spaces. eg.num_threads 1 memory_block_size 2(is 2K)\n");
    gets(input);

	time1 = getCPUusage();

    char *ptr, *retptr, *arr[50] = {"0"};
    int m;
    long l;

    ptr = input;

    // 把输入参数以空格为分隔符分割开，存入arr
    while ((retptr = strtok(ptr, " ")) != NULL) {
        arr[m] = retptr;
        ptr = NULL;
        m ++;
    }

	int j;
    for(j = 0; j < 50; j ++){
        if(arr[j] != NULL){
            if(strcmp(arr[j], "num_threads") == 0){
                num_threads = atoi(arr[j+1]);
            } else if(strcmp(arr[j], "memory_block_size") == 0){
                memory_block_size = atoi(arr[j+1]) * 1024;
            } else if(strcmp(arr[j], "memory_total_size") == 0){
                memory_total_size = atoi(arr[j+1]) * 1024 * 1024;
            } else if(strcmp(arr[j], "memory_oper") == 0){
                memory_oper = arr[j+1];
                if(strcmp(memory_oper, "write") == 0){
                    memory_oper_num = 1;
                } else if(strcmp(memory_oper, "read") == 0){
                    memory_oper_num = 2;
                } else if(strcmp(memory_oper, "none") == 0){
                    memory_oper_num = 0;
                }
            } else if(strcmp(arr[j], "memory_access_mode") == 0){
                memory_access_mode = arr[j+1];
            }
        }
    }


    printf("\nmemory_block_size: %dK\n",memory_block_size / 1024);
    printf("memory_total_size: %dG\n",memory_total_size / 1024 / 1024);
    printf("memory_oper: %s\n",memory_oper);
    printf("memory_access_mode: %s\n\n",memory_access_mode);

    struct timeval start, end;
    gettimeofday( &start, NULL );

    memory_init();

	pthread_mutex_init(&mut, NULL);
    thread_create();
    thread_wait();

	gettimeofday( &end, NULL );  // 获取执行完后的CPU时间占用值
    Total_time = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;

	time2 = getCPUusage();

	calUsage(time1, time2);

	getRAM();

	getDisk();

	printf("\n------ta time: %.1fμs\n", ta_time);

    printf("Operations performed: %d (%8.2f ops/μs)\n", total_ops, total_ops / Total_time);
    printf("%4.2f MB transferred (%4.2f MB/μs)\n\n", total_bytes / (1024.0*1024.0) * 1024.0, total_bytes / (1024.0*1024.0) * 1024.0 / Total_time);

    printf("total time: %.1fμs\n", Total_time); // 总消耗时间
    printf("all events: %d\n", total_ops); // 所有线程完成的event个数
    printf("events per micro-second: %.3f \n", total_ops / Total_time); // 所有线程平均每秒完成event的个数
    printf("The average running time per event: %0.3fμs\n", Total_time / total_ops); // 平均每个event的运行时间

    printf("\nthreads: \n");
    printf("num_threads: %d\n",num_threads);
    printf("events per thread: %0.3f \n", total_ops / (float)num_threads); // 平均每个线程完成event的个数
    printf("time per thread: %0.3fμs\n", thread_time / num_threads); // 平均每个线程平均耗时
    return 0;
}


