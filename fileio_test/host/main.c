#include <err.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* TA API: UUID and command IDs */
#include <cl_fileio_test_ta.h>

/* TEE resources */
struct test_ctx {
	TEEC_Context ctx;
	TEEC_Session sess;
};

int write_num = 10;
int read_num = 10;
int TEST_OBJECT_SIZE = 7000;
int num_threads = 1;
double Total_time;
double write_time;
double read_time;
double thread_time;
double delete_time;

pthread_mutex_t mut; //互斥锁类型

/* 函数功能：获取cpu使用情况 */
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

/* 函数功能：将两次的cpu使用情况进行运算，得到cpu利用率 */
float calUsage(int * time1,int * time2){
	int idle1 = *(time1 + 3) + *(time1 + 4);
	int idle2 = *(time2 + 3) + *(time2 + 4);
        int total1 = *(time1 + 0) + *(time1 + 1) + *(time1 + 2)+ *(time1 + 3) + *(time1 + 4)+ *(time1 + 5) + *(time1 + 6)+ *(time1 + 7);
	int total2 = *(time2 + 0) + *(time2 + 1) + *(time2 + 2)+ *(time2 + 3) + *(time2 + 4)+ *(time2 + 5) + *(time2 + 6)+ *(time2 + 7);
        float usage = ((float)((total2 - total1) - (idle2 - idle1)) / (total2 - total1)) * 100;
	printf("===cpu usage is : %.2f% \n",usage);
	return usage;

}

/* 函数功能：获取RAM利用率 */
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

/* 函数功能：获取磁盘利用率 */
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

/* 函数功能：将与ta通信时的准备工作封装成一个函数，该函数完成 initializeContext 和 openSession */
void prepare_tee_session(struct test_ctx *ctx) {
	TEEC_UUID uuid = TA_CL_FILEIO_TEST_UUID;
	uint32_t origin;
	TEEC_Result res;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx->ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/* Open a session with the TA */
	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, origin);
}

/* 函数功能：将与ta通信结束时的收尾工作封装成一个函数，该函数完成 closeSession 和 finalizeContext */
void terminate_tee_session(struct test_ctx *ctx) {
	TEEC_CloseSession(&ctx->sess);
	TEEC_FinalizeContext(&ctx->ctx);
}

/* 函数功能：读文件 */
TEEC_Result read_secure_object(struct test_ctx *ctx, char *id,
			char *data, size_t data_len) {
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(id);

	memset(&op, 0, sizeof(op));
	// 传入ta的参数类型为：临时存储区，该类型要定义存储的内容和存储长度
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE, TEEC_NONE);
	// 参数赋值
	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = id_len;

	op.params[1].tmpref.buffer = data;
	op.params[1].tmpref.size = data_len;

	// 调用ta里的对应方法，TA_CL_FILEIO_TEST_CMD_READ_RAW 为方法名，定义在路径 /ta/include/ 下的.h头文件中
	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_CL_FILEIO_TEST_CMD_READ_RAW,
				 &op, &origin);
	switch (res) {
	case TEEC_SUCCESS:
	case TEEC_ERROR_SHORT_BUFFER:
	case TEEC_ERROR_ITEM_NOT_FOUND:
		break;
	default:
		printf("Command READ_RAW failed: 0x%x / %u\n", res, origin);
	}

	return res;
}

/* 函数功能：写文件 */
TEEC_Result write_secure_object(struct test_ctx *ctx, char *id,
			char *data, size_t data_len) {
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(id);

	memset(&op, 0, sizeof(op));
	// 传入ta的参数类型为：临时存储区，该类型要定义存储的内容和存储长度
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE, TEEC_NONE);
	// 参数赋值
	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = id_len;

	op.params[1].tmpref.buffer = data;
	op.params[1].tmpref.size = data_len;

	// 调用ta里的对应方法，TA_CL_FILEIO_TEST_CMD_WRITE_RAW 为方法名，定义在路径 /ta/include/ 下的.h头文件中
	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_CL_FILEIO_TEST_CMD_WRITE_RAW,
				 &op, &origin);
	if (res != TEEC_SUCCESS)
		printf("Command WRITE_RAW failed: 0x%x / %u\n", res, origin);

	switch (res) {
	case TEEC_SUCCESS:
		break;
	default:
		printf("Command WRITE_RAW failed: 0x%x / %u\n", res, origin);
	}

	return res;
}

