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

/** @file closeTests.c  */
#include <test/fs/closeTests.h>
#include <test/fs/testsHeader.h>

void runAllcloseTests(){

	RUN_TEST(close,1);
	RUN_TEST(close,2);
	RUN_TEST(close,3);
	RUN_TEST(close,4);
	RUN_TEST(close,5);
	RUN_TEST(close,6);

	/* test not relevant - no temporary commit*/
//	RUN_TEST(close,7);
//	RUN_TEST(close,8);
}

/**
 * @brief
 * init close test
 *
 */
void init_closeTest(){
	if(nandInit())
		return;

	init_flash();
	initializeRamStructs();
	handleNoFs();
	init_fsbuf(fs_buffer);
}

/**
 * @brief
 * tear down close test
 *
 */
void tearDown_closeTest(){
	init_flash();

	nandTerminate();
	init_fsbuf(fs_buffer);
	initializeRamStructs();
}

/**
 * @brief
 * open file for writing and close it.
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest1(){
	int32_t fd, f_size, f_id, i;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'k';

	/* open file*/
	fd = creat(f_name, 0);
	VERIFY(IS_FD_LEGAL(fd));
	/* write to file*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, buf, INODE_FILE_DATA_SIZE)));
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));

	f_size = getFileSize(GET_FILE_ID_BY_FD(fd));
	f_id   = GET_FILE_ID_BY_FD(fd);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE, f_size));
//	assert(0);
//	PRINT("\nabout to close");
	VERIFY(!close(fd));

	/* verify no dirty cache entries left*/
	for(i=0; i< FS_CACHE_BUFFERS;i++){
		VERIFY(!CACHE_IS_DIRTY(i));
		VERIFY(IS_EMPTY_TID(CACHE_GET_TID(i)));
	}

//	PRINT("\nclose success");
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nfentries vnodes success");
//	PRINT_MSG_AND_NUM("\nfile size=",getFileSize(GET_FILE_ID_BY_FD(fd)));
//	PRINT_MSG_AND_NUM("\nexpected size=",f_size);

//	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
//	VERIFY(!getInode(fs_buffer, 2, ino_addr));
//	PRINT_MSG_AND_NUM("\nread file size=",INODE_GET_NBYTES(CAST_TO_INODE(fs_buffer)));

	VERIFY(COMPARE(getFileSize(f_id), f_size));
//	PRINT("\nfile size success");
	VERIFY(COMPARE(CACHE_GET_DIRTY_COUNT(), 0));

	return 1;
}

/**
 * @brief
 * close illegal fd and verify failure
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest2(){
	int32_t fd = FS_MAX_OPEN_FILES;

	VERIFY(IS_NEGATIVE(close(fd)));

	return 1;
}

/**
 * @brief
 * open file using creat, and close it.
 * verify no physical writes
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest3(){
	int32_t fd, offset;
	uint8_t *f_name = "/file1.dat";
	int32_t i;

	/* open file*/
	fd = creat(f_name, 0);
	VERIFY(IS_FD_LEGAL(fd));

	offset = GET_RECLAIMED_OFFSET();
	VERIFY(!close(fd));

	/* verify no dirty cache entries left*/
	for(i=0; i< FS_CACHE_BUFFERS;i++){
		VERIFY(!CACHE_IS_DIRTY(i));
		VERIFY(IS_EMPTY_TID(CACHE_GET_TID(i)));
	}
//	PRINT_MSG_AND_NUM("\nexpected offset=",offset);
//	PRINT_MSG_AND_NUM("\nrec offset=",GET_RECLAIMED_OFFSET());
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), offset));

	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));

	VERIFY(COMPARE(CACHE_GET_DIRTY_COUNT(), 0));
	return 1;
}

