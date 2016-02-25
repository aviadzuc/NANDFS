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

/** @file cacheTests.c  */
#include <test/fs/cacheTests.h>
#include <test/fs/testsHeader.h>

void runAllCacheTests(){
	
	RUN_TEST(cache,0);
	RUN_TEST(cache,1);	
	RUN_TEST(cache,2);
	RUN_TEST(cache,3);
	RUN_TEST(cache,4);
	RUN_TEST(cache,6);	
	RUN_TEST(cache,7);
	RUN_TEST(cache,8);
	RUN_TEST(cache,9);
	RUN_TEST(cache,10);
	RUN_TEST(cache,11);
	RUN_TEST(cache,12);
	RUN_TEST(cache,21);	
	RUN_TEST(cache,22);
	
}

/**
 * @brief 
 * init cache test
 * 
 */
void init_cacheTest(){
	if(nandInit())
		return;	
			
	init_flash();
	initializeRamStructs();
//	PRINT("\ninit_cacheTest() - handleNoFs");
	handleNoFs();
//	PRINT("\ndone");
	init_fsbuf(fs_buffer);		
}

/**
 * @brief 
 * tear down cache test
 * 
 */
void tearDown_cacheTest(){
	init_flash();
	nandTerminate();
	initializeRamStructs();	
}

/**
 *@brief
 * verify all cached are empty except a specific cache 
 * 
 * @return 1 if successful, 0 otherwise
 */
int32_t verifyCachesEmpty(int32_t cid){
	int32_t i;
	
	for(i=0; i<FS_CACHE_BUFFERS ; i++){
		/* don't check cid'th cache*/
		if(i==cid) continue;		
		
		if(!IS_INDIRECT_CACHE_FREE(i)){
			PRINT_MSG_AND_NUM("\ni=", i);
			PRINT_MSG_AND_HEX("\nINDIRECT_CACHE_GET_INO_NUM(CID)=", INDIRECT_CACHE_GET_INO_NUM( i));
			PRINT_MSG_AND_HEX("\nINDIRECT_CACHE_GET_TID(CID)=", INDIRECT_CACHE_GET_TID( i));
			
			PRINT_MSG_AND_NUM("\nis write cache=", IS_INDIRECT_CACHE_WRITE_BUF(i));
			PRINT_MSG_AND_NUM("\nis read cache=", IS_INDIRECT_CACHE_READ_BUF(i));
		}
		VERIFY(IS_INDIRECT_CACHE_FREE(i));
	}
	
	return 1;
}

/**
 * @brief
 * verify cache details in comparison with expected details
 * 
 * @return 1 if successful, 0 otherwise
 */
int32_t verifyCache(int32_t cid, int32_t ino_num, int32_t offset, int32_t tid){
//	PRINT_MSG_AND_NUM("\nverifyCache() - cid=", cid);
//	PRINT_MSG_AND_NUM(" expected ino_num=", ino_num);
//	PRINT_MSG_AND_NUM(" actual ino_num=", INDIRECT_CACHE_GET_INO_NUM(cid));
//	PRINT_MSG_AND_NUM(" expected offset=", offset);
//	PRINT_MSG_AND_NUM(" actual offset=", INDIRECT_CACHE_GET_OFFSET(cid));
//	PRINT_MSG_AND_NUM(" expected tid=", tid);
//	PRINT_MSG_AND_NUM(" actual tid=", INDIRECT_CACHE_GET_TID(cid));
	
	VERIFY(COMPARE(INDIRECT_CACHE_GET_INO_NUM(cid), ino_num));
	VERIFY(COMPARE(INDIRECT_CACHE_GET_OFFSET(cid), offset));
	VERIFY(COMPARE(INDIRECT_CACHE_GET_TID(cid), tid));
	
	return 1;
}

/**
 * @brief
 * find a read buffer realted to a given file
 * 
 * @return cache id, or CID_EMPTY if none found
 */ 
int32_t getReadCacheByInoNum(int32_t ino_num){
	int32_t i;
	
	for(i=0; i<FS_CACHE_BUFFERS ; i++){
		/* if the buffer is realted to the file and it is not
		 * a write buffer (tid not empty)*/
		if(INDIRECT_CACHE_GET_INO_NUM(i)==ino_num &&
		   IS_EMPTY_TID(INDIRECT_CACHE_GET_TID(i))){
		   return i;
		}
		
		VERIFY(IS_INDIRECT_CACHE_FREE(i));
	}
	
	return CID_EMPTY;
}

