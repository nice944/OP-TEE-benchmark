#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <cl_memory_test_ta.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SB_MAX_RND 0x3fffffffu

long int memory_block_size;
int *buf;
char *memory_access_mode;
int rand;
int tmp = 0;


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

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	IMSG("cl_memory_test!\n");

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	IMSG("Goodbye!\n");
}


static TEE_Result seq_write(uint32_t param_types,
	TEE_Param params[4])
{
	int i;
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	buf = params[0].memref.buffer;

	*buf = tmp;

	printf("seq_write");
	return TEE_SUCCESS;
}

static TEE_Result seq_read(uint32_t param_types,
	TEE_Param params[4])
{

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	buf = params[0].memref.buffer;

	tmp = *buf;

	printf("seq_read");
	return TEE_SUCCESS;
}

static TEE_Result rnd_write(uint32_t param_types,
	TEE_Param params[4])
{

	int idx = 0;

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						   TEE_PARAM_TYPE_VALUE_INPUT,
						   TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	memory_block_size = params[0].value.a;
	rand = params[1].value.a;
	buf = params[2].memref.buffer;

	idx = (int)((double)rand / (double)SB_MAX_RND *
		    (double)(memory_block_size / sizeof(int)));
	buf[idx] = tmp;

	printf("rnd_write");

	return TEE_SUCCESS;
}

static TEE_Result rnd_read(uint32_t param_types,
	TEE_Param params[4])
{
	int idx = 0;

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						   TEE_PARAM_TYPE_VALUE_INPUT,
						   TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	memory_block_size = params[0].value.a;
	rand = params[1].value.a;
	buf = params[2].memref.buffer;

	idx = (int)((double)rand / (double)SB_MAX_RND *
		    (double)(memory_block_size / sizeof(int)));
	tmp = buf[idx];
	printf("rnd_read");
	return TEE_SUCCESS;
}

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx; /* Unused parameter */

	switch (cmd_id) {
	case TA_CL_MEMORY_TEST_CMD_SEQ_WRITE:
		return seq_write(param_types, params);
	case TA_CL_MEMORY_TEST_CMD_SEQ_READ:
		return seq_read(param_types, params);
	case TA_CL_MEMORY_TEST_CMD_RND_WRITE:
		return rnd_write(param_types, params);
	case TA_CL_MEMORY_TEST_CMD_RND_READ:
		return rnd_read(param_types, params);
	default:
		return TEE_ERROR_NOT_SUPPORTED;
	}
}