/**
 * @brief
 * open file for writing, write and close.
 * verify transaction was commited properly
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest4(){
	int32_t fd, f_size, f_id, offset, total_free, i;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'k';
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);

	init_logical_address(log_addr);
	init_logical_address(ino_addr);

	/* open file*/
	fd = creat(f_name, 0);
	VERIFY(IS_FD_LEGAL(fd));

	/* write to file (til 1st direct entry)*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, buf, INODE_FILE_DATA_SIZE)));
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));

	f_size = getFileSize(GET_FILE_ID_BY_FD(fd));
	f_id   = GET_FILE_ID_BY_FD(fd);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE, f_size));
	offset = GET_RECLAIMED_OFFSET();
	total_free = calcTotalFreePages();
//	assert(0);
//	L("about to close \n\n\n\n\n\n");
	VERIFY(!close(fd));
//	PRINT("\nclose success");
	verifyFentriesVnodesEmpty(FD_EMPTY);
//	PRINT("\nfentries vnodes success");
//	PRINT_MSG_AND_NUM("\nfile size=",getFileSize(GET_FILE_ID_BY_FD(fd)));
//	PRINT_MSG_AND_NUM("\nexpected size=",f_size);

	VERIFY(!getInode(fs_buffer, f_id, ino_addr));
//	PRINT("\ngetInode success");

	for(i=0;i<INODE_FILE_DATA_SIZE;i++){
		VERIFY(COMPARE(fs_buffer[i], byte));
	}
//	PRINT("\nverified file data");
	VERIFY(verifyInode(ino_ptr, f_id, FTYPE_FILE, 2, f_size));
//	PRINT("\nverified inode");
//	PRINT("\nverifying direct entries");
	INODE_GET_DIRECT(ino_ptr,0,log_addr);
	init_fsbuf(buf);
	VERIFY(!fsReadBlock(log_addr,
						buf,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));

	for(i=0;i<FS_BLOCK_SIZE;i++){
		VERIFY(COMPARE(buf[i], byte));
	}

//	PRINT("\ninode data success");
	for(i=1; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\ndirect entries success");
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nindirect entry success");
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\ndouble entry success");
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nfile size success");
//	PRINT("\nverified entries");
	/* verify reclaimed offset has changed as expected:
	 * old offset
	 * + 6 (one block, new inode, new inode0, vots page, + 2* checkpoint) */
//	 PRINT_MSG_AND_NUM("\noffset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nexpected offset=",offset+4+2*CALCULATE_IN_PAGES(sizeof(filesystem_t)+SEQ_CHECKPOINT_SIZE));

	if(!IS_CACHE_EMPTY()){
	VERIFY(COMPARE(offset+4+2*CALCULATE_IN_PAGES(sizeof(filesystem_t)+SEQ_CHECKPOINT_SIZE), GET_RECLAIMED_OFFSET()));

	/* verify free pages - should change by 1
	 * since the commit writes pages which substitute other pages, except the cached
	 * first direct entry*/
//	L("expected total_free %d, actual %d", total_free, calcTotalFreePages());
	VERIFY(COMPARE(total_free-1, calcTotalFreePages()));
	}
	else{
		VERIFY(COMPARE(offset+3+2*CALCULATE_IN_PAGES(sizeof(filesystem_t)+SEQ_CHECKPOINT_SIZE), GET_RECLAIMED_OFFSET()));

		/* verify free pages - shouldn't change
		 * since the commit writes pages which substitute other pages*/
		VERIFY(COMPARE(total_free, calcTotalFreePages()));
	}
	/* verify no dirty cache entries left*/
	VERIFY(COMPARE(CACHE_GET_DIRTY_COUNT(), 0));
	for(i=0; i< FS_CACHE_BUFFERS;i++){
		VERIFY(!CACHE_IS_DIRTY(i));
		VERIFY(IS_EMPTY_TID(CACHE_GET_TID(i)));
	}

	return 1;
}

/**
 * @brief
 * open file for writing, write. try to open it again for writing (should fail)
 * close file, and try again to open for writing. verify success
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest5(){
	int32_t fd, f_size, f_id, offset, total_free, i, j;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'k';
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);

	init_logical_address(log_addr);
	init_logical_address(ino_addr);

	/* open file*/
	fd = creat(f_name, 0);
	VERIFY(IS_FD_LEGAL(fd));
	/* write to file*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, buf, INODE_FILE_DATA_SIZE)));
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));

	f_size = getFileSize(GET_FILE_ID_BY_FD(fd));
	f_id   = GET_FILE_ID_BY_FD(fd);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE, f_size));

	VERIFY(IS_NEGATIVE(open(f_name, NANDFS_O_WRONLY, 0)));
	/* 1st close */
	VERIFY(!close(fd));
