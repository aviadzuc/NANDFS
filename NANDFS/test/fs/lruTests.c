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

/** @file lruTests.c
 */
#include <test/fs/lruTests.h>
#include <test/fs/testsHeader.h>
#include <system.h>
#include <src/sequencing/sequencingUtils.h>
#include <src/sequencing/sequencing.h>
#include <src/sequencing/lfsr.h>
#include <src/fs/fsUtils.h>
#include <src/fs/fs.h>
#include <src/fs/transactions.h>
#include <lpc2000/uart.h>
#include <lpc2000/clocks.h>
#include <lpc2000/busywait.h>
#include <utils/print.h>
#include <utils/memlib.h>
#include <utils/string_lib.h>
#include <test/macroTest.h>
#include <test/testUtils.h>
#include <test/fs/testFsUtils.h>

extern transaction_t transactions[FS_MAX_N_TRANSACTIONS];
extern obs_pages_per_seg_counters *obs_counters_map_ptr;
extern uint8_t fs_buffer[FS_BLOCK_SIZE];
extern fs_t fs_ptr;
extern cache_lru_q lru_ptr;


void runAllLruTests(){
	/* run actual tests only if lru is larger than 0*/
	if (!IS_CACHE_EMPTY()){
		RUN_TEST(lru,1);
		RUN_TEST(lru,2);
		RUN_TEST(lru,3);
	}

}

/**
 * @brief
 * init lru test
 *
 */
void init_lruTest(){
	if(nandInit())
		return;

	init_flash();
	initializeRamStructs();

	/* do booting*/
	handleNoFs();
//	L("%s", "done ");
}

/**
 * @brief
 * tear down lru test
 *
 */
void tearDown_lruTest(){
	init_flash();
	initializeRamStructs();

	nandTerminate();
	init_fsbuf(fs_buffer);
	init_fs();
}

/******* actual tests **********/

/**
 * @brief
 * read inode of a file that's NOT part of a transaction. verify
 * - insertion to lru
 * - page is marked as read page
 * - no parent offset and id
 * - no tid
 *
 * perform more reads and verify cache as epxected
 * @return 1 if successful, 0 otherwise
 */
error_t
lruTest1(){
	uint8_t temp_buf[FS_BLOCK_SIZE], temp_buf2[FS_BLOCK_SIZE], new_root_buf[FS_BLOCK_SIZE];
	bool_t cpWritten;
	int32_t tid;
	int32_t parent_offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(org_log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(new_cached_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c', *f_full_name = "/file1.dat";
	int32_t i, res;
	int32_t data_type;
    int32_t is_write;

//    PRINT("\nstarting test");
    FENT();
	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);
	init_logical_address(org_log_addr_file);
	init_logical_address(new_cached_addr);

	/******* create file manually ************/
	/* write file inode */
//	PRINT("\nwrite new inode");
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill buffer data*/
	for(i=0; i< FS_BLOCK_SIZE ; i++){
		fs_buffer[i] = byte;
	}

//	L("\nread block from seg %d offset %d", GET_LOGICAL_SEGMENT(log_addr_file), GET_LOGICAL_OFFSET(log_addr_file));
	VERIFY(!readBlock(log_addr_file, fs_buffer));
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE));
	INODE_SET_NBLOCKS(ino_ptr, 1);
	fsMemcpy(temp_buf2, fs_buffer, FS_BLOCK_SIZE);

	/* write inode, and link it to root directory and inode0*/
//	PRINT("\nallocAndWriteBlock of inode");
	VERIFY(!allocAndWriteBlock(log_addr_file,
							   fs_buffer,
							   0,
							   prev_log_addr,
							   &cpWritten,
							   fsCheckpointWriter,
							   0));
	copyLogicalAddress(org_log_addr_file, log_addr_file);

	/* read inode0 for root dir inode, and read it for first entries blocks, and read first direct entry block*/
//	PRINT("\nread inode0");
	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_GET_DIRECT(ino_ptr, 0, log_addr_root);
