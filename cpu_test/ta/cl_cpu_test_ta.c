#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <cl_cpu_test_ta.h>

#include <stdio.h>
#include <stdlib.h>

TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("has been called");

	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
	DMSG("has been called");
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param __maybe_unused params[4],
		void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	(void)&params;
	(void)&sess_ctx;

	IMSG("cl_cpu_test!\n");

	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	IMSG("Goodbye!\n");
}

/* ta里不允许math.h的引用，因此算术平方根的计算要自己实现 */
double mySqrt(int num){
    double pre = 1e-7;  // 精度
    double low = 0.0;
    double high = num;
    double mid, squre;
    while (high - low > pre){
        mid = (low + high) / 2;
        squre = mid * mid;
        if (squre > num) {
            high = mid;
        }else {
            low = mid;
        }
    }
    return (low + high) / 2;
}

/* 要调用的功能定义在该函数中 */
static TEE_Result run_test(uint32_t param_types,
	TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	IMSG("Got value: %u from NW", params[0].value.a);


	long j, k;
	for(j = 1; j <= max_prime; j ++){ 
	// 求max_prime范围内的素数个数
		for(k = 2; k < mySqrt(j); k ++){
			if(j % k == 0)
				break;
		}
		if(k > mySqrt(j) && j != 1)
			printf("%ld ", j);
	}
	return TEE_SUCCESS;
}

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx; /* Unused parameter */

	switch (cmd_id) {
	case TA_CL_CPU_TEST_CMD_RUN_TEST:
		return run_test(param_types, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
