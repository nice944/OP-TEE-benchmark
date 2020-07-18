#ifndef __CL_FILEIO_TEST_H__
#define __CL_FILEIO_TEST_H__

/* UUID of the trusted application */
#define TA_CL_FILEIO_TEST_UUID \
		{ 0xfab750bb, 0x1437, 0x4fbf, \
			{ 0x87, 0x85, 0x8d, 0x35, 0x80, 0xc3, 0x49, 0x94 } }
/*
 * TA_CL_FILEIO_TEST_CMD_READ_RAW - Create and fill a secure storage file
 * param[0] (memref) ID used the identify the persistent object
 * param[1] (memref) Raw data dumped from the persistent object
 * param[2] unused
 * param[3] unused
 */
#define TA_CL_FILEIO_TEST_CMD_READ_RAW		0

/*
 * TA_CL_FILEIO_TEST_CMD_WRITE_RAW - Create and fill a secure storage file
 * param[0] (memref) ID used the identify the persistent object
 * param[1] (memref) Raw data to be writen in the persistent object
 * param[2] unused
 * param[3] unused
 */
#define TA_CL_FILEIO_TEST_CMD_WRITE_RAW		1

/*
 * TA_CL_FILEIO_TEST_CMD_DELETE - Delete a persistent object
 * param[0] (memref) ID used the identify the persistent object
 * param[1] unused
 * param[2] unused
 * param[3] unused
 */
#define TA_CL_FILEIO_TEST_CMD_DELETE		2

#endif /* __CL_FILEIO_TEST_H__ */