//	PRINT("\nread root");
	readBlock(log_addr_root, fs_buffer);
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
//	PRINT("\nread first block of root");
	readBlock(log_addr, fs_buffer);

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr)){
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}

	/* set file direntry
	 * NOTICE - f_full_name+1 to skip first '/'*/
//	L("setting file direntry. id %d", 2);
	setNewDirentry(dirent_ptr, 2, FTYPE_FILE, f_full_name+1);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write and mark first direntries block in directory as taken
	 * save root block for later comparison*/
	readBlock(log_addr_root, fs_buffer);
	SET_ADDR_FLAG(log_addr_file, ADDR_FLAG_TAKEN);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));
	fsMemcpy(new_root_buf, fs_buffer, FS_BLOCK_SIZE);

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);

	SET_ADDR_FLAG(log_addr_root, ADDR_FLAG_TAKEN);
	SET_ADDR_FLAG(log_addr_file, ADDR_FLAG_TAKEN);

	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/********* read via cache inode block**********/
	/* verify queue is empty */
	VERIFY(IS_CACHE_ID_EMPTY(CACHE_LRU_Q_GET_LRU()));
	VERIFY(IS_CACHE_ID_EMPTY(CACHE_LRU_Q_GET_MRU()));

	/* access cache*/
	tid = TID_EMPTY;
	parent_offset = CACHE_ENTRY_OFFSET_EMPTY;
	data_type = DATA_TYPE_INODE;
	is_write = CACHE_ACCESS_READ;

	copyLogicalAddress(new_cached_addr, log_addr_file);
	res = cache_access_cache(temp_buf,
							 new_cached_addr,
							 parent_offset,
							 tid,
							 data_type,
							 is_write);

//	L("res %d", res);
    /* verify temp_buf contains inode, and that it is identical to originaly written block
     * and new cached addr*/
	VERIFY(verifyInode(CAST_TO_INODE(temp_buf), 2, FTYPE_FILE, 1, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE)));
	VERIFY(IS_CACHED_ADDR(new_cached_addr));

	/* verify there is only one cache used*/
	VERIFY(!IS_CACHE_FREE(0));

	for(i=1; i< FS_CACHE_BUFFERS; i++){
		VERIFY(IS_CACHE_FREE(i));
	}

	/* verify cache details:
	 * 1. buffer identical to temp_buf
	 * 2. parent fields
	 * 3. other fields...*/
	VERIFY(verify_cache(0, /* cid*/
						CACHE_IS_VOTED_NO, /* not voted*/
						0, /* ref count*/
						CACHE_ID_EMPTY, /* empty parent id*/
						CACHE_ENTRY_OFFSET_EMPTY, /* no parent offset*/
						CACHE_ID_EMPTY, /* cache is only one in line*/
						CACHE_ID_EMPTY,
						TID_EMPTY, /* no transaction*/
						CACHE_CLEAN, /* cache not dirty*/
						log_addr_file, /* org log addr*/
						temp_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));
	VERIFY(compare_bufs(temp_buf2, temp_buf, FS_BLOCK_SIZE));

	/* verify queue contains one cache now*/
	VERIFY(COMPARE(CACHE_LRU_Q_GET_LRU(), 0));
	VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), 0));

	/* now try to access cache again, from cached addr
	 * and verify that what we get is the same as temp_buf*/
	init_logical_address(log_addr);
	copyLogicalAddress(log_addr, new_cached_addr);
	init_logical_address(new_cached_addr);

	copyLogicalAddress(new_cached_addr, log_addr);
	res = cache_access_cache(temp_buf,
							 new_cached_addr,
							 parent_offset,
							 tid,
							 data_type,
							 is_write);