/**
 * @brief
 * manipulate root idrectory to contain blocks until indirect offset.
 * read first root directory file block, verify one cache buffer is allocated to it.
 * read a block in indirect offset and make sure the cache changes to this offset, 
 * and there is still only one cache for this file.
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest0(){	
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(blk_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	int32_t i, cid;
	bool_t cpWritten;
		
	/* read root directory*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
//	PRINT("\n finished read root directory");
	
	/* write dummy indirect block of root*/
	fsMemset(fs_buffer, 0xff, FS_BLOCK_SIZE);
	BLOCK_SET_INDEX(fs_buffer, 0, root_addr);
	VERIFY(!allocAndWriteBlock(blk_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT("\n wrote dummy indirect block of root");
	
	/* set all direct entries in root (except 1st, already full) to some log addr*/
	VERIFY(!readBlock(root_addr, fs_buffer));
	for(i=1; i< DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i, log_addr);
	}
	
	INODE_SET_NBYTES(ino_ptr, INODE_FILE_DATA_SIZE+DIRECT_INDEX_ENTRIES*FS_BLOCK_SIZE+1)
	INODE_SET_NBLOCKS(ino_ptr, DIRECT_INDEX_ENTRIES+2)
	
	VERIFY(!allocAndWriteBlock(root_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT("\n set all direct entries in root ");
	
	/* change inode0 to point to new root inode*/	
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 0, root_addr);	
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT("\n changed inode0 to point to new root inode ");
	
	/* read first root directory block*/
	init_logical_address(root_addr);
//	PRINT("\nabout to read block");
	VERIFY(!readFileBlock(fs_buffer, 1, INODE_FILE_DATA_SIZE, root_addr, TID_EMPTY));
//	PRINT("\nabout to verify cache");
	
	/* verify one cache buffer is allocated to root inode */
	cid = getReadCacheByInoNum(1);
	VERIFY(!IS_EMPTY_CID(cid));
//	PRINT("\na cache exists for inode 1");
	VERIFY(verifyCachesEmpty(cid));
//	PRINT("\nall other caches empty");
	VERIFY(verifyCache(cid, 1, 0, TID_EMPTY));	
//	PRINT("\nverify one cache buffer");
	
	/* read a block in indirect offset and make sure the cache changes to this offset, 
 	 * and there is still only one cache for this file.
 	 * Just for kicks - don't initialize root addr*/
 	VERIFY(!readFileBlock(fs_buffer, 1, INDIRECT_DATA_OFFSET, root_addr, TID_EMPTY));
 	
 	/* verify one cache buffer is allocated to root inode */
	VERIFY(verifyCachesEmpty(cid));
	VERIFY(verifyCache(cid, 1, INDIRECT_DATA_OFFSET, TID_EMPTY));	
 	 
	return 1;
}

/**
 * @brief
 * test a situation with no dedicated read buffers - try to read, and verify no caching was performed.  
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest1(){
	int32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	
	init_logical_address(log_addr);
	
	/* verify we have no room for dedicated read buffers*/
	if(FS_EXTRA_INDIRECT_CACHES != 0)
		return 1;
		
	/* mark all buffers as related to a transaction*/
	for(i=0; i<FS_CACHE_BUFFERS; i++){
		INDIRECT_CACHE_SET_TID(i, 0);
	}
	
	/* try to read root file block*/
	VERIFY(!readFileBlock(fs_buffer, 1, INODE_FILE_DATA_SIZE, log_addr, TID_EMPTY));
	
	/* verify no caching was performef*/
	for(i=0; i<FS_CACHE_BUFFERS ; i++){		
		VERIFY(!IS_INDIRECT_CACHE_READ_BUF(i));
		VERIFY(COMPARE(INDIRECT_CACHE_GET_TID(i), 0));
	}
				
	return 1;
}

/**
 * @brief
 * create a file in root directory, write to it and verify there is a cache of the file inode
 * (if there is 1 read cache at least)
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest2(){
	uint8_t *f_name = "/file1.dat";
	int32_t i, fd, cid2;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	
//	PRINT("\ncacheTest2() - starting");
	
	/* verify we have at least two extra caches (otherwise, they will all be taken by the transaction)*/
	if(FS_EXTRA_INDIRECT_CACHES <1)
		return 1;
	
	/* create file */	
//	PRINT("\ncacheTest2() - creat");
	fd = creat(f_name, S_IRWXU);
	VERIFY(!IS_NEGATIVE(fd));
	
	fs_buffer[0] = 'a';
//	PRINT("\n\n\ncacheTest2() - about to write write");
	VERIFY(COMPARE(1, write(fd, fs_buffer, 1)));
	
//	for(i=0; i<FS_CACHE_BUFFERS; i++){		
//		PRINT_MSG_AND_NUM("\ncache ino num=", INDIRECT_CACHE_GET_INO_NUM(i));
//	}
	/* verify there is a cache for parent directory only
	 * (and for still open transaction)*/
// 	PRINT("\n\n\ncacheTest2() - get cache ids");
	cid2 = getCachedIndirect(2);
	VERIFY(!IS_EMPTY_CID(cid2));
//	PRINT("\nverify caches empty");
	for(i=0; i<FS_CACHE_BUFFERS; i++){
		if(INDIRECT_CACHE_GET_TID(i) == 0 || i== cid2)
			continue;
			
		VERIFY(IS_INDIRECT_CACHE_FREE(i)); 
	}
//	PRINT_MSG_AND_NUM("\nverify cache=", cid2);
	VERIFY(verifyCache(cid2, 2, 0, TID_EMPTY));
//	PRINT("\nverified");
	
	/* verify cache data, and that the buffer indeed contains file inode*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));	
	INODE_GET_DIRECT(CAST_TO_INODE(fs_buffer), 1, file_addr);
		
//	PRINT("\ncompare buffers");
	VERIFY(!readBlock(file_addr, fs_buffer));
	for(i=0; i< FS_BLOCK_SIZE; i++){		
		VERIFY(COMPARE(fs_buffer[i],INDIRECT_CACHE_GET_BUF_PTR(cid2)[i])); 
	}
	
	return 1;
}

/**
 * @brief
 * create a file, close it, and verify all indirect caches are empty
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest3(){
	uint8_t *f_name = "/file1.dat";
	int32_t fd;	
	
	/* verify we have at least two extra caches (otherwise, they will all be taken by the transaction)*/
	if(FS_EXTRA_INDIRECT_CACHES <1)
		return 1;
	
	/* create file */	
	fd = creat(f_name, S_IRWXU);
	
	VERIFY(verifyCachesEmpty(CID_EMPTY));	
	return 1;
}