//	PRINT("\n\n\n\n1st close success");
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	init_logical_address(ino_addr);
	VERIFY(!getInode(fs_buffer, f_id, ino_addr));
//	PRINT_MSG_AND_NUM("\nafter first close ino addr=", logicalAddressToPhysical(ino_addr));

	/* try to re-open file for writing */
	fd = open(f_name, NANDFS_O_WRONLY, 0);
	VERIFY(COMPARE(f_size, lseek(fd, 0, FS_SEEK_END)));
	VERIFY(!IS_NEGATIVE(fd));

	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));

	VERIFY(COMPARE(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE, f_size));
	offset = GET_RECLAIMED_OFFSET();
	total_free = calcTotalFreePages();
//	assert(0);
//	PRINT("\n\n\n\nabout to close");
	/* 2nd close */
	VERIFY(!close(fd));
//	PRINT("\n2nd close success");
	init_logical_address(ino_addr);
	VERIFY(!getInode(fs_buffer, f_id, ino_addr));
//	PRINT_MSG_AND_NUM("\nafter 2nd close ino addr=", logicalAddressToPhysical(ino_addr));

	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nfentries vnodes success");
//	PRINT_MSG_AND_NUM("\nfile size=",getFileSize(GET_FILE_ID_BY_FD(fd)));
//	PRINT_MSG_AND_NUM("\nexpected size=",f_size);
//	PRINT("\ngetInode success");

	for(i=0;i<INODE_FILE_DATA_SIZE;i++){
		VERIFY(COMPARE(fs_buffer[i], byte));
	}
//	PRINT("\ninode file data success");
	VERIFY(verifyInode(ino_ptr, f_id, FTYPE_FILE, 3, f_size+FS_BLOCK_SIZE));
//	PRINT("\ninode data success");
//	PRINT("\nverifying direct entries");
	for(j=0;j<2;j++){
		INODE_GET_DIRECT(ino_ptr,j,log_addr);
		init_fsbuf(buf);
		VERIFY(!fsReadBlock(log_addr,
							buf,
							TID_EMPTY,
							j,
							IS_CACHED_ADDR(log_addr)));

		for(i=0;i<FS_BLOCK_SIZE;i++){
			VERIFY(COMPARE(buf[i], byte));
		}
	}

	for(i=2; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\ndirect entries success");
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nindirect entry success");
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\ndouble entry success");
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified entries");
	/* verify reclaimed offset has changed as expected:
	 * old offset
	 * + 3+2cp (flushed cached direct entry, new inode, new inode0, vots page, 2*checkpoint) */
//	 PRINT_MSG_AND_NUM("\noffset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nexpected offset=",offset+3+2*CALCULATE_IN_PAGES(sizeof(filesystem_t)+SEQ_CHECKPOINT_SIZE));
	if(!IS_CACHE_EMPTY()){
	VERIFY(COMPARE(offset+4+2*CALCULATE_IN_PAGES(sizeof(filesystem_t)+SEQ_CHECKPOINT_SIZE), GET_RECLAIMED_OFFSET()));

	/* verify free pages - should change by 1
	 * since the commit writes pages which substitute other pages, except the cached
	 * first direct entry*/
//	L("expected total_free %d, actual %d", total_free, calcTotalFreePages());
	VERIFY(COMPARE(total_free-1, calcTotalFreePages()));
	}
	else{
	//	 PRINT_MSG_AND_NUM("\noffset=",GET_RECLAIMED_OFFSET());
	//	PRINT_MSG_AND_NUM("\nexpected offset=",offset+3+2*CALCULATE_IN_PAGES(sizeof(filesystem_t)+SEQ_CHECKPOINT_SIZE));
		VERIFY(COMPARE(offset+3+2*CALCULATE_IN_PAGES(sizeof(filesystem_t)+SEQ_CHECKPOINT_SIZE), GET_RECLAIMED_OFFSET()));

		/* verify free pages - shouldn't change
		 * since the commit writes pages which substitute other pages*/
		VERIFY(COMPARE(total_free, calcTotalFreePages()));
	}
	/* verify no dirty cache entries left*/
	VERIFY(COMPARE(CACHE_GET_DIRTY_COUNT(), 0));
	for(i=0; i< FS_CACHE_BUFFERS;i++){
		VERIFY(!CACHE_IS_DIRTY(i));
		VERIFY(IS_EMPTY_TID(CACHE_GET_TID(i)));
	}

	return 1;
}



