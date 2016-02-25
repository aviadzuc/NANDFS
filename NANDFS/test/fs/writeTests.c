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

/** @file writeTests.c  */
#include <test/fs/writeTests.h>
#include <test/fs/testsHeader.h>

void runAllwriteTests(){

//	RUN_TEST(write,1);
//	RUN_TEST(write,2);
//	RUN_TEST(write,3);
//	RUN_TEST(write,4);
	RUN_TEST(write,5);
	RUN_TEST(write,6);
	RUN_TEST(write,7);
	RUN_TEST(write,8);
	RUN_TEST(write,9);
	/* tests that rely on lseek, close*/
	RUN_TEST(write,10);
	RUN_TEST(write,11);
	RUN_TEST(write,12);
	RUN_TEST(write,14);
	RUN_TEST(write,16);
	RUN_TEST(write,17);

	RUN_TEST(write,21);
	RUN_TEST(write,22);

	/* tests that rely on lseek, close, read*/
	RUN_TEST(write,26);

#if 0
	/* tests that are no longer relevant (no temporary commit)*/
	RUN_TEST(write,20);
	RUN_TEST(write,23);
	RUN_TEST(write,24);
	RUN_TEST(write,25);
#endif
}

/**
 * @brief
 * init write test
 *
 */
void init_writeTest(){
	if(nandInit())
		return;

	init_flash();
	initializeRamStructs();

	int32_t i;
	/* mark eu's as bad*/
//	for(i=0; i< 4; i++){
//		PRINT_MSG_AND_NUM("\nmark first EU as bad in slot ", i*(SEQ_N_SLOTS_COUNT/4));
//		markEuAsMockBad(CALC_ADDRESS(i*(SEQ_N_SLOTS_COUNT/4), 0, 0));
//	}
//	PRINT_MSG_AND_NUM("\nmark first EU as bad in slot ", SEQ_N_SLOTS_COUNT-1);
//	markEuAsMockBad(CALC_ADDRESS(SEQ_N_SLOTS_COUNT-1, 0, 0));
	handleNoFs();
	init_fsbuf(fs_buffer);
}

/**
 * @brief
 * tear down write test
 *
 */
void tearDown_writeTest(){
	init_flash();
	init_file_entries();
	init_vnodes();
	init_transactions();

	nandTerminate();
	init_fsbuf(fs_buffer);
	initializeRamStructs();
}

/**
 * @brief
 * write to a new file from offset 0 at length smaller than inode file data size. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest1(){
	int32_t tid, fd, res, f_id;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	int32_t buf_size = INODE_FILE_DATA_SIZE-50, write_size = buf_size-50;
	uint8_t write_buf[INODE_FILE_DATA_SIZE-50];
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	user_id user = 1;
	inode_t *ino_ptr;

	init_logical_address(log_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	fsMemset(write_buf, byte+1, buf_size);
	fsMemset(write_buf, byte, write_size);

	/* create file */
	fd = open(f_name, CREAT_FLAGS, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\n\n\n\n\nopen success");
	f_id = GET_FILE_ID_BY_FD(fd);
	tid = getTidByFileId(f_id);

	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	/* write to file*/
	res = write(fd, write_buf, write_size);
	tid = getTidByFileId(f_id);
//	assert(0);
	L("res %d, write_size %d", res, write_size);
	VERIFY(COMPARE(res, write_size))
//	PRINT("\nwrite success");
	VERIFY(!IS_ADDR_EMPTY(TRANSACTION_GET_INO_ADDR_PTR(tid)));
//	PRINT("\nino addr not empty");
	/* verify open file entry */
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), write_size));

	/* verify reclaimed offset increased
	 * and that the old one is the new inode address*/
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), GET_LOGICAL_OFFSET(log_addr)));
//	PRINT("\nrec offset comparison success");
//	VERIFY(COMPARE(GET_LOGICAL_OFFSET(TRANSACTION_GET_INO_ADDR_PTR(tid)), GET_LOGICAL_OFFSET(log_addr)));
////	PRINT("\nino addr offset comparison success");
//	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(TRANSACTION_GET_INO_ADDR_PTR(tid)), GET_LOGICAL_SEGMENT(log_addr)));
//	PRINT("\ninode address success");
//	PRINT_MSG_AND_NUM("\ninode address=",logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));

//	VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),fs_buffer ));
//	for(i=0;i<FS_BLOCK_SIZE;i++){
//		if(0xff == fs_buffer[i]){
//			continue;
//		}
//		PRINT_MSG_AND_NUM("\nbyte ", i);
//		PRINT_MSG_AND_NUM(". ", fs_buffer[i]);
//	}
//	assert(0);
	/* verify file size */
//	PRINT_MSG_AND_NUM("\nafter write file size=",getFileSize(f_id));
	VERIFY(COMPARE(getFileSize(f_id), write_size));

	fsMemcpy(fs_buffer, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE);
//	VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid), fs_buffer));
//	PRINT("\nfile size success");
	/* verify file data, and that the physical block data after written data
	 * contains empty bytes */
	for(i=0; i<write_size; i++){
		VERIFY(COMPARE(fs_buffer[i], byte))
	}

	for(;i<INODE_FILE_DATA_SIZE;i++){
		VERIFY(COMPARE(fs_buffer[i], 0xff))
	}
//	PRINT("\nwritten data success");
	/* verify inode details */
	ino_ptr = CAST_TO_INODE(fs_buffer);
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), write_size));
//	PRINT("\nnbytes success");
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1));
//	PRINT("\nnblocks success");
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), f_id));
//	PRINT("\nf_id success");
//	PRINT_MSG_AND_NUM("\nf_type=",INODE_GET_FILE_TYPE(ino_ptr));
	VERIFY(COMPARE(INODE_GET_FILE_TYPE(ino_ptr), FTYPE_FILE));
//	PRINT("\ninode data success");
	for(i=0; i<DIRECT_INDEX_ENTRIES;i++){
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

	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write to a new file from offset 0 so that data is stored in direct entry.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest2(){
	int32_t tid, fd, res, f_id, flags = CREAT_FLAGS;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	int32_t buf_size = INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE, write_size = buf_size-50;
	uint8_t write_buf[INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE], buf[FS_BLOCK_SIZE];
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	user_id user = 1;
	inode_t *ino_ptr;

	init_logical_address(log_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	fsMemset(write_buf, byte+1, buf_size);
	fsMemset(write_buf, byte, write_size);

	/* create file */
	fd = open(f_name, flags, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nopen success");
	f_id = GET_FILE_ID_BY_FD(fd);

	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	/* write to file*/
	res = write(fd, write_buf, write_size);
	tid = getTidByFileId(f_id);
	VERIFY(COMPARE(res, write_size))
//	PRINT("\nwrite success");

	/* verify open file entry */
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), write_size));

	if(!IS_CACHE_EMPTY()){
	/* verify reclaimed offset did not increase
	 * and everything was cached*/
//	 PRINT_MSG_AND_NUM("\nrec offset=",GET_RECLAIMED_OFFSET());
//	 PRINT_MSG_AND_NUM("\nold rec offset=",GET_LOGICAL_OFFSET(log_addr));
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), GET_LOGICAL_OFFSET(log_addr)));
	}
	else{
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), GET_LOGICAL_OFFSET(log_addr)+2));
	}
//	PRINT("\nrec offset comparison success");
//	VERIFY(COMPARE(GET_LOGICAL_OFFSET(TRANSACTION_GET_INO_ADDR_PTR(tid)), GET_LOGICAL_OFFSET(log_addr)));
//	PRINT("\nino addr offset comparison success");
//	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(TRANSACTION_GET_INO_ADDR_PTR(tid)), GET_LOGICAL_SEGMENT(log_addr)));
//	PRINT("\ninode address success");
//	PRINT_MSG_AND_NUM("\ninode address=",logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));

	/* verify file size */
//	PRINT_MSG_AND_NUM("\nafter write file size=",getFileSize(f_id));
	VERIFY(COMPARE(getFileSize(f_id), write_size));

	fsMemcpy(fs_buffer, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE);
//	VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid), fs_buffer));
//	PRINT("\nfile size success");
	/* verify file data, and that the physical block data after written data
	 * contains empty bytes */
	for(i=0; i<INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(fs_buffer[i], byte))
	}

//	PRINT("\nwritten data success");
	/* verify inode details */
	ino_ptr = CAST_TO_INODE(fs_buffer);
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE));
//	PRINT("\nnbytes success");
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1));
//	PRINT("\nnblocks success");
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), f_id));
//	PRINT("\nf_id success");
//	PRINT_MSG_AND_NUM("\nf_type=",INODE_GET_FILE_TYPE(ino_ptr));
	VERIFY(COMPARE(INODE_GET_FILE_TYPE(ino_ptr), FTYPE_FILE));
//	PRINT("\ninode data success");

	/* verify first two blocks in the transaction inode*/
	ino_ptr = CAST_TO_INODE(TRANSACTION_GET_INDIRECT_PTR(tid));
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));

	if(!IS_CACHE_EMPTY()){
	VERIFY(IS_CACHED_ADDR(log_addr));
	fsMemcpy(buf, CACHE_GET_BUF_PTR(CACHE_GET_CID_FROM_ADDR(log_addr)), FS_BLOCK_SIZE);
	}
	else{
	VERIFY(!fsReadBlockSimple(log_addr, buf));
	}

	for(i=0; i< FS_BLOCK_SIZE; i++){
//		PRINT_MSG_AND_NUM("\n", i);
//		PRINT_MSG_AND_NUM(". ", buf[i]);
		VERIFY(COMPARE(buf[i], byte));
	}
//	PRINT("\nfirst direct entry success");
	INODE_GET_DIRECT(ino_ptr, 1, log_addr);
	if(!IS_CACHE_EMPTY()){
	VERIFY(IS_CACHED_ADDR(log_addr));
	fsMemcpy(buf, CACHE_GET_BUF_PTR(CACHE_GET_CID_FROM_ADDR(log_addr)), FS_BLOCK_SIZE);
	}
	else{
	VERIFY(!fsReadBlockSimple(log_addr, buf));
	}

	for(i=0; i< write_size-FS_BLOCK_SIZE-INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(buf[i], byte));
	}
//	PRINT("\nsecond direct entry data success");
	for(; i< FS_BLOCK_SIZE; i++){
		VERIFY(COMPARE(buf[i], 0xff));
	}

//	PRINT("\nfirst 2 blocks success");
	/* verify other firec t entries are empty*/
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

	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write to a new file from offset 0 so that data is stored in indirect block
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest3(){
	int32_t tid, fd, res, f_id, indirect_entry = 3;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	uint8_t write_buf[FS_BLOCK_SIZE], buf[FS_BLOCK_SIZE];
	uint32_t i, j;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	int32_t write_size = INDIRECT_DATA_OFFSET+indirect_entry*FS_BLOCK_SIZE;
	user_id user = 1;
	inode_t *ino_ptr;

	init_logical_address(log_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	fsMemset(write_buf,byte, FS_BLOCK_SIZE);

	/* create file */
	fd = open(f_name, CREAT_FLAGS, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nopen success");
	f_id = GET_FILE_ID_BY_FD(fd);

	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	/* write to inode file data*/
	res = write(fd, write_buf, INODE_FILE_DATA_SIZE);
	tid = getTidByFileId(f_id);
	VERIFY(COMPARE(res, INODE_FILE_DATA_SIZE));

	/* verify open file entry */
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), INODE_FILE_DATA_SIZE));

	/* write direct entries*/
	for(i=0; i< DIRECT_INDEX_ENTRIES;i++){
		/* write to file*/
//		L("");
		res = write(fd, write_buf, FS_BLOCK_SIZE);
		VERIFY(COMPARE(res, FS_BLOCK_SIZE));
	}

	/* verify open file entry */
//	L("");
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), INODE_FILE_DATA_SIZE+DIRECT_INDEX_ENTRIES*FS_BLOCK_SIZE));

	/* write indirect entries*/
	for(i=0; i< indirect_entry;i++){
		/* write to file*/
//		L("i %d", i);
		res = write(fd, write_buf, FS_BLOCK_SIZE);
		VERIFY(COMPARE(res, FS_BLOCK_SIZE));
		return 1;
	}

	/* verify open file entry */
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), write_size));
//	PRINT("\nwrite success");

	/* verify file size */
//	PRINT_MSG_AND_NUM("\nafter write file size=",getFileSize(f_id));
	VERIFY(COMPARE(getFileSize(f_id), write_size));
	VERIFY(IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	fsReadCache(TRANSACTION_GET_INO_ADDR_PTR(tid), fs_buffer);
//	L("file size success");
	/* verify file data, and that the physical block data after written data
	 * contains empty bytes */
	for(i=0; i<INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(fs_buffer[i], byte))
	}

//	PRINT("\nwritten inode file data success");
	/* verify inode details */
//	L("");
	ino_ptr = CAST_TO_INODE(fs_buffer);
//	PRINT_MSG_AND_NUM("\nflash nbytes=",INODE_GET_NBYTES(ino_ptr));
//	PRINT_MSG_AND_NUM("\nINDIRECT_DATA_OFFSET=",INDIRECT_DATA_OFFSET);
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INDIRECT_DATA_OFFSET));
//	PRINT("\nnbytes success");
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1+DIRECT_INDEX_ENTRIES));
//	PRINT("\nnblocks success");
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), f_id));
//	PRINT("\nf_id success");
//	PRINT_MSG_AND_NUM("\nf_type=",INODE_GET_FILE_TYPE(ino_ptr));
	VERIFY(COMPARE(INODE_GET_FILE_TYPE(ino_ptr), FTYPE_FILE));