/**
 * @brief
 * write to a file in inode data offset. close, read file.
 * verify all caches empty 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest4(){	
	uint8_t *f_name = "/file1.dat", write_buf[FS_BLOCK_SIZE];
	int32_t fd;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	init_logical_address(root_addr);
	
//	PRINT("\ncacheTest2() - starting");
	
	/* verify we have at least two extra caches (otherwise, they will all be taken by the transaction)*/
	if(FS_EXTRA_INDIRECT_CACHES <1)
		return 1;
	
	/* create file */	
//	PRINT("\ncacheTest2() - creat");
	fd = creat(f_name, S_IRWXU);
	VERIFY(!IS_NEGATIVE(fd));
	
	write_buf[0] = 'a';
//	PRINT("\n\n\ncacheTest2() - about to write write");
	VERIFY(COMPARE(1, write(fd, write_buf, 1)));
	VERIFY(!close(fd));
	
//	for(i=0; i<FS_CACHE_BUFFERS; i++){		
//		PRINT_MSG_AND_NUM("\ncache ino num=", INDIRECT_CACHE_GET_INO_NUM(i));
//	}
	/* verify all caches are empty*/
	VERIFY(verifyCachesEmpty(CID_EMPTY));	
	
	return 1;
}

///**
// * @brief
// * write to a file until direct entries offset. read file.
// * verify a cache contains the file inode 
// * 
// * @return 1 if successful, 0 otherwise
// */
//error_t cacheTest5(){	
//	return 1;
//}

/**
 * @brief
 * write to a file until indirect entry data offset. seek to beginning, write again, read file in indirect offset.
 * verify a cache contains the indirect block 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest6(){	
	uint8_t *f_name = "/file1.dat", byte = 'a', write_buf[FS_BLOCK_SIZE];
	int32_t i, fd, offset, cid2, tid, f_id;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	
//	PRINT("\ncacheTest2() - starting");
	
	/* verify we have at least two extra caches (otherwise, they will all be taken by the transaction)*/
	if(FS_EXTRA_INDIRECT_CACHES <1)
		return 1;
	
	/* create file */	
//	PRINT("\ncacheTest2() - creat");
	fd = creat(f_name, S_IRWXU);
	VERIFY(!IS_NEGATIVE(fd));
	
	/* initialize buffer*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
		write_buf[i] = byte;
	}
		
//	PRINT("\n\n\ncacheTest6() - about to write inode file data");
	/* write to file until indirect data offset*/
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, write_buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\n\n\ncacheTest6() - about to write until indirect");	
	for(offset = INODE_FILE_DATA_SIZE; offset< INDIRECT_DATA_OFFSET; offset+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
	}
	
	/* write a different buffer in indirect offset*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
		write_buf[i] = byte+1;
	}
	f_id = GET_FILE_ID_BY_FD(fd);
	tid = getTidByFileId(f_id);
	
//	printBlock(TRANSACTION_GET_INDIRECT_PTR(tid));
//	PRINT("\n\n\ncacheTest6() - about to write indirect 1st block");
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
		
	/* seek to start and write 1 block*/
//	printBlock(TRANSACTION_GET_INDIRECT_PTR(tid));
//	PRINT("\n\n\ncacheTest6() - about to write from beginning again");
	VERIFY(COMPARE(0, lseek(fd, 0, SEEK_SET)));
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
	VERIFY(COMPARE(TRANSACTION_GET_FILE_OFFSET(tid), 0));
	
	/* read a block from indirect offset*/
//	PRINT("\n\n\ncacheTest6() - about to read from indirect offset");	
//	PRINT_MSG_AND_NUM(", tid=", tid);
//	PRINT_MSG_AND_NUM(", tid ino addr=",logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	init_logical_address(file_addr);
	
	VERIFY(!readFileBlock(write_buf, f_id, INDIRECT_DATA_OFFSET, file_addr, tid));
//	PRINT_MSG_AND_HEX("\nreadFileBlock() finished, file_addr=", *((uint32_t*)file_addr));
//	PRINT_MSG_AND_NUM("\nreadFileBlock() finished, inode addr saves is ",logicalAddressToPhysical(file_addr));
//	PRINT("\n\n\ncacheTest6() - verify read block");
	/* verify block */
	for(i=0; i< FS_BLOCK_SIZE; i++){
		if(!COMPARE(write_buf[i], byte+1)){
//			PRINT_MSG_AND_NUM("\ni=", i);
//			PRINT_MSG_AND_HEX(" expected=", byte+1);
//			PRINT_MSG_AND_HEX(" actual=", write_buf[i]);
		}
		VERIFY(COMPARE(write_buf[i], byte+1));
	}
	
//	for(i=0; i<FS_CACHE_BUFFERS; i++){		
//		PRINT_MSG_AND_NUM("\ncache ino num=", INDIRECT_CACHE_GET_INO_NUM(i));
//	}
	/* verify there is a cache for the file only
	 * (and for still open transaction)*/
// 	PRINT("\n\n\ncacheTest6() - get cache id");
	cid2 = getCachedIndirect(f_id);
	VERIFY(!IS_EMPTY_CID(cid2));
//	PRINT("\nverify caches empty");
	for(i=0; i<FS_CACHE_BUFFERS; i++){
		if(INDIRECT_CACHE_GET_TID(i) == tid || i== cid2)
			continue;
			
		VERIFY(IS_INDIRECT_CACHE_FREE(i)); 
	}
