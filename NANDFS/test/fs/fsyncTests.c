/*************************************************
 * Copyright (C) 2009 Aviad Zuck & Sivan Toledo
 * This file is part of NANDFS.
 *
 * To license NANDFS under a different license, please
 * contact the authors.
 *
 * NANDFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NANDFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with NANDFS.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fsyncTests.c  */
#include <test/fs/fsyncTests.h>
#include <test/fs/testsHeader.h>

void runAllfsyncTests(){

	RUN_TEST(fsync,0);
	RUN_TEST(fsync,1);
	RUN_TEST(fsync,2);
	RUN_TEST(fsync,3);

	/* tests that requie temp commit?*/
//	RUN_TEST(fsync,4);
//	RUN_TEST(fsync,5);

}

/**
 * @brief
 * init fsync test
 *
 */
void init_fsyncTest(){
	if(nandInit())
		return;

	init_flash();
	initializeRamStructs();
	handleNoFs();
	init_fsbuf(fs_buffer);
}

/**
 * @brief
 * tear down fsync test
 *
 */
void tearDown_fsyncTest(){
	init_flash();
	nandTerminate();
	initializeRamStructs();
}

/**
 * @brief
 * fsync a file with uncommited data in inode
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest0(){
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'f';
	int32_t fd, frees, rec_addr, i, writeSize = 400;
	user_id uid = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(buf);

	init_logical_address(ino_addr);
	init_logical_address(log_addr);

	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	frees    = calcTotalFreePages();
	/* write to file */
	fsMemset(buf, byte, FS_BLOCK_SIZE);
//	PRINT_MSG_AND_NUM("\nwrite bytes ", writeSize);
	VERIFY(COMPARE(write(fd, buf, writeSize), writeSize));
//	PRINT("\ncall fsync");
	/* fsync*/
	VERIFY(!fsync(fd))
//	PRINT("\nfsync done");
	/* verify data was committed*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyOpenFileEntry(fd , CREAT_FLAGS, writeSize, uid, 0));
//	PRINT("\nfentries transactions verified");
	VERIFY(!getInode(buf, GET_FILE_ID_BY_FD(fd), ino_addr));
//	PRINT("\ngetInode verified");
	VERIFY(verifyInode(ino_ptr, GET_FILE_ID_BY_FD(fd), FTYPE_FILE, 1, writeSize));
//	PRINT("\ninode verified");
	for(i=0; i<writeSize; i++){
		VERIFY(COMPARE(buf[i], byte));
	}
//	PRINT("\ninode file data verified");
//	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
//	VERIFY(!fsReadBlock(log_addr, buf));
//
//	for(i=0; i<FS_BLOCK_SIZE-INODE_FILE_DATA_SIZE; i++){
//		VERIFY(COMPARE(buf[i], byte));
//	}
//
//	for(; i<FS_BLOCK_SIZE; i++){
//		VERIFY(COMPARE(buf[i], 0xff));
//	}

//	PRINT("\nfileblock verified");
	VERIFY(!getInode(buf, GET_FILE_ID_BY_FD(fd), ino_addr));
	for(i=0; i< DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

//	PRINT("\nverify frees");
//	PRINT_MSG_AND_NUM("\nold frees=", frees);
//	PRINT_MSG_AND_NUM("\nnew frees=", calcTotalFreePages());
	/* commiting should have used only one free page for the new file block*/
	VERIFY(COMPARE(frees, calcTotalFreePages()));

	return 1;
}

/**
 * @brief
 * fsync a file with uncommited data and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest1(){
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'f';
	int32_t fd, frees, rec_addr, i;
	user_id uid = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(buf);

	init_logical_address(ino_addr);
	init_logical_address(log_addr);

	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	frees    = calcTotalFreePages();
	/* write to file */
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));

	/* fsync*/
	VERIFY(!fsync(fd))
//	PRINT("\nfsync start");
	/* verify data was committed*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyOpenFileEntry(fd , CREAT_FLAGS, FS_BLOCK_SIZE, uid, 0));
//	PRINT("\nfentries transactions verified");
	VERIFY(!getInode(buf, GET_FILE_ID_BY_FD(fd), ino_addr));
	VERIFY(verifyInode(ino_ptr, GET_FILE_ID_BY_FD(fd), FTYPE_FILE, 2, FS_BLOCK_SIZE));
//	PRINT("\ninode verified");
	for(i=0; i<INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(buf[i], byte));
	}
//	PRINT("\ninode file data verified");
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlockSimple(log_addr, buf));

	for(i=0; i<FS_BLOCK_SIZE-INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(buf[i], byte));
	}

	for(; i<FS_BLOCK_SIZE; i++){
		VERIFY(COMPARE(buf[i], 0xff));
	}

//	PRINT("\nfileblock verified");
	VERIFY(!getInode(buf, GET_FILE_ID_BY_FD(fd), ino_addr));
	for(i=1; i< DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

//	PRINT("\nverify frees");
	/* commiting should have used only one free page for the new file block*/
	VERIFY(COMPARE(frees, calcTotalFreePages()+1));

	return 1;
}