/**
 * @brief
 * open 1 file for reading, open 2 files, write to them, and close them both.
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest6(){
	int32_t fd1, f_id1, offset, i, j;
	int32_t fd2, f_id2;
	int32_t fd3;
	uint8_t *f_name1 = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'k';
	uint8_t *f_name2 = "/file2.dat", *f_name3 = "/file3.dat";
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	user_id uid = 1;

	if(FS_MAX_N_TRANSACTIONS < 2){
		return 1;
	}

	init_logical_address(log_addr);
	init_logical_address(indirect_addr);
	init_logical_address(ino_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	/* open first file for reading.*/
	/* first create and close*/
	fd3 = creat(f_name3, 0);
	offset = GET_RECLAIMED_OFFSET();
	VERIFY(!close(fd3));
	L("b4 verifyFentriesVnodesEmpty() data first byte is %x", TRANSACTION_GET_DATA_BUF_PTR(0)[0]);
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	L("b4 verifyTransactionsEmpty() data first byte is %x", TRANSACTION_GET_DATA_BUF_PTR(0)[0]);
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(COMPARE(offset,GET_RECLAIMED_OFFSET()));

	/* oen for reading*/
	fd3 = open(f_name3, NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd3));

	/* open file 1*/
	fd1 = creat(f_name1, 0);
	VERIFY(IS_FD_LEGAL(fd1));

	/* open file 2*/
	fd2 = creat(f_name2, 0);
	VERIFY(IS_FD_LEGAL(fd2));

	f_id1 = GET_FILE_ID_BY_FD(fd1);
	f_id2 = GET_FILE_ID_BY_FD(fd2);

	VERIFY(verifyOpenFileEntry(fd1, CREAT_FLAGS, 0, uid, 1));
	VERIFY(verifyOpenFileEntry(fd2, CREAT_FLAGS, 0, uid, 2));
	VERIFY(verifyOpenFileEntry(fd3, NANDFS_O_RDONLY, 0, uid, 0));

	/* write to files*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd1, buf, INODE_FILE_DATA_SIZE)));
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd2, buf, INODE_FILE_DATA_SIZE)));
	VERIFY(verifyOpenFileEntry(fd1, CREAT_FLAGS, INODE_FILE_DATA_SIZE, uid, 1));
	VERIFY(verifyOpenFileEntry(fd2, CREAT_FLAGS, INODE_FILE_DATA_SIZE, uid, 2));

	/* close first file */
	VERIFY(!close(fd1));
	VERIFY(verifyOpenFileEntry(fd3, NANDFS_O_RDONLY, 0, uid, 0));
	VERIFY(verifyOpenFileEntry(fd2, CREAT_FLAGS, INODE_FILE_DATA_SIZE, uid, 2));

	/* verify all other file entries and vnodes are empty*/
	for(i=0; i<FS_MAX_OPEN_FILES; i++){
		if(i==fd3 || i== fd2)
			continue;

		VERIFY(verifyOpenFileEntryEmpty(i));
	}

	for(i=0; i< FS_MAX_VNODES;i++){
		if(i== OPEN_FILE_GET_VNODE(fd3) || i== OPEN_FILE_GET_VNODE(fd2)){
			continue;
		}

		VERIFY(verifyVnodeEmpty(i));
	}

//	PRINT("\nfirst close success");
	/* verify first file data */
	VERIFY(!getInode(fs_buffer, f_id1, ino_addr));
	VERIFY(verifyInode(ino_ptr, f_id1, FTYPE_FILE, 1, INODE_FILE_DATA_SIZE));
//	PRINT("\ninode 1 verified");

	for(i=0; i<INODE_FILE_DATA_SIZE;i++){
		VERIFY(COMPARE(fs_buffer[i], byte));
	}

	for(i=0; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr,i,log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr,log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr,log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr,log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

	init_fsbuf(fs_buffer);
	init_logical_address(ino_addr);

	/* write to second file, until first indirectly mapped block */
	for(i=0; i <DIRECT_INDEX_ENTRIES+1; i++){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd2, buf, FS_BLOCK_SIZE)));
	}