//	PRINT_MSG_AND_NUM("\nverify cache=", cid2);
	VERIFY(verifyCache(cid2, 2, INDIRECT_DATA_OFFSET, TID_EMPTY));
//	PRINT("\nverified");
	
	/* verify cache data, and that the buffer indeed contains indirect block.*/
	BLOCK_GET_INDEX(INDIRECT_CACHE_GET_BUF_PTR(cid2), 0 ,log_addr);
	VERIFY(!readBlock(log_addr, write_buf));
//	PRINT("\nread direct");
	for(i=0; i< FS_BLOCK_SIZE; i++){		
		VERIFY(COMPARE(write_buf[i],byte+1)); 
	}
	
	/* verify all other addresses in cache buffer are empty as expected*/
	for(i=1; i< LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_GET_INDEX(INDIRECT_CACHE_GET_BUF_PTR(cid2), i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}	
	
	return 1;
}

/**
 * @brief
 * write to a file until double entry data offset. seek to beginning, write again, read file in double offset.
 * verify a cache contains the double indirect block  
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest7(){	
	uint8_t *f_name = "/file1.dat", byte = 'a', write_buf[FS_BLOCK_SIZE];
	int32_t i, fd, offset, cid2, tid, f_id;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	
//	PRINT("\ncacheTest7() - starting");
	
	/* verify we have at least two extra caches (otherwise, they will all be taken by the transaction)*/
	if(FS_EXTRA_INDIRECT_CACHES <1)
		return 1;
	
	/* create file */	
//	PRINT("\ncacheTest2() - creat");
	fd = creat(f_name, S_IRWXU);
	VERIFY(!IS_NEGATIVE(fd));
	
	/* initialize buffer*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
		write_buf[i] = byte;
	}
		
//	PRINT("\n\n\ncacheTest7() - about to write inode file data");
	/* write to file until indirect data offset*/
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, write_buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\n\n\ncacheTest6() - about to write until indirect");	
	for(offset = INODE_FILE_DATA_SIZE; offset< DOUBLE_DATA_OFFSET; offset+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
	}
	
	/* write a different buffer in double offset*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
		write_buf[i] = byte+1;
	}
	f_id = GET_FILE_ID_BY_FD(fd);
	tid = getTidByFileId(f_id);
	
//	printBlock(TRANSACTION_GET_INDIRECT_PTR(tid));
//	PRINT("\n\n\ncacheTest7() - about to write indirect 1st block");
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
		
	/* seek to start and write 1 block*/
//	printBlock(TRANSACTION_GET_INDIRECT_PTR(tid));
//	PRINT("\n\n\ncacheTest7() - about to write from beginning again");
	VERIFY(COMPARE(0, lseek(fd, 0, SEEK_SET)));
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
	VERIFY(COMPARE(TRANSACTION_GET_FILE_OFFSET(tid), 0));
	
	/* read a block from double offset*/
//	PRINT("\n\n\ncacheTest7() - about to read from double offset");	
//	PRINT_MSG_AND_NUM(", tid=", tid);
//	PRINT_MSG_AND_NUM(", tid ino addr=",logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	init_logical_address(file_addr);	
	VERIFY(!readFileBlock(write_buf, f_id, DOUBLE_DATA_OFFSET, file_addr, tid));
//	PRINT_MSG_AND_HEX("\nreadFileBlock() finished, file_addr=", *((uint32_t*)file_addr));
//	PRINT_MSG_AND_NUM("\nreadFileBlock() finished, inode addr saves is ",logicalAddressToPhysical(file_addr));
//	PRINT("\n\n\ncacheTest7() - verify read block");

	/* verify block */
	for(i=0; i< FS_BLOCK_SIZE; i++){
		if(!COMPARE(write_buf[i], byte+1)){
//			PRINT_MSG_AND_NUM("\ni=", i);
//			PRINT_MSG_AND_HEX(" expected=", byte+1);
//			PRINT_MSG_AND_HEX(" actual=", write_buf[i]);
		}
		VERIFY(COMPARE(write_buf[i], byte+1));
	}
	
//	for(i=0; i<FS_CACHE_BUFFERS; i++){		
//		PRINT_MSG_AND_NUM("\ncache ino num=", INDIRECT_CACHE_GET_INO_NUM(i));
//	}
	/* verify there is a cache for the file only
	 * (and for still open transaction)*/
// 	PRINT("\n\n\ncacheTest7() - get cache id");
	cid2 = getCachedIndirect(f_id);
	VERIFY(!IS_EMPTY_CID(cid2));
//	PRINT("\nverify caches empty");
	for(i=0; i<FS_CACHE_BUFFERS; i++){
		if(INDIRECT_CACHE_GET_TID(i) == tid || i== cid2)
			continue;
			
		VERIFY(IS_INDIRECT_CACHE_FREE(i)); 
	}
//	PRINT_MSG_AND_NUM("\nverify cache=", cid2);
	VERIFY(verifyCache(cid2, 2, DOUBLE_DATA_OFFSET, TID_EMPTY));
//	PRINT("\nverified");
	
	/* verify cache data, and that the buffer indeed contains indirect block.*/	
	BLOCK_GET_INDEX(INDIRECT_CACHE_GET_BUF_PTR(cid2), 0, log_addr);
	VERIFY(!readBlock(log_addr, write_buf));
//	PRINT("\nread direct");
	for(i=0; i< FS_BLOCK_SIZE; i++){		
		VERIFY(COMPARE(write_buf[i],byte+1)); 
	}