//	L("res %d", res);

	/* verify temp_buf contains inode, and that it is identical to originaly written block
	 * and new cached addr*/
	VERIFY(verifyInode(CAST_TO_INODE(temp_buf), 2, FTYPE_FILE, 1, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE)));
	VERIFY(IS_CACHED_ADDR(new_cached_addr));

	/* verify there is only one cache used*/
	VERIFY(!IS_CACHE_FREE(0));

	for(i=1; i< FS_CACHE_BUFFERS; i++){
		VERIFY(IS_CACHE_FREE(i));
	}

	/* verify cache details:
	 * 1. buffer identical to temp_buf
	 * 2. parent fields
	 * 3. other fields...*/
	VERIFY(verify_cache(0, /* cid*/
						CACHE_IS_VOTED_NO, /* not voted*/
						0, /* ref count*/
						CACHE_ID_EMPTY, /* empty parent id*/
						CACHE_ENTRY_OFFSET_EMPTY, /* no parent offset*/
						CACHE_ID_EMPTY, /* cache is only one in line*/
						CACHE_ID_EMPTY,
						TID_EMPTY, /* no transaction*/
						CACHE_CLEAN, /* cache not dirty*/
						log_addr_file, /* org log addr*/
						temp_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));
	VERIFY(compare_bufs(temp_buf2, temp_buf, FS_BLOCK_SIZE));

	/* verify queue contains one cache now*/
	VERIFY(COMPARE(CACHE_LRU_Q_GET_LRU(), 0));
	VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), 0));

	/* verify temp_buf contains inode, and that it is identical to originaly written block
	 * and new cached addr*/
	VERIFY(verifyInode(CAST_TO_INODE(temp_buf), 2, FTYPE_FILE, 1, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE)));
	VERIFY(IS_CACHED_ADDR(new_cached_addr));

	/* verify there is only one cache used*/
	VERIFY(!IS_CACHE_FREE(0));

	for(i=1; i< FS_CACHE_BUFFERS; i++){
		VERIFY(IS_CACHE_FREE(i));
	}

	/* read root inode to cache, and verify the extra cached block*/
	init_logical_address(new_cached_addr);
	copyLogicalAddress(new_cached_addr, log_addr_root);
	res = cache_access_cache(temp_buf,
							 new_cached_addr,
							 parent_offset,
							 tid,
							 data_type,
							 is_write);

//	L("res %d", res);

	/* verify there are only 2 caches used*/
	VERIFY(!IS_CACHE_FREE(0));
	VERIFY(!IS_CACHE_FREE(1));

	for(i=2; i< FS_CACHE_BUFFERS; i++){
		VERIFY(IS_CACHE_FREE(i));
	}

	/* verify cache details:
	 * 1. buffer identical to temp_buf
	 * 2. parent fields
	 * 3. other fields...*/
	VERIFY(verify_cache(1, /* cid*/
						CACHE_IS_VOTED_NO, /* not voted*/
						0, /* ref count*/
						CACHE_ID_EMPTY, /* empty parent id*/
						CACHE_ENTRY_OFFSET_EMPTY, /* no parent offset*/
						0, /* prev is 0*/
						CACHE_ID_EMPTY, /* no next. 1 is mru*/
						TID_EMPTY, /* no transaction*/
						CACHE_CLEAN, /* cache not dirty*/
						log_addr_root, /* org log addr*/
						temp_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));
	VERIFY(compare_bufs(new_root_buf, temp_buf, FS_BLOCK_SIZE));

	/* verify cache 0 again*/
	VERIFY(verify_cache(0, /* cid*/
						CACHE_IS_VOTED_NO, /* not voted*/
						0, /* ref count*/
						CACHE_ID_EMPTY, /* empty parent id*/
						CACHE_ENTRY_OFFSET_EMPTY, /* no parent offset*/
						CACHE_ID_EMPTY, /* cache is last in line*/
						1,
						TID_EMPTY, /* no transaction*/
						CACHE_CLEAN, /* cache not dirty*/
						log_addr_file, /* org log addr*/
						temp_buf2,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

	/* verify queue contains one cache now*/
	VERIFY(COMPARE(CACHE_LRU_Q_GET_LRU(), 0));
	VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), 1));

	/* access block cached by cache 0 again, and verify it is moved up in queue*/
//	L("access cache 0 again", NULL);
	init_logical_address(new_cached_addr);
	copyLogicalAddress(new_cached_addr, log_addr_file);
	res = cache_access_cache(temp_buf,
							 new_cached_addr,
							 parent_offset,
							 tid,
							 data_type,
							 is_write);