//	PRINT("\nfirst file verified");
	VERIFY(!close(fd2));
//	PRINT("\nsecond close success");
	/* verify second file data */
	VERIFY(!getInode(fs_buffer, f_id2, ino_addr));
	VERIFY(verifyInode(ino_ptr, f_id2, FTYPE_FILE, 1+DIRECT_INDEX_ENTRIES+1, INODE_FILE_DATA_SIZE+(DIRECT_INDEX_ENTRIES+1)*FS_BLOCK_SIZE));

	for(i=0; i<INODE_FILE_DATA_SIZE;i++){
		VERIFY(COMPARE(fs_buffer[i], byte));
	}

	for(i=0; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr,i,log_addr);
		VERIFY(!IS_ADDR_EMPTY(log_addr));

		VERIFY(!fsReadBlock(log_addr,
							fs_buffer,
							TID_EMPTY,
							i,
							IS_CACHED_ADDR(log_addr)));
		for(j=0; j<FS_BLOCK_SIZE; j++){
			VERIFY(COMPARE(fs_buffer[i], byte));
		}

		VERIFY(!fsReadBlock(ino_addr,
							fs_buffer,
							TID_EMPTY,
							0,
							IS_CACHED_ADDR(ino_addr)));
	}
//	PRINT("\ndirect entries verified");
	INODE_GET_DOUBLE(ino_ptr,log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr,log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

	INODE_GET_INDIRECT(ino_ptr,indirect_addr);
	VERIFY(!IS_ADDR_EMPTY(indirect_addr));
	VERIFY(!fsReadBlock(indirect_addr,
						fs_buffer,
						TID_EMPTY,
						INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(indirect_addr)));

	BLOCK_GET_INDEX(fs_buffer, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(log_addr)));
	for(j=0; j<FS_BLOCK_SIZE; j++){
		VERIFY(COMPARE(fs_buffer[i], byte));
	}
//	PRINT("\nsecond file verified");
	init_fsbuf(fs_buffer);
	init_logical_address(ino_addr);

	VERIFY(verifyTransactionsEmpty(TID_EMPTY));

	offset = GET_RECLAIMED_OFFSET();
	VERIFY(!close(fd3));
	VERIFY(COMPARE(offset, GET_RECLAIMED_OFFSET()));

	/* verify no dirty cache entries left*/
	VERIFY(COMPARE(CACHE_GET_DIRTY_COUNT(), 0));
	for(i=0; i< FS_CACHE_BUFFERS;i++){
		VERIFY(!CACHE_IS_DIRTY(i));
		VERIFY(IS_EMPTY_TID(CACHE_GET_TID(i)));
	}

	return 1;
}

/**
 * @brief
 * write to file until it exhausts all free pages. then try to close it.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest7(){
	int32_t fd1, f_id1, f_size, count = 0, expected_size, frees;
	uint8_t *f_name1 = "/file1.dat", byte = 'z';
	uint8_t buf[FS_BLOCK_SIZE];
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	user_id user = 1;

	init_logical_address(ino_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
//	PRINT("\ncreat start");
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
	f_id1 = GET_FILE_ID_BY_FD(fd1);
//	PRINT("\ncreat success");
	/* write to inode file data1*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd1, buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\nwrite success");
//
//	{
//		/* write to file 1 until we exhaust all spare pages*/
//			frees = calcTotalFreePages();
//			while(!(IS_MIN_FREE_PAGES_REACHED())){
//				VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
//		//		if(count%1000 == 0) {
//		//			PRINT_MSG_AND_NUM("\nwrote pages count=", count);
//		//		}
//				count++;
//		    }
//
//		    f_size = getFileSize(f_id1);
//		    expected_size = count*FS_BLOCK_SIZE+INODE_FILE_DATA_SIZE;
//		    VERIFY(COMPARE(f_size, expected_size));
//
//		//    PRINT("\n\n\nabout to close");
//		    VERIFY(!close(fd1));
//		    return 1;
//	}

	/* write to file 1 until we exhaust all spare pages*/