//	PRINT("\ndirect verified");
	/* verify all other addresses in cache buffer are empty as expected*/
	for(i=1; i< LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_GET_INDEX(INDIRECT_CACHE_GET_BUF_PTR(cid2), i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}	
	
	return 1;
}

/**
 * @brief
 * write to a file until triple entry data offset. seek to beginning, write again, read file in triple offset.
 * verify a cache contains the triple block 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest8(){	
	uint8_t *f_name = "/file1.dat", byte = 'a', write_buf[FS_BLOCK_SIZE];
	int32_t i, fd, offset, cid2, tid, f_id;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	int32_t max_offset= TRIPLE_DATA_OFFSET;
	
	if(max_offset>FS_MAX_FILESIZE){
		max_offset = DOUBLE_DATA_OFFSET;
	}
//	PRINT("\ncacheTest8() - starting");
	
	/* verify we have at least one extra cache (otherwise, they will all be taken by the transaction)*/
	if(FS_EXTRA_INDIRECT_CACHES <1)
		return 1;
	
	/* create file */	
//	PRINT("\ncacheTest8() - creat");
	fd = creat(f_name, S_IRWXU);
	VERIFY(!IS_NEGATIVE(fd));
	
	/* initialize buffer*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
		write_buf[i] = byte;
	}
		
//	PRINT("\n\n\ncacheTest8() - about to write inode file data");
	/* write to file until indirect data offset*/
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, write_buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\n\n\ncacheTest6() - about to write until indirect");	
	for(offset = INODE_FILE_DATA_SIZE; offset< max_offset; offset+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
	}
	
	/* write a different buffer in double offset*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
		write_buf[i] = byte+1;
	}
	f_id = GET_FILE_ID_BY_FD(fd);
	tid = getTidByFileId(f_id);
	
//	printBlock(TRANSACTION_GET_INDIRECT_PTR(tid));
//	PRINT("\n\n\ncacheTest7() - about to write indirect 1st block");
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
		
	/* seek to start and write 1 block*/
//	printBlock(TRANSACTION_GET_INDIRECT_PTR(tid));
//	PRINT("\n\n\ncacheTest7() - about to write from beginning again");
	VERIFY(COMPARE(0, lseek(fd, 0, SEEK_SET)));
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
	VERIFY(COMPARE(TRANSACTION_GET_FILE_OFFSET(tid), 0));
	
	/* read a block from double offset*/
//	PRINT("\n\n\ncacheTest7() - about to read from double offset");	
//	PRINT_MSG_AND_NUM(", tid=", tid);
//	PRINT_MSG_AND_NUM(", tid ino addr=",logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	init_logical_address(file_addr);	
	VERIFY(!readFileBlock(write_buf, f_id, max_offset, file_addr, tid));
//	PRINT_MSG_AND_HEX("\nreadFileBlock() finished, file_addr=", *((uint32_t*)file_addr));
//	PRINT_MSG_AND_NUM("\nreadFileBlock() finished, inode addr saves is ",logicalAddressToPhysical(file_addr));
//	PRINT("\n\n\ncacheTest7() - verify read block");

	/* verify block */
	for(i=0; i< FS_BLOCK_SIZE; i++){
		if(!COMPARE(write_buf[i], byte+1)){
//			PRINT_MSG_AND_NUM("\ni=", i);
//			PRINT_MSG_AND_HEX(" expected=", byte+1);
//			PRINT_MSG_AND_HEX(" actual=", write_buf[i]);
		}
		VERIFY(COMPARE(write_buf[i], byte+1));
	}
	
//	for(i=0; i<FS_CACHE_BUFFERS; i++){		
//		PRINT_MSG_AND_NUM("\ncache ino num=", INDIRECT_CACHE_GET_INO_NUM(i));
//	}
	/* verify there is a cache for the file only
	 * (and for still open transaction)*/
// 	PRINT("\n\n\ncacheTest7() - get cache id");
	cid2 = getCachedIndirect(f_id);
	VERIFY(!IS_EMPTY_CID(cid2));
//	PRINT("\nverify caches empty");
	for(i=0; i<FS_CACHE_BUFFERS; i++){
		if(INDIRECT_CACHE_GET_TID(i) == tid || i== cid2)
			continue;
			
		VERIFY(IS_INDIRECT_CACHE_FREE(i)); 
	}
//	PRINT_MSG_AND_NUM("\nverify cache=", cid2);
	VERIFY(verifyCache(cid2, 2, max_offset, TID_EMPTY));
//	PRINT("\nverified");
	
	/* verify cache data, and that the buffer indeed contains indirect block.*/	
	BLOCK_GET_INDEX(INDIRECT_CACHE_GET_BUF_PTR(cid2), 0, log_addr);
	VERIFY(!readBlock(log_addr, write_buf));
//	PRINT("\nread direct");
	for(i=0; i< FS_BLOCK_SIZE; i++){		
		VERIFY(COMPARE(write_buf[i],byte+1)); 
	}
//	PRINT("\ndirect verified");
	/* verify all other addresses in cache buffer are empty as expected*/
	for(i=1; i< LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_GET_INDEX(INDIRECT_CACHE_GET_BUF_PTR(cid2), i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}	
	
	return 1;
}