//	PRINT("\ninode data success");

	/* verify data offset, and blocks written dor this indirect block*/
//	L("");
	VERIFY(COMPARE(TRANSACTION_GET_FILE_OFFSET(tid), INDIRECT_DATA_OFFSET));
	VERIFY(COMPARE(TRANSACTION_GET_BLOCKS_ALLOCS(tid), indirect_entry));
	VERIFY(COMPARE(TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid), indirect_entry*FS_BLOCK_SIZE));
//	PRINT("\ntransaction data success");
	/* verify first direct entries in indirect block*/

	for(j=0; j< indirect_entry; j++){
		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), j, log_addr);
		VERIFY(IS_CACHED_ADDR(log_addr));
		fsReadCache(log_addr, buf);

		for(i=0; i< FS_BLOCK_SIZE; i++){
	//		PRINT_MSG_AND_NUM("\n", i);
	//		PRINT_MSG_AND_NUM(". ", buf[i]);
			VERIFY(COMPARE(buf[i], byte));
		}
	}

	for(; j< LOG_ADDRESSES_PER_BLOCK; j++){
		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), j, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}



/**
 * @brief
 * write to a new file from offset 0 so that data is stored in double block offset
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest4(){
	int32_t tid, fd, res, f_id, indirect_entry = 22, double_entry = 2;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	uint8_t write_buf[FS_BLOCK_SIZE], buf[FS_BLOCK_SIZE];
	uint32_t i, j;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	int32_t write_size = DOUBLE_DATA_OFFSET+indirect_entry*FS_BLOCK_SIZE+double_entry*INODE_INDIRECT_DATA_SIZE;
	user_id user = 1;
	inode_t *ino_ptr;

	init_logical_address(log_addr);
	init_logical_address(double_addr);
	init_logical_address(indirect_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	fsMemset(write_buf,byte, FS_BLOCK_SIZE);

	/* create file */
	fd = open(f_name, CREAT_FLAGS, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nopen success\n\n\n\n\n");
	f_id = GET_FILE_ID_BY_FD(fd);

	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	/* write to inode file data*/
	res = write(fd, write_buf, INODE_FILE_DATA_SIZE);
	assert(CACHE_GET_BUF_PTR(0)[527] != 0x41);
	tid = getTidByFileId(f_id);
	VERIFY(COMPARE(res, INODE_FILE_DATA_SIZE));

	/* verify open file entry */
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), INODE_FILE_DATA_SIZE));

	/* write direct entries*/
	for(i=0; i< DIRECT_INDEX_ENTRIES;i++){
		/* write to file*/
		res = write(fd, write_buf, FS_BLOCK_SIZE);
		assert(CACHE_GET_BUF_PTR(0)[527] != 0x41);
		VERIFY(COMPARE(res, FS_BLOCK_SIZE));
	}

	/* verify open file entry */
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), INODE_FILE_DATA_SIZE+DIRECT_INDEX_ENTRIES*FS_BLOCK_SIZE));

	/* write indirect block*/
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK;i++){
//		L("real indirect entry i #%d", i);
		/* write to file*/
		res = write(fd, write_buf, FS_BLOCK_SIZE);
		VERIFY(COMPARE(res, FS_BLOCK_SIZE));
	}

//	L("\nindirect entries write success");
	/* write double entries*/
	for(i=0; i< double_entry;i++){
//		L("double entry i #%d", i);
		for(j=0; j<LOG_ADDRESSES_PER_BLOCK;j++){
			/* write to file*/
			res = write(fd, write_buf, FS_BLOCK_SIZE);
			VERIFY(COMPARE(res, FS_BLOCK_SIZE));
		}
	}

//	L("\ndouble entries write success");
	/* write last indirect entries*/
	/* write indirect block*/
	for(i=0; i< indirect_entry;i++){
		/* write to file*/
//		L("double-indirect entry i #%d", i);
		res = write(fd, write_buf, FS_BLOCK_SIZE);
		VERIFY(COMPARE(res, FS_BLOCK_SIZE));
	}
//	L("last doubly indexed entries write success");
//	L("all writes successful");
	/* try to read a file block from the first indirect block*/
	/* verify open file entry */
//	PRINT_MSG_AND_NUM("\nopen file entry offset=",OPEN_FILE_GET_OFFSET(fd));
//	PRINT_MSG_AND_NUM("\nwrite_size",write_size);
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), write_size));
//	L("\nwrite size success");

//	/* verify file size */
////	PRINT_MSG_AND_NUM("\nafter write file size=",getFileSize(f_id));
	VERIFY(COMPARE(getFileSize(f_id), write_size));
	/* verify file data, and that the physical block data after written data
	 * contains empty bytes */
//	L("get inode from addr %x (seg %d offset %d)", TRANSACTION_GET_INO_ADDR_PTR(tid),
//												   GET_LOGICAL_SEGMENT(TRANSACTION_GET_INO_ADDR_PTR(tid)),
//												   GET_LOGICAL_OFFSET(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),
						fs_buffer,
						tid,
						CACHE_ENTRY_OFFSET_EMPTY,
						FLAG_CACHEABLE_READ_YES));
//	PRINT_MSG_AND_NUM("\nread inode from address=", logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));
//	PRINT_MSG_AND_NUM(", nblocks=", INODE_GET_NBLOCKS(CAST_TO_INODE(fs_buffer)));
//	assert(0);
//	L("\nfile size success");
//	printBlock(fs_buffer);
	for(i=0; i<INODE_FILE_DATA_SIZE; i++){
		if(!COMPARE(fs_buffer[i], byte)){
//			L("ERROR in byte %d expected byte %x got %x", i, byte, fs_buffer[i]);
		}
		VERIFY(COMPARE(fs_buffer[i], byte));
	}
//	L("written inode file data success");

	/* verify inode details */

	ino_ptr = CAST_TO_INODE(fs_buffer);
	INODE_GET_INDIRECT(ino_ptr, indirect_addr);
	INODE_GET_DOUBLE(ino_ptr, double_addr);
	VERIFY(!IS_ADDR_EMPTY(indirect_addr));
	VERIFY(!IS_ADDR_EMPTY(double_addr));
//	PRINT_MSG_AND_NUM("\nflash nbytes=",INODE_GET_NBYTES(ino_ptr));
//	PRINT_MSG_AND_NUM("\nINDIRECT_DATA_OFFSET=",INDIRECT_DATA_OFFSET);
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), DOUBLE_DATA_OFFSET+double_entry*INODE_INDIRECT_DATA_SIZE));
//	PRINT("\nnbytes success");
//	PRINT_MSG_AND_NUM("\nexpected nblocks=",1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK+1+LOG_ADDRESSES_PER_BLOCK*double_entry+1+double_entry);
//	PRINT_MSG_AND_NUM("\nreal nblocks=",INODE_GET_NBLOCKS(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK+(LOG_ADDRESSES_PER_BLOCK)*double_entry));
//	PRINT("\nnblocks success");
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), f_id));
//	PRINT("\nf_id success");
//	PRINT_MSG_AND_NUM("\nf_type=",INODE_GET_FILE_TYPE(ino_ptr));
	VERIFY(COMPARE(INODE_GET_FILE_TYPE(ino_ptr), FTYPE_FILE));
//	PRINT("\ninode data success");

	/* verify data offset, and blocks written dor this indirect block*/
	VERIFY(COMPARE(TRANSACTION_GET_FILE_OFFSET(tid), DOUBLE_DATA_OFFSET+double_entry*INODE_INDIRECT_DATA_SIZE));
	VERIFY(COMPARE(TRANSACTION_GET_BLOCKS_ALLOCS(tid), indirect_entry));
	VERIFY(COMPARE(TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid), indirect_entry*FS_BLOCK_SIZE));
//	PRINT("\ntransaction data success");

	/* verify indirect block*/
//	L("");
	VERIFY(!fsReadBlock(indirect_addr,
						buf,
						tid,
						INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						FLAG_CACHEABLE_READ_YES));
//	L("read success");
	VERIFY(verifyIndirectBlock(buf, byte, LOG_ADDRESSES_PER_BLOCK));
//	L("indirectly mapped blocks success");

	/* verify first double entries */
	/* write double entries*/
	for(i=0; i< double_entry;i++){
//		L("verify first double entries. i=%d", i);
		VERIFY(!fsReadBlock(double_addr,
							buf,
							tid,
							DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
							FLAG_CACHEABLE_READ_YES));

		BLOCK_GET_INDEX(buf, i, log_addr);
		VERIFY(!fsReadBlock(indirect_addr,
						    buf,
						    tid,
						    i,
						    FLAG_CACHEABLE_READ_YES));

//		L("call verifyIndirectBlock()");
		VERIFY(verifyIndirectBlock(buf, byte, LOG_ADDRESSES_PER_BLOCK));
	}
//	L("doubly mapped written blocks success");

	/* verify other double entries are empty */
	VERIFY(!fsReadBlock(double_addr,
						buf,
						tid,
						DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
						FLAG_CACHEABLE_READ_YES));
	for(; i< LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_GET_INDEX(buf, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

//	PRINT("\ndoubly mapped empty entries success");
	VERIFY(verifyIndirectBlock(TRANSACTION_GET_INDIRECT_PTR(tid), byte, indirect_entry));
//	PRINT("\ntransaction indirect block success");

	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write to a new file from offset 0 so that data is stored in triple block offset
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest5(){
	int32_t tid, fd, res, f_id, indirect_entry = 22, double_entry = 2, triple_entry = 1;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	uint8_t write_buf[FS_BLOCK_SIZE], buf[FS_BLOCK_SIZE];
	uint8_t triple_buf[FS_BLOCK_SIZE], double_buf[FS_BLOCK_SIZE], indirect_buf[FS_BLOCK_SIZE];
	uint32_t i, l;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(triple_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	int32_t write_size = TRIPLE_DATA_OFFSET+indirect_entry*FS_BLOCK_SIZE+double_entry*INODE_INDIRECT_DATA_SIZE+triple_entry*INODE_DOUBLE_DATA_SIZE;
	user_id user = 1;
	inode_t *ino_ptr;

//	L("\nTRIPLE_DATA_OFFSET=%d, triple_entry*INODE_DOUBLE_DATA_SIZE=%d, double_entry*INODE_INDIRECT_DATA_SIZE=%d", TRIPLE_DATA_OFFSET, triple_entry*INODE_DOUBLE_DATA_SIZE, double_entry*INODE_INDIRECT_DATA_SIZE);
	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < write_size){
		return 1;
	}

	init_logical_address(log_addr);
	init_logical_address(double_addr);
	init_logical_address(triple_addr);
	init_logical_address(indirect_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	fsMemset(write_buf,byte, FS_BLOCK_SIZE);

	/* create file */
	fd = open(f_name, CREAT_FLAGS, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nopen success");
	f_id = GET_FILE_ID_BY_FD(fd);

	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	/* write to inode file data*/
	res = write(fd, write_buf, INODE_FILE_DATA_SIZE);
	tid = getTidByFileId(f_id);
	VERIFY(COMPARE(res, INODE_FILE_DATA_SIZE));
//	PRINT("\nfirst write success");
	/* verify open file entry */
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), INODE_FILE_DATA_SIZE));

	for(i=0; i<write_size-INODE_FILE_DATA_SIZE; i+= FS_BLOCK_SIZE){
//		if(i % 512000 == 0){
//			PRINT_MSG_AND_NUM("\nwrite to offset ", i+INODE_FILE_DATA_SIZE);
//		}
//		n++;
//		if(n%100 == 0) {
//			PRINT_MSG_AND_NUM("\nwrote pages n=", n);
//		}
//		L("write #%d", i);
		res = write(fd, write_buf, FS_BLOCK_SIZE);
		VERIFY(COMPARE(res, FS_BLOCK_SIZE));
//		L("write success. file offset=%d",OPEN_FILE_GET_OFFSET(fd));
//		PRINT_MSG_AND_NUM(", TRIPLE_DATA_OFFSET=",TRIPLE_DATA_OFFSET);
//		PRINT_MSG_AND_NUM(", INODE_DOUBLE_DATA_SIZE=",INODE_DOUBLE_DATA_SIZE);
	}
//	PRINT("\nfinished writing");
	/* verify open file entry */
//	PRINT_MSG_AND_NUM("\nopen file entry offset=",OPEN_FILE_GET_OFFSET(fd));
//	PRINT_MSG_AND_NUM("\nwrite_size",write_size);
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), write_size));
//	PRINT("\nwrite size success");

	/* verify file size */
//	PRINT_MSG_AND_NUM("\nafter write file size=",getFileSize(f_id));
	VERIFY(COMPARE(getFileSize(f_id), write_size));

	/* verify file data, and that the physical block data after written data
	 * contains empty bytes */
//	L("");
	VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),
						fs_buffer,
						tid,
						0,
						FLAG_CACHEABLE_READ_YES));
//	PRINT_MSG_AND_NUM("\nread inode from address=", logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));
//	PRINT_MSG_AND_NUM(", nblocks=", INODE_GET_NBLOCKS(CAST_TO_INODE(fs_buffer)));
//	assert(0);
//	L("file size success");
	for(i=0; i<INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(fs_buffer[i], byte))
	}
//	L("written inode file data success");

	/* verify inode details */
	ino_ptr = CAST_TO_INODE(fs_buffer);
	INODE_GET_INDIRECT(ino_ptr, indirect_addr);
	INODE_GET_DOUBLE(ino_ptr, double_addr);
	INODE_GET_TRIPLE(ino_ptr, triple_addr);

	VERIFY(!fsReadBlock(indirect_addr,
						indirect_buf,
						tid,
						INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(indirect_addr)));

	VERIFY(!fsReadBlock(double_addr,
						double_buf,
						tid,
						DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(double_addr)));

	VERIFY(!fsReadBlock(triple_addr,
						triple_buf,
						tid,
						TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(triple_addr)));