/**
 * @brief
 * fsync illegal fd and verify failure
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest2(){
	int32_t fd = FS_MAX_OPEN_FILES;

	VERIFY(IS_NEGATIVE(fsync(fd)));

	return 1;
}

/**
 * @brief
 * fsync file that has no uncommited data.
 * should succeed without performing any writes to flash
 *
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest3(){
	uint8_t *f_name = "/file1.dat";
	int32_t fd, frees, rec_addr;

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	frees    = calcTotalFreePages();

	VERIFY(!fsync(fd));

	VERIFY(COMPARE(rec_addr, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	VERIFY(COMPARE(frees, calcTotalFreePages()));

	return 1;
}

/**
 * @brief
 * write to file until it exhausts all free pages. then try to fsync it.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest4(){
	int32_t fd, f_id, f_size, count = 0, expected_size;
	uint8_t *f_name1 = "/file1.dat", byte = 'z';
	uint8_t buf[FS_BLOCK_SIZE];
	uint32_t i,j;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	user_id uid = 1;

	init_logical_address(ino_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	/* create file */
//	PRINT("\ncreat start");
	fd = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd));
	f_id = GET_FILE_ID_BY_FD(fd);

	/* write to inode file data1*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, buf, INODE_FILE_DATA_SIZE)));

	/* write to file 1 until we exhaust all spare pages*/
	while(!(IS_MIN_FREE_PAGES_REACHED())){
		VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
		count++;
    }

    f_size = getFileSize(f_id);
    expected_size = count*FS_BLOCK_SIZE+INODE_FILE_DATA_SIZE;
    VERIFY(COMPARE(f_size, expected_size));
//    PRINT("\n\n\nabout to fsync");
    VERIFY(!fsync(fd));
//    PRINT("\nfsync success");
    VERIFY(verifyFentriesVnodesEmpty(fd));
    VERIFY(verifyOpenFileEntry(fd , CREAT_FLAGS, expected_size, uid, 0));
    VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//    PRINT("\nfentries etc empty success");

    VERIFY(!getInode(fs_buffer, f_id, ino_addr));
    f_size = INODE_GET_NBYTES(ino_ptr);
//     PRINT_MSG_AND_NUM("\n expected_size=",expected_size);
//    PRINT_MSG_AND_NUM("\n inode file size=",f_size);
    VERIFY(COMPARE(f_size, expected_size));

    f_size = getFileSize(f_id);
//    PRINT_MSG_AND_NUM("\n expected_size=",expected_size);
//    PRINT_MSG_AND_NUM("\n f_size=",f_size);
    VERIFY(COMPARE(f_size, expected_size));
//    PRINT("\nfile size success");
    /* verify file data */
    init_fsbuf(buf);
    VERIFY(!readFileBlock(buf, f_id, 0, ino_addr, TID_EMPTY));

	for(i=0; i<INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(buf[i], byte));
	}
//	PRINT("\ninode file data success");
	for(j=INODE_FILE_DATA_SIZE; j<expected_size; j+=FS_BLOCK_SIZE){
		VERIFY(!readFileBlock(buf, f_id, j, ino_addr, TID_EMPTY));

		for(i=0; i<FS_BLOCK_SIZE; i++){
			VERIFY(COMPARE(buf[i], byte));
		}
//		PRINT_MSG_AND_NUM("\n file data success in offset ", j);
	}

	return 1;
}

/**
 * @brief
 * write to file until it exhausts all free pages, and doesn't have enough free vots.
 * then try to fsync it. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest5(){
	int32_t tid, fd1, res, f_id1;
	uint8_t *f_name1 = "/file1.dat", byte = 'z';
	uint8_t buf[FS_BLOCK_SIZE];
	user_id user = 1;

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
//	PRINT("\ncreat start");
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
	f_id1 = GET_FILE_ID_BY_FD(fd1);

	/* write to inode file data1*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd1, buf, INODE_FILE_DATA_SIZE)));
	tid = getTidByFileId(f_id1);
	/* write to file 1 until we exhaust all spare pages, and not enough free pages left*/
	while(! (IS_MIN_FREE_PAGES_REACHED() && TRANSACTION_GET_VOTS_COUNT(tid) <= TRANSACTION_COMMIT_MIN_VOTS)){
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}
//	PRINT("\nwrites success");
//	PRINT("\nabout to do last commit!!!");
	/* verify close fails*/
	res = fsync(fd1);
	VERIFY(IS_NEGATIVE(res));

	return 1;
}