/**
 * @brief
 * open a file in a directory. verify a cache contains the parent directory inode
 * (since namei required it...) 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest9(){
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE];
	int32_t fd, cid1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	init_logical_address(ino_addr);
	
	/* create file*/
	fd = creat(f_name, S_IRWXU);
	VERIFY(fd>=0);
	VERIFY(!close(fd));
	
	/* open file again*/	
	fd = open(f_name, NANDFS_O_RDONLY, S_IRWXU);
	VERIFY(fd>=0);
	
	/* verify cache for parent*/
	cid1 = getCachedIndirect(1);
	VERIFY(verifyCachesEmpty(cid1));
	
	VERIFY(verifyCache(cid1, 1, 0, TID_EMPTY)); 
	VERIFY(!getInode(buf, 1, ino_addr));
	
	VERIFY(compare_fsbufs(buf, INDIRECT_CACHE_GET_BUF_PTR(cid1)));
	
	return 1;
}


/**
 * @brief
 * write to file until triple offset, close.
 * read file and verify on each indirect offset transfer, that the cache changes accordingly
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest10(){	
	uint8_t *f_name = "/file1.dat", byte = 'a', write_buf[FS_BLOCK_SIZE], expected_cache[FS_BLOCK_SIZE];
	int32_t max_offset= TRIPLE_DATA_OFFSET;
	int32_t i, fd, offset, cid1, cid2, f_id, f_size;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	init_logical_address(ino_addr);
	
	if(max_offset>FS_MAX_FILESIZE){
		max_offset = DOUBLE_DATA_OFFSET;
	}
		
	f_size = max_offset+LOG_ADDRESSES_PER_BLOCK*FS_BLOCK_SIZE;	
//	PRINT("\ncacheTest7() - starting");
	
	/* verify we have at least one extra cache (otherwise, they will all be taken by the transaction)*/
	if(FS_EXTRA_INDIRECT_CACHES <1)
		return 1;
	
	/* create file */	
//	PRINT("\ncacheTest2() - creat");
	fd = creat(f_name, S_IRWXU);
	VERIFY(!IS_NEGATIVE(fd));
	
	/* initialize buffer*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
		write_buf[i] = byte;
	}
		
//	PRINT("\n\n\ncacheTest7() - about to write inode file data");
	/* write to file until indirect data offset*/
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, write_buf, INODE_FILE_DATA_SIZE)));
//	PRINT("\n\n\ncacheTest6() - about to write until indirect");	
	for(offset = INODE_FILE_DATA_SIZE; offset< f_size; offset+=FS_BLOCK_SIZE){
		/* initialize buffer*/
		for(i=0; i< FS_BLOCK_SIZE; i++){
			write_buf[i] = byte+offset%22;
		}		
		
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, write_buf, FS_BLOCK_SIZE)));
	}
	
	f_id = GET_FILE_ID_BY_FD(fd);
	VERIFY(!close(fd));	
	VERIFY(verifyCachesEmpty(CID_EMPTY));
	
	/* open file*/
//	PRINT("\nopen file");
	fd = open(f_name, NANDFS_O_RDONLY, S_IRWXU);
	VERIFY(fd >=0);
	cid1 = getCachedIndirect(1); 
	/* verify all caches empty ,besides the one for the parent directory*/
	VERIFY(verifyCachesEmpty(cid1));
	init_indirect_caches();
	
	/* read inode */