//	L("res %d", res);

	VERIFY(verify_cache(0, /* cid*/
						CACHE_IS_VOTED_NO, /* not voted*/
						0, /* ref count*/
						CACHE_ID_EMPTY, /* empty parent id*/
						CACHE_ENTRY_OFFSET_EMPTY, /* no parent offset*/
						1, /* 1 is now lru*/
						CACHE_ID_EMPTY, /* 0 is now mru*/
						TID_EMPTY, /* no transaction*/
						CACHE_CLEAN, /* cache not dirty*/
						log_addr_file, /* org log addr*/
						temp_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));
//	L("compare bufs", NULL);
	VERIFY(compare_bufs(temp_buf2, temp_buf, FS_BLOCK_SIZE));
	VERIFY(compare_bufs(temp_buf2, CACHE_GET_BUF_PTR(0), FS_BLOCK_SIZE));
//	L("verify cache 1 hasn't chnaged", NULL);
	VERIFY(verify_cache(1, /* cid*/
						CACHE_IS_VOTED_NO, /* not voted*/
						0, /* ref count*/
						CACHE_ID_EMPTY, /* empty parent id*/
						CACHE_ENTRY_OFFSET_EMPTY, /* no parent offset*/
						CACHE_ID_EMPTY, /* 1 is last now*/
						0, /* 0 is mru*/
						TID_EMPTY, /* no transaction*/
						CACHE_CLEAN, /* cache not dirty*/
						log_addr_root, /* org log addr*/
						new_root_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));
	VERIFY(compare_bufs(new_root_buf, CACHE_GET_BUF_PTR(1), FS_BLOCK_SIZE));

	/* verify there are only 2 caches used*/
	VERIFY(!IS_CACHE_FREE(0));
	VERIFY(!IS_CACHE_FREE(1));

	for(i=2; i< FS_CACHE_BUFFERS; i++){
		VERIFY(IS_CACHE_FREE(i));
	}

	/* verify queue contains one cache now*/
	VERIFY(COMPARE(CACHE_LRU_Q_GET_LRU(), 1));
	VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), 0));

	return 1;
}

/**
 * @brief
 * write addresses pointed by an indirect buffer.
 * fill the cache, and then try to write another block.
 * make sure the first written block is flushed as expected
 * @return 1 if successful, 0 otherwise
 */
error_t lruTest2(){
	uint32_t old_offset;
	int32_t i, res;
	int32_t data_type;
    int32_t is_write;
    int32_t tid, tid1 = 1;
    int32_t parent_offset;
    int32_t prev;
    uint8_t buf[FS_BLOCK_SIZE];
    uint8_t buf1[FS_BLOCK_SIZE];
    INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(new_cached_addr);
    INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
    INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_addr);
    init_logical_address(log_addr);
    fsMemset(buf, 'b', FS_BLOCK_SIZE);

    /* need at least 2 transactions*/
    if(FS_MAX_N_TRANSACTIONS <2){
    	return 1;
    }
//    L("main_area_writes[7]=%d", main_area_writes[7]);
	/********* read via cache inode block**********/
	/* read inode to transaction indirect*/
    /*set necessary transaction details*/
	tid = 0;
	init_transaction(tid);
	TRANSACTION_SET_FILE_OFFSET(tid, INDIRECT_DATA_OFFSET);

	/* set cache details*/
	parent_offset = CACHE_ENTRY_OFFSET_EMPTY;
	data_type = DATA_TYPE_REGULAR;
	is_write = CACHE_ACCESS_WRITE;

	old_offset = GET_RECLAIMED_OFFSET();

	prev = CACHE_ID_EMPTY;
	for(i=0; i< FS_CACHE_BUFFERS; i++){
		init_logical_address(new_cached_addr);
		res = cache_access_cache(buf,
								 new_cached_addr,
								 i,
								 tid,
								 data_type,
								 is_write);

		/* verify access*/
//		L("res %d", res);
		VERIFY(!res);

		/* verify no write was actually performed*/
		VERIFY(COMPARE(old_offset, GET_RECLAIMED_OFFSET()));
		VERIFY(IS_CACHED_ADDR(new_cached_addr));
		VERIFY(COMPARE(GET_LOGICAL_OFFSET(new_cached_addr), i));
		BLOCK_SET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), i, new_cached_addr);