/* 函数功能：删除文件 */
TEEC_Result delete_secure_object(struct test_ctx *ctx, char *id) {
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(id);

	memset(&op, 0, sizeof(op));
	// 传入ta的参数类型为：临时存储区，该类型要定义存储的内容和存储长度
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE, TEEC_NONE, TEEC_NONE);
	// 参数赋值
	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = id_len;

	// 调用ta里的对应方法，TA_CL_FILEIO_TEST_CMD_DELETE 为方法名，定义在路径 /ta/include/ 下的.h头文件中
	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_CL_FILEIO_TEST_CMD_DELETE,
				 &op, &origin);

	switch (res) {
	case TEEC_SUCCESS:
	case TEEC_ERROR_ITEM_NOT_FOUND:
		break;
	default:
		printf("Command DELETE failed: 0x%x / %u\n", res, origin);
	}

	return res;
}

/* 函数功能：根据需要进行文件读或写 */
void test(){
	struct test_ctx ctx;
	char obj1_id[] = "object#1";		/* string identification for the object */
	char obj2_id[] = "object#2";		/* string identification for the object */
	char obj1_data[TEST_OBJECT_SIZE];
	char read_data[TEST_OBJECT_SIZE];
	TEEC_Result res;

	printf("Prepare session with the TA\n");
	prepare_tee_session(&ctx);

	int m;
	// 写文件
	for(m = 0; m < write_num; m ++){

		memset(obj1_data, 0xA1, sizeof(obj1_data));

		res = write_secure_object(&ctx, obj1_id,
				  obj1_data, sizeof(obj1_data));
		if (res != TEEC_SUCCESS)
			errx(1, "Failed to create an object in the secure storage");
	}

	// 读文件
	for(m = 0; m < read_num; m ++){

		res = read_secure_object(&ctx, obj1_id,
					 read_data, sizeof(read_data));
		if (res != TEEC_SUCCESS)
			errx(1, "Failed to read an object from the secure storage");
		if (memcmp(obj1_data, read_data, sizeof(obj1_data)))
			errx(1, "Unexpected content found in secure storage");
	}

	terminate_tee_session(&ctx);

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
	struct timeval total_start, total_end;

	int *time1;
	int *time2;

	char input[50];

	// 获取用户输入
	printf("initialization parameter: \n");
    	printf("num_threads: %d\n",num_threads);
    	printf("write_num: %d\n",write_num);
    	printf("read_num: %d\n",read_num);
    	printf("TEST_OBJECT_SIZE: %d\n",TEST_OBJECT_SIZE);
   	printf("If you don't want to change it, just Enter.\n");
    	printf("Input parameters are separated by Spaces. eg. num_threads 1 write_num 20\n");
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

	// 分析用户输入，存入指定变量
	int j;
    	for(j = 0; j < 50; j ++){
        	if(arr[j] != NULL){
            		if(strcmp(arr[j], "num_threads") == 0){
                		num_threads = atoi(arr[j+1]);
            		} else if(strcmp(arr[j], "write_num") == 0){
                		write_num = atoi(arr[j+1]);
            		} else if(strcmp(arr[j], "read_num") == 0){
                		read_num = atoi(arr[j+1]);
            		} else if(strcmp(arr[j], "TEST_OBJECT_SIZE") == 0){
                		TEST_OBJECT_SIZE = atoi(arr[j+1]);
           		}
        	}
    	}

	// 赋值后的变量，显示给用户以作为二次检查
	printf("\nnum_threads: %d\n",num_threads);
    	printf("write_num: %d\n",write_num);
    	printf("read_num: %d\n",read_num);
    	printf("TEST_OBJECT_SIZE: %d\n",TEST_OBJECT_SIZE);

	gettimeofday( &total_start, NULL );

	struct test_ctx ctx;
	char obj1_id[] = "object#1";
	char obj2_id[] = "object#2";
	char obj1_data[TEST_OBJECT_SIZE];
	char read_data[TEST_OBJECT_SIZE];
	TEEC_Result res;

	printf("Prepare session with the TA\n");
	prepare_tee_session(&ctx);

	struct timeval write_start, write_end;

	gettimeofday( &write_start, NULL );

	printf("\n- Create and load object in the TA secure storage\n");
	int m;
	// 写
	for(m = 0; m < write_num; m ++){
		memset(obj1_data, 0xA1, sizeof(obj1_data));

		res = write_secure_object(&ctx, obj1_id,
                            obj1_data, sizeof(obj1_data));
		if (res != TEEC_SUCCESS)
			errx(1, "Failed to create an object in the secure storage");

		printf("Create object %d\n",m+1);
	}
	gettimeofday( &write_end, NULL );
    	write_time = 1000000 * ( write_end.tv_sec - write_start.tv_sec ) + write_end.tv_usec - write_start.tv_usec;

	struct timeval read_start, read_end;
	gettimeofday( &read_start, NULL );

	printf("\n- Read back the object\n");
	// 读
	for(m = 0; m < read_num; m ++){

		res = read_secure_object(&ctx, obj1_id,
                           read_data, sizeof(read_data));
		if (res != TEEC_SUCCESS)
			errx(1, "Failed to read an object from the secure storage");
		if (memcmp(obj1_data, read_data, sizeof(obj1_data)))
			errx(1, "Unexpected content found in secure storage");
		printf("Read object %d\n",m+1);
	}
	gettimeofday( &read_end, NULL );
    	read_time = 1000000 * ( read_end.tv_sec - read_start.tv_sec ) + read_end.tv_usec - read_start.tv_usec;

	struct timeval delete_start, delete_end;
	gettimeofday( &delete_start, NULL );

	printf("- Delete the object\n");
	// 删除
	res = delete_secure_object(&ctx, obj1_id);
	if (res != TEEC_SUCCESS)
		errx(1, "Failed to delete the object: 0x%x", res);

	printf("\nWe're done, close and release TEE resources\n");
	terminate_tee_session(&ctx);

	gettimeofday( &delete_end, NULL );
    	delete_time = 1000000 * ( delete_end.tv_sec - delete_start.tv_sec ) + delete_end.tv_usec - delete_start.tv_usec;

	// 用默认属性初始化互斥锁
	pthread_mutex_init(&mut, NULL);
    	thread_create();
    	thread_wait();

	time2 = getCPUusage();

	calUsage(time1, time2);

	getRAM();

	getDisk();

	gettimeofday( &total_end, NULL );

	Total_time = 1000000 * ( total_end.tv_sec - total_start.tv_sec ) + total_end.tv_usec - total_start.tv_usec;
	
	printf("\nfileio speed:\n");
	printf("total time: %.1fms\n", Total_time);  // 总消耗时间
    	printf("file size: %.1fK\n", TEST_OBJECT_SIZE / (float)1024);  // 文件大小
    	printf("write num:%d  write time: %.1f\n", write_num, write_time);  // 写操作花费时间
    	printf("read num:%d   read time: %.1f\n", read_num, read_time);  // 读操作花费时间
	printf("delete time: %.1f\n", delete_time);  // 删除操作花费时间

    	printf("\nthreads:\n");
	printf("num_threads: %d\n",num_threads);  // 线程数
    	printf("time per thread: %0.1fms\n", thread_time / num_threads); // 每个线程平均耗时

	return 0;
}