//	PRINT("\nread inode");
	VERIFY(!getInode(expected_cache, f_id, ino_addr));
	VERIFY(verifyCachesEmpty(CID_EMPTY));
	
	/* read beginning of file*/
	offset=0;
	VERIFY(COMPARE(read(fd, write_buf, INODE_FILE_DATA_SIZE),INODE_FILE_DATA_SIZE));
	for(i=0; i< INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(write_buf[i], byte));
	}
			
	/* read direct entries*/
	for(offset=INODE_FILE_DATA_SIZE; offset < INDIRECT_DATA_OFFSET; offset+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(read(fd, write_buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
		
		/* verify block */
		for(i=0; i< FS_BLOCK_SIZE; i++){
			VERIFY(COMPARE(write_buf[i], byte+offset%22));
		}
	}
	
	/* verify the cache buffer is the file inode*/
	cid2 = getCachedIndirect(f_id);
	for(i=0; i< FS_BLOCK_SIZE; i++){
		VERIFY(COMPARE(INDIRECT_CACHE_GET_BUF_PTR(cid2)[i], expected_cache[i]));
	}
	
	/* read from indirect and onwards */
	for(; offset< f_size; offset+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(read(fd, write_buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
				
		/* verify block */
		for(i=0; i< FS_BLOCK_SIZE; i++){
			VERIFY(COMPARE(write_buf[i], byte+offset%22));
		}
		
		/* verify indirect cache */
		VERIFY(verifyCachesEmpty(cid2));
		VERIFY(!readIndirectToBuffer(offset, ino_addr, expected_cache));
		for(i=0; i< FS_BLOCK_SIZE; i++){
			VERIFY(COMPARE(INDIRECT_CACHE_GET_BUF_PTR(cid2)[i], expected_cache[i]));
		}
		VERIFY(verifyCachesEmpty(cid2));
	}
	
	return 1;
}

/* itoa implementation from http://en.wikipedia.org/wiki/Itoa*/
/* reverse:  reverse string s in place */
error_t fsStrlen (uint8_t *ptr)
{
    uint32_t* ip;
    uint8_t*  s;
 
    // NOTE: that this check is rarley done in C libraries.
    if (NULL == ptr)
       return 0; //or 0
 
    // Handle alignment here somehow if not handled within string.
    // String has to be aligned on integer boundary
    // or some architectures might behave unexpectedly!
    // NOTE: This is also required so that you don't overflow the
    // bounds of the memory by looking at bytes beyond the end of
    // the C-string.
    ip = (uint32_t *) ptr;
 
    // multi-skip (less times around the loop)
    while (1) { /* check for all zero */
        uint32_t x = *ip; /* check for not-all zero */
        if (((((x & 0x7f7f7f7f) + 0x7f7f7f7f7f) | x) & 0x80808080) != 0x80808080)
            break;
        ++ip; /* moves forward 4 bytes at once */
    }
 
    // count any remaining bytes
    for (s = (uint8_t*)ip; 0 != *s; ++s)
        /* do nothing */ ;
 
    return s - ptr;
}

void reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = fsStrlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n, char s[]){
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
} 
/*************************************************************/

/**
 * @brief
 * read 3 files, and verify each one of them has a buffer
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest11(){
	int32_t i, fd2, fd3, fd4, cid2, cid3, cid4;
	uint8_t f_name[20] = "/", str[20], buf[FS_BLOCK_SIZE];
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	init_logical_address(ino_addr);
	
	/* create and open all files*/	
	itoa(2, str);
	fsStrcpy(&(f_name[1]), str);	
	fd2 = creat(f_name, S_IRWXU);
	VERIFY(fd2>=0);
	
	itoa(3, str);
	fsStrcpy(&(f_name[1]), str);	
	fd3 = creat(f_name, S_IRWXU);
	VERIFY(fd3>=0);
	
	itoa(4, str);
	fsStrcpy(&(f_name[1]), str);	
	fd4 = creat(f_name, S_IRWXU);
	VERIFY(fd4>=0);
	
	/* write to all files*/
	for(i=0; i<FS_BLOCK_SIZE; i++){
		buf[i] = 'a';
	}
	VERIFY(COMPARE(200, write(fd2, buf, 200)));
	VERIFY(!close(fd2));
	VERIFY(COMPARE(200, write(fd3, buf, 200)));
	VERIFY(!close(fd3));
	VERIFY(COMPARE(200, write(fd4, buf, 200)));
	VERIFY(!close(fd4));
	
	/* re-open for reading*/
	itoa(2, str);
	fsStrcpy(&(f_name[1]), str);	
	fd2 = open(f_name, NANDFS_O_RDONLY, S_IRWXU);
	VERIFY(fd2>=0);
	
	itoa(3, str);
	fsStrcpy(&(f_name[1]), str);	
	fd3 = open(f_name, NANDFS_O_RDONLY, S_IRWXU);
	VERIFY(fd3>=0);
	
	itoa(4, str);
	fsStrcpy(&(f_name[1]), str);	
	fd4 = open(f_name, NANDFS_O_RDONLY, S_IRWXU);
	VERIFY(fd4>=0);
	
	/* read beginning of every file*/
//	PRINT("\nread files");
	VERIFY(COMPARE(100,read(fd2, buf, 100)));
	VERIFY(COMPARE(100,read(fd3, buf, 100)));
	VERIFY(COMPARE(100,read(fd4, buf, 100)));
	
	/* verify there is a cache for each file that contains the file inode*/
//	PRINT("\nget indirect caches");
	cid2 = getCachedIndirect(2);
	cid3 = getCachedIndirect(3);
	cid4 = getCachedIndirect(4);
//	PRINT_MSG_AND_NUM("\ncid2=", cid2);
//	PRINT_MSG_AND_NUM("\ncid3=", cid3);
//	PRINT_MSG_AND_NUM("\ncid4=", cid4);
	VERIFY(!IS_EMPTY_CID(cid2));
	VERIFY(!IS_EMPTY_CID(cid3));
	VERIFY(!IS_EMPTY_CID(cid4));
	
//	PRINT("\nverify caches");
	VERIFY(verifyCache(cid2, 2, 0, TID_EMPTY));
	VERIFY(verifyCache(cid3, 3, 0, TID_EMPTY));
	VERIFY(verifyCache(cid4, 4, 0, TID_EMPTY));
	
	VERIFY(!getInode(buf, 2, ino_addr));
	VERIFY(compare_bufs(buf, INDIRECT_CACHE_GET_BUF_PTR(cid2), FS_BLOCK_SIZE));
	VERIFY(!getInode(buf, 3, ino_addr));
	VERIFY(compare_bufs(buf, INDIRECT_CACHE_GET_BUF_PTR(cid3), FS_BLOCK_SIZE));
	VERIFY(!getInode(buf, 4, ino_addr));
	VERIFY(compare_bufs(buf, INDIRECT_CACHE_GET_BUF_PTR(cid4), FS_BLOCK_SIZE));
	
	return 1;
}

/**
 * @brief
 * create file, write, close.
 * unlink file, create again.
 * try to read file and verify failure
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest12(){
	unsigned char *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'a';
	int32_t i, fd;
	
	/*create file and write*/
	fd = creat(f_name, S_IRWXU);
	VERIFY(fd >=0);
	 
	for(i=0; i< FS_BLOCK_SIZE; i++){
		buf[i] = byte;	
	}
	
	VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
	VERIFY(!close(fd));
	
	/* unlink*/
	VERIFY(!unlink(f_name));
	
	/* creat again, read*/
	fd = creat(f_name, S_IRWXU);
	VERIFY(fd >=0);
		
	VERIFY(IS_NEGATIVE(read(fd, buf, FS_BLOCK_SIZE)));
	
	return 1;	
}