//	L("indirect_addr %x, double_addr %x, triple_addr %x", ADDR_PRINT(indirect_addr), ADDR_PRINT(double_addr), ADDR_PRINT(triple_addr));
//	PRINT_MSG_AND_NUM("\nflash nbytes=",INODE_GET_NBYTES(ino_ptr));
//	PRINT_MSG_AND_NUM("\nINDIRECT_DATA_OFFSET=",INDIRECT_DATA_OFFSET);
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), TRIPLE_DATA_OFFSET+triple_entry*INODE_DOUBLE_DATA_SIZE+double_entry*INODE_INDIRECT_DATA_SIZE));
//	L("nbytes success");
	i  = 1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK; /* direct_indirect blocks*/
	i += LOG_ADDRESSES_PER_BLOCK*LOG_ADDRESSES_PER_BLOCK; /* double blocks*/
    i += triple_entry*(LOG_ADDRESSES_PER_BLOCK*LOG_ADDRESSES_PER_BLOCK); /* triple*/
    i += double_entry*(LOG_ADDRESSES_PER_BLOCK);

//	PRINT_MSG_AND_NUM("\nexpected nblocks=",i);
//	PRINT_MSG_AND_NUM("\nreal nblocks=",INODE_GET_NBLOCKS(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), i));
//	PRINT("\nnblocks success");
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), f_id));
//	PRINT("\nf_id success");
//	PRINT_MSG_AND_NUM("\nf_type=",INODE_GET_FILE_TYPE(ino_ptr));
	VERIFY(COMPARE(INODE_GET_FILE_TYPE(ino_ptr), FTYPE_FILE));
//	PRINT("\ninode data success");

	/* verify data offset, and blocks written dor this indirect block*/
	VERIFY(COMPARE(TRANSACTION_GET_FILE_OFFSET(tid), TRIPLE_DATA_OFFSET+triple_entry*INODE_DOUBLE_DATA_SIZE+double_entry*INODE_INDIRECT_DATA_SIZE));
	VERIFY(COMPARE(TRANSACTION_GET_BLOCKS_ALLOCS(tid), indirect_entry));
	VERIFY(COMPARE(TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid), indirect_entry*FS_BLOCK_SIZE));
//	L("transaction data success");

	/* verify indirect block*/
	VERIFY(verifyIndirectBlock(indirect_buf, byte, LOG_ADDRESSES_PER_BLOCK));
//	L("indirectly mapped blocks success");

	/* verify double entries */
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK;i++){
		BLOCK_GET_INDEX(double_buf, i, log_addr);
		VERIFY(!fsReadBlock(indirect_addr,
							buf,
							tid,
							i,
							IS_CACHED_ADDR(indirect_addr)));
		VERIFY(verifyIndirectBlock(buf, byte, LOG_ADDRESSES_PER_BLOCK));
	}
//	L("doubly mapped written blocks success");

	/* verify first triple entries */
	for(l=0; l< triple_entry;l++){
		BLOCK_GET_INDEX(triple_buf, l, double_addr);

		/* verify double entries */
		for(i=0; i< LOG_ADDRESSES_PER_BLOCK;i++){
			VERIFY(!fsReadBlock(double_addr,
								buf,
								tid,
								l,
								IS_CACHED_ADDR(double_addr)));
			BLOCK_GET_INDEX(buf, i, log_addr);
			VERIFY(!fsReadBlock(log_addr,
								buf,
								tid,
								i,
								IS_CACHED_ADDR(log_addr)));

			VERIFY(verifyIndirectBlock(buf, byte, LOG_ADDRESSES_PER_BLOCK));
		}
	}

//	L("triply mapped data entries success");
	/* verify other last triple entry is only partially written */
	BLOCK_GET_INDEX(triple_buf, l, double_addr);

	for(i=0; i< double_entry;i++){
		VERIFY(!fsReadBlock(double_addr,
							buf,
							tid,
							l,
							IS_CACHED_ADDR(double_addr)));
		BLOCK_GET_INDEX(buf, i, log_addr);
		VERIFY(!fsReadBlock(log_addr,
							buf,
							tid,
							i,
							IS_CACHED_ADDR(log_addr)));

		VERIFY(verifyIndirectBlock(buf, byte, LOG_ADDRESSES_PER_BLOCK));
	}


	/* verify last double entris are empty  */
	VERIFY(!fsReadBlock(double_addr,
						buf,
						tid,
						i,
						IS_CACHED_ADDR(double_addr)));

	for(; i< LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_GET_INDEX(buf, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

//	L("triply mapped empty entries success");
	VERIFY(verifyIndirectBlock(TRANSACTION_GET_INDIRECT_PTR(tid), byte, indirect_entry));
//	PRINT("\ntransaction indirect block success");

	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write to a new file not from offset 0 so that data is stored in inode file data
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest6(){
	int32_t tid, fd, res, f_id, basic_offset = 50;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	int32_t buf_size = INODE_FILE_DATA_SIZE-basic_offset, write_size = buf_size-50;
	uint8_t write_buf[INODE_FILE_DATA_SIZE-50];
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	user_id user = 1;

	init_logical_address(log_addr);
	init_logical_address(ino_log_addr);

	fsMemset(write_buf, byte+1, buf_size);
	fsMemset(write_buf, byte, write_size);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
	fd = creat(f_name, 0);
	f_id = GET_FILE_ID_BY_FD(fd);
	VERIFY(COMPARE(f_id, 2));
//	PRINT("\ncreat success");
	/* re-write inode*/
	VERIFY(!getInode(fs_buffer, f_id, ino_log_addr));
	fsMemset(fs_buffer, byte+1, basic_offset);
	INODE_SET_NBYTES(ino_ptr,basic_offset );
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* write ino0 again*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT("\nino0 write success");
	/* write to file*/
	VERIFY(COMPARE(basic_offset, lseek(fd, basic_offset, FS_SEEK_SET)));
	res = write(fd, write_buf, write_size);
//	PRINT_MSG_AND_NUM("\nwrite res=",res);
	tid = getTidByFileId(f_id);
	VERIFY(COMPARE(res, write_size));
//	PRINT("\nwrite success");
	/* verify transaction data */
	VERIFY(verifyTransactionData(tid, 0, 0, basic_offset+write_size));

	for(i=0; i <basic_offset; i++){
		VERIFY(COMPARE(byte+1,TRANSACTION_GET_INDIRECT_PTR(tid)[i]));
	}

	for(; i <basic_offset+write_size; i++){
		VERIFY(COMPARE(byte,TRANSACTION_GET_INDIRECT_PTR(tid)[i]));
	}
//	PRINT("\ntransaction verify success");
	/* verify written data*/
	fsMemcpy(fs_buffer, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE);
//	VERIFY(!fsReadBlock(TRANSACTION_GET_PREV_ADDR_PTR(tid), fs_buffer));
	VERIFY(verifyInode(ino_ptr, 2, FTYPE_FILE, 1, basic_offset+write_size));

//	PRINT("\nino details verify success");
	for(i=0; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr))

	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size+basic_offset,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write to a new file not from offset 0 so that data is stored in inode file data
 * and first direct entry. should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest7(){
	int32_t tid, fd, res, f_id, basic_offset = 50;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	int32_t buf_size = FS_BLOCK_SIZE+INODE_FILE_DATA_SIZE-50, write_size = buf_size-50;
	uint8_t write_buf[FS_BLOCK_SIZE+INODE_FILE_DATA_SIZE-50];
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	user_id user = 1;

	init_logical_address(log_addr);
	init_logical_address(ino_log_addr);

	fsMemset(write_buf, byte+1, buf_size);
	fsMemset(write_buf, byte, write_size);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
	fd = creat(f_name, 0);
	f_id = GET_FILE_ID_BY_FD(fd);
	VERIFY(COMPARE(f_id, 2));
//	PRINT("\ncreat success");
	/* re-write inode*/
	VERIFY(!getInode(fs_buffer, f_id, ino_log_addr));
	fsMemset(fs_buffer, byte+1, basic_offset);
	INODE_SET_NBYTES(ino_ptr,basic_offset );
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* write ino0 again*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT("\nino0 write success");
	/* write to file*/
	VERIFY(COMPARE(basic_offset, lseek(fd, basic_offset, FS_SEEK_SET)));
	res = write(fd, write_buf, write_size);
//	PRINT_MSG_AND_NUM("\nwrite res=",res);
	tid = getTidByFileId(f_id);
	VERIFY(COMPARE(res, write_size));
//	PRINT("\nwrite success");
	/* verify transaction data */
	VERIFY(verifyTransactionData(tid, 0, 1, basic_offset+write_size));

	for(i=0; i <basic_offset; i++){
		VERIFY(COMPARE(byte+1,TRANSACTION_GET_INDIRECT_PTR(tid)[i]));
	}

	for(; i <INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(byte,TRANSACTION_GET_INDIRECT_PTR(tid)[i]));
	}
//	PRINT("\ntransaction verify success");
	/* verify written data*/
	fsMemcpy(fs_buffer, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE);
//	VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid), fs_buffer));
	VERIFY(verifyInode(ino_ptr, 2, FTYPE_FILE, 1, INODE_FILE_DATA_SIZE));

//	PRINT("\nino details verify success");
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));

	for(i=1; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr))

	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

	/* verify last written block (first direct entry) */
	if(!IS_CACHE_EMPTY()){
	fsMemcpy(fs_buffer, CACHE_GET_BUF_PTR(CACHE_LRU_Q_GET_MRU()), FS_BLOCK_SIZE);
	}
	else{
	VERIFY(!fsReadBlockSimple(TRANSACTION_GET_PREV_ADDR_PTR(tid), fs_buffer));
	}

	for(i=0; i <write_size-(INODE_FILE_DATA_SIZE-basic_offset); i++){
		VERIFY(COMPARE(byte,fs_buffer[i]));
	}

	for(; i <FS_BLOCK_SIZE; i++){
		VERIFY(COMPARE(0xff,fs_buffer[i]));
	}

	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size+basic_offset,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write to a new file from indirect offset until first direct block of double offset
 * so that file size is increased.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest8(){
	int32_t tid, fd, res, f_id, basic_offset = DOUBLE_DATA_OFFSET-70, last_indirect_write_size = DOUBLE_DATA_OFFSET-basic_offset;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	int32_t buf_size = FS_BLOCK_SIZE+70, write_size = buf_size-50;
	uint8_t write_buf[FS_BLOCK_SIZE+70], buf[FS_BLOCK_SIZE];
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten, isVOTs;
	user_id user = 1;
	int32_t mru_after_write;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr);
	init_logical_address(indirect_log_addr);
	init_logical_address(ino_log_addr);
	fsMemset(write_buf, byte+1, buf_size);
	fsMemset(write_buf, byte, write_size);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
	fd   = creat(f_name, 0);
	f_id = GET_FILE_ID_BY_FD(fd);
	VERIFY(COMPARE(f_id, 2));
//	PRINT("\ncreat success");

	/* write last indirect entry */
	init_fsbuf(buf);
	fsMemset(buf, byte,FS_BLOCK_SIZE-last_indirect_write_size);
	VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote direct to ",logicalAddressToPhysical(log_addr));
	init_fsbuf(buf);
	BLOCK_SET_INDEX(buf, LOG_ADDRESSES_PER_BLOCK-1, log_addr);
	VERIFY(!allocAndWriteBlock(indirect_log_addr, buf, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote indirect to ",logicalAddressToPhysical(indirect_log_addr));

	/* re-write inode*/
	VERIFY(!getInode(fs_buffer, f_id, ino_log_addr));
	INODE_SET_NBYTES(ino_ptr,basic_offset );
	INODE_SET_NBLOCKS(ino_ptr,1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK+1);
	INODE_SET_INDIRECT(ino_ptr, indirect_log_addr);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* write ino0 again*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT("\nino0 write success\n");

	/* write to file*/
//	L("perform write. write size %d", write_size);
	VERIFY(COMPARE(basic_offset, lseek(fd, basic_offset, FS_SEEK_SET)));
	res = write(fd, write_buf, write_size);
	mru_after_write = CACHE_LRU_Q_GET_MRU();
//	L("\nPERFORMED WRITE. res %d",res);
	tid = getTidByFileId(f_id);
	VERIFY(COMPARE(res, write_size));
//	L("after write tid prev addr is %d", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
//	PRINT("\nwrite success");

	/* verify transaction data */
	VERIFY(verifyTransactionData(tid, DOUBLE_DATA_OFFSET, 1, write_size-last_indirect_write_size));

	/* verify indirect block of transaction*/
	BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid),0,log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));

	for(i=1; i <LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid),i,log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	L("\ntransaction verify success");
	/* verify written data*/
	VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),
						fs_buffer,
						tid,
						0,
						IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid))));
	VERIFY(verifyInode(ino_ptr, 2, FTYPE_FILE, 1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK+1, DOUBLE_DATA_OFFSET));