//	frees = calcTotalFreePages();
//	L("about to write. frees %d FS_MIN_FREE_PAGES-5 %d", frees, FS_MIN_FREE_PAGES-5);
//	while((FS_TOTAL_FREE_PAGES > FS_MIN_FREE_PAGES)){
	while(calcTotalFreePages() >= (FS_MIN_FREE_PAGES)){
//		L("write #%d", count);
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
//		if(count%1000 == 0) {
//			PRINT_MSG_AND_NUM("\nwrote pages count=", count);
//		}
//		count++;
    }
	L("done writing. frees %d ", frees);
    f_size = getFileSize(f_id1);
    expected_size = count*FS_BLOCK_SIZE+INODE_FILE_DATA_SIZE;
    VERIFY(COMPARE(f_size, expected_size));

    L("about to close.  FS_TOTAL_FREE_PAGES %d (obs %d frees %d dirtys %d) \n\n\n\n\n\n", FS_TOTAL_FREE_PAGES, GET_OBS_COUNT(), GET_FREE_COUNTER(), CACHE_GET_DIRTY_COUNT());
    L("transction vots are %d", TRANSACTION_GET_VOTS_COUNT(0));
    frees = FS_TOTAL_FREE_PAGES+TRANSACTION_GET_VOTS_COUNT(0);
    VERIFY(!close(fd1));
    L("close success. FS_TOTAL_FREE_PAGES %d (obs %d frees %d dirtys %d) \n\n\n\n\n\n", FS_TOTAL_FREE_PAGES, GET_OBS_COUNT(), GET_FREE_COUNTER(), CACHE_GET_DIRTY_COUNT());
#ifndef TEMP_COMMIT
    /* 23.4.08 */
    VERIFY(verifyTransactionsEmpty(TID_EMPTY));
    L("expected frees %d, FS_TOTAL_FREE_PAGES %d", frees, FS_TOTAL_FREE_PAGES);
    VERIFY(COMPARE(frees, FS_TOTAL_FREE_PAGES)); /* this should be tested!!*/
#else

    VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
    VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//    PRINT("\nfentries etc empty success");

    VERIFY(!getInode(fs_buffer, f_id1, ino_addr));
    f_size = INODE_GET_NBYTES(ino_ptr);
//     PRINT_MSG_AND_NUM("\n expected_size=",expected_size);
//    PRINT_MSG_AND_NUM("\n inode file size=",f_size);
    VERIFY(COMPARE(f_size, expected_size));

    f_size = getFileSize(f_id1);
//    PRINT_MSG_AND_NUM("\n expected_size=",expected_size);
//    PRINT_MSG_AND_NUM("\n f_size=",f_size);
    VERIFY(COMPARE(f_size, expected_size));
//    PRINT("\nfile size success");
    /* verify file data */
    init_fsbuf(buf);
    VERIFY(!readFileBlock(buf, f_id1, 0, ino_addr, TID_EMPTY));

	for(i=0; i<INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(buf[i], byte));
	}
//	PRINT("\ninode file data success");
	for(j=INODE_FILE_DATA_SIZE; j<expected_size; j+=FS_BLOCK_SIZE){
		VERIFY(!readFileBlock(buf, f_id1, j, ino_addr, TID_EMPTY));

		for(i=0; i<FS_BLOCK_SIZE; i++){
			VERIFY(COMPARE(buf[i], byte));
		}
//		PRINT_MSG_AND_NUM("\n file data success in offset ", j);
	}
#endif
	return 1;
}

/**
 * @brief
 * write to file until it exhausts all free pages, and doesn't have enough free vots.
 * then try to close it. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest8(){
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
//	PRINT("\nstart writing");
//	int i=0;
	while(! (IS_MIN_FREE_PAGES_REACHED() && TRANSACTION_GET_VOTS_COUNT(tid) <= TRANSACTION_COMMIT_MIN_VOTS)){
//		PRINT_MSG_AND_NUM("\nwrite ", i);
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
//		i++;
	}
//	PRINT("\nwrites success");
//	PRINT("\nabout to close");
	/* verify close fails*/
	res = close(fd1);
//	PRINT_MSG_AND_NUM("\nres=",res);
	VERIFY(IS_NEGATIVE(res));

	return 1;
}