//		init_logical_address(new_cached_addr);
//		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), i, new_cached_addr);
//		L("IS_CACHED_ADDR(new_cached_addr) %d. new_cached_addr cid=%d", IS_CACHED_ADDR(new_cached_addr), CACHE_GET_CID_FROM_ADDR(new_cached_addr));
//		return 0;

		/* verify details*/
		VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), i));

		VERIFY(verify_cache(i, /* cid*/
							CACHE_IS_VOTED_NO, /* not voted*/
							0, /* ref count*/
							CACHE_ID_EMPTY, /* empty parent id*/
							i,
							prev,
							CACHE_ID_EMPTY,
							tid,
							CACHE_DIRTY, /* cache dirty*/
							log_addr, /* org log addr*/
							buf,
							CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

		prev = i;
	}
//	L("main_area_writes[7]=%d", main_area_writes[7]);
	/* iterate queue backwards and verify parent offset and pointers*/
	VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), FS_CACHE_BUFFERS-1));
	VERIFY(COMPARE(CACHE_LRU_Q_GET_LRU(), 0));

	for(i=0; i< FS_CACHE_BUFFERS; i++){
		if(i==0){
			VERIFY(COMPARE(CACHE_GET_LESS_RECENTLY_USED(i), CACHE_ID_EMPTY));
			VERIFY(COMPARE(CACHE_GET_MORE_RECENTLY_USED(i), 1));
			continue;
		}

		if(i==FS_CACHE_BUFFERS-1){
			VERIFY(COMPARE(CACHE_GET_LESS_RECENTLY_USED(i), FS_CACHE_BUFFERS-2));
			VERIFY(COMPARE(CACHE_GET_MORE_RECENTLY_USED(i), CACHE_ID_EMPTY));
			continue;
		}

		VERIFY(COMPARE(CACHE_GET_LESS_RECENTLY_USED(i), i-1));
		VERIFY(COMPARE(CACHE_GET_MORE_RECENTLY_USED(i), i+1));
	}
//	L("main_area_writes[7]=%d", main_area_writes[7]);
	/* perform another write that should flush first cache to flash.*/
	init_logical_address(new_cached_addr);
	copyLogicalAddress(old_addr, GET_RECLAIMED_ADDRESS_PTR());
	res = cache_access_cache(buf,
							 new_cached_addr,
							 i,
							 tid,
							 data_type,
							 is_write);

	/* verify access*/
//	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(some_addr);
//	init_logical_address(some_addr);
//	BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), 0, some_addr);
//	VERIFY(COMPARE(0, GET_LOGICAL_OFFSET(some_addr)));
//	VERIFY(IS_CACHED_ADDR(some_addr));
//	L("res %d", res);
	VERIFY(!res);
//	L("main_area_writes[7]=%d", main_area_writes[7]);
	/* verify a write was actually performed */
	VERIFY(COMPARE(old_offset+1, GET_RECLAIMED_OFFSET()));
	VERIFY(IS_CACHED_ADDR(new_cached_addr));
	BLOCK_SET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), i, new_cached_addr);
//	L("main_area_writes[7]=%d", main_area_writes[7]);
	/* verify that cache 0 is now mru*/
	VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), 0));
	VERIFY(COMPARE(CACHE_GET_MORE_RECENTLY_USED(0), CACHE_ID_EMPTY));
	VERIFY(COMPARE(CACHE_GET_LESS_RECENTLY_USED(0), FS_CACHE_BUFFERS-1));

	init_logical_address(log_addr);
	VERIFY(verify_cache(0, /* cid*/
						CACHE_IS_VOTED_NO, /* not voted*/
						0, /* ref count*/
						CACHE_ID_EMPTY, /* empty parent id*/
						i,
						FS_CACHE_BUFFERS-1, /* 0 is first now, FS_CACHE_BUFFERS-1 is before last written*/
						CACHE_ID_EMPTY, /* 0 is mru*/
						tid,
						CACHE_DIRTY, /* cache dirty*/
						log_addr,
						buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));