//	PRINT("\nino details verify success");
	for(i=0; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	L("\ndirect entries verify success");
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
	VERIFY(!fsReadBlock(log_addr,
						buf,
						tid,
						INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));
	BLOCK_GET_INDEX(buf, LOG_ADDRESSES_PER_BLOCK-1,log_addr);
	VERIFY(!fsReadBlock(log_addr,
						buf,
						tid,
						LOG_ADDRESSES_PER_BLOCK-1,
						IS_CACHED_ADDR(log_addr)));
//	L("\nread last indirect");
//	PRINT("\nabout to verify last indirect entry");
	for(i=0; i<FS_BLOCK_SIZE;i++){
//		PRINT_MSG_AND_NUM("\nverifying byte ",i);
//		PRINT_MSG_AND_HEX("= ",buf[i]);
		VERIFY(COMPARE(byte,buf[i]));
	}
//	PRINT("\nindirect entries verify success");
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr))

	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	L("\ndouble triple entries verify success");
	/* verify last written block (first direct entry) */
	/* read 2 prev backwards*/
//	L("read last accessed cache in write %d", mru_after_write);
	if(!IS_CACHE_EMPTY()){
	fsMemcpy(fs_buffer, CACHE_GET_BUF_PTR(mru_after_write), FS_BLOCK_SIZE);
	}
	else{
	VERIFY(!fsReadBlockSimple(TRANSACTION_GET_PREV_ADDR_PTR(tid), fs_buffer));
	}
////	PRINT_MSG_AND_NUM("\nprev addr=",logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
	for(i=0; i <write_size-last_indirect_write_size; i++){
//		L("byte #%d is %x. expected %x", i, fs_buffer[i], byte);
		VERIFY(COMPARE(byte,fs_buffer[i]));
	}
//	PRINT("\nlast written block first part");
	for(; i <FS_BLOCK_SIZE; i++){
//		L("byte #%d is %x", i, fs_buffer[i]);
		VERIFY(COMPARE(0xff,fs_buffer[i]));
	}
//	PRINT("\nlast written block success");
	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size+basic_offset,user,0));
//	PRINT("\nopen file entry success");
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
//	PRINT("\nvnode success");
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write to a new file from indirect offset until first direct entry in triple offset, so that file size is increased.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest9(){
	int32_t tid, fd, res, f_id, basic_offset = DOUBLE_DATA_OFFSET-70;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	int32_t write_size = TRIPLE_DATA_OFFSET-basic_offset+100, last_indirect_write_size=DOUBLE_DATA_OFFSET-basic_offset;
	int32_t lastWrittenBlockOffset = write_size+basic_offset-TRIPLE_DATA_OFFSET;
	uint8_t buf[FS_BLOCK_SIZE];
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	user_id user = 1;
	int32_t tempWriteSize;
	int32_t mru_after_write;
	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < TRIPLE_DATA_OFFSET){
		return 1;
	}

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr);
	init_logical_address(indirect_log_addr);
	init_logical_address(double_log_addr);
	init_logical_address(ino_log_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
	fd = creat(f_name, 0);
	f_id = GET_FILE_ID_BY_FD(fd);
	VERIFY(COMPARE(f_id, 2));
//	PRINT("\ncreat success");

	/* write last double entry */
	init_fsbuf(buf);
	fsMemset(buf, byte,FS_BLOCK_SIZE-last_indirect_write_size);
	VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote direct to ",logicalAddressToPhysical(log_addr));
	init_fsbuf(buf);
	BLOCK_SET_INDEX(buf, LOG_ADDRESSES_PER_BLOCK-1, log_addr);
	VERIFY(!allocAndWriteBlock(indirect_log_addr, buf, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote indirect to ",logicalAddressToPhysical(indirect_log_addr));

	/* re-write inode*/
	VERIFY(!getInode(fs_buffer, f_id, ino_log_addr));
	INODE_SET_NBYTES(ino_ptr,basic_offset );
	INODE_SET_NBLOCKS(ino_ptr,1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK);
	INODE_SET_INDIRECT(ino_ptr, indirect_log_addr);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* write ino0 again*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
	L("ino0 write success");

	/* write to file*/
	VERIFY(COMPARE(basic_offset, lseek(fd, basic_offset, FS_SEEK_SET)));

	init_fsbuf(buf);
	fsMemset(buf, byte,FS_BLOCK_SIZE);

	tempWriteSize = write_size;
	while(tempWriteSize>FS_BLOCK_SIZE){
		res = write(fd, buf, FS_BLOCK_SIZE);
		VERIFY(COMPARE(res, FS_BLOCK_SIZE));
		tempWriteSize-=FS_BLOCK_SIZE;
	}
	res = write(fd, buf, tempWriteSize);
	mru_after_write = CACHE_LRU_Q_GET_MRU();
	VERIFY(COMPARE(res, tempWriteSize));
	tid = getTidByFileId(f_id);

	L("write success");
	/* verify transaction data */
	VERIFY(verifyTransactionData(tid, TRIPLE_DATA_OFFSET, 1, lastWrittenBlockOffset));
	L("transaction verified");

	/* verify indirect block of transaction*/
	BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid),0,log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));

	for(i=1; i <LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid),i,log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\ntransaction indirect block verify success");
	/* verify written data*/
	VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),
						fs_buffer,
						tid,
						0,
						IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid))));
	VERIFY(verifyInode(ino_ptr, 2, FTYPE_FILE, 1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK+LOG_ADDRESSES_PER_BLOCK*LOG_ADDRESSES_PER_BLOCK, TRIPLE_DATA_OFFSET));

	/* verify triple is empty*/
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nino details verify success");
	for(i=0; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\ndirect entries verify success");
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));

	/* verify last indirectly mapped direct entry*/
	VERIFY(!fsReadBlock(log_addr,
						buf,
						tid,
						INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));
	BLOCK_GET_INDEX(buf, LOG_ADDRESSES_PER_BLOCK-1,log_addr);
	VERIFY(!fsReadBlock(log_addr,
						buf,
						tid,
						LOG_ADDRESSES_PER_BLOCK-1,
						IS_CACHED_ADDR(log_addr)));
//	L("about to verify last indirect entry");
	for(i=0; i<FS_BLOCK_SIZE;i++){
//		PRINT_MSG_AND_NUM("\nverifying byte ",i);
//		PRINT_MSG_AND_HEX("= ",buf[i]);
		VERIFY(COMPARE(byte,buf[i]));
	}

	L("indirect entries verify success");
	/* verify double entries*/
	INODE_GET_DOUBLE(ino_ptr, double_log_addr);
	VERIFY(!IS_ADDR_EMPTY(double_log_addr))

	VERIFY(!fsReadBlock(double_log_addr,
						fs_buffer,
						tid,
						DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(double_log_addr)));
	for(i=0; i<LOG_ADDRESSES_PER_BLOCK;i++){
//		PRINT_MSG_AND_NUM("\nverify indirect block at offset (in double block) of ",i);
		BLOCK_GET_INDEX(fs_buffer,i, log_addr);
		VERIFY(!fsReadBlock(log_addr,
							buf,
							tid,
							i,
							IS_CACHED_ADDR(log_addr)));
		VERIFY(verifyIndirectBlock(buf, byte, LOG_ADDRESSES_PER_BLOCK));
	}

	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size+basic_offset,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write to a new file from last entry of indirect offset , so that file size doesn't change.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest10(){
	int32_t tid, fd, res, f_id, basic_offset = DOUBLE_DATA_OFFSET-70;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	int32_t buf_size = FS_BLOCK_SIZE+70, write_size = DOUBLE_DATA_OFFSET-basic_offset, old_rec_offset;
	uint8_t write_buf[FS_BLOCK_SIZE+70], buf[FS_BLOCK_SIZE];
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	user_id user = 1;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr);
	init_logical_address(indirect_log_addr);
	init_logical_address(ino_log_addr);
	fsMemset(write_buf, byte, buf_size);
	fsMemset(write_buf, byte+1, write_size);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
	fd = creat(f_name, 0);
	f_id = GET_FILE_ID_BY_FD(fd);
	VERIFY(COMPARE(f_id, 2));
//	PRINT("\ncreat success");

	/* write indirect entries */
	init_fsbuf(buf);
	init_fsbuf(fs_buffer);
	fsMemset(buf, byte,FS_BLOCK_SIZE);
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK; i++){
		VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
		BLOCK_SET_INDEX(fs_buffer, i, log_addr);
	}

	VERIFY(!allocAndWriteBlock(indirect_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote indirect to ",logicalAddressToPhysical(indirect_log_addr));

	/* re-write inode*/
	VERIFY(!getInode(fs_buffer, f_id, ino_log_addr));
	INODE_SET_NBYTES(ino_ptr,DOUBLE_DATA_OFFSET);
	INODE_SET_NBLOCKS(ino_ptr,1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK+1);
	INODE_SET_INDIRECT(ino_ptr, indirect_log_addr);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* write ino0 again*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT("\nino0 write success");

	/* write to file*/
	VERIFY(COMPARE(basic_offset, lseek(fd, basic_offset, FS_SEEK_SET)));
//	PRINT("\nlseek success");
	old_rec_offset = GET_RECLAIMED_OFFSET();
	res = write(fd, write_buf, write_size);
	VERIFY(COMPARE(res, write_size));
	tid = getTidByFileId(f_id);
//	PRINT("\nwrite success");
	/* verify transaction data */
	VERIFY(verifyTransactionData(tid, INDIRECT_DATA_OFFSET, 0, INODE_INDIRECT_DATA_SIZE));
//	PRINT("\ntransaction verify success");
	/* verify indirect block of transaction*/
	BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), LOG_ADDRESSES_PER_BLOCK-1, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));

	VERIFY(!fsReadBlock(log_addr,
						buf,
						0,
						LOG_ADDRESSES_PER_BLOCK-1,
						IS_CACHED_ADDR(log_addr)));
	for(i=0; i<FS_BLOCK_SIZE-write_size;i++){
		VERIFY(COMPARE(byte, buf[i]));
	}
	for(; i<FS_BLOCK_SIZE;i++){
		VERIFY(COMPARE(byte+1, buf[i]));
	}

	if(!IS_CACHE_EMPTY()){
	/* verify we  cached the write*/
	VERIFY(COMPARE(old_rec_offset, GET_RECLAIMED_OFFSET()));
	}
	else{
	/* verify we only wrote one block*/
	VERIFY(COMPARE(old_rec_offset, GET_RECLAIMED_OFFSET()-1));
	}
	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size+basic_offset,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write 2 blocks to a new file from double offset , so that file size doesn't change.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest11(){
	int32_t tid, fd, res, f_id, basic_offset = DOUBLE_DATA_OFFSET;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	int32_t write_size = FS_BLOCK_SIZE*2, old_rec_offset;
	uint8_t write_buf[FS_BLOCK_SIZE*2], buf[FS_BLOCK_SIZE];
	uint32_t i,k;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	user_id user = 1;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr);
	init_logical_address(indirect_log_addr);
	init_logical_address(double_log_addr);
	init_logical_address(ino_log_addr);
	fsMemset(write_buf, byte+1, write_size);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
	fd = creat(f_name, 0);
	f_id = GET_FILE_ID_BY_FD(fd);
	VERIFY(COMPARE(f_id, 2));
//	PRINT("\ncreat success");

	/* write indirect entries */
	init_fsbuf(buf);
	init_fsbuf(fs_buffer);
	fsMemset(buf, byte,FS_BLOCK_SIZE);
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK; i++){
		VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
		BLOCK_SET_INDEX(fs_buffer, i, log_addr);
	}

	VERIFY(!allocAndWriteBlock(indirect_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote indirect to ",logicalAddressToPhysical(indirect_log_addr));
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer,0,indirect_log_addr);
	VERIFY(!allocAndWriteBlock(double_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote double to ",logicalAddressToPhysical(double_log_addr));

	/* re-write inode*/
	VERIFY(!getInode(fs_buffer, f_id, ino_log_addr));
	INODE_SET_NBYTES(ino_ptr,((int32_t)(DOUBLE_DATA_OFFSET+INODE_INDIRECT_DATA_SIZE)));
	INODE_SET_NBLOCKS(ino_ptr,((int32_t)(1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK+1+LOG_ADDRESSES_PER_BLOCK+2)));
	INODE_SET_DOUBLE(ino_ptr, double_log_addr);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* write ino0 again*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT("\nino0 write success");

	/* write to file*/
	VERIFY(COMPARE(basic_offset, lseek(fd, basic_offset, FS_SEEK_SET)));
//	PRINT("\nlseek success");
	old_rec_offset = GET_RECLAIMED_OFFSET();
	res = write(fd, write_buf, write_size);
	VERIFY(COMPARE(res, write_size));
	tid = getTidByFileId(f_id);
//	PRINT("\nwrite success");
	/* verify transaction data */
	VERIFY(verifyTransactionData(tid, DOUBLE_DATA_OFFSET, 0, write_size+(basic_offset-DOUBLE_DATA_OFFSET)));
//	PRINT("\ntransaction verify success");
	/* verify indirect block of transaction*/
	for(k=0; k<2; k++){
		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), k, log_addr);
		VERIFY(!IS_ADDR_EMPTY(log_addr));

		VERIFY(!fsReadBlock(log_addr,
							buf,
							0,
							k,
							IS_CACHED_ADDR(log_addr)));
		for(i=0; i<FS_BLOCK_SIZE;i++){
			VERIFY(COMPARE(byte+1, buf[i]));
		}
	}
//	PRINT("\nfirst two transaction indirect entries success");
	/* verify all other blocks haven't changed*/
	for(; k<LOG_ADDRESSES_PER_BLOCK; k++){
//		PRINT_MSG_AND_NUM("\nverify transaction indirect entry ", k);
		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), k, log_addr);
		VERIFY(!IS_ADDR_EMPTY(log_addr));

		VERIFY(!fsReadBlock(log_addr,
							buf,
							0,
							k,
							IS_CACHED_ADDR(log_addr)));
		for(i=0; i<FS_BLOCK_SIZE;i++){
//			PRINT_MSG_AND_NUM("\nbyte=", buf[i]);
			VERIFY(COMPARE(byte, buf[i]));
		}
	}

	if(!IS_CACHE_EMPTY()){
//	PRINT("\nfirst two transaction indirect entries success");
	/* verify we cached all writes*/
	VERIFY(COMPARE(old_rec_offset, GET_RECLAIMED_OFFSET()));
	}
	else{
	/* verify we only wrote 2 blocks*/
	VERIFY(COMPARE(old_rec_offset, GET_RECLAIMED_OFFSET()-2));
	}
	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size+basic_offset,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}