/**
 * @brief
 * read two files concurrently, and verify we have two caches relating to each file
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest21(){
	uint8_t *f_name1 = "/file1.dat", *f_name2 = "/file2.dat", byte = 'a', write_buf[FS_BLOCK_SIZE];
	int32_t i, fd1, fd2;
	int32_t cid1, cid2, cid3;
	
	if(FS_EXTRA_INDIRECT_CACHES % 3 <2)
		return 1;
	
	/* create files*/
	fd1 = creat(f_name1, S_IRWXU);
	VERIFY(fd1>=0);	
	
	/* write to first file*/	
	fsMemset(write_buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd1, write_buf, INODE_FILE_DATA_SIZE)));	
	VERIFY(!close(fd1));
	
	fd2 = creat(f_name2, S_IRWXU);	
	VERIFY(fd2>=0);
	
	/* write to 2nd file*/	
	fsMemset(write_buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd2, write_buf, INODE_FILE_DATA_SIZE)));	
	VERIFY(!close(fd2));
	
	/* open file for reading*/
	fd1 = open(f_name1, NANDFS_O_RDONLY, S_IRWXU);
	VERIFY(fd1>=0);	
	fd2 = open(f_name2, NANDFS_O_RDONLY, S_IRWXU);
	VERIFY(fd2>=0);
			
	/* we should now have caches for both file inodes, and parent directory*/
	cid1 = getCachedIndirect(1);
	cid2 = getCachedIndirect(GET_FILE_ID_BY_FD(fd1));
	cid3 = getCachedIndirect(GET_FILE_ID_BY_FD(fd2));
			
	VERIFY(verifyCache(cid1, 1, 0, TID_EMPTY));		
	VERIFY(verifyCache(cid2, 2, 0, TID_EMPTY));
	VERIFY(verifyCache(cid3, 3, 0, TID_EMPTY));
			
	for(i=0; i< FS_CACHE_BUFFERS; i++){
		if(i==cid1 || i==cid2 || i==cid3){
			continue;
		}
		
		VERIFY(IS_INDIRECT_CACHE_FREE(i));
	}		
			
	return 1;
}

/**
 * @brief
 * write to 2 files, when we have room for 2 transactions, and verify that we use both of them 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest22(){
	uint8_t *f_name1 = "/file1.dat", *f_name2 = "/file2.dat", byte = 'a', write_buf[FS_BLOCK_SIZE];
	int32_t i, fd1, fd2, offset;
	int32_t tid1, tid2, blk_count1 = 0, blk_count2 = 0;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	int32_t max_offset= TRIPLE_DATA_OFFSET;
	
	if(max_offset>FS_MAX_FILESIZE){
		max_offset = DOUBLE_DATA_OFFSET;
	}
	
	init_logical_address(ino_addr);	
	/* verify we have at least 2 transcations*/
	if(FS_MAX_N_TRANSACTIONS < 2)
		return 1;
	
	/* create files*/
	fd1 = creat(f_name1, S_IRWXU);
	fd2 = creat(f_name2, S_IRWXU);
	VERIFY(fd1>=0);
	VERIFY(fd2>=0);
	verifyCachesEmpty(CID_EMPTY);
	
	/* write to first file*/	
	fsMemset(write_buf, byte, FS_BLOCK_SIZE);
//	PRINT_MSG_AND_NUM("\n\noffset=",0);
//	PRINT(", 1st file");	
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd1, write_buf, INODE_FILE_DATA_SIZE)));
//	PRINT_MSG_AND_NUM("\n\noffset=",0);
//	PRINT(", 2nd file");
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd2, write_buf, INODE_FILE_DATA_SIZE)));
	tid1 = getTidByFileId(GET_FILE_ID_BY_FD(fd1));	
	tid2 = getTidByFileId(GET_FILE_ID_BY_FD(fd2));
	for(offset=INODE_FILE_DATA_SIZE; offset<max_offset; offset+=FS_BLOCK_SIZE){
//		for(i=0; i< 3;i++){
//			PRINT_MSG_AND_NUM("\nverify cache ", i);
//			VERIFY(INDIRECT_CACHE_GET_TID(i)==0);
//			PRINT_MSG_AND_NUM("\nverify cache ", i+3);
//			VERIFY(INDIRECT_CACHE_GET_TID(i+3)==1);
//		}
		
//		PRINT_MSG_AND_NUM("\n\noffset=",offset);
//		PRINT(", 1st file");
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd1, write_buf, FS_BLOCK_SIZE)));
//		PRINT_MSG_AND_NUM("\n\noffset=",offset);
//		PRINT(", 2nd file");
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd2, write_buf, FS_BLOCK_SIZE)));
		
//		for(i=0; i<3; i++){
//			VERIFY(verifyCache(i, INO_NUM_EMPTY, 0xffffffff, tid1));		
//		}
//		for(; i<6; i++){
//			VERIFY(verifyCache(i, INO_NUM_EMPTY, 0xffffffff, tid2));
//		}
	}	
	
	
	/* verify we have two transactions*/
//	PRINT("\nverify caches");
	for(i=0; i<FS_CACHE_BUFFERS; i++){
		if(INDIRECT_CACHE_GET_TID(i) == tid1){
			VERIFY(verifyCache(i, INO_NUM_EMPTY, 0xffffffff, tid1));
			blk_count1++;
		}
		else if(INDIRECT_CACHE_GET_TID(i) == tid2){
			VERIFY(verifyCache(i, INO_NUM_EMPTY, 0xffffffff, tid2));
			blk_count2++;
		}		
		else{
			verifyCache(i, INO_NUM_EMPTY, 0xffffffff, 0xffffffff);
			VERIFY(IS_INDIRECT_CACHE_FREE(i));
		}
	}		
	
	VERIFY(COMPARE(blk_count1,3));
	VERIFY(COMPARE(blk_count2,3));
			
			
			
	return 1;
}