//	L("main_area_writes[7]=%d", main_area_writes[7]);
	/* verify written block to flash*/
	BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), 0, log_addr);
	VERIFY(!IS_CACHED_ADDR(log_addr));
	VERIFY(!readBlock(log_addr, fs_buffer));
	compare_bufs(fs_buffer, buf, FS_BLOCK_SIZE);

	/* now write a block from an other transaction
	 * and verify that the flushed buffer which is
	 * from the initial transaction*/
	init_transaction(tid1);
	TRANSACTION_SET_FILE_OFFSET(tid1, INDIRECT_DATA_OFFSET);
//	L("main_area_writes[7]=%d", main_area_writes[7]);
	/* perform another write that should flush second cache to flash.*/
//	L("perform another write that should flush second cache to flash");
	copyLogicalAddress(old_addr, GET_RECLAIMED_ADDRESS_PTR());
	init_logical_address(new_cached_addr);
	fsMemset(buf1, 'z', FS_BLOCK_SIZE);
	res = cache_access_cache(buf1,
							 new_cached_addr,
							 0, /* offset in parent*/
							 tid1,
							 data_type,
							 is_write);

	/* verify a write was actually performed */
//	L("verify a write was actually performed ");
	VERIFY(COMPARE(old_offset+2, GET_RECLAIMED_OFFSET()));
	VERIFY(IS_CACHED_ADDR(new_cached_addr));

//	BLOCK_SET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), i, new_cached_addr);

	/* verify that cache 0 is now mru*/
//	L("verify that cache 0 is now mru");
	VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), 1));
	VERIFY(COMPARE(CACHE_GET_MORE_RECENTLY_USED(1), CACHE_ID_EMPTY));
	VERIFY(COMPARE(CACHE_GET_LESS_RECENTLY_USED(1), 0));

	init_logical_address(log_addr);
	VERIFY(verify_cache(1, /* cid*/
						CACHE_IS_VOTED_NO, /* not voted*/
						0, /* ref count*/
						CACHE_ID_EMPTY, /* empty parent id*/
						0,/* offset in parent*/
						0, /* 0 is previously written*/
						CACHE_ID_EMPTY, /* 1 is mru*/
						tid1,
						CACHE_DIRTY, /* cache dirty*/
						log_addr,
						buf1,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

	/* verify written block to flash*/
//	L("verify written block to flashu");
	BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), 1, log_addr);
	VERIFY(!IS_CACHED_ADDR(log_addr));
	VERIFY(!readBlock(log_addr, fs_buffer));
	compare_bufs(fs_buffer, buf, FS_BLOCK_SIZE);

	return 1;
}

/**
 * @brief
 * test parent cid mechanism.
 * write
 * @return 1 if successful, 0 otherwise
 */
error_t lruTest3(){
	uint32_t old_offset;
	int32_t i, res;
	int32_t data_type;
	int32_t is_write;
	int32_t tid;
	int32_t parent_offset;
	int32_t prev;
	int32_t n_cached = (LRU_CACHE_SIZE>10)?10:(LRU_CACHE_SIZE-2);
	uint8_t buf[FS_BLOCK_SIZE];
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(new_cached_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_addr);
	logical_addr_t entry_ptr;
	init_logical_address(log_addr);
	fsMemset(buf, 'b', FS_BLOCK_SIZE);

	/********* read via cache inode block**********/
	/* read inode to transaction indirect*/
	/*set necessary transaction details*/
	tid = 0;
	init_transaction(tid);
	TRANSACTION_SET_FILE_OFFSET(tid, INDIRECT_DATA_OFFSET);

	/* set cache details*/
	parent_offset = CACHE_ENTRY_OFFSET_EMPTY;
	data_type = DATA_TYPE_REGULAR;
	is_write  = CACHE_ACCESS_WRITE;

	old_offset = GET_RECLAIMED_OFFSET();

	prev = CACHE_ID_EMPTY;
	for(i=0; i<n_cached ; i++){
		init_logical_address(new_cached_addr);
		res = cache_access_cache(buf,
								 new_cached_addr,
								 i,
								 tid,
								 data_type,
								 is_write);

		/* verify access*/
//		L("res %d", res);
		VERIFY(!res);

		/* verify no write was actually performed*/
		VERIFY(COMPARE(old_offset, GET_RECLAIMED_OFFSET()));
		VERIFY(IS_CACHED_ADDR(new_cached_addr));
		VERIFY(COMPARE(GET_LOGICAL_OFFSET(new_cached_addr), i));
		BLOCK_SET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), i, new_cached_addr);