/**
 * @brief
 * write to a new file from triple offset , so that file size doesn't change.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest12(){
	int32_t tid, fd, res, f_id, basic_offset = TRIPLE_DATA_OFFSET;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	int32_t write_size = FS_BLOCK_SIZE*2, old_rec_offset;
	uint8_t write_buf[FS_BLOCK_SIZE*2], buf[FS_BLOCK_SIZE];
	uint32_t i,k;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(triple_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	user_id user = 1;

	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < TRIPLE_DATA_OFFSET){
		return 1;
	}

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr);
	init_logical_address(indirect_log_addr);
	init_logical_address(double_log_addr);
	init_logical_address(triple_log_addr);
	init_logical_address(ino_log_addr);
	fsMemset(write_buf, byte+1, write_size);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
	fd = creat(f_name, 0);
	f_id = GET_FILE_ID_BY_FD(fd);
	VERIFY(COMPARE(f_id, 2));
//	PRINT("\ncreat success");

	/* write indirect entries */
	init_fsbuf(buf);
	init_fsbuf(fs_buffer);
	fsMemset(buf, byte,FS_BLOCK_SIZE);
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK; i++){
		VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
		BLOCK_SET_INDEX(fs_buffer, i, log_addr);
	}

	VERIFY(!allocAndWriteBlock(indirect_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote indirect to ",logicalAddressToPhysical(indirect_log_addr));
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer,0,indirect_log_addr);
	VERIFY(!allocAndWriteBlock(double_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer,0,double_log_addr);
	VERIFY(!allocAndWriteBlock(triple_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote double to ",logicalAddressToPhysical(double_log_addr));

	/* re-write inode*/
	VERIFY(!getInode(fs_buffer, f_id, ino_log_addr));
	INODE_SET_NBYTES(ino_ptr,((int32_t)(TRIPLE_DATA_OFFSET+INODE_INDIRECT_DATA_SIZE)));
	INODE_SET_NBLOCKS(ino_ptr,((int32_t)(1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK+1+(LOG_ADDRESSES_PER_BLOCK+1)*LOG_ADDRESSES_PER_BLOCK+1+LOG_ADDRESSES_PER_BLOCK+1+1+1)));
	INODE_SET_TRIPLE(ino_ptr, triple_log_addr);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* write ino0 again*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT("\nino0 write success");

	/* write to file*/
	VERIFY(COMPARE(basic_offset, lseek(fd, basic_offset, FS_SEEK_SET)));
//	PRINT("\nlseek success");
	old_rec_offset = GET_RECLAIMED_OFFSET();
	res = write(fd, write_buf, write_size);
	VERIFY(COMPARE(res, write_size));
	tid = getTidByFileId(f_id);
//	PRINT("\nwrite success");
	/* verify transaction data */
	VERIFY(verifyTransactionData(tid, TRIPLE_DATA_OFFSET, 0, write_size+(basic_offset-TRIPLE_DATA_OFFSET)));
//	PRINT("\ntransaction verify success");
	/* verify indirect block of transaction*/
	for(k=0; k<2; k++){
		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), k, log_addr);
		VERIFY(!IS_ADDR_EMPTY(log_addr));

		VERIFY(!fsReadBlock(log_addr,
							buf,
							0,
							k,
							IS_CACHED_ADDR(log_addr)));
		for(i=0; i<FS_BLOCK_SIZE;i++){
			VERIFY(COMPARE(byte+1, buf[i]));
		}
	}
//	PRINT("\nfirst two transaction indirect entries success");
	/* verify all other blocks haven't changed*/
	for(; k<LOG_ADDRESSES_PER_BLOCK; k++){
//		PRINT_MSG_AND_NUM("\nverify transaction indirect entry ", k);
		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), k, log_addr);
		VERIFY(!IS_ADDR_EMPTY(log_addr));

		VERIFY(!fsReadBlock(log_addr,
							buf,
							0,
							k,
							IS_CACHED_ADDR(log_addr)));
		for(i=0; i<FS_BLOCK_SIZE;i++){
//			PRINT_MSG_AND_NUM("\nbyte=", buf[i]);
			VERIFY(COMPARE(byte, buf[i]));
		}
	}
//	PRINT("\nfirst two transaction indirect entries success");
	if(!IS_CACHE_EMPTY()){
	/* verify we cached all writes*/
	VERIFY(COMPARE(old_rec_offset, GET_RECLAIMED_OFFSET()));
	}
	else{
	/* verify we only wrote 2 blocks*/
	VERIFY(COMPARE(old_rec_offset, GET_RECLAIMED_OFFSET()-2));
	}

	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size+basic_offset,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write to offset in double area. do creat.
 * continue writing to first file.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest14(){
	int32_t tid, fd1, fd2, res, f_id, f_id2, basic_offset = DOUBLE_DATA_OFFSET;
	uint8_t *f_name1 = "/file1.dat", byte = 'c', *f_name2 = "/file2.dat";
	int32_t write_size = FS_BLOCK_SIZE*2;
	uint8_t write_buf[FS_BLOCK_SIZE*2], buf[FS_BLOCK_SIZE];
	uint32_t i,k;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	user_id user = 1;
	/* set user in process*/
	SET_CURRENT_USER(user);

	if(FS_MAX_N_TRANSACTIONS < 2){
		return 1;
	}

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr);
	init_logical_address(indirect_log_addr);
	init_logical_address(double_log_addr);
	init_logical_address(ino_log_addr);

	fsMemset(write_buf, byte+1, write_size);

	/* create file */
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
	f_id = GET_FILE_ID_BY_FD(fd1);
	VERIFY(COMPARE(f_id, 2));
//	PRINT("\ncreat success");
//	PRINT_MSG_AND_NUM("\nino0 addr after first creat=",logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	/* write indirect entries */
	init_fsbuf(buf);
	init_fsbuf(fs_buffer);
	fsMemset(buf, byte,FS_BLOCK_SIZE);
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK; i++){
		VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
		BLOCK_SET_INDEX(fs_buffer, i, log_addr);
	}

	VERIFY(!allocAndWriteBlock(indirect_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote indirect to ",logicalAddressToPhysical(indirect_log_addr));
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer,0,indirect_log_addr);
	VERIFY(!allocAndWriteBlock(double_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nwrote double to ",logicalAddressToPhysical(double_log_addr));

	/* re-write inode*/
	VERIFY(!getInode(fs_buffer, f_id, ino_log_addr));
	INODE_SET_NBYTES(ino_ptr,((int32_t)(DOUBLE_DATA_OFFSET+INODE_INDIRECT_DATA_SIZE)));
	INODE_SET_NBLOCKS(ino_ptr,((int32_t)(1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK+1+(LOG_ADDRESSES_PER_BLOCK+1)+1)));
	INODE_SET_DOUBLE(ino_ptr, double_log_addr);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* write ino0 again*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	MARK_BLOCK_NO_HOLE(log_addr);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT("\nino0 write success");

	/* write to file*/
	VERIFY(COMPARE(basic_offset, lseek(fd1, basic_offset, FS_SEEK_SET)));
//	PRINT("\nlseek success");
	res = write(fd1, write_buf, write_size/2);
	VERIFY(COMPARE(res, write_size/2));
	tid = getTidByFileId(f_id);
//	PRINT("\nfirst write success");
//	PRINT_MSG_AND_NUM("\nino0 addr after first write=",logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//	assert(0);
	/* create another file */
	fd2 = creat(f_name2, 0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT("\nsecond creat success");
	f_id2 = GET_FILE_ID_BY_FD(fd2);
	VERIFY(COMPARE(f_id2, 3));

	/* write again*/
	res = write(fd1, write_buf, write_size/2);
	VERIFY(COMPARE(res, write_size/2));

//	PRINT("\nwrite success");
	/* verify transaction data */
	VERIFY(verifyTransactionData(tid, DOUBLE_DATA_OFFSET, 0, write_size+(basic_offset-DOUBLE_DATA_OFFSET)));
//	PRINT("\ntransaction verify success");
	/* verify indirect block of transaction*/
	for(k=0; k<2; k++){
		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), k, log_addr);
		VERIFY(!IS_ADDR_EMPTY(log_addr));

		VERIFY(!fsReadBlock(log_addr,
							buf,
							0,
							k,
							IS_CACHED_ADDR(log_addr)));
		for(i=0; i<FS_BLOCK_SIZE;i++){
			VERIFY(COMPARE(byte+1, buf[i]));
		}
	}
//	PRINT("\nfirst two transaction indirect entries success");
	/* verify all other blocks haven't changed*/
	for(; k<LOG_ADDRESSES_PER_BLOCK; k++){
//		PRINT_MSG_AND_NUM("\nverify transaction indirect entry ", k);
		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), k, log_addr);
		VERIFY(!IS_ADDR_EMPTY(log_addr));

		VERIFY(!fsReadBlock(log_addr,
							buf,
							0,
							k,
							IS_CACHED_ADDR(log_addr)));
		for(i=0; i<FS_BLOCK_SIZE;i++){
//			PRINT_MSG_AND_NUM("\nbyte=", buf[i]);
			VERIFY(COMPARE(byte, buf[i]));
		}
	}
//	PRINT("\nother transaction indirect entries success");

	VERIFY(verifyOpenFileEntry(fd1,CREAT_FLAGS,write_size+basic_offset,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd1),f_id,1));
//	PRINT("\nfirst file file entry and vnode verified");
	VERIFY(verifyOpenFileEntry(fd2,CREAT_FLAGS,0,user,1));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd2),f_id2,1));
//	PRINT("\nsecond file file entry and vnode verified");
	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd1 || i==fd2)
			continue;

		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nopen files entries verified");
	for(i=0; i<FS_MAX_VNODES;i++){
		if(i==OPEN_FILE_GET_VNODE(fd1) ||i==OPEN_FILE_GET_VNODE(fd2))
			continue;

		VERIFY(verifyVnodeEmpty(i));
	}

	return 1;
}

/**
 * @brief
 * write to file, so the indirect block doesn't commit to flash.
 * lseek to file end (the transactional one, not the flash one!)
 * write again, make sure writing was done to transactional file end
 * should succeed.
 *
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest16(){
	int32_t tid, fd, res, f_id;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	int32_t buf_size = INODE_FILE_DATA_SIZE-50, write_size = buf_size-50;
	uint8_t write_buf[INODE_FILE_DATA_SIZE-50];
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	inode_t *ino_ptr;
	user_id user = 1;

	init_logical_address(log_addr);
	/* set user in process*/
	SET_CURRENT_USER(user);

	fsMemset(write_buf, byte+1, buf_size);
	fsMemset(write_buf, byte, write_size);

	/* create file */
	fd = open(f_name, CREAT_FLAGS, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\n\n\n\n\nopen success");
	f_id = GET_FILE_ID_BY_FD(fd);
	tid = getTidByFileId(f_id);

	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	/* write to file*/
	res = write(fd, write_buf, write_size);
	tid = getTidByFileId(f_id);
//	assert(0);
	VERIFY(COMPARE(res, write_size))
//	PRINT("\nwrite success");

	/* verify open file entry */
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), write_size));

	/* seek to file end (should seek to transactional file end, not the flash one!*/
	VERIFY(COMPARE(lseek(fd, 0, FS_SEEK_END), write_size));

	/* verify reclaimed offset increased
	 * and that the old one is the new inode address*/
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), GET_LOGICAL_OFFSET(log_addr)));
//	PRINT("\nrec offset comparison success");
//	VERIFY(COMPARE(GET_LOGICAL_OFFSET(TRANSACTION_GET_INO_ADDR_PTR(tid)), GET_LOGICAL_OFFSET(log_addr)));
//	PRINT("\nino addr offset comparison success");
//	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(TRANSACTION_GET_INO_ADDR_PTR(tid)), GET_LOGICAL_SEGMENT(log_addr)));
//	PRINT("\ninode address success");

	/* verify file size */
//	PRINT_MSG_AND_NUM("\nafter write file size=",getFileSize(f_id));
	VERIFY(COMPARE(getFileSize(f_id), write_size));
	fsMemcpy(fs_buffer, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE);
//	VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid), fs_buffer));
//	PRINT("\nfile size success");
	/* verify file data, and that the physical block data after written data
	 * contains empty bytes */
	for(i=0; i<write_size; i++){
		VERIFY(COMPARE(fs_buffer[i], byte))
	}

	for(;i<INODE_FILE_DATA_SIZE;i++){
		VERIFY(COMPARE(fs_buffer[i], 0xff))
	}
//	PRINT("\nwritten data success");
	/* verify inode details */
	ino_ptr = CAST_TO_INODE(fs_buffer);
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), write_size));
//	PRINT("\nnbytes success");
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1));
//	PRINT("\nnblocks success");
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), f_id));
//	PRINT("\nf_id success");
//	PRINT_MSG_AND_NUM("\nf_type=",INODE_GET_FILE_TYPE(ino_ptr));
	VERIFY(COMPARE(INODE_GET_FILE_TYPE(ino_ptr), FTYPE_FILE));