//		init_logical_address(new_cached_addr);
//		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), i, new_cached_addr);
//		L("IS_CACHED_ADDR(new_cached_addr) %d. new_cached_addr cid=%d", IS_CACHED_ADDR(new_cached_addr), CACHE_GET_CID_FROM_ADDR(new_cached_addr));
//		return 0;

		/* verify details*/
		VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), i));

		VERIFY(verify_cache(i, /* cid*/
							CACHE_IS_VOTED_NO, /* not voted*/
							0, /* ref count*/
							CACHE_ID_EMPTY, /* empty parent id*/
							i,
							prev,
							CACHE_ID_EMPTY,
							tid,
							CACHE_DIRTY, /* cache dirty*/
							log_addr, /* org log addr*/
							buf,
							CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

		prev = i;
	}

	/* now cache indirect block*/
	/* perform another write that should flush first cache to flash.*/
	init_logical_address(new_cached_addr);
	parent_offset = CALC_IN_LOG_ADDRESSES(INDIRECT_INDEX_LOCATION); /* supposedly first indirect*/
	data_type = DATA_TYPE_ENTRIES;
	is_write = CACHE_ACCESS_WRITE;
	res = cache_access_cache(TRANSACTION_GET_INDIRECT_PTR(tid),
							 new_cached_addr,
							 parent_offset,
							 tid,
							 data_type,
							 is_write);

	/* now verify cid mechanism:
	 * 1. ref count
	 * 2. leafs have parent cid*/
	VERIFY(COMPARE(CACHE_GET_REF_COUNT(n_cached), n_cached));

	for(i=0; i< n_cached; i++){
		VERIFY(COMPARE(CACHE_GET_PARENT_CACHE_ID(i), n_cached));
	}

	/* flush a child and verify changes in parent*/
	/* fill the flash with something*/
	i++;
	prev++;
	old_offset = GET_RECLAIMED_OFFSET();
	data_type = DATA_TYPE_REGULAR;
	fsMemset(buf, 'x', FS_BLOCK_SIZE);
	for(; i<FS_CACHE_BUFFERS ; i++){
		init_logical_address(new_cached_addr);
		res = cache_access_cache(buf,
								 new_cached_addr,
								 i,
								 tid,
								 data_type,
								 is_write);

		/* verify access*/
//		L("res %d", res);
		VERIFY(!res);

		/* verify no write was actually performed*/
		VERIFY(COMPARE(old_offset, GET_RECLAIMED_OFFSET()));
		VERIFY(IS_CACHED_ADDR(new_cached_addr));
		VERIFY(COMPARE(GET_LOGICAL_OFFSET(new_cached_addr), i));
//		init_logical_address(new_cached_addr);
//		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), i, new_cached_addr);
//		L("IS_CACHED_ADDR(new_cached_addr) %d. new_cached_addr cid=%d", IS_CACHED_ADDR(new_cached_addr), CACHE_GET_CID_FROM_ADDR(new_cached_addr));
//		return 0;

		/* verify details*/
		VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), i));

		VERIFY(verify_cache(i, /* cid*/
							CACHE_IS_VOTED_NO, /* not voted*/
							0, /* ref count*/
							CACHE_ID_EMPTY, /* empty parent id*/
							i,
							prev,
							CACHE_ID_EMPTY,
							tid,
							CACHE_DIRTY, /* cache dirty*/
							log_addr, /* org log addr*/
							buf,
							CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

		prev = i;
	}

	/* perform another write.
	 * this should flush one the leafs of the cached indirect*/
	parent_offset = 60;
	copyLogicalAddress(old_addr, GET_RECLAIMED_ADDRESS_PTR());
	init_logical_address(new_cached_addr);
	res = cache_access_cache(buf,
							 new_cached_addr,
							 parent_offset,
							 tid,
							 data_type,
							 is_write);

	/* verify access*/
//		L("res %d", res);
	VERIFY(!res);

	/* now verify cid mechanism:
	 * 1. ref count
	 * 2. flushed leaf has no parent cid etc
	 * 3. parent cid points to real flushed addr*/
	VERIFY(COMPARE(CACHE_GET_REF_COUNT(n_cached), n_cached-1));

	VERIFY(IS_CACHE_ID_EMPTY(CACHE_GET_PARENT_CACHE_ID(0)));
//	L("cache 0 parent offset %d, expected %d", CACHE_GET_PARENT_OFFSET(0), parent_offset);
	VERIFY(COMPARE(CACHE_GET_PARENT_OFFSET(0), parent_offset));
	VERIFY(compare_bufs(CACHE_GET_BUF_PTR(0), buf, FS_BLOCK_SIZE));

	entry_ptr = BLOCK_GET_ADDR_PTR_BY_INDEX(CACHE_GET_BUF_PTR(n_cached), 0);
//	L("expected seg %d offset %d, actual seg %d offset %d", GET_LOGICAL_SEGMENT(old_addr), GET_LOGICAL_OFFSET(old_addr), GET_LOGICAL_SEGMENT(entry_ptr), GET_LOGICAL_OFFSET(entry_ptr));
	VERIFY(COMPARE_ADDR(old_addr, entry_ptr));

	return 1;
}