//	PRINT("\ninode data success");
	for(i=0; i<DIRECT_INDEX_ENTRIES;i++){
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

	VERIFY(verifyOpenFileEntry(fd,CREAT_FLAGS,write_size,user,0));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd),2,1));
	VERIFY(verifyFentriesVnodesEmpty(fd));

	return 1;
}

/**
 * @brief
 * write to file, and re-write parts already written.
 * verify vot count
 *
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest17(){
	int32_t tid, fd, f_id, vot_count;
	uint8_t *f_name = "/file1.dat", byte = 'c';
	uint8_t write_buf[FS_BLOCK_SIZE];
	user_id user = 1;

	/* set user in process*/
	SET_CURRENT_USER(user);

	fsMemset(write_buf, byte, FS_BLOCK_SIZE);

	/* create file */
	fd = open(f_name, CREAT_FLAGS, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\n\n\n\n\nopen success");
	f_id = GET_FILE_ID_BY_FD(fd);
	tid = getTidByFileId(f_id);

	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, write_buf, INODE_FILE_DATA_SIZE)));
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
	vot_count = TRANSACTION_GET_VOTS_COUNT(tid);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, lseek(fd, INODE_FILE_DATA_SIZE, FS_SEEK_SET)));
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
//	L("after re-write expected vot count %d, actual %d", vot_count+1, TRANSACTION_GET_VOTS_COUNT(tid));
	if(IS_CACHE_EMPTY()){
	/* vot count shouldn cahnge since we rewrote a page*/
	VERIFY(COMPARE(vot_count+1, TRANSACTION_GET_VOTS_COUNT(tid)));
	}
	else{
	/* vot count shouldn't cahnge since everyhintg is cached*/
	VERIFY(COMPARE(vot_count, TRANSACTION_GET_VOTS_COUNT(tid)));
	}

	return 1;
}

/**
 * @brief
 * peform write that exhausts all spare pages, so that the transaction commits even though
 * we didn't perform close. in regular allocation mode
 * also test expected vots count, and free pages count
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest20(){
	int32_t tid, fd, f_id, old_free_count, old_vots, max_offset, f_offset, f_size, offset;
	uint8_t *f_name = "/file1.dat", byte = 'z';
	uint8_t buf[FS_BLOCK_SIZE];
	uint32_t i,j,k;
	user_id user = 1;

	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < TRIPLE_DATA_OFFSET){
		return 1;
	}

	/* set user in process*/
	SET_CURRENT_USER(user);

	fsMemset(buf, byte, FS_BLOCK_SIZE);

	/* create file */
//	PRINT("\ncreat start");
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	old_free_count = GET_FREE_COUNTER();
//	PRINT("\ncreat success\n\n\n\n");
	/* write to inode file data*/
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\ninode data write success");
	f_id = GET_FILE_ID_BY_FD(fd);
	tid  = getTidByFileId(f_id);
//	PRINT("\nvots success");
	VERIFY(COMPARE(GET_FREE_COUNTER(), old_free_count));
	old_free_count = GET_FREE_COUNTER();
//	PRINT("\ninode file data success");

	/* write direct entries */
	for(i=0; i < DIRECT_INDEX_ENTRIES; i++){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	}

	/* verify free count.
	 * file inode was commited+DIRECT_INDEX_ENTRIES were written.
	 * verify free pages count*/
	VERIFY(COMPARE(GET_FREE_COUNTER(), old_free_count-DIRECT_INDEX_ENTRIES));
	old_free_count = GET_FREE_COUNTER();
//	PRINT("\ndirect success");
	/* write indirect block*/
	for(i=0; i <LOG_ADDRESSES_PER_BLOCK;i++){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	}
//	PRINT_MSG_AND_NUM("\nvots count=",TRANSACTION_GET_VOTS_COUNT(tid));
	VERIFY(COMPARE(TRANSACTION_GET_VOTS_COUNT(tid), 1));
	old_vots = TRANSACTION_GET_VOTS_COUNT(tid);

	/* verify free count.
	 * file inode was commited, +LOG_ADDRESSES_PER_BLOCK indirect entries.
	 * verify free pages count*/
	VERIFY(COMPARE(GET_FREE_COUNTER(), old_free_count-LOG_ADDRESSES_PER_BLOCK-1));
	old_free_count = GET_FREE_COUNTER();

//	PRINT("\nindirect success");
	/* write double block*/
	/* write first two double indirect block*/
	for(j=0; j<2; j++){
		for(i=0; i <LOG_ADDRESSES_PER_BLOCK;i++){
			VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
		}
	}
//	PRINT_MSG_AND_NUM("\nvots count=",TRANSACTION_GET_VOTS_COUNT(tid));
	VERIFY(COMPARE(TRANSACTION_GET_VOTS_COUNT(tid), old_vots+2));
	old_vots = TRANSACTION_GET_VOTS_COUNT(tid);

	/* verify free count.
	 * former indirect block was commited, and file inode was commited+2*LOG_ADDRESSES_PER_BLOCK indirect entries
	 * +double indirect block commited, double block, and another inode commit
	 * verify free pages count*/
	VERIFY(COMPARE(GET_FREE_COUNTER(), old_free_count-2* LOG_ADDRESSES_PER_BLOCK-2 -3));
	old_free_count = GET_FREE_COUNTER();
//	PRINT("\nfirst 2 double entries success");

	/* write all double indirect block*/
	for(; j<LOG_ADDRESSES_PER_BLOCK; j++){
//		PRINT_MSG_AND_NUM("\ndouble entry ", j);
		for(i=0; i <LOG_ADDRESSES_PER_BLOCK;i++){
			VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
		}

		if(old_vots / LOG_ADDRESSES_PER_BLOCK != (old_vots+2)/ LOG_ADDRESSES_PER_BLOCK) old_free_count--;
//		PRINT_MSG_AND_NUM("\nvots count=",TRANSACTION_GET_VOTS_COUNT(tid));
		VERIFY(COMPARE(TRANSACTION_GET_VOTS_COUNT(tid), old_vots+2));
		old_vots = TRANSACTION_GET_VOTS_COUNT(tid);

//		PRINT_MSG_AND_NUM(", after write file size=", CALC_IN_BLOCKS(getFileSize(GET_FILE_ID_BY_FD(fd))));
		/* verify free count - every write starts with commiting previous indirect block (and new double+new inode)*/
//		PRINT_MSG_AND_NUM(" free=", GET_FREE_COUNTER());
//		PRINT_MSG_AND_NUM(" expected=", old_free_count-LOG_ADDRESSES_PER_BLOCK-3);
		VERIFY(COMPARE(GET_FREE_COUNTER(), old_free_count-LOG_ADDRESSES_PER_BLOCK-3));
//		PRINT(", free counter success");
		old_free_count = GET_FREE_COUNTER();
	}
//	PRINT("\nall double entries success");
//	PRINT_MSG_AND_NUM("\nbefore starting riple writes, vots count=",TRANSACTION_GET_VOTS_COUNT(tid));
	/* write first two triple entries*/
	for(k=0; k<2; k++){
		/* write all double indirect block*/
		for(j=0; j<LOG_ADDRESSES_PER_BLOCK; j++){
//			PRINT_MSG_AND_NUM("\nfile offset=",TRANSACTION_GET_FILE_OFFSET(tid));
//			PRINT_MSG_AND_NUM(" k=",k);
//			PRINT_MSG_AND_NUM(" j=",j);
			for(i=0; i <LOG_ADDRESSES_PER_BLOCK;i++){
				VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
			}
			old_free_count -= LOG_ADDRESSES_PER_BLOCK; /* actual file blocks*/

//			PRINT_MSG_AND_NUM(" vots count=",TRANSACTION_GET_VOTS_COUNT(tid));

			/* we commited last double indirect*/
			if(k == 0 && j==0){
				if(old_vots / LOG_ADDRESSES_PER_BLOCK != (old_vots+2)/ LOG_ADDRESSES_PER_BLOCK || (old_vots+2) % LOG_ADDRESSES_PER_BLOCK == 0) old_free_count--;
				old_free_count -= 3; /* re-write of double block*/
//				PRINT_MSG_AND_NUM("\nfree=", GET_FREE_COUNTER());
//				PRINT_MSG_AND_NUM(" expected=", old_free_count);
				VERIFY(COMPARE(GET_FREE_COUNTER(), old_free_count));

				VERIFY(COMPARE(TRANSACTION_GET_VOTS_COUNT(tid), old_vots+2));
				old_vots = TRANSACTION_GET_VOTS_COUNT(tid);
				continue;
			}

			old_free_count -= 4; /* re-write of double block*/

			/* we commited first triple indexed indirect block*/
			if(k==0 && j==1){
				if(old_vots / LOG_ADDRESSES_PER_BLOCK != (old_vots+1)/ LOG_ADDRESSES_PER_BLOCK || (old_vots+1) % LOG_ADDRESSES_PER_BLOCK == 0) old_free_count--;
//				PRINT_MSG_AND_NUM("\nfree=", GET_FREE_COUNTER());
//				PRINT_MSG_AND_NUM(" expected=", old_free_count);
				VERIFY(COMPARE(GET_FREE_COUNTER(), old_free_count));

				VERIFY(COMPARE(TRANSACTION_GET_VOTS_COUNT(tid), old_vots+1));
				old_vots = TRANSACTION_GET_VOTS_COUNT(tid);
				continue;
			}

			/* we commited first indirect entry in a triple-double block*/
			if(j == 1){
				if(old_vots / LOG_ADDRESSES_PER_BLOCK != (old_vots+2)/ LOG_ADDRESSES_PER_BLOCK || (old_vots+2) % LOG_ADDRESSES_PER_BLOCK == 0) old_free_count--;
//				PRINT_MSG_AND_NUM("\nfree=", GET_FREE_COUNTER());
//				PRINT_MSG_AND_NUM(" expected=", old_free_count);
				VERIFY(COMPARE(GET_FREE_COUNTER(), old_free_count));

				VERIFY(COMPARE(TRANSACTION_GET_VOTS_COUNT(tid), old_vots+2));
				old_vots = TRANSACTION_GET_VOTS_COUNT(tid);
				continue;
			}
			if(old_vots / LOG_ADDRESSES_PER_BLOCK != (old_vots+3)/ LOG_ADDRESSES_PER_BLOCK || (old_vots+3) % LOG_ADDRESSES_PER_BLOCK == 0) old_free_count--;
//			PRINT_MSG_AND_NUM("\nfree=", GET_FREE_COUNTER());
//			PRINT_MSG_AND_NUM(" expected=", old_free_count);
			VERIFY(COMPARE(GET_FREE_COUNTER(), old_free_count));

			VERIFY(COMPARE(TRANSACTION_GET_VOTS_COUNT(tid), old_vots+3));
			old_vots = TRANSACTION_GET_VOTS_COUNT(tid);
		}
	}
//	PRINT("\nfirst 2 triple entries success");
	/* verify free count - every write starts with commiting previous indirect block (and new double+new inode)
	 * +2*LOG_ADDRESSES_PER_BLOCK*LOG_ADDRESSES_PER_BLOCK of direct blocks + triple block, double blocks, indirect blocks
	 * and new inode write for every indirect block commited*/
//	PRINT_MSG_AND_NUM("\nfree=", GET_FREE_COUNTER());

//	old_free_count  = old_free_count-2*LOG_ADDRESSES_PER_BLOCK*LOG_ADDRESSES_PER_BLOCK;
//	old_free_count -= (2*LOG_ADDRESSES_PER_BLOCK-1); /* written indirect blocks*/
//	old_free_count -= (2*LOG_ADDRESSES_PER_BLOCK-1); /* double blocks (including re-writes)*/
//	old_free_count -= (2*LOG_ADDRESSES_PER_BLOCK-1); /* triple block (including re-writes)*/
//	old_free_count -= (2*LOG_ADDRESSES_PER_BLOCK-1); /* inode block (including re-writes)*/
//	PRINT_MSG_AND_NUM(" expected=", old_free_count);
	VERIFY(COMPARE(GET_FREE_COUNTER(), old_free_count));
	old_free_count = GET_FREE_COUNTER();
//	PRINT("\nall triple entries success");

	/* write until last page possible*/
	while(!IS_MIN_FREE_PAGES_REACHED()){
//		PRINT_MSG_AND_NUM("\nfree=", GET_FREE_COUNTER());
//		PRINT_MSG_AND_NUM(", total free pages=", calcTotalFreePages());

		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	}
//	PRINT_MSG_AND_NUM("\nFS_MIN_FREE_PAGES=", FS_MIN_FREE_PAGES);
//	PRINT("\nabout to check obs+free counters");
	old_vots = TRANSACTION_GET_VOTS_COUNT(tid);
//	PRINT_MSG_AND_NUM("\nold_vots=", old_vots);
	old_vots += GET_OBS_COUNT();
	old_free_count = GET_FREE_COUNTER();
//	PRINT_MSG_AND_NUM("\nfree=", GET_FREE_COUNTER());

//	PRINT_MSG_AND_NUM("\nold obs=", GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM("\nstate rec? ",IS_STATE_RECLAMATION());

	max_offset  = TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid);
	f_offset = TRANSACTION_GET_FILE_OFFSET(tid);
	fsMemset(buf, byte+1, FS_BLOCK_SIZE);
	/* perform another write.
	 * should commit transaction temporarily:
	 * - perform all vots
	 * - commit inode
	 * - commit to inode0
	 * - init transaction
	 * - and perofrm write*/
//	PRINT("\nDO LAST WRITE!");
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));