///**
// * @brief
// * same as test3 for the inode
// * @return 1 if successful, 0 otherwise
// */
//error_t lruTest4(){
//
//	return 1;
//}
//
///**
// * @brief
// * call readfile block on a NON inode-aligned block in a file we're not writing to, and verify
// * - pages in cache
// * - pages marked as read page
// * - pages first in lru
// * - all indirect blocks and inode are also in cache
// * @return 1 if successful, 0 otherwise
// */
//error_t lruTest5(){
//
//	return 1;
//}
//
///**
// * @brief
// * read sequentially more blocks than in lru and verify order in the queue, and that there is no
// * @return 1 if successful, 0 otherwise
// */
//error_t lruTest6(){
//
//	return 1;
//}
//
///**
// * @brief
// * call readfile block on a block that is already in cache and verify that the page wasn't
// * read from media (by previously changing content of block in cache)
// * @return 1 if successful, 0 otherwise
// */
//error_t lruTest7(){
//
//	return 1;
//}
//
///**
// * @brief
// * call writeFileBlock on inode-aligned block in a file and verify
// * - page is in cache
// * - page is marked as write page
// * - page first in lru
// * @return 1 if successful, 0 otherwise
// */
//error_t lruTest8(){
//
//	return 1;
//}
//
///**
// * @brief
// * push written file block more than once out of lru, so that it is still in queue
// * but a more recently used read block is pushed out. verify access count increased.
// *
// * repeat until block is flushed
// * @return 1 if successful, 0 otherwise
// */
//error_t lruTest9(){
//
//	return 1;
//}
//
///**
// * @brief
// * write block that fills transaction vot page, verify that it is actually written and not cached
// * @return 1 if successful, 0 otherwise
// */
//error_t lruTest10(){
//
//	return 1;
//}
//
///**
// * @brief
// * create a file and verify that the pages were actually written (due to a transaction commit)
// * @return 1 if successful, 0 otherwise
// */
//error_t lruTest11(){
//
//	return 1;
//}
//
///**
// * @brief
// * write and close, verify that pages were actually written
// * @return 1 if successful, 0 otherwise
// */
//error_t lruTest12(){
//
//	return 1;
//}