//	PRINT_MSG_AND_NUM("\nstate rec? ",IS_STATE_RECLAMATION());
//	PRINT("\nlast write success");
	VERIFY(COMPARE(TRANSACTION_GET_VOTS_COUNT(tid),0));
//	PRINT("\nlast vots count success");
	VERIFY(COMPARE(TRANSACTION_GET_VOTS_OFFSET(tid),0));
//	PRINT("\nlast vots offset success");
	VERIFY(COMPARE(TRANSACTION_GET_INO(tid),f_id));
//	PRINT("\nino num success");
//	PRINT_MSG_AND_NUM("\nmax written ofset",TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid));
	VERIFY(COMPARE(TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid), max_offset+FS_BLOCK_SIZE));
//	PRINT("\nmax offset success");
	VERIFY(COMPARE(TRANSACTION_GET_BLOCKS_ALLOCS(tid), 1));
//	PRINT("\nblock allocs success");
	VERIFY(COMPARE(TRANSACTION_GET_TYPE(tid),T_TYPE_WRITE));
//	PRINT("\ntype success");
//	PRINT_MSG_AND_NUM("\nfile offset",TRANSACTION_GET_FILE_OFFSET(tid));
//	PRINT_MSG_AND_NUM(" expected=",f_offset);
	VERIFY(COMPARE(TRANSACTION_GET_FILE_OFFSET(tid),f_offset));
//	PRINT("\nfile offset success");
	VERIFY(!IS_ADDR_EMPTY(TRANSACTION_GET_INO_ADDR_PTR(tid)));
//	PRINT("\nblock file offset success");

	/* verify last */
	VERIFY(!fsReadBlockSimple(TRANSACTION_GET_PREV_ADDR_PTR(tid), fs_buffer));
	for(i=0; i<FS_BLOCK_SIZE;i++){
//		PRINT_MSG_AND_HEX("\n",i);
//		PRINT_MSG_AND_HEX(". ",fs_buffer[i]);
//		PRINT_MSG_AND_HEX(" byte+1=",byte+1);
		VERIFY(COMPARE(fs_buffer[i], byte+1));
	}
//	VERIFY(COMPARE(TRANSACTION_GET_TOTAL_BLOCK_ALLOCS(tid), 1));
	/* new obs count should be old vots
	 * + 3 (new inode, triple, double blocks)
	 * + 1 (for inode0 re-write)
	 * + vot pages*/
	i = (old_vots / LOG_ADDRESSES_PER_BLOCK) +1; /* calc vot pages */
//	PRINT_MSG_AND_NUM("\nvot pages=",i);
//	PRINT_MSG_AND_NUM("\nold_vots=",old_vots);
//	PRINT_MSG_AND_NUM("\nobs=", GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" expected=", old_vots+i+3+1);
	VERIFY(COMPARE(old_vots+i+3+1,GET_OBS_COUNT()));
//	PRINT("\nobs check verified");
	/* verify page read*/
	f_size = getFileSize(f_id);

	/*read file inode data */
	VERIFY(!readFileBlock(fs_buffer, f_id ,0, TRANSACTION_GET_INO_ADDR_PTR(tid), tid));
	for(i=0;i<INODE_FILE_DATA_SIZE;i++){
		VERIFY(COMPARE(fs_buffer[i],byte));
	}

	for(;i<FS_BLOCK_SIZE;i++){
		VERIFY(COMPARE(fs_buffer[i],0xff));
	}

//	PRINT("\ninode file data success");
	/* verify all file blocks, except last one*/
	for(offset = INODE_FILE_DATA_SIZE; offset< f_size-FS_BLOCK_SIZE;offset+=FS_BLOCK_SIZE){
		init_fsbuf(fs_buffer);
		VERIFY(!readFileBlock(fs_buffer, f_id ,offset, TRANSACTION_GET_INO_ADDR_PTR(tid), tid));

		for(i=0;i<FS_BLOCK_SIZE;i++){
//			printBlock(fs_buffer);
			VERIFY(COMPARE(fs_buffer[i],byte));
		}
	}
//	PRINT("\nall blocks except last success");

	/*verify last block */
	VERIFY(!readFileBlock(fs_buffer, f_id ,offset, TRANSACTION_GET_INO_ADDR_PTR(tid), tid));

	for(i=0;i<FS_BLOCK_SIZE;i++){
		VERIFY(COMPARE(fs_buffer[i],byte+1));
	}
	return 1;
}

/**
 * @brief
 * perform write that exhausts all spare pages, so we don't have anymore left.
 * keep writing until an error is returned, and verify that the transaction was aborted
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest21(){
	int32_t tid1, fd1, res, f_id1, old_free_count, total_free;
	uint8_t *f_name1 = "/file1.dat", byte = 'z';
	uint8_t buf[FS_BLOCK_SIZE];
	uint32_t i;
	user_id user = 1;

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
//	PRINT("\ncreat start");
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\ncreat 1 success\n\n\n\n");

	/* save total free pages before transaction */
	total_free = calcTotalFreePages();

	old_free_count = GET_FREE_COUNTER();

	/* write to inode file data 1*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd1, buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\ninode data write success");
	f_id1 = GET_FILE_ID_BY_FD(fd1);
	tid1  = getTidByFileId(f_id1);
//		i=0;
	while(1){

		res = write(fd1, buf, FS_BLOCK_SIZE);

		if(res < 0){
			break;
		}

//		PRINT_MSG_AND_NUM("\n", i);
//		PRINT_MSG_AND_NUM(". res=", res);
		i++;
	}

	/* verify no transactions*/
	for(i=0; i<FS_MAX_N_TRANSACTIONS;i++){
		L("transaction ino is %d", TRANSACTION_GET_INO(i));
		VERIFY(IS_TRANSACTION_EMPTY(i));
	}

	VERIFY(COMPARE(res, FS_ERROR_WRT_THRSHLD));

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
 * perform write that exhausts all spare pages, so that the transaction aborts
 * other concurrent transactions in order to complete
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest22(){
	int32_t tid1, fd1, f_id1, total_free;
	int32_t tid2, fd2, f_id2, count;
	uint8_t *f_name1 = "/file1.dat", *f_name2 = "/file2.dat", byte = 'z';
	uint8_t buf[FS_BLOCK_SIZE];
	uint32_t i;
	user_id user = 1;

	if(FS_MAX_N_TRANSACTIONS < 2){
		return 1;
	}

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
//	PRINT("\ncreat start");
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\ncreat1 success\n\n\n\n");

	/* create file */
//	PRINT("\ncreat start");
	fd2 = creat(f_name2, 0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT("\ncreat2 success\n\n\n\n");

	/* save total free pages before transaction */
	total_free = calcTotalFreePages();

	/* write to inode file data1*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd1, buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\ninode 1 data write success");
	f_id1 = GET_FILE_ID_BY_FD(fd1);
	tid1  = getTidByFileId(f_id1);

	/* write to inode file data2*/
	fsMemset(buf, byte-1, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd2, buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\ninode 2 data write success");
	f_id2 = GET_FILE_ID_BY_FD(fd2);
	tid2  = getTidByFileId(f_id2);

	/* perform a write that forces file 2 inode to commit*/
	for(i=0;i<DIRECT_INDEX_ENTRIES+1;i++){
		write(fd2, buf, FS_BLOCK_SIZE);
	}
	/* save count of pages that will be flushed from cache when
	 * the other transaction takes over -
	 * direct entries+indirect entry*/
	count = DIRECT_INDEX_ENTRIES+1;
//	PRINT("\nfinished writing to file2");
	fsMemset(buf, byte, FS_BLOCK_SIZE);

	/* write to file 1 until we exhaust all spare pages*/
	while(FS_TOTAL_FREE_PAGES > FS_MIN_FREE_PAGES){
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}
//	PRINT("\nfinished writing until min almost reached");

	/* perform last write
	 * - verify tid2 aborted
	 * - verify we have total free pages of :
	 * FS_MIN_FREE_PAGES
	 * + count
	 * -1 for the last write*/
	 for(i=0; i< 5; i++){
		 VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	 }
//	 PRINT("\nlast write done");
//	 PRINT_MSG_AND_NUM("\ntid2 ino num=",TRANSACTION_GET_INO(tid2));
	 VERIFY(IS_TRANSACTION_EMPTY(tid2));
//	 PRINT_MSG_AND_NUM("\nexpected free=",FS_MIN_FREE_PAGES+count-5);
//	 PRINT_MSG_AND_NUM("\nactual free=",FS_TOTAL_FREE_PAGES);
//	 VERIFY(COMPARE(FS_TOTAL_FREE_PAGES, FS_MIN_FREE_PAGES+count-5)); /* this should be tested!!*/
//	 PRINT_MSG_AND_NUM("\nfile size=",getFileSize(f_id2));

	 /* verify file 2 size hasn't chnaged (since all writes have been aborted */
	 VERIFY(COMPARE(getFileSize(f_id2),0));

	 return 1;
}

/**
 * @brief
 * peform write that exhausts all spare pages, so that the transaction aborts
 * other concurrent transactions in order to complete, and also performs
 * a temporary commit
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest23(){
	int32_t tid1, fd1, f_id1, old_free_count, old_vots, total_free, old_obs;
	int32_t tid2, fd2, f_id2, count;
	uint8_t *f_name1 = "/file1.dat", *f_name2 = "/file2.dat", byte = 'z';
	uint8_t buf[FS_BLOCK_SIZE];
	uint32_t i;
	user_id user = 1;
	bool_t is_triple = FS_MAX_FILESIZE > TRIPLE_DATA_OFFSET;

	if(FS_MAX_N_TRANSACTIONS < 2){
		return 1;
	}

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
//	PRINT("\ncreat start");
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\ncreat1 success\n\n\n\n");

	/* create file */
//	PRINT("\ncreat start");
	fd2 = creat(f_name2, 0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT("\ncreat2 success\n\n\n\n");

	/* save total free pages before transaction */
	total_free = calcTotalFreePages();

	/* write to inode file data1*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd1, buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\ninode 1 data write success");
	f_id1 = GET_FILE_ID_BY_FD(fd1);
	tid1  = getTidByFileId(f_id1);

	count = calcTotalFreePages();
	/* write to inode file data2*/
	fsMemset(buf, byte-1, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd2, buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\ninode 2 data write success");
	f_id2 = GET_FILE_ID_BY_FD(fd2);
	tid2  = getTidByFileId(f_id2);

	/* force file 2 inode to commit*/
	for(i=0;i<DIRECT_INDEX_ENTRIES+1;i++){
		write(fd2, buf, FS_BLOCK_SIZE);
	}

	/* save count of pages written by this transaction*/
	count = count-calcTotalFreePages();
//	PRINT("\nfinished writing to file2");
	fsMemset(buf, byte, FS_BLOCK_SIZE);

	/* write to file 1 until we exhaust all spare pages
	 * and force second transaction to abort*/
	while(!IS_MIN_FREE_PAGES_REACHED()){
//		PRINT_MSG_AND_NUM("\nbytes until we reach max file size=", FS_MAX_FILESIZE-getFileSize(GET_FILE_ID_BY_FD(fd1)));
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}

//	PRINT_MSG_AND_NUM("\nb4 aborting tid2, tid1 prev addr=", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(0)));
	VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
//	PRINT("\naborted second transaction");
//	PRINT_MSG_AND_NUM("\nFS_MAX_FILESIZE=", FS_MAX_FILESIZE);
	/* keep writing until we again exhaust all pages*/
	while(!IS_MIN_FREE_PAGES_REACHED()){
//		PRINT_MSG_AND_NUM("\nbytes until we reach max file size=", FS_MAX_FILESIZE-getFileSize(GET_FILE_ID_BY_FD(fd1)));
//		PRINT_MSG_AND_HEX("\ntid1 prev addr=", *CAST_TO_UINT32(TRANSACTION_GET_PREV_ADDR_PTR(tid1)));
//		PRINT_MSG_AND_NUM("\nprev addr=", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid1)));
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}

//	PRINT("\nabout to temporarily commit first transaction");
	total_free = calcTotalFreePages();
	old_vots = TRANSACTION_GET_VOTS_COUNT(tid1);
	old_free_count = GET_FREE_COUNTER();
	old_obs = GET_OBS_COUNT();
//	PRINT_MSG_AND_NUM("\nb4 last write old_vots=",old_vots);
//	PRINT_MSG_AND_NUM("\nfrees=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nobs=",GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM("\ntotal_free=",total_free);
//	PRINT("\n\n\n\n\ndo last write");
//	PRINT_MSG_AND_NUM("\ntransaction max written offset=",TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid1));
	VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
//	PRINT_MSG_AND_NUM("\nstate is rec?=",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM("\nexpected total frees=",total_free+old_vots+i-1-1);
//	PRINT_MSG_AND_NUM("\nfrees=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\ntotal frees=",calcTotalFreePages());

	VERIFY(verifyTransactionsEmpty(getTidByFileId(GET_FILE_ID_BY_FD(fd1))));
//	PRINT_MSG_AND_NUM("\nrec addr=",logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
	/* calc number of vot pages */
	i = (old_vots / LOG_ADDRESSES_PER_BLOCK) +1;
//	PRINT_MSG_AND_NUM("\nobs=",GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" expected obs=",old_obs+old_vots+i+3+is_triple);
//	PRINT_MSG_AND_NUM("\nfrees=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM(" expected frees=",old_free_count-6-is_triple);
//	PRINT_MSG_AND_NUM("\nis_triple=",is_triple);
//	PRINT_MSG_AND_NUM(" is_double=",is_double);
	 /* verify new obs count=
	  * old obs count
	  * + old vots
	  * + vots pages
	  * + 4 (old inode0, inode, triple, double) */

	VERIFY(COMPARE(GET_OBS_COUNT(), old_obs+old_vots+i+3+is_triple));
	/* verify new free count=
	 * old free count
	 * - 1 new vot page
	 * - 6 (new data page, indirect, double,triple,inode, inode0) */
	VERIFY(COMPARE(GET_FREE_COUNTER(),old_free_count-6-is_triple))

	/* NOTICE - in state of reclamation the last page is using an obsolete page*/
	return 1;
}

/**
 * @brief
 * peform write that exhausts all spare pages, so that the transaction commits even though
 * we didn't perform close, in reclamation mode.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest24(){
	int32_t tid1, fd1, f_id1, old_free_count, old_vots, total_free, old_obs;
	uint8_t *f_name1 = "/file1.dat", byte = 'z';
	uint8_t buf[FS_BLOCK_SIZE];
	uint32_t i;
	user_id user = 1;

	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < TRIPLE_DATA_OFFSET){
		return 1;
	}

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
//	PRINT("\ncreat start");
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\ncreat1 success\n\n\n\n");
	f_id1 = GET_FILE_ID_BY_FD(fd1);
	tid1  = getTidByFileId(f_id1);
	/* write to inode file data1*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd1, buf, INODE_FILE_DATA_SIZE)));

	/* now repeat check in reclaamtion mode*/
	/* continue writing until we start reclamation */
	while(!IS_STATE_RECLAMATION()){
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}
//	PRINT("\nwrote until reclamation state");
	/* keep writing until we again exhaust all pages*/
	while(!IS_MIN_FREE_PAGES_REACHED()){
//		PRINT_MSG_AND_HEX("\ntid1 prev addr=", *CAST_TO_UINT32(TRANSACTION_GET_PREV_ADDR_PTR(tid1)));
//		PRINT_MSG_AND_NUM("\nprev addr=", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid1)));
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}
//	PRINT("\nwrote in reclamation state until min pages reached");
//	PRINT_MSG_AND_NUM("\ntid 1 empty?=",IS_TRANSACTION_EMPTY(tid1));
	old_vots = TRANSACTION_GET_VOTS_COUNT(tid1);
//	PRINT_MSG_AND_NUM("\nb4 last write old_vots=",old_vots);
	old_free_count = GET_FREE_COUNTER();
//	PRINT_MSG_AND_NUM("\nfrees=",GET_FREE_COUNTER());
	old_obs = GET_OBS_COUNT();
//	PRINT_MSG_AND_NUM("\nobs=",GET_OBS_COUNT());
	total_free = calcTotalFreePages();
//	PRINT_MSG_AND_NUM("\ntotal_free=",total_free);

//	PRINT("\ndo last write");
	VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
//	PRINT_MSG_AND_NUM("\nstate is rec?=",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM("\nfrees=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nobs=",GET_OBS_COUNT());

	/* calc number of vot pages */
	i = (old_vots / LOG_ADDRESSES_PER_BLOCK) +1;

	/* verify total free count=
	 * old free pages
	 * + vots pages (-1 for the last written)
	 * + 4 (old doube, triple ,inode, inode0)
	 * -6 new data page, indirect, double,triple, inode, inode0
	 */
//	PRINT_MSG_AND_NUM("\ntotal frees=",calcTotalFreePages());
//	PRINT_MSG_AND_NUM(" expected=",total_free+old_vots+(i-1)-6+4);
	VERIFY(COMPARE(calcTotalFreePages(), total_free+old_vots+(i-1)-6+4));
//	 /* verify new obs count=
//	  * old obs count
//	  * + vots
//	  * + vots pages
//	  * + 4 (old inode0, inode, triple, double) */
//	VERIFY(COMPARE(GET_OBS_COUNT(), old_obs+old_vots+i+4));
//	/* verify new free count=
//	 * old free count
//	 * - 1 new vot page
//	 * - 6 (new data page, indirect, double,triple,inode, inode0) */
//	VERIFY(COMPARE(GET_FREE_COUNTER(),old_free_count-7));
	return 1;
}

/**
 * @brief
 * peform write that exhaustst all spare pages, so we don't have anymore left and
 * a temporary commit is performed. do some more writing so we abort
 * and commited bytes count (<write count) is returned.
 *
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest25(){
	int32_t tid1, fd1, res, f_id1, total_free, page_count = TRANSACTION_COMMIT_MIN_VOTS+1;
	uint8_t *f_name1 = "/file1.dat", byte = 'z';
	int32_t buf_size = FS_BLOCK_SIZE*20;
	uint8_t buf[FS_BLOCK_SIZE*20];
	uint32_t i,j;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(vot_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	user_id user = 1;

	/* on the board we cannot test writing from external memory, since we may not have
	 * enough RAM for this test */
#ifdef ARM
	return 1;
#endif
	init_logical_address(log_addr);
	init_logical_address(ino_log_addr);
	init_logical_address(vot_log_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
//	PRINT("\ncreat start");
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\ncreat 1 success\n\n\n\n");
	/* save total free pages before transaction */
	total_free = calcTotalFreePages();

	/* write to inode file data 1*/
	fsMemset(buf, byte, buf_size);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd1, buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\ninode data write success");
	f_id1 = GET_FILE_ID_BY_FD(fd1);
	tid1  = getTidByFileId(f_id1);

	/* allow writing of only page_count pages, until we perform a temporary commit*/
	SET_OBS_COUNT(0);
	SET_FREE_COUNTER(FS_MIN_FREE_PAGES+page_count);

	/* set mock vots */
	SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, 2,2);
	SET_LOGICAL_SEGMENT(vot_log_addr, 2);
	SET_LOGICAL_OFFSET(vot_log_addr, 4);

	for(i=0; i<page_count; i++){
		SET_LOGICAL_OFFSET(vot_log_addr, i);
		TRANSACTION_ADD_VOT(tid1, vot_log_addr);
		TRANSACTION_INC_VOTS_OFFSET(tid1);
		TRANSACTION_INC_VOTS_COUNT(tid1);
	}
	/* perofrm large write.
	 * should write page_count blocks until a temporary commit is performed.
	 * then the next writes would succeed (because we added page_count obsolete pages)
	 * and then an error would occur. write should return page_count*FS_BLOCK_SIZE (that were actualy committed) */
//	PRINT("\n\n\n perofrm big write");
	res = write(fd1, buf, buf_size);

	/* verify no transactions*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));

//	PRINT_MSG_AND_NUM("\n res=",res);
//	PRINT_MSG_AND_NUM("\n page_count=",page_count);
	VERIFY(COMPARE(res, page_count*FS_BLOCK_SIZE));

	/* check file, that indeed only page_count writes succeeded (and all others failed)*/
	VERIFY(!getInode(fs_buffer, 2, ino_log_addr));
	for(i=0; i < page_count;i++){
		VERIFY(!fsReadBlockSimple(ino_log_addr, fs_buffer));
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
		for(j=0;j<FS_BLOCK_SIZE; j++){
			VERIFY(COMPARE(byte, fs_buffer[j]));
		}
	}

	VERIFY(!fsReadBlockSimple(ino_log_addr, fs_buffer));
	for(;i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	return 1;
}

/**
 * @brief
 * write to a file that contains data that has already been written, and is open for reading.
 * write beyond the read offset, and verify that when we continue to read the read data is the
 *
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest26(){
	int32_t fd1, fd2, user = 0, i, j;
	uint8_t buf[FS_BLOCK_SIZE], byte = 'a', f_name[12] = "/file1.dat";

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* create file */
//	PRINT("\ncreat start");
	fd1 = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\ncreat success");
	fsMemset(buf, byte, FS_BLOCK_SIZE);

	/* write to file*/
	VERIFY(COMPARE(write(fd1, buf, INODE_FILE_DATA_SIZE), INODE_FILE_DATA_SIZE));
	for(i=0; i<100; i++){
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
	}

	VERIFY(!close(fd1));
//	PRINT("\nfirst write success");
	/* read 50 blocks*/
	fd1 = open(f_name, NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\nfd1 re-open success");

	fsMemset(buf, 0, FS_BLOCK_SIZE);
	VERIFY(COMPARE(read(fd1, buf, INODE_FILE_DATA_SIZE), INODE_FILE_DATA_SIZE));

	for(i=0; i<50; i++){
		VERIFY(COMPARE(read(fd1, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));

		for(j=0; j< FS_BLOCK_SIZE; j++){
			VERIFY(COMPARE(buf[j], byte));
		}
	}
//	PRINT("\nfirst read success");

	/* re-open file for writing*/
	fd2 = open(f_name, NANDFS_O_WRONLY, 0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT("\nsecond opend success");
	VERIFY(COMPARE(lseek(fd2, FS_BLOCK_SIZE * 50, SEEK_SET), FS_BLOCK_SIZE * 50));
//	PRINT("\nlseek success");
	/* write again different data*/
	fsMemset(buf, byte+1, FS_BLOCK_SIZE);
	for(i=0; i<100; i++){
		VERIFY(COMPARE(write(fd2, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
	}
//	PRINT("\nwrite again success");

	/* and now read again from first fd*/
	for(i=0; i<50; i++){
		VERIFY(COMPARE(read(fd1, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
//		PRINT_MSG_AND_NUM("\nread #", i);
		for(j=0; j< FS_BLOCK_SIZE; j++){
			VERIFY(COMPARE(buf[j], byte+1));
		}
	}
//	PRINT("\nkeep reading success");

	return 1;
}



//error_t writeTest26(){
//	int fd = 0, i;
//	char f_name[12] = "/file1", buf[FS_BLOCK_SIZE/2], byte = 'a';
//
//	/* write to non-open file. verify failure*/
//	VERIFY(write(fd, buf, INODE_FILE_DATA_SIZE/2)<=0, "\nunexpected successful write to non-open file");
//
//	/* open file*/
//	fd = open(f_name, NANDFS_O_CREAT | NANDFS_O_WRONLY, 0);
//	VERIFY(fd>=0, "\nerror creating file");
//
//	/* write inode data*/
//	fsMemset(buf, byte, FS_BLOCK_SIZE/2);
//	VERIFY(write(fd, buf, INODE_FILE_DATA_SIZE/2)==INODE_FILE_DATA_SIZE/2, "\ninode data write failure");
//	VERIFY(write(fd, buf, INODE_FILE_DATA_SIZE/2)==INODE_FILE_DATA_SIZE/2, "\ninode data write failure");
//	PRINT("\nwrote inode data");
//
//	/* write direct data*/
//	fsMemset(buf, byte+1, FS_BLOCK_SIZE/2);
//	for(i=0; i < DIRECT_INDEX_ENTRIES*2; i++){
//		VERIFY(write(fd, buf, FS_BLOCK_SIZE/2)==FS_BLOCK_SIZE/2, "\ndirect data write failure");
//	}
//	PRINT("\nwrote direct data");
//
//	/* write indirect data*/
//	fsMemset(buf, byte+2, FS_BLOCK_SIZE/2);
//	for(i=0; i < LOG_ADDRESSES_PER_BLOCK*2; i++){
//		VERIFY(write(fd, buf, FS_BLOCK_SIZE/2)==FS_BLOCK_SIZE/2, "\nindirect data write failure");
//	}
//	PRINT("\nwrote indirect data");
//
//	/* write double data*/
//	fsMemset(buf, byte+3, FS_BLOCK_SIZE/2);
//	for(i=0; i < LOG_ADDRESSES_PER_BLOCK*LOG_ADDRESSES_PER_BLOCK*2; i++){
//		if(i%LOG_ADDRESSES_PER_BLOCK==0){ PRINT_MSG_AND_NUM("\nwrote double data #", i);}
//		VERIFY(write(fd, buf, FS_BLOCK_SIZE/2)==FS_BLOCK_SIZE/2, "\ndouble data write failure");
//	}
//	PRINT("\nwrote double data");
//
//	/* write triple data*/
//	fsMemset(buf, byte+4, FS_BLOCK_SIZE/2);
//	for(i=0; i < LOG_ADDRESSES_PER_BLOCK*LOG_ADDRESSES_PER_BLOCK*2; i++){
//		if(i%LOG_ADDRESSES_PER_BLOCK==0){ PRINT_MSG_AND_NUM("\nwrote triple data #", i);}
//
//		VERIFY(write(fd, buf, FS_BLOCK_SIZE/2)==FS_BLOCK_SIZE/2, "\ntriple data write failure");
//	}
//
//	PRINT("\nwrote triple data");
//
//	/* close and re-open file*/
//	VERIFY(!close(fd), "\nfirst close failure");
//	VERIFY(write(fd, buf, INODE_FILE_DATA_SIZE/2)<=0, "\nunexpected successful 2nd write to non-open file");
//
//	fd = open(f_name, NANDFS_O_WRONLY, 0);
//	VERIFY(fd>=0, "\nerror opening file");
//	/* write something again*/
//	fsMemset(buf, byte, FS_BLOCK_SIZE/2);
//	VERIFY(write(fd, buf, INODE_FILE_DATA_SIZE/2)==INODE_FILE_DATA_SIZE/2, "\ninode data write failure");
//
//	/* close file*/
//	VERIFY(!close(fd), "\nfinal close failure");
//
//	return 1;
//}
///********************* TODO:??**********************/
///**
// * @brief
// * perform write of vots page that is the write that crosses the free pages threshold
// * @return 1 if successful, 0 otherwise
// */
//error_t writeTest26(){
//	return 1;
//}
//
///**
// * @brief
// * perform write of some entries page that is the write that crosses the free pages threshold
// * @return 1 if successful, 0 otherwise
// */
//error_t writeTest27(){
//	return 1;
//}
