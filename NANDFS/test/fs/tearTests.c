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

/** @file tearTests.c  */
#include <test/fs/tearTests.h>
#include <test/fs/testsHeader.h>
#ifdef Profiling
#include <profilingTests.h>
#endif
void runAllfsBootingTests(){

	RUN_TEST(fsBooting,1);
	RUN_TEST(fsBooting,3);
	RUN_TEST(fsBooting,4);
	RUN_TEST(fsBooting,5);
	RUN_TEST(fsBooting,6);
	RUN_TEST(fsBooting,7);
	RUN_TEST(fsBooting,9);
	RUN_TEST(fsBooting,10);
	RUN_TEST(fsBooting,11);
	RUN_TEST(fsBooting,12);

	RUN_TEST(fsBooting,17);
	RUN_TEST(fsBooting,18);
	RUN_TEST(fsBooting,15);

//	/* test that require delicate rewrite (crashing in the middle of functions)*/
//	RUN_TEST(fsBooting,13);
//	RUN_TEST(fsBooting,14);
}

/**
 * @brief
 * init fsBooting test
 */
void init_fsBootingTest(){
	if(nandInit())
		return;

	init_flash();
	initializeRamStructs();
}

/**
 * @brief
 * tear down fsBooting test
 */
void tearDown_fsBootingTest(){
	init_flash();
	initializeRamStructs();

	nandTerminate();
	init_fsbuf(fs_buffer);
	init_fs();
}

//#if 1
/**
 * @brief
 * boot empty flash.
 * verify inode0, root directory exists.
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest1(){
	int32_t i;
	uint8_t buf[FS_BLOCK_SIZE];
	inode_t *ino_ptr = CAST_TO_INODE(buf);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	dirent_flash *de_ptr = CAST_TO_DIRENT(buf);

//	PRINT("\nfsBootingTest1 starting");
	init_logical_address(log_addr);
	init_logical_address(root_addr);

	/* boot */
	VERIFY(!fsBooting());
//	PRINT("\nbooting success");

	/* verify inode0 */
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), buf));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nino0 inode verify success");
	VERIFY(verifyFileSuffixlyEmpty(ino_ptr));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
//	PRINT("\nino0 success");
	/* verify root inode */
	VERIFY(!fsReadBlockSimple(root_addr, buf));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");
	VERIFY(verifyFileSuffixlyEmpty(ino_ptr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlockSimple(log_addr, buf));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}

	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));

	return 1;
}

/**
 * @brief
 * booting failed before creating inode0. verify recovery
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest3(){
	int32_t i, res, old_root_addr, tid;
	bool_t pendingVOTs;
	uint8_t buf[FS_BLOCK_SIZE];
	inode_t *ino_ptr = CAST_TO_INODE(buf);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	dirent_flash *de_ptr = CAST_TO_DIRENT(buf);

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	tid = getFreeTransactionId();

	/* boot sequencing layer -
	 * locate most recent checkpoint, erase following data etc. */
	RES_VERIFY(res, sequencingBooting(fs_ptr, sizeof(filesystem_t), &pendingVOTs, fsCheckpointWriter));
	L("sequencingBooting success. rec offset %d", GET_RECLAIMED_OFFSET());
	TRANSACTION_SET_TYPE(tid, T_TYPE_WRITE);
	TRANSACTION_SET_INO(tid, 0);

	/* write root directory inode, and save it's address*/
	TR_RES_VERIFY(res, setNewInode(1, 1, FTYPE_DIR, tid));
//	PRINT("\nhandleNoFs() - setNewInode success");
	TR_RES_VERIFY(res, allocAndWriteBlockTid(log_addr,
											 TRANSACTION_GET_INDIRECT_PTR(tid),
											 DATA_TYPE_REGULAR,
											 CACHE_ENTRY_OFFSET_EMPTY,
											 tid));
	L("wrote root addr to %x", ADDR_PRINT(log_addr));
	old_root_addr = logicalAddressToPhysical(log_addr);

	/* mock reboot */
	initializeRamStructs();

	/* boot */
	L("now really call fsBooting. \n\n");
	VERIFY(!fsBooting());

	/* verify inode0 */
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), buf));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nino0 inode verify success");
	VERIFY(verifyFileSuffixlyEmpty(ino_ptr));
//	PRINT("\nino0 success");

	/* verify root inode */
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!fsReadBlockSimple(root_addr, buf));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");
	VERIFY(verifyFileSuffixlyEmpty(ino_ptr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlockSimple(log_addr, buf));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}

	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));

	/* verify there are no obsolete pages */
	VERIFY(COMPARE(GET_OBS_COUNT(), 0));

	/* verify root addr is as expected. this means we erased initial not-completed
	 * file system pages.*/
	L("expected root addr %d, actual one %d", old_root_addr, logicalAddressToPhysical(root_addr));
	VERIFY(COMPARE(old_root_addr, logicalAddressToPhysical(root_addr)));

	return 1;
}

/**
 * @brief
 * in allocation mode - creat files write to them, close.
 * verify on reboot we return state where all files are empty
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest4(){
	int32_t i, fd1, fd2, write_size = FS_BLOCK_SIZE*3, f_size;
	uint8_t f_name[11] = "/file1.dat", byte = 'a';
	int32_t buf_size = FS_BLOCK_SIZE*5+200;
	uint8_t buf[FS_BLOCK_SIZE*5+200];

	/* we must be able to have at least two simultaneous transactions for this test to succeed*/
	if(FS_MAX_N_TRANSACTIONS <2){
		return 1;
	}

	/* boot */
	VERIFY(!fsBooting());

	/* create files, and write to them*/
	fd1 = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd1));
	fsMemset(buf, byte, buf_size);
	VERIFY(COMPARE(write(fd1, buf, write_size), write_size));
//	PRINT("\nwrite 1 success");

	f_name[5]++;
	fd2 = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT_MSG_AND_NUM("\nlast cp address=", logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())-1);
	VERIFY(COMPARE(write(fd2, buf, write_size), write_size));
//	PRINT("\nwrite 2 success");


	for(i=0; i < 10; i++){
		VERIFY(COMPARE(write(fd1, buf, write_size), write_size));
		VERIFY(COMPARE(write(fd2, buf, write_size), write_size));
	}
//	PRINT("\nvarious writes success");

//	PRINT_MSG_AND_NUM("\nb4 reboot rec address=", logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
	/* reboot */
//	PRINT("\n\n\n\n");
	VERIFY(!fsBooting());
//	PRINT("\nreboot success");
	f_name[5] -= 1;

	/* verify all files are empty */
	for(i=0; i< 2; i++){
		f_size = getFileSize(i+2);
//		PRINT_MSG_AND_NUM("\nf_size=", f_size);
		VERIFY(COMPARE(f_size, 0));

		f_name[5]++;
	}

	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));

	return 1;
}

//static int32_t
//verifyLRU(){
//	int32_t i, lru, mru;
//
//	/* verify each cache individually*/
//	for(i=0; i< FS_CACHE_BUFFERS; i++){
//		lru = CACHE_GET_LESS_RECENTLY_USED(i);
//		mru = CACHE_GET_MORE_RECENTLY_USED(i);
//
//		/* cache is free?
//		 * if so shouldn't have mru or lru, or be global something*/
//		if(IS_CACHE_FREE(i)){
//			VERIFY(IS_CACHE_ID_EMPTY(lru));
//			VERIFY(IS_CACHE_ID_EMPTY(mru));
//			VERIFY(CACHE_LRU_Q_GET_LRU() != i);
//			VERIFY(CACHE_LRU_Q_GET_MRU() != i);
//			continue;
//		}
//
//		/* if we have less used, verify that for it cod is more used*/
//		if(!IS_CACHE_ID_EMPTY(lru)){
//			VERIFY(CACHE_GET_MORE_RECENTLY_USED(lru)==i);
//		}
//		/* non-free cache with no less used - should be global lru*/
//		else{
//			if(!COMPARE(CACHE_LRU_Q_GET_LRU(), i)){
//				L("expected global lru %d, actual %d", i, CACHE_LRU_Q_GET_LRU());
//			}
//			VERIFY(COMPARE(CACHE_LRU_Q_GET_LRU(), i));
//		}
//
//		if(!IS_CACHE_ID_EMPTY(mru)){
//			VERIFY(CACHE_GET_LESS_RECENTLY_USED(mru)==i);
//		}
//		/* non-free cache with no more used - should be global mru*/
//		else{
//			VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), i));
//		}
//
//		/* i is not free. if we have at least one used cache
//		 * and this cache has no more and less used,
//		 * it should be global mru and lru */
//		if(CACHE_LRU_Q_GET_DIRTY_COUNT() !=0){
//			if(IS_CACHE_ID_EMPTY(lru) && IS_CACHE_ID_EMPTY(mru)){
//				VERIFY(COMPARE(CACHE_LRU_Q_GET_LRU(), i));
//				VERIFY(COMPARE(CACHE_LRU_Q_GET_MRU(), i));
//			}
//		}
//	}
//
//	/* follow lru and verify */
//	i=CACHE_LRU_Q_GET_LRU();
//	while(!IS_CACHE_ID_EMPTY(i)){
//		lru = CACHE_GET_LESS_RECENTLY_USED(i);
//		mru = CACHE_GET_MORE_RECENTLY_USED(i);
//
//		/* if we have less used, verify that for it cod is more used*/
//		if(!IS_CACHE_ID_EMPTY(lru)){
//			VERIFY(CACHE_GET_MORE_RECENTLY_USED(lru)==i);
//		}
//		/* else i should be global lru*/
//		else{
//			VERIFY(COMPARE(i, CACHE_LRU_Q_GET_LRU()));
//		}
//
//		if(!IS_CACHE_ID_EMPTY(mru)){
//			VERIFY(CACHE_GET_LESS_RECENTLY_USED(mru)==i);
//		}
//		/* else i should be global mru*/
//		else{
//			VERIFY(COMPARE(i, CACHE_LRU_Q_GET_MRU()));
//		}
//
//		i = CACHE_GET_MORE_RECENTLY_USED(i);
//	}
//
//	return 1;
//}

/**
 * @brief
 * in reclamation mode -  creat file write , close.
 * verify on reboot we return to same state
 *
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest5(){
	int32_t i, fd1, fd2;
	int32_t write_size = FS_BLOCK_SIZE*3, f_size;
	int32_t seg ,new_slot, old_slot;
	uint8_t f_name[11] = "/file1.dat", byte = 'a';
	int32_t buf_size = FS_BLOCK_SIZE*5+200;
	uint8_t buf[FS_BLOCK_SIZE*5+200];
	/* we must be able to have at least two simultaneous transactions for this test to succeed*/
	if(FS_MAX_N_TRANSACTIONS <2){
		return 1;
	}

	/* boot */
//	L("call boot");
	VERIFY(!fsBooting());

	/* create files, and write to them*/
//	L("create files, and write to them");
	fd1 = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd1));
	fsMemset(buf, byte, buf_size);
	VERIFY(COMPARE(write(fd1, buf, write_size), write_size));
//	L("\nwrite 1 success");

	/* create 2nd file*/
	f_name[5]++;
	fd2 = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT_MSG_AND_NUM("\nlast cp address=", logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())-1);
	VERIFY(COMPARE(write(fd2, buf, write_size), write_size));
//	PRINT("\nwrite 2 success");

	/* write to 2 files. one of them will be aborted*/
	while(!IS_STATE_RECLAMATION()){
//		PRINT("\n");
//		L("1st write. rec seg %d offset %d", GET_RECLAIMED_OFFSET(), GET_RECLAIMED_OFFSET());
		if(!IS_TRANSACTION_EMPTY(0)){
			(COMPARE(FS_BLOCK_SIZE, write(fd1, buf, FS_BLOCK_SIZE)));
		}

//		L("2nd write. rec seg %d offset %d", GET_RECLAIMED_OFFSET(), GET_RECLAIMED_OFFSET());
		if(!IS_TRANSACTION_EMPTY(1)){
			VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd2, buf, FS_BLOCK_SIZE)));
		}
	}

	seg = GET_RECLAIMED_SEGMENT();
	new_slot = GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr);
	old_slot = GET_RECLAIMED_SEGMENT_SLOT();
//	PRINT("\nvarious writes success");
//	PRINT_MSG_AND_NUM("\nrec seg=", GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM("\nrec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nb4 reboot rec address=", logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
	/* reboot */
//	L("call 1st reboot");
	VERIFY(!fsBooting());
//	L("reboot success");
	f_name[5] -= 1;

	/* verify all files are empty */
	for(i=0; i< 2; i++){
		f_size = getFileSize(i+2);
//		PRINT_MSG_AND_NUM("\nf_size=", f_size);
		VERIFY(COMPARE(f_size, 0));

		f_name[5]++;
	}
//	PRINT("\nverified file sizes");
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	/* verify we returned to state before reboot */
//	PRINT_MSG_AND_NUM("\nrec seg=", GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" expected=", seg);
	VERIFY(COMPARE(seg, GET_RECLAIMED_SEGMENT()));
//	PRINT_MSG_AND_NUM("\nnew_slot=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" expected=", new_slot);
	VERIFY(COMPARE(new_slot, GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr)));
//	PRINT_MSG_AND_NUM("\nold_slot=", GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" expected=", old_slot);
	VERIFY(COMPARE(old_slot, GET_RECLAIMED_SEGMENT_SLOT()));
//	PRINT("\nverified state");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
	}

	return 1;
}

/**
 * @brief
 * in reclamation mode -  creat files write to them, close, re-open and write uncommited data.
 * verify on reboot we return to last commited state
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest6(){
	int32_t i, fd1, fd2, write_size = FS_BLOCK_SIZE*3, f_size1, f_size2;
	uint8_t f_name[11] = "/file1.dat", byte = 'a';
	int32_t buf_size = FS_BLOCK_SIZE*5+200;
	uint8_t buf[FS_BLOCK_SIZE*5+200];

	/* we must be able to have at least two simultaneous transactions for this test to succeed*/
	if(FS_MAX_N_TRANSACTIONS <2){
		return 1;
	}

	/* initial boot */
	VERIFY(!fsBooting());
	fsMemset(buf, byte, buf_size);

	/* create files, and write to them*/
	fd1 = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd1));
	VERIFY(COMPARE(write(fd1, buf, write_size), write_size));
//	PRINT("\nwrite 1 success");

	f_name[5]++;
	fd2 = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT_MSG_AND_NUM("\nlast cp address=", logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())-1);
	VERIFY(COMPARE(write(fd2, buf, write_size), write_size));
//	PRINT("\nwrite 2 success");

	/* write to 2 files. one of them will be aborted*/
	while(GET_RECLAIMED_SEGMENT() < SEQ_SEGMENTS_COUNT / 2){
//		PRINT_MSG_AND_NUM("\nrec seg=", GET_RECLAIMED_SEGMENT());
//		PRINT_MSG_AND_NUM(", offset=", GET_RECLAIMED_OFFSET());
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd1, buf, FS_BLOCK_SIZE)));
//		PRINT_MSG_AND_NUM("\nwrite res=", res);
//		PRINT_MSG_AND_NUM(" state rec=", IS_STATE_RECLAMATION());
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd2, buf, FS_BLOCK_SIZE)));
	}
//	PRINT("\nmore writes success");
	/* close transactions and verify they are not marked in file system */
	VERIFY(!close(fd1));
	VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(0)));
	VERIFY(!IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(1)));
	VERIFY(!close(fd2));
	VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(0)));
	VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(1)));
//	PRINT("\nclose success");
//	{
//		fsReadBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
//		inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
//		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
//		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//
//		INODE_GET_DIRECT(ino_ptr, 0, root_addr);
//		fsReadBlock(root_addr, fs_buffer);
//		INODE_GET_DIRECT(ino_ptr, 0, log_addr);
//		fsReadBlock(log_addr, fs_buffer);
//
//		dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);
//		printBlock(fs_buffer);
//		PRINT("\nread root files");
//		while(!IS_DIRENT_EMPTY(de_ptr)){
//			PRINT_MSG_AND_STR("\nroot file name=", DIRENT_GET_NAME(de_ptr));
//			PRINT_MSG_AND_NUM(" ino_num=", DIRENT_GET_INO_NUM(de_ptr));
//			moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
//		}
//	}

	f_size1 = getFileSize(2);
	f_size2 = getFileSize(3);
//	PRINT_MSG_AND_NUM("\nf_size1=", f_size1);
//	PRINT_MSG_AND_NUM("\nf_size2=", f_size2);
//	PRINT_MSG_AND_NUM("\nafter closes, ino0 addr=",logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));

	/* re-open files*/
	f_name[5]--;
	fd1 = open(f_name, NANDFS_O_WRONLY, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\nre-open 1 success");
	f_name[5]++;
	fd2 = open(f_name, NANDFS_O_WRONLY, 0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT("\nre-open 2 success");
	/* keep writing */
	while(GET_RECLAIMED_SEGMENT() < SEQ_SEGMENTS_COUNT -3){
		f_size1 = getFileSize(2);
		f_size2 = getFileSize(3);
//		PRINT_MSG_AND_NUM("\nf_size1=", f_size1);
//		PRINT_MSG_AND_NUM(", f_size2=", f_size2);
//		PRINT_MSG_AND_NUM("\nrec seg=", GET_RECLAIMED_SEGMENT());
//		PRINT_MSG_AND_NUM(", offset=", GET_RECLAIMED_OFFSET());
//		if(GET_RECLAIMED_SEGMENT() == 105 && GET_RECLAIMED_OFFSET()==14){
//			PRINT("");
//		}
//		PRINT("\n\nwrite to 1st file");
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd1, buf, FS_BLOCK_SIZE)));
//		PRINT_MSG_AND_NUM(" state rec=", IS_STATE_RECLAMATION());
//		PRINT("\n\nwrite to 2nd file");
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd2, buf, FS_BLOCK_SIZE)));
//		PRINT(", write done");
	}

//	PRINT("\nvarious writes success");
//	PRINT_MSG_AND_NUM("\nrec seg=", GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM("\nrec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nb4 reboot rec address=", logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
	/* reboot */
//	PRINT("\n\n\n\n");
	VERIFY(!fsBooting());
//	PRINT("\nreboot success");
	f_name[5] -= 1;

	VERIFY(COMPARE(f_size1, getFileSize(2)));
	f_name[5]++;
	VERIFY(COMPARE(f_size2, getFileSize(3)));

//	PRINT("\nverified file sizes");
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
	}

	return 1;
}

/**
 * @brief
 * crash in the middle of writing, verify success
 *
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest7(){
	int32_t i, fd, rec_addr, obs, free, f_size;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'g';

	VERIFY(!fsBooting());

	/* create file*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	fsMemset(buf, byte, FS_BLOCK_SIZE);
//	PRINT("\ncreat success");
	/* write, close, partial write to file*/
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
//	PRINT_MSG_AND_NUM("\nb4 commiting transaction, obs=",GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(", transaction vots=",TRANSACTION_GET_VOTS_COUNT(0));
	VERIFY(!close(fd));
//	PRINT("\nclose success");

	obs  = GET_OBS_COUNT();
	free = GET_FREE_COUNTER();
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	f_size = getFileSize(2);

	fd = open(f_name, NANDFS_O_WRONLY, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nre-opened file");

	/* execute write function code of first partial write*/
	{
		int32_t tid, res = 0, fileOffset, ino_num , writeSize, f_size, count = FS_BLOCK_SIZE;
		int32_t tempCount = count;
		int32_t bytesWritten = 0;
		uint8_t *data_buf;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

		init_logical_address(log_addr);

		FS_VERIFY(!verifyFd(fd));

		/* verify count is legal */
		FS_VERIFY(count >= 0);

		/* verify file open for writing*/
		FS_VERIFY(IS_WRONLY(OPEN_FILE_GET_FLAGS(fd)));
		/* if count is 0, return*/
		if(count == 0){
			return 0;
		}

		ino_num = GET_FILE_ID_BY_FD(fd);
		f_size  = getFileSize(ino_num);

		/* if we're trying to write beyond file size, abort*/
		FS_VERIFY(OPEN_FILE_GET_OFFSET(fd) <= f_size);
		/* check for existing transaction handling the write*/
		tid     = getTidByFileId(ino_num);
		if(IS_EMPTY_TID(tid)){
			return 0;
		}
		TRANSACTION_SET_TYPE(tid,T_TYPE_WRITE);
		TRANSACTION_SET_FILE_SIZE(tid, f_size);
		TRANSACTION_SET_FTYPE(tid, FTYPE_FILE);

		data_buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
		fileOffset = OPEN_FILE_GET_OFFSET(fd);
		/* handle first blcok, so we can move on to block-size writes.
		 * if we are in inode data offset, make sure to change only until INODE_FILE_DATA_SIZE. */
		TR_RES_VERIFY(res, readFileBlock(data_buf, ino_num, fileOffset, TRANSACTION_GET_INO_ADDR_PTR(tid), tid));
		if(fileOffset < INODE_FILE_DATA_SIZE){
			/* copy data to inode.
			 * if we have more to write than inode file data size: */
			if(tempCount > INODE_FILE_DATA_SIZE-fileOffset){
				writeSize = INODE_FILE_DATA_SIZE-fileOffset;
			}else{
				writeSize = tempCount;
			}
			fsMemcpy(data_buf+fileOffset, CAST_TO_UINT8(buf), writeSize);
			/* write block. this will also read indirect block to transaction (if it is empty)*/
			TR_RES_VERIFY(res, writeFileBlock(0, fileOffset+writeSize, DATA_TYPE_REGULAR, tid, log_addr, 1));
		}
		else{
			if(tempCount> FS_BLOCK_SIZE - CALC_OFFSET_IN_FILE_BLOCK(fileOffset)){
				writeSize = FS_BLOCK_SIZE - CALC_OFFSET_IN_FILE_BLOCK(fileOffset);
			}else{
				writeSize = tempCount;
			}
			fsMemcpy(data_buf+CALC_OFFSET_IN_FILE_BLOCK(fileOffset), CAST_TO_UINT8(buf), writeSize);
			/* write block. this will also read indirect block to transaction (if it is empty)*/
			TR_RES_VERIFY(res, writeFileBlock(fileOffset-CALC_OFFSET_IN_FILE_BLOCK(fileOffset), CALC_OFFSET_IN_FILE_BLOCK(fileOffset)+writeSize, DATA_TYPE_REGULAR, tid, log_addr, 1));
		}

		OPEN_FILE_INC_OFFSET(fd, writeSize);
		bytesWritten += writeSize;
		tempCount    -= writeSize;

		/* the inode was commited before the write was completed and marked
		 * in indirect block. therefore commitedBytesWritten is not changed */
		if(TRANSACTION_GET_WAS_COMMITED(tid)){
			TRANSACTION_SET_WAS_COMMITED(tid, 0);
		}
	}
//	PRINT_MSG_AND_NUM("\nafter partial write, obs count=", GET_OBS_COUNT());
//	PRINT("\nabout to reboot");
//	PRINT("\nprint obs map");
//	for(i=0; i<sizeof(obs_pages_per_seg_counters);i ++){
//		PRINT_MSG_AND_NUM("\n", i);
//		PRINT_MSG_AND_HEX(". ", CAST_TO_UINT8(obs_counters_map_ptr)[i]);
//	}
//	obs_pages_per_seg_counters temp_map;
//	fsMemcpy(&temp_map,obs_counters_map_ptr, sizeof(obs_pages_per_seg_counters));
	/* reboot */
	VERIFY(!fsBooting());
//	PRINT("\nafter reboot print obs map");
//	for(i=0; i<sizeof(obs_pages_per_seg_counters);i ++){
//		PRINT_MSG_AND_NUM("\n", i);
//		PRINT_MSG_AND_HEX(". ", CAST_TO_UINT8(obs_counters_map_ptr)[i]);
//		VERIFY(COMPARE(CAST_TO_UINT8(obs_counters_map_ptr)[i], CAST_TO_UINT8(&temp_map)[i]));
//	}

	/* verify returning to stable state (after creat) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverifyied fentries etc...");
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
//	PRINT("\nverified last closed tid");
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
//		PRINT("\nverified empty tid");
	}
//	PRINT("\nverify stable state");

//	PRINT_MSG_AND_NUM("\nexpected frees=", free);
//	PRINT_MSG_AND_NUM(" frees=", GET_FREE_COUNTER());
	VERIFY(COMPARE(free , GET_FREE_COUNTER()));
//	PRINT("\nverified frees");
	VERIFY(COMPARE(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()), rec_addr));
//	PRINT("\nverified rec address");
	VERIFY(COMPARE(getFileSize(2), f_size));
//	PRINT("\nverified f_size");

//	PRINT_MSG_AND_NUM("\nobs=", GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" expected=", obs);
	VERIFY(COMPARE(obs , GET_OBS_COUNT()));
//	PRINT("\nverified obs");
	return 1;
}

/**
 * @brief
 * crash just before commiting transaction at the end of creat.
 * verify returning to stable state (file doesn't exist)
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest9(){
	uint8_t *pathname = "/file1.dat";
	int32_t rec_addr, i;

	/* create fs*/
	VERIFY(!fsBooting());
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* creat until commit transaction*/
	{
		int32_t tid, ino_num, dir_num, res = 0, de_offset;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_log_addr);
		int32_t offset, nameLen;
		uint32_t name_offset, f_type;
		uint8_t *buf;
		inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);

		init_logical_address(log_addr);
		init_logical_address(ino_log_addr);
		init_logical_address(ino0_log_addr);

		/* try getting a free trnasaction */
		tid = getFreeTransactionId();
		FS_VERIFY(IS_TRANSACTION_EMPTY(tid));
		buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
	//	PRINT_MSG_AND_NUM("\ncreat() - tid=",tid);
		/* verify file doesn't already exist - error returned indicates no matching direntry exists*/
		ino_num = namei(pathname, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
	//	PRINT_MSG_AND_NUM("\ncreat() - ino_num=", ino_num);
	//	PRINT_MSG_AND_STR("\ncreat() - after namei pathname=",pathname);
		FS_VERIFY(IS_NEGATIVE(ino_num) && ino_num != ERROR_IO);

	//	PRINT("\ncreat() - file doesn't alreday exist");
		/* verify prefix */
		FS_VERIFY(!verifyPrefix(pathname, TRANSACTION_GET_DATA_BUF_PTR(tid), &dir_num, &name_offset));
		nameLen = calcNameLen(&(pathname[name_offset]));
		FS_VERIFY(verifyNameLen(nameLen));

		init_fsbuf(TRANSACTION_GET_DATA_BUF_PTR(tid));
	//	PRINT_MSG_AND_NUM("\ncreat() - prefix verified. parent file id=(dir_num)", dir_num);
	//	assert(0);
		/* if there is any transaction/open file entry involving dir_num - abort*/
		FS_VERIFY(!verifyFileNotOpen(dir_num));

	//	PRINT("\ncreat() - no open transaction involves parent");
		/* find offset of a sparse block in inode0, and calculate inode number
		 * according to offset*/
		FS_VERIFY(!findEmptySparseBlock(0,
									    log_addr,
									    &offset,
									    tid));
	//	PRINT_MSG_AND_NUM("\ncreat() - found sparse block at offset=", offset);
		ino_num = CALC_IN_INODES(offset);
	//	PRINT_MSG_AND_NUM("\ncreat() - new file id(ino_num)=", ino_num);
	//	assert(0);
		TRANSACTION_SET_TYPE(tid, T_TYPE_WRITE);
		TRANSACTION_SET_INO(tid, ino_num);
		TRANSACTION_SET_FILE_OFFSET(tid, 0);
		TRANSACTION_SET_FILE_SIZE(tid, -1);
		TRANSACTION_SET_FTYPE(tid, FTYPE_FILE);
		init_logical_address(ino_log_addr);
		/* set new inode in tid indirect buffer
		 * and commit the inode to flash*/
		TR_RES_VERIFY(res, setNewInode(ino_num, dir_num, FTYPE_FILE, tid));
	//	PRINT("\ncreat() - setNewInode success");
		TR_RES_VERIFY(res, commitInode(tid));
	//	PRINT("\ncreat() - commitInode success");
	//	PRINT_MSG_AND_NUM("\ncreat() - new inode address=", logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));
		/* save inode address */
		copyLogicalAddress(ino_log_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));

		/* commit inode0 with pointer to newly created file inode*/
		copyLogicalAddress(ino0_log_addr, FS_GET_INO0_ADDR_PTR());
		TR_RES_VERIFY(res, commitInodeToInode0(ino0_log_addr, ino_num, ino_log_addr, tid));
	//	PRINT("\ncreat() - commitInodeToInode0 success");
	//	assert(0);
	//	PRINT_MSG_AND_NUM("\ncreat() - new ino0 address=", logicalAddressToPhysical(ino0_log_addr));

		/* add direntry to parent directory.
		 * - set directory details to transaction
		 * - write directory entry to file block */
		TRANSACTION_SET_INO(tid, dir_num);
		TRANSACTION_SET_FILE_OFFSET(tid,-1);
	//	TRANSACTION_SET_FTYPE(tid, FTYPE_DIR);
		TR_RES_VERIFY(res, getInode(fs_buffer, dir_num, TRANSACTION_GET_INO_ADDR_PTR(tid)));
		TRANSACTION_SET_FILE_SIZE(tid, INODE_GET_NBYTES(ino_ptr));
		TRANSACTION_SET_FTYPE(tid, FTYPE_DIR);

		init_fsbuf(fs_buffer);
	//	init_logical_address(TRANSACTION_GET_INO_ADDR_PTR(tid));
		TR_RES_VERIFY(res, writeDirEntry(ino_num, &(pathname[name_offset]), FTYPE_FILE, tid));
	}

	/* reboot */
	VERIFY(!fsBooting());

	/* verify returning to stable state (after creat) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
	}

	/* verify file doesn't exist*/
	VERIFY(IS_NEGATIVE(open(pathname, NANDFS_O_RDONLY,0)));

	/* verify file system returned to last checkpoint*/
	VERIFY(COMPARE(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()), rec_addr));

	/* verify returning to stable state (after creat) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
//	PRINT("\nverified last closed tid");
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
//		PRINT("\nverified empty tid");
	}

	return 1;
}

//#else
/**
 * @brief
 * crash right after writing new checkpoint at the end of creat.
 * verify returning to stable state (file exists)
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest10(){
	uint8_t *pathname = "/file1.dat";
	int32_t rec_addr, i, fd, isFinal = 1, tid;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_old_addr);
	init_logical_address(ino0_log_addr);
	init_logical_address(ino0_old_addr);

	/* create fs*/
	VERIFY(!fsBooting());
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	/* do creat until commit transaction */
	{
		int32_t new_type = FTYPE_FILE;
		int32_t ino_num, dir_num, res = 0, de_offset;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_log_addr);
		int32_t offset, nameLen;
		uint32_t name_offset, f_type;
		uint8_t *buf;
		inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	//	FENT();

		init_logical_address(prev_log_addr);
		init_logical_address(log_addr);
		init_logical_address(ino_log_addr);
		init_logical_address(dir_log_addr);
		init_logical_address(ino0_log_addr);

	//	PRINT_MSG_AND_STR(" pathanme=", pathname);

		/* try getting a free trnasaction */
		tid = getFreeTransactionId();
	//	L(" tid %d", tid);
	//	PRINT_MSG_AND_HEX("\nino=", TRANSACTION_GET_INO(tid));
		EMPTY_TR_VERIFY(IS_TRANSACTION_EMPTY(tid));
		buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
	//	PRINT_MSG_AND_NUM("\ncreat() - tid=",tid);

		/* verify file doesn't already exist - error returned indicates no matching direntry exists*/
		ino_num = namei(pathname, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
//		L("ino_num %d", ino_num);
	//	PRINT_MSG_AND_STR("\ncreat() - after namei pathname=",pathname);
		EMPTY_TR_VERIFY(IS_NEGATIVE(ino_num) && ino_num != FS_ERROR_IO);

	//	PRINT("\ncreat() - file doesn't alreday exist");

		/* verify prefix */
	//	L("call verifyPrefix()");
		EMPTY_TR_VERIFY(!verifyPrefix(pathname, TRANSACTION_GET_DATA_BUF_PTR(tid), &dir_num, &name_offset));
	//	L("verifyPrefix() success");
		/* verify name length */
		nameLen = calcNameLen(&(pathname[name_offset]));
		EMPTY_TR_VERIFY(verifyNameLen(nameLen));

		init_fsbuf(TRANSACTION_GET_DATA_BUF_PTR(tid));
	//	PRINT_MSG_AND_NUM("\ncreat() - prefix verified. parent file id=(dir_num)", dir_num);

		/* if there is any transaction/open file entry involving dir_num - abort*/
//		L("call verifyFileNotOpen()");
		EMPTY_TR_VERIFY(!verifyFileNotOpen(dir_num));
	//	L("verifyFileNotOpen() success");
	//	PRINT("\ncreat() - no open transaction involves parent");
		/* find offset of a sparse block in inode0, and calculate inode number
		 * according to offset*/
//		L("call findEmptySparseBlock() in inode0");
		EMPTY_TR_VERIFY(!findEmptySparseBlock(0, log_addr, &offset, tid));
//		L("findEmptySparseBlock() inode0 success");
	//	PRINT_MSG_AND_NUM("\ncreat() - found sparse block at offset=", offset);
		ino_num = CALC_IN_INODES(offset);
	//	PRINT_MSG_AND_NUM("\ncreat() - new file id(ino_num)=", ino_num);

		TRANSACTION_SET_TYPE(tid, T_TYPE_WRITE);
		TRANSACTION_SET_INO(tid, ino_num);
		TRANSACTION_SET_FILE_OFFSET(tid, 0);
		TRANSACTION_SET_FILE_SIZE(tid, -1);
		TRANSACTION_SET_FTYPE(tid, new_type);

		init_logical_address(ino_log_addr);
		/* set new inode in tid indirect buffer
		 * and commit the inode to flash*/
//		L("call setNewInode()");
		TR_RES_VERIFY(res, setNewInode(ino_num, dir_num, new_type, tid));
//		L("setNewInode done");
		TR_RES_VERIFY(res, commitInode(tid));
//		L("commitInode success");
	//	L("new inode address %x", ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));

		/* flush all transaction related cached blocks.
		 * inode0 changes are not cached, so this is ok*/
//		L("flush all transaction related cached blocks, call cache_flush_transaction_blocks()");
		RES_VERIFY(res, cache_flush_transaction_blocks(tid));
//		L("NOW new inode address %x", ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));
		/* save inode address */
		copyLogicalAddress(ino_log_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));

		/* commit inode0 with pointer to newly created file inode*/
		copyLogicalAddress(ino0_log_addr, FS_GET_INO0_ADDR_PTR());
//		L("call commitInodeToInode0()");
		TR_RES_VERIFY(res, commitInodeToInode0(ino0_log_addr, ino_num, ino_log_addr, tid));
//		L("commitInodeToInode0 success");
//		L("@@@@@@@@@@@@@@@ new ino0 address=%x", ADDR_PRINT(ino0_log_addr));

		/* add direntry to parent directory.
		 * - set directory details to transaction
		 * - write directory entry to file block */
		TRANSACTION_SET_INO(tid, dir_num);
		TRANSACTION_SET_FILE_OFFSET(tid,-1);
		TR_RES_VERIFY(res, getInode(fs_buffer, dir_num, TRANSACTION_GET_INO_ADDR_PTR(tid)));
		TRANSACTION_SET_FILE_SIZE(tid, INODE_GET_NBYTES(ino_ptr));
		TRANSACTION_SET_FTYPE(tid, FTYPE_DIR);

		init_fsbuf(fs_buffer);
	//	init_logical_address(TRANSACTION_GET_INO_ADDR_PTR(tid));
//		L("call writeDirEntry() writeDirEntry success");
		TR_RES_VERIFY(res, writeDirEntry(ino_num, &(pathname[name_offset]), new_type, tid));
	}
//	L("commit transaction");
	copyLogicalAddress(ino0_old_addr, ino0_log_addr);
//	L("");
	/* execute commit transaction until after writing checkpoint*/
	{
		int32_t res, offset = 0, f_type = 0;
		bool_t cpWritten;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_ino_addr);
		int32_t ino_num = TRANSACTION_GET_INO(tid), t_type = TRANSACTION_GET_TYPE(tid);
		inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
//		L("");
//		FENT();
		init_logical_address(file_ino_addr);
//		L("COMMIT TRANSACTION %d, INO0_OLD_ADDR %x", tid, ADDR_PRINT(ino0_old_addr));

		/* check if we are already commiting an inode, or transaction
		 * used to prevent nested commits in temporary commit*/
		if(TRANSACTION_GET_IS_COMMITING(tid)){
			return FS_ERROR_SUCCESS;
		}
//		L("");
		/* indentify transaction as in the middle of commiting*/
		TRANSACTION_SET_IS_COMMITING(tid, 1);

		/* if temporary commit, mark it in transaction type so we can allocate
		 * regardless of minimum free pages status.
		 * NOTICE - we only perform temporary commit after previously verifying
		 * we will have achieved enough free pages after committing   */
		if(!isFinal){
//			L("starting temp commit");
	//		PRINT_MSG_AND_NUM(", vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//		PRINT_MSG_AND_NUM(", frees=", calcTotalFreePages());
			TRANSACTION_SET_TYPE(tid, T_TYPE_TEMP);
			offset = TRANSACTION_GET_FILE_OFFSET(tid);
			f_type = TRANSACTION_GET_FTYPE(tid);
		}

//		PRINT_MSG_AND_NUM("  tid=", tid);
//		PRINT_MSG_AND_NUM("  ino=", TRANSACTION_GET_INO(tid));
//		PRINT_MSG_AND_NUM("  t_type=", TRANSACTION_GET_TYPE(tid));
//		PRINT_MSG_AND_NUM("  f_type=", TRANSACTION_GET_FTYPE(tid));
//		PRINT_MSG_AND_NUM(", isFinal=", isFinal);
//		PRINT_MSG_AND_NUM(", vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//	PRINT_MSG_AND_HEX("\ncommitTransaction() - first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - commiting for inode=", TRANSACTION_GET_INO(tid));

		/* commit inode */
//		L("do commitInode()");
		TR_RES_VERIFY(res, commitInode(tid));
//		L("@@@@@@@@@@@@@ commitInode() done for ino_num %d. new ino addr %x", TRANSACTION_GET_INO(tid), ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	//	PRINT_MSG_AND_NUM("\nafter commitInode() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//	PRINT_MSG_AND_HEX("\ncommitTransaction() - 1. write page. first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);

		/* flush all transaction related cached blocks.
		 * inode0 changes are not cached, so this is ok*/
//		L("flush all transaction related cached blocks, call cache_flush_transaction_blocks()");
		RES_VERIFY(res, cache_flush_transaction_blocks(tid));
//		L("@@@@@@@@@@@@@@@@@@@@@done flusing. new inode addr is %x", ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	//	{
	//		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	//		SET_LOGICAL_OFFSET(log_addr, 9);
	//		SET_LOGICAL_SEGMENT(log_addr, 0);
	//		uint8_t buf[FS_BLOCK_SIZE];
	//		fsReadBlockSimple(log_addr, buf);
	//		INODE_GET_DIRECT(CAST_TO_INODE(buf), 0, log_addr);
	//		L("firt direct entry of inode is %x", ADDR_PRINT(log_addr));
	//		assert(!IS_CACHED_ADDR(log_addr));
	//	}
		copyLogicalAddress(file_ino_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));
	//	L("commited inode to addr %x",*((uint32_t*)file_ino_addr));

		/* final commit inode0 with pointer to re-written directory inode*/
//		L("final commit of inode0, call commitInodeToInode0(), old ino0 addr is %x", ADDR_PRINT(ino0_old_addr));
		TR_RES_VERIFY(res, commitInodeToInode0(ino0_old_addr, TRANSACTION_GET_INO(tid), file_ino_addr, tid));
//		PRINT_MSG_AND_NUM("\nafter commitInodeToInode0() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//	PRINT("\ncommitTransaction() - commitInodeToInode0 success");
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - new ino0 address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - write vots buffer?=", !IS_VOTS_BUF_EMPTY(tid));

		/* write final vots buffer (if necessary)*/
		if(!IS_VOTS_BUF_EMPTY(tid)){
//			L("write final vots buffer");
			RES_VERIFY(res, allocAndWriteBlockTid(TRANSACTION_GET_PREV_ADDR_PTR(tid),
												  TRANSACTION_GET_VOTS_BUF_PTR(tid),
												  DATA_TYPE_VOTS,
												  CACHE_ENTRY_OFFSET_EMPTY, /* we won't cache this so it doesn't matter*/
												  tid));
//			PRINT_MSG_AND_NUM("\nafter last vots block write vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
//			L("wrote VOTs to %x", ADDR_PRINT(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
		}

		/* set file system:
		 * - new ino0 address
		 * - last written block address of closed transaction
		 * - open transactions last written block addresses*/
//		L("set new inode0 addr %x", ADDR_PRINT(ino0_old_addr));
		FS_SET_INO0_ADDR(ino0_old_addr);
		FS_SET_LAST_CLOSED_TID_ADDR(TRANSACTION_GET_PREV_ADDR_PTR(tid));

	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - after set file system, tid prev=", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - new inode0 address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	//
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - wrote inode 0 to=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - b4 fsCheckpointWriter() frees=",GET_FREE_COUNTER());
	//	PRINT_MSG_AND_NUM(", obs=",GET_OBS_COUNT());
		/* write checkpoint.
		 * we temporarily change obsolete count before writing, for consistency*/
//		L("write checkpoint");
		TR_RES_VERIFY(res, fsCheckpointWriter(0));
	}
//	PRINT("\n\nreboot");

	/* reboot */
	VERIFY(!fsBooting());
//	PRINT("\nreboot success");
	/* verify returning to stable state (after creat) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
	}
//	PRINT("\nverified transaction etc...");

	/* verify file exists*/
	fd = open(pathname, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nopened file successfuly");
//	L("GET FILE SIZE");
	int32_t temp = getFileSize(2);
//	L("file 2 size is %d expected 0");
	VERIFY(COMPARE(temp, 0));
//	PRINT("\nfile size success");
	/* verify file system returned to last checkpoint, not the one before creat*/
	VERIFY(!COMPARE(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()), rec_addr));

	return 1;
}

/**
 * @brief
 * crash during unlink.
 * verify returning to stable state (file exists)
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest11(){
	uint8_t *pathname = "/file1.dat";
	int32_t rec_addr, i, fd, free_count;
	int32_t new_type = FTYPE_FILE;
	/* create fs*/
	VERIFY(!fsBooting());

	/* create file */
	fd = creat(pathname, 0);
	VERIFY(!IS_NEGATIVE(fd));
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	free_count = calcTotalFreePages();

	/* execute unlink() without committing transaction */
	{
		int32_t res, ino_num, tid, dir_num, de_offset, f_type;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_log_addr);
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_log_addr);
		uint8_t *buf;
		inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
		bool_t isEmpty = 0;
		uint32_t name_offset;
	//	FENT();

		init_logical_address(ino_log_addr);
		init_logical_address(dir_log_addr);
		init_logical_address(ino0_log_addr);

		/* verify file exists, and is not directorys*/
		ino_num = namei(pathname, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
	//	PRINT_MSG_AND_NUM("\nremoveFile() - ino_num=", ino_num);
	//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
		FS_VERIFY(!IS_NEGATIVE(ino_num));
	//	PRINT_MSG_AND_NUM("\nremoveFile() - f_type=", f_type);
		FS_VERIFY(f_type == new_type);
	//	PRINT_MSG_AND_NUM("\nremoveFile() - ino_num=", ino_num);
	//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
	//	PRINT_MSG_AND_NUM("\n********** cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));

		/* try getting a free trnasaction */
		tid = getFreeTransactionId();
	//	PRINT_MSG_AND_NUM("\nremoveFile() - tid=", tid);
		EMPTY_TR_VERIFY(IS_TRANSACTION_EMPTY(tid));
		buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
	//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));

		/* verify file prefix*/
	//	PRINT("\nremoveFile() - verifyPrefix ");
		EMPTY_TR_VERIFY(!verifyPrefix(pathname, buf, &dir_num, &name_offset));
	//	PRINT_MSG_AND_NUM("\nremoveFile() - after verifyPrefix dir_num=", dir_num);

	//	PRINT_MSG_AND_NUM("\nremoveFile() - IS_DIRECTORY(new_type)=", IS_DIRECTORY(new_type));
		if(IS_DIRECTORY(new_type)){
	//		L("check not erasing .. or .");
			/* verify "." or ".." are not the pathname suffix*/
			EMPTY_TR_VERIFY(fsStrcmp("..", &(pathname[name_offset])));
			EMPTY_TR_VERIFY(fsStrcmp(".", &(pathname[name_offset])));

			/* verify not erasing root directory */
	//		PRINT("\nremoveFile() - directory root?");
			EMPTY_TR_VERIFY(ino_num != 1);
	//		PRINT("\nremoveFile() - directory not root");

			/* verify directory is empty before deleting it*/
			EMPTY_TR_VERIFY(!verifyDirectoryEmpty(ino_num, &isEmpty));
	//		PRINT_MSG_AND_NUM("\nremoveFile() - verifyDirectoryEmpty res=", isEmpty);
			EMPTY_TR_VERIFY(isEmpty);
		}

		/* verify file and parent directory are not open*/
		EMPTY_TR_VERIFY(!verifyFileNotOpen(dir_num));
	//	PRINT("\nremoveFile() - parent not open");
		EMPTY_TR_VERIFY(!verifyFileNotOpen(ino_num));
	//	PRINT("\nremoveFile() - file and parent dir not open");

		TRANSACTION_SET_TYPE(tid, T_TYPE_UNLINK);
		TRANSACTION_SET_INO(tid, ino_num);
		TRANSACTION_SET_FTYPE(tid, new_type);

		/* vot all file pages*/
		TR_RES_VERIFY(res, votFile(tid));
	//	PRINT("\nremoveFile() - voted file succesfuly");
		/* commit empty inode address to inode0*/
		copyLogicalAddress(ino0_log_addr, FS_GET_INO0_ADDR_PTR());
		TR_RES_VERIFY(res, commitInodeToInode0(ino0_log_addr, ino_num, ino_log_addr, tid));
	//	L("commitInodeToInode0() success");

		/* delete direntry*/
		/* add direntry to parent directory.
		 * - set directory details to transaction (dir_num, dir address, file type, no file offset)
		 * - write directory entry to file block */
		TRANSACTION_SET_INO(tid, dir_num);
		TRANSACTION_SET_FILE_OFFSET(tid,-1);
		TR_RES_VERIFY(res, getInode(fs_buffer, dir_num, TRANSACTION_GET_INO_ADDR_PTR(tid)));
		TRANSACTION_SET_FILE_SIZE(tid, INODE_GET_NBYTES(ino_ptr));
		TRANSACTION_SET_FTYPE(tid, FTYPE_DIR);
		init_fsbuf(fs_buffer);

	//	L("about to deleteDirEntry()");
		TR_RES_VERIFY(res, deleteDirEntry(ino_num, de_offset, tid));
	}

	/* reboot */
	VERIFY(!fsBooting());

	/* verify returning to stable state (after creat) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
	}

	/* verify file doesn't exist*/
	fd = open(pathname, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd));

	/* verify file system returned to last checkpoint*/
	VERIFY(COMPARE(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()), rec_addr));

	/* verify returning to stable state (after creat) */
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
//	PRINT("\nverified last closed tid");
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
//		PRINT("\nverified empty tid");
	}

	VERIFY(COMPARE(free_count, calcTotalFreePages()));

	return 1;
}

/**
 * @brief
 * crash during unlink, right after writing new checkpoint.
 * verify returning to stable state (file exists)
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest12(){
	uint8_t *pathname = "/file1.dat";
	int32_t rec_addr, i, fd, isFinal = 1, tid, expected_obs;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_old_addr);
	int32_t new_type = FTYPE_FILE;

	init_logical_address(ino0_log_addr);
	init_logical_address(ino0_old_addr);
	tid = getFreeTransactionId();
	/* create fs*/
	VERIFY(!fsBooting());

	/* create file */
	fd = creat(pathname, 0);
	VERIFY(!IS_NEGATIVE(fd));

	/* execute unlink() without committing transaction */
	{
		int32_t res, ino_num, tid, dir_num, de_offset, f_type;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_log_addr);
		uint8_t *buf;
		inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
		bool_t isEmpty = 0;
		uint32_t name_offset;
	//	FENT();

		init_logical_address(ino_log_addr);
		init_logical_address(dir_log_addr);
		init_logical_address(ino0_log_addr);

		/* verify file exists, and is not directorys*/
		ino_num = namei(pathname, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
	//	PRINT_MSG_AND_NUM("\nremoveFile() - ino_num=", ino_num);
	//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
		FS_VERIFY(!IS_NEGATIVE(ino_num));
	//	PRINT_MSG_AND_NUM("\nremoveFile() - f_type=", f_type);
		FS_VERIFY(f_type == new_type);
	//	PRINT_MSG_AND_NUM("\nremoveFile() - ino_num=", ino_num);
	//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
	//	PRINT_MSG_AND_NUM("\n********** cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));

		/* try getting a free trnasaction */
		tid = getFreeTransactionId();
	//	PRINT_MSG_AND_NUM("\nremoveFile() - tid=", tid);
		EMPTY_TR_VERIFY(IS_TRANSACTION_EMPTY(tid));
		buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
	//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));

		/* verify file prefix*/
	//	PRINT("\nremoveFile() - verifyPrefix ");
		EMPTY_TR_VERIFY(!verifyPrefix(pathname, buf, &dir_num, &name_offset));
	//	PRINT_MSG_AND_NUM("\nremoveFile() - after verifyPrefix dir_num=", dir_num);

	//	PRINT_MSG_AND_NUM("\nremoveFile() - IS_DIRECTORY(new_type)=", IS_DIRECTORY(new_type));
		if(IS_DIRECTORY(new_type)){
	//		L("check not erasing .. or .");
			/* verify "." or ".." are not the pathname suffix*/
			EMPTY_TR_VERIFY(fsStrcmp("..", &(pathname[name_offset])));
			EMPTY_TR_VERIFY(fsStrcmp(".", &(pathname[name_offset])));

			/* verify not erasing root directory */
	//		PRINT("\nremoveFile() - directory root?");
			EMPTY_TR_VERIFY(ino_num != 1);
	//		PRINT("\nremoveFile() - directory not root");

			/* verify directory is empty before deleting it*/
			EMPTY_TR_VERIFY(!verifyDirectoryEmpty(ino_num, &isEmpty));
	//		PRINT_MSG_AND_NUM("\nremoveFile() - verifyDirectoryEmpty res=", isEmpty);
			EMPTY_TR_VERIFY(isEmpty);
		}

		/* verify file and parent directory are not open*/
		EMPTY_TR_VERIFY(!verifyFileNotOpen(dir_num));
	//	PRINT("\nremoveFile() - parent not open");
		EMPTY_TR_VERIFY(!verifyFileNotOpen(ino_num));
	//	PRINT("\nremoveFile() - file and parent dir not open");

		TRANSACTION_SET_TYPE(tid, T_TYPE_UNLINK);
		TRANSACTION_SET_INO(tid, ino_num);
		TRANSACTION_SET_FTYPE(tid, new_type);

		/* vot all file pages*/
		TR_RES_VERIFY(res, votFile(tid));
	//	PRINT("\nremoveFile() - voted file succesfuly");
		/* commit empty inode address to inode0*/
		copyLogicalAddress(ino0_log_addr, FS_GET_INO0_ADDR_PTR());
		TR_RES_VERIFY(res, commitInodeToInode0(ino0_log_addr, ino_num, ino_log_addr, tid));
	//	L("commitInodeToInode0() success");

		/* delete direntry*/
		/* add direntry to parent directory.
		 * - set directory details to transaction (dir_num, dir address, file type, no file offset)
		 * - write directory entry to file block */
		TRANSACTION_SET_INO(tid, dir_num);
		TRANSACTION_SET_FILE_OFFSET(tid,-1);
		TR_RES_VERIFY(res, getInode(fs_buffer, dir_num, TRANSACTION_GET_INO_ADDR_PTR(tid)));
		TRANSACTION_SET_FILE_SIZE(tid, INODE_GET_NBYTES(ino_ptr));
		TRANSACTION_SET_FTYPE(tid, FTYPE_DIR);
		init_fsbuf(fs_buffer);

	//	L("about to deleteDirEntry()");
		TR_RES_VERIFY(res, deleteDirEntry(ino_num, de_offset, tid));
	}

	copyLogicalAddress(ino0_old_addr, ino0_log_addr);
	/* execute commit transaction until after writing checkpoint*/
	{
		int32_t res, offset = 0, f_type = 0;
		bool_t cpWritten;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_ino_addr);
		int32_t ino_num = TRANSACTION_GET_INO(tid), t_type = TRANSACTION_GET_TYPE(tid);
		inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
		FENT();
		init_logical_address(file_ino_addr);
		FENT();
		L("COMMIT TRANSACTION %d, INO0_OLD_ADDR %x", tid, ADDR_PRINT(ino0_old_addr));
		/* check if we are already commiting an inode, or transaction
		 * used to prevent nested commits in temporary commit*/
		if(TRANSACTION_GET_IS_COMMITING(tid)){
			return FS_ERROR_SUCCESS;
		}

		/* indentify transaction as in the middle of commiting*/
		TRANSACTION_SET_IS_COMMITING(tid, 1);

		/* if temporary commit, mark it in transaction type so we can allocate
		 * regardless of minimum free pages status.
		 * NOTICE - we only perform temporary commit after previously verifying
		 * we will have achieved enough free pages after committing   */
		if(!isFinal){
	//		PRINT("\ncommitTransaction() - starting temp commit");
	//		PRINT_MSG_AND_NUM(", vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//		PRINT_MSG_AND_NUM(", frees=", calcTotalFreePages());
			TRANSACTION_SET_TYPE(tid, T_TYPE_TEMP);
			offset = TRANSACTION_GET_FILE_OFFSET(tid);
			f_type = TRANSACTION_GET_FTYPE(tid);
		}

		PRINT_MSG_AND_NUM("  tid=", tid);
		PRINT_MSG_AND_NUM("  ino=", TRANSACTION_GET_INO(tid));
		PRINT_MSG_AND_NUM("  t_type=", TRANSACTION_GET_TYPE(tid));
		PRINT_MSG_AND_NUM("  f_type=", TRANSACTION_GET_FTYPE(tid));
		PRINT_MSG_AND_NUM(", isFinal=", isFinal);
		PRINT_MSG_AND_NUM(", vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//	PRINT_MSG_AND_HEX("\ncommitTransaction() - first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - commiting for inode=", TRANSACTION_GET_INO(tid));

		/* commit inode */
		L("do commitInode()");
		TR_RES_VERIFY(res, commitInode(tid));
		L("commitInode() done");
	//	PRINT_MSG_AND_NUM("\nafter commitInode() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//	PRINT_MSG_AND_HEX("\ncommitTransaction() - 1. write page. first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);

		/* flush all transaction related cached blocks.
		 * inode0 changes are not cached, so this is ok*/
		L("flush all transaction related cached blocks, call cache_flush_transaction_blocks()");
		RES_VERIFY(res, cache_flush_transaction_blocks(tid));
		L("done flusing");
	//	{
	//		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	//		SET_LOGICAL_OFFSET(log_addr, 9);
	//		SET_LOGICAL_SEGMENT(log_addr, 0);
	//		uint8_t buf[FS_BLOCK_SIZE];
	//		fsReadBlockSimple(log_addr, buf);
	//		INODE_GET_DIRECT(CAST_TO_INODE(buf), 0, log_addr);
	//		L("firt direct entry of inode is %x", ADDR_PRINT(log_addr));
	//		assert(!IS_CACHED_ADDR(log_addr));
	//	}
		copyLogicalAddress(file_ino_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));
	//	L("commited inode to addr %x",*((uint32_t*)file_ino_addr));

		/* final commit inode0 with pointer to re-written directory inode*/
		L("final commit of inode0, call commitInodeToInode0()");
		TR_RES_VERIFY(res, commitInodeToInode0(ino0_old_addr, TRANSACTION_GET_INO(tid), file_ino_addr, tid));
		PRINT_MSG_AND_NUM("\nafter commitInodeToInode0() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//	PRINT("\ncommitTransaction() - commitInodeToInode0 success");
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - new ino0 address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - write vots buffer?=", !IS_VOTS_BUF_EMPTY(tid));

		/* write final vots buffer (if necessary)*/
		if(!IS_VOTS_BUF_EMPTY(tid)){
			L("write final vots buffer");
			RES_VERIFY(res, allocAndWriteBlockTid(TRANSACTION_GET_PREV_ADDR_PTR(tid),
												  TRANSACTION_GET_VOTS_BUF_PTR(tid),
												  DATA_TYPE_VOTS,
												  CACHE_ENTRY_OFFSET_EMPTY, /* we won't cache this so it doesn't matter*/
												  tid));
			PRINT_MSG_AND_NUM("\nafter last vots block write vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//		L("wrote VOTs to %x", ADDR_PRINT(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
		}

		/* set file system:
		 * - new ino0 address
		 * - last written block address of closed transaction
		 * - open transactions last written block addresses*/
		FS_SET_INO0_ADDR(ino0_old_addr);
		FS_SET_LAST_CLOSED_TID_ADDR(TRANSACTION_GET_PREV_ADDR_PTR(tid));

	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - after set file system, tid prev=", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - new inode0 address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	//
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - wrote inode 0 to=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - b4 fsCheckpointWriter() frees=",GET_FREE_COUNTER());
	//	PRINT_MSG_AND_NUM(", obs=",GET_OBS_COUNT());
		/* write checkpoint.
		 * we temporarily change obsolete count before writing, for consistency*/
		L("write checkpoint");
		TR_RES_VERIFY(res, fsCheckpointWriter(0));

	}
	expected_obs = GET_OBS_COUNT() +TRANSACTION_GET_VOTS_COUNT(0);
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* reboot */
	VERIFY(!fsBooting());

	/* verify returning to stable state (after creat) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
	}

	/* verify file doesn't exist*/
	VERIFY(IS_NEGATIVE(open(pathname, NANDFS_O_RDONLY,0)));

	/* verify returning to stable state (after creat) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
//	PRINT("\nverified last closed tid");
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
//		PRINT("\nverified empty tid");
	}

	VERIFY(COMPARE(rec_addr, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	VERIFY(COMPARE(expected_obs, GET_OBS_COUNT()));

	return 1;
}

/**
 * @brief
 * write to file and close. crash in the middle of handling transaction vots.
 * verify transaction completes upon reboot.
 *
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest13(){
	uint8_t *pathname = "/file1.dat", buf[FS_BLOCK_SIZE], byte= 'a';
	int32_t rec_addr, i, j, fd, isFinal = 1, expected_obs, tid;
	int32_t initial_writes = 2000;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_old_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);

	init_logical_address(ino0_old_addr);
	init_logical_address(prev_log_addr);
	tid = getFreeTransactionId();

	/* create fs*/
	VERIFY(!fsBooting());

	/* reach reclamation mode be re-creating the same file over and over again*/
	while(!IS_STATE_RECLAMATION()){
		/* create file */
		fd = creat(pathname, 0);
		VERIFY(!IS_NEGATIVE(fd));

		/* write inode file data*/
		VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, buf, INODE_FILE_DATA_SIZE)));
		for(i=0; i<1024; i++){
			VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
		}
		/* close and unlink file*/
		VERIFY(!close(fd));
		VERIFY(!unlink(pathname));
	}

	PRINT("\ngot to reclamation state");
	/* re-create file */
	fd = creat(pathname, 0);
	VERIFY(!IS_NEGATIVE(fd));

	/* now perform the actual test */
	/* write inode file data*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, buf, INODE_FILE_DATA_SIZE)));

	/* write more blocks */
	for(j=0; j< initial_writes; j++){
		for(i=0; i < FS_BLOCK_SIZE;i++){
			buf[i] = byte + (OPEN_FILE_GET_OFFSET(fd) % 20);
		}

		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	}

	/* lseek file backwards*/
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, lseek(fd, INODE_FILE_DATA_SIZE, FS_SEEK_SET)));

	/* creat VOTs by re-writing blocks */
	for(j=0; j< initial_writes; j++){
		for(i=0; i < FS_BLOCK_SIZE;i++){
			buf[i] = byte + ((OPEN_FILE_GET_OFFSET(fd)+i) % 20);
		}

//		if(j==0){
//			printBlock(buf);
//			PRINT_MSG_AND_NUM("\nfile offset=",OPEN_FILE_GET_OFFSET(fd));
//		}
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	}

	/* close file, until vots */

	/* commit transaction */
	copyLogicalAddress(ino0_old_addr, FS_GET_INO0_ADDR_PTR());
	{
		int32_t res, offset, f_type;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_ino_addr);

		init_logical_address(file_ino_addr);

		/* if temporary commit, transform transaction (temporarily) to an unlink transaction
		 * so it can allocate regardless of min free pages status */
		if(!isFinal){
			TRANSACTION_SET_TYPE(tid, T_TYPE_UNLINK);
			offset = TRANSACTION_GET_FILE_OFFSET(tid);
			f_type = TRANSACTION_GET_FTYPE(tid);
		}

		PRINT("\n\n\n\ncommitTransaction() - starting");
		PRINT_MSG_AND_NUM(", isFinal=", isFinal);
		PRINT_MSG_AND_NUM(", vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//	PRINT_MSG_AND_HEX("\ncommitTransaction() - first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);

	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - commiting for inode=", TRANSACTION_GET_INO(tid));
	//
		/* commit inode onlt in a write transaction. in unlink, new inode is simply empty */
		PRINT("\ncommitTransaction() - do commitInode()");

		/* commit inode, if any writes were performed since last inode commit/transaction beginning */
		TR_RES_VERIFY(res, commitInode(tid));
		PRINT_MSG_AND_NUM("\nafter commitInode() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
		copyLogicalAddress(file_ino_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));

		PRINT_MSG_AND_NUM("\ncommitTransaction() - commited inode to addr=",logicalAddressToPhysical(file_ino_addr));
		PRINT_MSG_AND_HEX("\ncommitTransaction() - 1. write page. first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);
		/* final commit inode0 with pointer to re-written directory inode*/
		TR_RES_VERIFY(res, commitInodeToInode0(ino0_old_addr, TRANSACTION_GET_INO(tid), file_ino_addr, tid));

//		PRINT_MSG_AND_NUM("\nafter commitInodeToInode0() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
		PRINT("\ncommitTransaction() - commitInodeToInode0 success");
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - new ino0 address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));

	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - write vots buffer?=", !IS_VOTS_BUF_EMPTY(tid));
		/* write final vots buffer (if necessary)*/
		if(!IS_VOTS_BUF_EMPTY(tid)){
			RES_VERIFY(res, allocAndWriteBlockTid(TRANSACTION_GET_PREV_ADDR_PTR(tid),
												  TRANSACTION_GET_VOTS_BUF_PTR(tid),
												  DATA_TYPE_VOTS,
												  CACHE_ENTRY_OFFSET_EMPTY,
												  tid));
	//		PRINT_MSG_AND_NUM("\nafter last vots block write vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
			PRINT_MSG_AND_NUM("\ncommitTransaction() - wrote VOTs to ", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
		}

		/* set file system:
		 * - new ino0 address
		 * - last written block address of closed transaction
		 * - open transactions last written block addresses*/
		FS_SET_INO0_ADDR(ino0_old_addr);
		FS_SET_LAST_CLOSED_TID_ADDR(TRANSACTION_GET_PREV_ADDR_PTR(tid));

		PRINT_MSG_AND_NUM("\ncommitTransaction() - after set file system, tid prev=", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - new inode0 address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));

	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - wrote inode 0 to=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - b4 fsCheckpointWriter() frees=",GET_FREE_COUNTER());
	//	PRINT_MSG_AND_NUM(", obs=",GET_OBS_COUNT());

		/* write checkpoint.
		 * we temporarily change obsolete count before writing, for consistency*/
	//	MARK_OBS_COUNT_TEMPORARY();
		PRINT_MSG_AND_NUM("\ncommitTransaction() - before handleTransactionVOTs() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//	PRINT_MSG_AND_NUM(" obs count=", GET_OBS_COUNT());
//		PRINT_MSG_AND_NUM("\n writing cp tid vots=",TRANSACTION_GET_VOTS_COUNT(tid));
//		PRINT_MSG_AND_NUM(" total obs=", GET_OBS_COUNT());
		TR_RES_VERIFY(res, fsCheckpointWriter(0));
	}

	/* expected obs count after reboot = count before reboot +1 vots page + vots count*/
	PRINT_MSG_AND_NUM("\n b4 reboot tid vots=",TRANSACTION_GET_VOTS_COUNT(tid));
	PRINT_MSG_AND_NUM(" total obs=", GET_OBS_COUNT());
	expected_obs = GET_OBS_COUNT()+ CALCULATE_IN_PAGES(TRANSACTION_GET_VOTS_COUNT(tid)*4) +TRANSACTION_GET_VOTS_COUNT(tid)-1;
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	copyLogicalAddress(prev_log_addr, TRANSACTION_GET_PREV_ADDR_PTR(tid));
	i=0;
	/* execute handleTransactionVOTs, but only partially */
	{
		bool_t isVOTs;
		int32_t res;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

		init_logical_address(log_addr);

		PRINT("\nhandleTransactionVOTs() - starting");
		while(!IS_ADDR_EMPTY(prev_log_addr)){
			copyLogicalAddress(log_addr, prev_log_addr);
			/* read previous*/
			PRINT_MSG_AND_NUM("\nhandleTransactionVOTs() - log addr seg=", GET_LOGICAL_SEGMENT(log_addr));
			PRINT_MSG_AND_NUM(", offset=", GET_LOGICAL_OFFSET(log_addr));
	//		PRINT_MSG_AND_NUM("\nhandleTransactionVOTs() - read from=", logicalAddressToPhysical(log_addr));
			FS_VERIFY(!readVOTsAndPrev(log_addr, fs_buffer, prev_log_addr, &isVOTs));
	//		PRINT_MSG_AND_NUM("\nhandleTransactionVOTs() - is it VOTs?=", isVOTs);

			/* if is vots delete all, otherwise continue...*/
			if(!isVOTs){
				continue;
			}

			/* vot all */
			RES_VERIFY(res, performVOTs(fs_buffer, FLAG_FROM_REBOOT_NO));
	//		PRINT_MSG_AND_NUM("\nhandleTransactionVOTs() - make obsolete vots page=", logicalAddressToPhysical(log_addr));
			FS_VERIFY(!markAsObsolete(log_addr, FLAG_FROM_REBOOT_NO));

			/* break  */
			break;
		}
	}
//	PRINT_MSG_AND_NUM("\n after partial handling of vots obs=",GET_OBS_COUNT());
#ifdef Profiling
	init_acces_acounters();
#endif
	/* reboot */
	PRINT("\nPERFORM BOOTING");
	VERIFY(!fsBooting());
	PRINT_MSG_AND_NUM("\n after reboot obs=",GET_OBS_COUNT());
#ifdef Profiling
	return 1;
#endif

	/* verify file doesn't exist*/
	fd = open(pathname, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd));

	/* verify file system returned to last checkpoint (after unlink)*/
	VERIFY(COMPARE(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()), rec_addr));

	/* verify returning to stable state (after creat) */
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
	PRINT("\nverified last closed tid");
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
		PRINT("\nverified empty tid");
	}
	PRINT("\nabout to verify obs. ");
	PRINT_MSG_AND_NUM(", expected_obs=",expected_obs);
	PRINT_MSG_AND_NUM(", actual obs=",GET_OBS_COUNT());
	VERIFY(COMPARE(expected_obs, GET_OBS_COUNT()));
	PRINT("\nverified obs");

	/* verify file data*/
	VERIFY(COMPARE(read(fd, buf, INODE_FILE_DATA_SIZE), INODE_FILE_DATA_SIZE));
	for(i=0; i< INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(byte, buf[i]));
	}
	PRINT("\nfirst read success");

	for(j=0; j< initial_writes; j++){
		VERIFY(COMPARE(read(fd, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));

//		if(j==0){
//			printBlock(buf);
//			PRINT_MSG_AND_NUM("\nfile offset=",OPEN_FILE_GET_OFFSET(fd));
//		}

		for(i=0; i < FS_BLOCK_SIZE; i++){
//			PRINT_MSG_AND_HEX("\nexpected byte=", byte +((OPEN_FILE_GET_OFFSET(fd) +i) %20));
//			PRINT_MSG_AND_HEX(" actual byte=", buf[i]);
//			VERIFY(COMPARE(buf[i], byte + ((OPEN_FILE_GET_OFFSET(fd) +i) %20)));
//			PRINT_MSG_AND_HEX("\nexpected byte=", byte + ((OPEN_FILE_GET_OFFSET(fd)+i-FS_BLOCK_SIZE) % 20));
//			PRINT_MSG_AND_HEX(" actual byte=", buf[i]);
			VERIFY(COMPARE(buf[i], byte + ((OPEN_FILE_GET_OFFSET(fd)+i-FS_BLOCK_SIZE) % 20)));
		}
//		PRINT_MSG_AND_NUM("\nsuccess read page ", j);
	}

	return 1;
}

/**
 * @brief
 * unlink file and close. crash in the middle of handling transaction vots.
 * verify transaction completes upon reboot (file doesn't exist)
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest14(){
	uint8_t *pathname = "/file1.dat", buf[FS_BLOCK_SIZE];
	int32_t rec_addr, i, fd, isFinal = 1, tid, expected_obs;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_old_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);

	init_logical_address(ino0_log_addr);
	init_logical_address(ino0_old_addr);
	init_logical_address(prev_log_addr);
	tid = getFreeTransactionId();

	/* create fs*/
	VERIFY(!fsBooting());

	/* create file */
	fd = creat(pathname, 0);
	VERIFY(!IS_NEGATIVE(fd));

	/* write to file */
	fsMemset(buf, 'a', FS_BLOCK_SIZE);

	for(i=0; i < 20 ; i++){
		VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}

	/* close */
	VERIFY(!close(fd));

	/* execute unlink() without committing transaction */
	{
		int32_t res, f_type, ino_num, dir_num, name_offset, de_offset;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
		uint8_t *buf;
		inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);

		init_logical_address(ino_log_addr);

	//	PRINT_MSG_AND_STR("\nunlink() - starting. unlink ", pathname);

		/* verify file exists, and is not directorys*/
		ino_num = namei(pathname, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
	//	PRINT_MSG_AND_NUM("\nunlink() - ino_num=", ino_num);
		FS_VERIFY(!IS_NEGATIVE(ino_num));
	//	PRINT_MSG_AND_NUM("\nunlink() - f_type=", f_type);
		FS_VERIFY(f_type == FTYPE_FILE);
	//	PRINT_MSG_AND_NUM("\nunlink() - ino_num=", ino_num);

		/* try getting a free trnasaction */
		tid = getFreeTransactionId();
		FS_VERIFY(IS_TRANSACTION_EMPTY(tid));
		buf = TRANSACTION_GET_DATA_BUF_PTR(tid);

		FS_VERIFY(!verifyPrefix(pathname, buf, &dir_num, &name_offset));
	//	PRINT_MSG_AND_NUM("\nunlink() - dir_num=", dir_num);

		/* verify file and parent directory are not open*/
		FS_VERIFY(!verifyFileNotOpen(dir_num));
		FS_VERIFY(!verifyFileNotOpen(ino_num));
	//	PRINT("\nunlink() - file and parent dir not open");

		TRANSACTION_SET_TYPE(tid, T_TYPE_UNLINK);
		TRANSACTION_SET_INO(tid, ino_num);
		TRANSACTION_SET_FTYPE(tid, FTYPE_FILE);

		/* vot all file pages*/
		TR_RES_VERIFY(res, votFile(tid));
	//	PRINT("\nunlink() - voted file succesfuly");
		/* commit empty inode address to inode0*/
		copyLogicalAddress(ino0_log_addr, FS_GET_INO0_ADDR_PTR());
		TR_RES_VERIFY(res, commitInodeToInode0(ino0_log_addr, ino_num, ino_log_addr, tid));
	//	PRINT("\nunlink() - commitInodeToInode0() success");
		/* delete direntry*/
		/* add direntry to parent directory.
		 * - set directory details to transaction (dir_num, dir address, file type, no file offset)
		 * - write directory entry to file block */
		TRANSACTION_SET_INO(tid, dir_num);
		TRANSACTION_SET_FILE_OFFSET(tid,-1);
		TR_RES_VERIFY(res, getInode(fs_buffer, dir_num, TRANSACTION_GET_INO_ADDR_PTR(tid)));
		TRANSACTION_SET_FILE_SIZE(tid, INODE_GET_NBYTES(ino_ptr));
		TRANSACTION_SET_FTYPE(tid, FTYPE_DIR);
		init_fsbuf(fs_buffer);

	//	PRINT("\nunlink() - about to deleteDirEntry()");
		TR_RES_VERIFY(res, deleteDirEntry(ino_num, de_offset, tid));
	}

	copyLogicalAddress(ino0_old_addr, ino0_log_addr);
	/* execute commit transaction until after writing checkpoint*/
	{
		int32_t res, offset, f_type;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_ino_addr);
	//	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);

		/* if temporary commit, transform transaction (temporarily) to an unlink transaction
		 * so it can allocate regardless of min free pages status */
		if(!isFinal){
			TRANSACTION_SET_TYPE(tid, T_TYPE_UNLINK);
			offset = TRANSACTION_GET_FILE_OFFSET(tid);
			f_type = TRANSACTION_GET_FTYPE(tid);
		}

	//	PRINT("\n\n\n\ncommitTransaction() - starting");
	//	PRINT_MSG_AND_NUM(", isFinal=", isFinal);
	//	PRINT_MSG_AND_NUM(", vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//	PRINT_MSG_AND_HEX("\ncommitTransaction() - first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);

	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - commiting for inode=", TRANSACTION_GET_INO(tid));
	//
		/* commit inode onlt in a write transaction. in unlink, new inode is simply empty */
	//	PRINT("\ncommitTransaction() - do commitInode()");
		/* commit inode, if any writes were performed since last inode commit/transaction beginning */
		TR_RES_VERIFY(res, commitInode(tid));
	//	PRINT_MSG_AND_NUM("\nafter commitInode() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
		copyLogicalAddress(file_ino_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));

	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - commited inode to addr=",logicalAddressToPhysical(file_ino_addr));
	//	PRINT_MSG_AND_HEX("\ncommitTransaction() - 1. write page. first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);
		/* final commit inode0 with pointer to re-written directory inode*/
		TR_RES_VERIFY(res, commitInodeToInode0(ino0_old_addr, TRANSACTION_GET_INO(tid), file_ino_addr, tid));

	//	PRINT_MSG_AND_NUM("\nafter commitInodeToInode0() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//	PRINT("\ncommitTransaction() - commitInodeToInode0 success");
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - new ino0 address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));

	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - write vots buffer?=", !IS_VOTS_BUF_EMPTY(tid));
		/* write final vots buffer (if necessary)*/
		if(!IS_VOTS_BUF_EMPTY(tid)){
			RES_VERIFY(res, allocAndWriteBlockTid(TRANSACTION_GET_PREV_ADDR_PTR(tid),
												  TRANSACTION_GET_VOTS_BUF_PTR(tid),
												  DATA_TYPE_VOTS,
												  CACHE_ENTRY_OFFSET_EMPTY,
												  tid));
	//		PRINT_MSG_AND_NUM("\nafter last vots block write vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
	//		PRINT_MSG_AND_NUM("\ncommitTransaction() - wrote VOTs to ", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
		}

		/* set file system:
		 * - new ino0 address
		 * - last written block address of closed transaction
		 * - open transactions last written block addresses*/
		FS_SET_INO0_ADDR(ino0_old_addr);
		FS_SET_LAST_CLOSED_TID_ADDR(TRANSACTION_GET_PREV_ADDR_PTR(tid));

	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - after set file system, tid prev=", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - new inode0 address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));

	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - wrote inode 0 to=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	//	PRINT_MSG_AND_NUM("\ncommitTransaction() - b4 fsCheckpointWriter() frees=",GET_FREE_COUNTER());
	//	PRINT_MSG_AND_NUM(", obs=",GET_OBS_COUNT());

		/* write checkpoint.
		 * we temporarily change obsolete count before writing, for consistency*/
	//	MARK_OBS_COUNT_TEMPORARY();
//		PRINT_MSG_AND_NUM("\ncommitTransaction() - before handleTransactionVOTs() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
//		PRINT_MSG_AND_NUM(" obs count=", GET_OBS_COUNT());
		TR_RES_VERIFY(res, fsCheckpointWriter(0));
	}

	/* expected obs = current obs count + vots + 1 vots page*/
	expected_obs = GET_OBS_COUNT() +TRANSACTION_GET_VOTS_COUNT(tid)+1;
	rec_addr     = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	copyLogicalAddress(prev_log_addr, TRANSACTION_GET_PREV_ADDR_PTR(tid));

	i=0;
	/* execute handleTransactionVOTs, but only partially */
	{
		bool_t isVOTs;
		int32_t res;
		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	//	PRINT("\nhandleTransactionVOTs() - starting");
		while(!IS_ADDR_EMPTY(prev_log_addr)){
			copyLogicalAddress(log_addr, prev_log_addr);
			/* read previous*/
	//		PRINT_MSG_AND_NUM("\nhandleTransactionVOTs() - log addr seg=", GET_LOGICAL_SEGMENT(log_addr));
	//		PRINT_MSG_AND_NUM(", offset=", GET_LOGICAL_OFFSET(log_addr));
	//		PRINT_MSG_AND_NUM("\nhandleTransactionVOTs() - read from=", logicalAddressToPhysical(log_addr));
			FS_VERIFY(!readVOTsAndPrev(log_addr, fs_buffer, prev_log_addr, &isVOTs));
	//		PRINT_MSG_AND_NUM("\nhandleTransactionVOTs() - is it VOTs?=", isVOTs);

			/* if is vots delete all, otherwise continue...*/
			if(!isVOTs){
				continue;
			}

			/* vot all */
			RES_VERIFY(res, performVOTs(fs_buffer, FLAG_FROM_REBOOT_NO));
	//		PRINT_MSG_AND_NUM("\nhandleTransactionVOTs() - make obsolete vots page=", logicalAddressToPhysical(log_addr));
			FS_VERIFY(!markAsObsolete(log_addr, FLAG_FROM_REBOOT_NO));
			i++;

			/* break after initial_writes vots */
			if(i> 5){
				break;
			}
		}
	}

	/* reboot */
	VERIFY(!fsBooting());

	/* verify file doesn't exist*/
	VERIFY(IS_NEGATIVE(open(pathname, NANDFS_O_RDONLY,0)));

	/* verify returning to stable state (after unlink) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
//	PRINT("\nverified last closed tid");
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
//		PRINT("\nverified empty tid");
	}

	VERIFY(COMPARE(rec_addr, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT_MSG_AND_NUM(", expected_obs=",expected_obs);
//	PRINT_MSG_AND_NUM(", actual obs=",GET_OBS_COUNT());
	VERIFY(COMPARE(expected_obs, GET_OBS_COUNT()));

	return 1;
}

/**
 * @brief
 * create files, write to them, close, re-open. re-write, unlink etc.
 * reboot and verify we return to previous state
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest15(){
	int32_t fd1, fd2, fd3, fd4;
	int32_t f_size1, f_size2, f_size3, f_size4;
	int32_t f_id1, f_id2, f_id3, f_id4;
	int32_t i, j, free_count, res;
	uint8_t *f_name1 = "/file1.dat", *f_name2 = "/file2.dat", *f_name3 = "/file3.dat", *f_name4 = "/file4.dat";
	uint8_t buf[FS_BLOCK_SIZE], byte = 'a';

	/* we must be able to have at least two simultaneous transactions for this test to succeed*/
	if(FS_MAX_N_TRANSACTIONS <2){
		return 1;
	}

	/* mark eu's as bad*/
	for(i=0; i< 4; i++){
//		PRINT_MSG_AND_NUM("\nmark first EU as bad in slot ", i*(SEQ_N_SLOTS_COUNT/4));
		markEuAsMockBad(CALC_ADDRESS(i*(SEQ_N_SLOTS_COUNT/4), 0, 0));
	}
//	PRINT_MSG_AND_NUM("\nmark first EU as bad in slot ", SEQ_N_SLOTS_COUNT-1);
	markEuAsMockBad(CALC_ADDRESS(SEQ_N_SLOTS_COUNT-1, 0, 0));

	/* create fs */
	VERIFY(!fsBooting());

	fsMemset(buf, byte, FS_BLOCK_SIZE);

	/* create 1st file*/
//	PRINT("\ncreate 1st file");
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));

//	PRINT("\nwrite to 1st file inode");
	VERIFY(COMPARE(write(fd1, buf, INODE_FILE_DATA_SIZE), INODE_FILE_DATA_SIZE));
//	PRINT("\nwrite to 1st file");
	for(i=0; i< 15; i++){
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
	}
//	PRINT("\ncreat 1 success");

	/* create 2nd file*/
	fd2 = creat(f_name2, 0);
	VERIFY(!IS_NEGATIVE(fd2));
	f_id2 =GET_FILE_ID_BY_FD(fd2);
	for(i=0; i< 115; i++){
		VERIFY(COMPARE(write(fd2, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
	}
//	PRINT("\ncreat 2 success");
	VERIFY(!close(fd2));

	/* create 3rd file*/
	fd3 = creat(f_name3, 0);
	VERIFY(!IS_NEGATIVE(fd3));
	f_id3 =GET_FILE_ID_BY_FD(fd3);

	L("WRITE INODE FILE DATA FOR FD3");
	VERIFY(COMPARE(write(fd3, buf, INODE_FILE_DATA_SIZE), INODE_FILE_DATA_SIZE));

	/* write files 1,3 1/4 of flash each*/
	PRINT("\n\n\n\n");
	for(i=0; i< (SEQ_SEGMENTS_COUNT / 4) * SEQ_PAGES_PER_SLOT; i++){
		L("write #%d. open file entry offset %d", i, OPEN_FILE_GET_OFFSET(fd3));
		VERIFY(COMPARE(write(fd3, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
	}


//	PRINT("\ncreat 3 success");

	/* unlink file 1*/
	VERIFY(!close(fd1));
	VERIFY(!unlink(f_name1));
//	PRINT("\nunlink success");

	/* create 4th file, verify it was re-created on inode2*/
	fd4 = creat(f_name4, 0);
	VERIFY(!IS_NEGATIVE(fd4));
	VERIFY(GET_FILE_ID_BY_FD(fd4) < f_id3);
	VERIFY(COMPARE(write(fd4, buf, INODE_FILE_DATA_SIZE), INODE_FILE_DATA_SIZE));

	/* write files 3,4 1/5 of flash each*/
	for(i=0; i< (SEQ_SEGMENTS_COUNT / 6) * SEQ_PAGES_PER_SLOT; i++){
//		PRINT_MSG_AND_NUM("\ni=", i);
//		PRINT_MSG_AND_NUM(" frees=", calcTotalFreePages());
//		PRINT_MSG_AND_NUM(" (FS_MIN_FREE_PAGES=", FS_MIN_FREE_PAGES);
//		PRINT(")");
		res = write(fd3, buf, FS_BLOCK_SIZE);
//		if(IS_STATE_RECLAMATION()){
//			PRINT(" (state rec)");
//		}
		VERIFY(COMPARE(res, FS_BLOCK_SIZE));

//		PRINT(" (2nd write)");
		res = write(fd4, buf, FS_BLOCK_SIZE);
//		if(res != FS_BLOCK_SIZE){
//			PRINT_MSG_AND_NUM("\nres=", res);
//		}
//
//		if(IS_STATE_RECLAMATION()){
//			PRINT(" (state rec)");
//		}
		VERIFY(COMPARE(res, FS_BLOCK_SIZE));

	}
//	PRINT("\ncreat 4 success");
	f_id4 =GET_FILE_ID_BY_FD(fd4);
	VERIFY(!close(fd3));
	VERIFY(!close(fd4));
//	PRINT("\nfile closes success");

	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\nre-creat success");
	f_id1 =GET_FILE_ID_BY_FD(fd1);
//	PRINT("\nre-creat inode id success");

	f_size1 = getFileSize(f_id1);
	f_size2 = getFileSize(f_id2);
	f_size3 = getFileSize(f_id3);
	f_size4 = getFileSize(f_id4);

//	rec_offset = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	/* try to create existing file */
	VERIFY(IS_NEGATIVE(creat(f_name4, 0)));

//	PRINT("\nprint fs bytes");
//	for(i=0;i<FILESYSTEM_T_SIZE;i++){
//		PRINT_MSG_AND_NUM("\n",i);
//		PRINT_MSG_AND_HEX(". ", fs_ptr->bytes[i]);
//	}

	fd4 = open(f_name4, NANDFS_O_WRONLY, 0);
	VERIFY(!IS_NEGATIVE(fd4));
//	PRINT("\nre-open file 4 inode id success");

	{
		for(i=0; i<FS_MAX_N_TRANSACTIONS; i++){
//			PRINT_MSG_AND_NUM("\nb4 final writes transaction vots=", TRANSACTION_GET_VOTS_COUNT(i));
		}
	}

	free_count = calcTotalFreePages();
//	PRINT_MSG_AND_NUM("\nb4 final writes, free_count=",free_count);

	for(i=0; i< (SEQ_SEGMENTS_COUNT / 7) * SEQ_PAGES_PER_SLOT; i++){
		res = write(fd4, buf, FS_BLOCK_SIZE);
		if(res != FS_BLOCK_SIZE){
//			PRINT_MSG_AND_NUM("\nwrite failed res=", res);
//			PRINT_MSG_AND_NUM("\ni=", i);
			return 0;
		}
	}
//	PRINT("\nlast writes success");
//	PRINT_MSG_AND_HEX("\nb4 booting ino0 addr=", *CAST_TO_UINT32(FS_GET_INO0_ADDR_PTR()))
//	PRINT_MSG_AND_NUM(", free_count=",calcTotalFreePages());

	/* reboot */
//	PRINT("\n\n\nabout to boot");
	VERIFY(!fsBooting());
//	PRINT("\n2nd reboot success");
//	PRINT_MSG_AND_HEX("\nafter booting ino0 addr=", *CAST_TO_UINT32(FS_GET_INO0_ADDR_PTR()));

	/* verify returning to stable state */
	VERIFY(COMPARE(f_size1, getFileSize(f_id1)));
	VERIFY(COMPARE(f_size2, getFileSize(f_id2)));
	VERIFY(COMPARE(f_size3, getFileSize(f_id3)));
	VERIFY(COMPARE(f_size4, getFileSize(f_id4)));
//	PRINT("\nfile sizes verified ");

	fd1 = open(f_name1, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd1));
	fd2 = open(f_name2, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd2));
	fd3 = open(f_name3, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd3));
	fd4 = open(f_name4, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd4));
//	PRINT("\nopen files success");

	init_fsbuf(buf);
	VERIFY(COMPARE(read(fd3, buf, INODE_FILE_DATA_SIZE), INODE_FILE_DATA_SIZE));
	printBlock(buf);
	for(i=0; i< INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(buf[i], byte));
	}

	VERIFY(COMPARE(read(fd4, buf, INODE_FILE_DATA_SIZE), INODE_FILE_DATA_SIZE));
	for(i=0; i< INODE_FILE_DATA_SIZE; i++){
		VERIFY(COMPARE(buf[i], byte));
	}

//	PRINT("\nfirst reads success");
	for(i=0; i<f_size1; i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(read(fd1, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));

		for(j=0; j< FS_BLOCK_SIZE; j++){
			VERIFY(COMPARE(buf[j], byte));
		}
	}
//	PRINT("\nfile1 read success");
//	PRINT_MSG_AND_NUM("\nfile 2 size=", f_size2);
	for(i=0; i<f_size2; i+=FS_BLOCK_SIZE){
//		PRINT_MSG_AND_NUM("\nread from file 2 offset=", OPEN_FILE_GET_OFFSET(fd2));
		VERIFY(COMPARE(read(fd2, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));

		for(j=0; j< FS_BLOCK_SIZE; j++){
			VERIFY(COMPARE(buf[j], byte));
		}
	}
//	PRINT("\nfile2 read success");

//	PRINT_MSG_AND_NUM("\nfile 3 size=", f_size3);
	for(i=INODE_FILE_DATA_SIZE; i<f_size3; i+=FS_BLOCK_SIZE){
//		if(OPEN_FILE_GET_OFFSET(fd3) > 18864063){
//		PRINT_MSG_AND_NUM("\nread from file 3 offset=", OPEN_FILE_GET_OFFSET(fd3));
//		}
		VERIFY(COMPARE(read(fd3, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
//		PRINT_MSG_AND_NUM("\nsuccess read from offset ", i);
//		PRINT_MSG_AND_NUM(" f_size3=", f_size3);
		for(j=0; j< FS_BLOCK_SIZE; j++){
			if(!COMPARE(buf[j], byte)){
				printBlock(buf);
				return 0;
			}
			VERIFY(COMPARE(buf[j], byte));
		}
	}
//	PRINT("\nfile3 read success");

	for(i=INODE_FILE_DATA_SIZE; i<f_size4; i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(read(fd4, buf, FS_BLOCK_SIZE), FS_BLOCK_SIZE));
//		PRINT_MSG_AND_NUM("\nsuccess read from offset ", i);
//		PRINT_MSG_AND_NUM(" f_size4=", f_size3);
		for(j=0; j< FS_BLOCK_SIZE; j++){
			VERIFY(COMPARE(buf[j], byte));
		}
	}
//	PRINT("\nfile4 read success");
//	PRINT_MSG_AND_NUM("\nrec_offset=",rec_offset);
//	PRINT_MSG_AND_NUM(" actual offset=",logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
//	VERIFY(COMPARE(rec_offset, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\nrec offset verified ");
//	VERIFY(COMPARE(free_count, calcTotalFreePages()));
//	PRINT("\nfree count verified ");
	VERIFY(!close(fd1));
	VERIFY(!close(fd2));
	VERIFY(!close(fd3));
	VERIFY(!close(fd4));

	/* verify returning to stable state (after unlink) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nverified transaction etc...");

	VERIFY(IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR()));
//	PRINT("\nverified last closed tid");
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		VERIFY(IS_ADDR_EMPTY(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
	}

	return 1;
}

/**
 * @brief
 * create multiple small file, which have only one page.
 * reboot. verify stability
 * unlink all files, verify success.
 * reboot, verify stability
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest16(){
	int32_t fd, i, j = 0, frees, res, max_files=SEQ_PAGES_PER_SLOT * 4, b4_frees, after_frees;
	uint8_t f_name[230] = "/", last_name[230];
	uint8_t buf[FS_BLOCK_SIZE], byte = 'a';
	user_id uid = 1;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	for(i=1; i<229; i++){
		f_name[i]  = '0';
//		first_name[i] = '0';
	}
	f_name[229] = '\0';
	fsMemset(buf, byte, FS_BLOCK_SIZE);

	/* boot*/
	VERIFY(!fsBooting());
	frees = calcTotalFreePages();
//	PRINT("\nbooting success");
	j=0;
	/* - create file
	 * - write one block
	 * - close
	 * until we have very little free pages */
	i = 1;
	while(!IS_MIN_FREE_PAGES_REACHED() || !(TRANSACTION_GET_VOTS_COUNT(0) <= (TRANSACTION_COMMIT_MIN_VOTS+8))){
		b4_frees = 	calcTotalFreePages();
//		PRINT_MSG_AND_STR("\ncreat ", f_name);
//		PRINT_MSG_AND_NUM(" frees b4=", calcTotalFreePages());
		fd = creat(f_name, 0);
//		PRINT_MSG_AND_NUM("\nafter creat fd=", fd);
//		PRINT_MSG_AND_NUM(" after=", calcTotalFreePages());
		after_frees = calcTotalFreePages();
//		if(((j%2 == 0) && ((b4_frees-after_frees) !=1)) ||
//		   ((j%2 == 1) && ((b4_frees-after_frees) !=2)) || (j==max_files-1)){
//		   PRINT_MSG_AND_NUM("\nj=", j);
//		   PRINT_MSG_AND_NUM(" frees b4=", b4_frees);
//		   PRINT_MSG_AND_NUM(" after=", after_frees);
////		   PRINT_MSG_AND_NUM("\ninode0 offset=", INODE_FILE_DATA_SIZE+((j+1)*FS_BLOCK_SIZE));
////		   PRINT_MSG_AND_NUM(" root dir offset=", INODE_FILE_DATA_SIZE+((j+1)/2)*FS_BLOCK_SIZE);
//		}
//		PRINT_MSG_AND_NUM("\nafter printing fd=", fd);
//		if(IS_NEGATIVE(fd)){
//			PRINT_MSG_AND_STR("\nfailed creating ", f_name);
//		}
//		fsStrcpy(names[j], f_name);
		if(IS_NEGATIVE(fd)){
//			PRINT_MSG_AND_STR("\nerror creating ", f_name);
		}
//		PRINT_MSG_AND_NUM("\nverify fd=", fd);

		VERIFY(!IS_NEGATIVE(fd));

//		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
		VERIFY(!close(fd));
		/* boot*/
		VERIFY(!fsBooting());
		if(j==max_files){
//			PRINT_MSG_AND_NUM("\ncreated files=", j);
//			PRINT_MSG_AND_STR(". last created=" , f_name);
//			PRINT_MSG_AND_NUM(" frees=", calcTotalFreePages());
			break;
		}
//		PRINT_MSG_AND_NUM("\nj=", j);
//		PRINT_MSG_AND_HEX("\ndidn't finish, f_name[i]=", f_name[i]);
		if(f_name[i] == 126){
			i++;
//			PRINT_MSG_AND_NUM("\ncreated another 229. i=", i);
		}

		f_name[i]++;
//		PRINT("\nchanged i");
		if(i==229){
			break;
		}
//		PRINT("\nchange f_name");

		j++;
	}
//	PRINT("\ncreats success");
//	PRINT_MSG_AND_STR(". last created=" , f_name);
	fsStrcpy(last_name, f_name);

	for(i=1; i<229; i++){
		f_name[i] = '0';
	}
	f_name[229] = '\0';
	i = 1;
	j=0;

//	PRINT_MSG_AND_NUM(" frees=", calcTotalFreePages());
	/* unlink all files, verify system is empty!*/
	while(fsStrcmp(last_name, f_name)){
//		PRINT_MSG_AND_STR("\nunlink ", f_name);
		b4_frees = 	calcTotalFreePages();
		res = unlink(f_name);
//		after_frees = 	calcTotalFreePages();
//		PRINT_MSG_AND_NUM(" frees b4=", b4_frees);
//		 PRINT_MSG_AND_NUM(" after=", after_frees);
//		if(((j%2 == 0) && (after_frees-b4_frees !=2)) ||
//		   ((j%2 == 1) && (after_frees-b4_frees !=1)) || (j==max_files)){
//		   PRINT_MSG_AND_NUM("\nj=", j);
//		   PRINT_MSG_AND_NUM(" frees b4=", b4_frees);
//		   PRINT_MSG_AND_NUM(" after=", after_frees);
////		   PRINT_MSG_AND_NUM("\ninode0 offset=", INODE_FILE_DATA_SIZE+((j+1)*FS_BLOCK_SIZE));
////		   PRINT_MSG_AND_NUM(" root dir offset=", INODE_FILE_DATA_SIZE+((j+1)/2)*FS_BLOCK_SIZE);
//		}
//		if(res){
//			PRINT_MSG_AND_NUM("\nfailure. res=", res);
//			PRINT_MSG_AND_NUM(". j=", j);
//			PRINT_MSG_AND_STR(" file name=", f_name);
//			PRINT_MSG_AND_NUM("\ntotal free=", calcTotalFreePages());
//			PRINT_MSG_AND_NUM(" expected=", frees);
//		}
//		if(res){
////			PRINT_MSG_AND_STR("\nfailed unlink ", f_name);
//		}

		VERIFY(!res);
//		if(res){
//			PRINT_MSG_AND_STR("\nerror unlinking ", f_name);
//			break;
//		}

		/* boot*/
		VERIFY(!fsBooting());

//		if(IS_STATE_RECLAMATION()){
////			PRINT_MSG_AND_NUM("\nstate is reclamation. j=", j);
//		}
		if(j==max_files){
//			PRINT_MSG_AND_NUM("\nunlinked files=", j);
//			PRINT_MSG_AND_STR(". last unlinked=" , f_name);
			break;
		}
//		PRINT_MSG_AND_NUM("\nj=", j);
//		PRINT_MSG_AND_HEX("\ndidn't finish, f_name[i]=", f_name[i]);
		if(f_name[i] == 126){
			i++;
//			PRINT_MSG_AND_NUM("\nunlinked another 229. i=", i);
		}

		f_name[i]++;
//		PRINT("\nchanged i");
		if(i==229){
			break;
		}
//		PRINT("\nchange f_name");

		j++;
	}

//	PRINT_MSG_AND_STR("\nunlink ", f_name);
	b4_frees = 	calcTotalFreePages();
	res = unlink(f_name);
	after_frees = 	calcTotalFreePages();
//	PRINT_MSG_AND_NUM("\n b4 last unlink frees=", b4_frees);
//	 PRINT_MSG_AND_NUM(" after=", after_frees);
//	if(IS_STATE_RECLAMATION()){
//		PRINT_MSG_AND_NUM("\nstate is reclamation. j=", j);
//		nandReadPageTotal(fs_buffer, GET_RECLAIMED_SEGMENT_SLOT()+GET_RECLAIMED_OFFSET());
//		PRINT_MSG_AND_NUM("\nis rec page obsolete? ", GET_OBS_FLAG(flags)==OBS_FLAG_TRUE);
//	}
//	PRINT_MSG_AND_STR("\nlast_name ", last_name);
//	PRINT("\nunlinks success");
//	PRINT_MSG_AND_NUM("\ntotal free=", calcTotalFreePages());
//	PRINT_MSG_AND_NUM("\nexpected=", frees);

	VERIFY(COMPARE(frees, calcTotalFreePages()));
	return 1;
}

/**
 * @brief
 * create directories, files, write to them, close, re-open. re-write, unlink, rmdir etc.
 * reboot and verify we return to previous state
 * NOTICE - do everything with only 1 transaction active at all times
 *
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest17(){
	int32_t fd1, fd2, fd3, fd4, fd5;
	int32_t f_size1 = 40000, f_size2 = 40, f_size3 = 10, f_size4 = 7530, f_size5 = 0;
	int32_t f_id1, f_id2, f_id3, f_id4, f_id5;
	int32_t i, free_count;
	uint8_t *f_name1 = "/file1.dat", *f_name2 = "/file2.dat", *f_name3 = "/dir1/file3.dat", *f_name4 = "/dir1/file4.dat", *f_name5 = "/dir1/dir3/file5.dat";
	uint8_t buf[FS_BLOCK_SIZE], byte = 'a';
	uint8_t *dir_name1 = "/dir1", *dir_name2 = "/dir2", *dir_name3 = "/dir1/dir3", *dir_name4 = "/dir1/dir4";

	/* create fs */
	VERIFY(!fsBooting());
	fsMemset(buf, byte, FS_BLOCK_SIZE);

	/* create 1st directory */
	VERIFY(!mkdir(dir_name1, 0));
//	PRINT_MSG_AND_NUM("\nafter 1st mkdir cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));

	/* create 2nd directory */
	VERIFY(!mkdir(dir_name2, 0));
//	PRINT_MSG_AND_NUM("\nafter 2nd mkdir cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));

	/* create files in root directory*/
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT_MSG_AND_NUM("\nafter 1st creat cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));

	fd2 = creat(f_name2, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT_MSG_AND_NUM("\nafter 2nd creat cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));

	/* create inner directories and files*/
	VERIFY(!mkdir(dir_name3, 0));
//	PRINT_MSG_AND_NUM("\nafter 3rd mkdir cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
	VERIFY(!mkdir(dir_name4, 0));
//	PRINT_MSG_AND_NUM("\nafter 4th mkdir cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));

	fd3 = creat(f_name3, 0);
//	PRINT_MSG_AND_NUM("\nafter 3rd creat cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
	VERIFY(!IS_NEGATIVE(fd3));

	fd4 = creat(f_name4, 0);
	VERIFY(!IS_NEGATIVE(fd4));
//	PRINT_MSG_AND_NUM("\nafter 4th mkdir cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
//		VERIFY(COMPARE(INDIRECT_CACHE_GET_TID(0), TID_EMPTY));

	fd5 = creat(f_name5, 0);
//	PRINT_MSG_AND_NUM("\nafter 5th mkdir cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
//		VERIFY(COMPARE(INDIRECT_CACHE_GET_TID(0), TID_EMPTY));
	VERIFY(!IS_NEGATIVE(fd5));

	f_id1 = GET_FILE_ID_BY_FD(fd1);
	f_id2 = GET_FILE_ID_BY_FD(fd2);
	f_id3 = GET_FILE_ID_BY_FD(fd3);
	f_id4 = GET_FILE_ID_BY_FD(fd4);
	f_id5 = GET_FILE_ID_BY_FD(fd5);

//	PRINT("\ncreats success");
	/* write to files and close them*/
	for(i = 0; i < f_size1; i++){
//		PRINT_MSG_AND_NUM("\ni=",i);
		VERIFY(COMPARE(1, write(fd1, buf, 1)));
	}
	VERIFY(!close(fd1));
//	PRINT("\n1st write success");
//	PRINT_MSG_AND_NUM("\nafter close fd1 cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
	for(i = 0; i < f_size2; i++){
		VERIFY(COMPARE(1, write(fd2, buf, 1)));
	}
	VERIFY(!close(fd2));
//	PRINT("\n2nd write success");
//	PRINT_MSG_AND_NUM("\nafter close fd21 cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
	for(i = 0; i < f_size3; i++){
		VERIFY(COMPARE(1, write(fd3, buf, 1)));
	}
	VERIFY(!close(fd3));
//	PRINT("\n3rd write success");
//	PRINT_MSG_AND_NUM("\nafter close fd3 cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
	for(i = 0; i < f_size4; i++){
		VERIFY(COMPARE(1, write(fd4, buf, 1)));
	}
	VERIFY(!close(fd4));
//	PRINT("\n4th write success");
//	PRINT_MSG_AND_NUM("\nafter close fd4 cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
	/* write to file 5 and dont close*/
	free_count = calcTotalFreePages();
//	PRINT_MSG_AND_NUM("\nsave frees. state reclamation=", IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" new slot=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" obs count=", GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" free counter=", GET_FREE_COUNTER());
//	{
//		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//		uint8_t buf[NAND_TOTAL_SIZE];
//		INIT_FLAGS_POINTER_TO_BUFFER(flags, buf);
//		copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
//		SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
//		nandReadPageTotal(buf, logicalAddressToPhysical(log_addr));
//		PRINT_MSG_AND_NUM("\ncp flag=", GET_CHECKPOINT_FLAG(flags));
//		PRINT_MSG_AND_NUM("\ncp obs counter=", ((obs_pages_per_seg_counters*)(&(buf[sizeof(filesystem_t)])))->counter);
//		PRINT_MSG_AND_NUM("\ncp free counter=", ((obs_pages_per_seg_counters*)(&(buf[sizeof(filesystem_t)])))->free_counter);
//		PRINT_MSG_AND_NUM("\nrec page obsolete=", is_page_marked_obsolete(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	}
	for(i = 0; i < f_size5; i++){
		VERIFY(COMPARE(1, write(fd5, buf, 1)));
	}
//	PRINT("\n5th write success");

	/* reboot and verify file sizes*/
	VERIFY(!fsBooting());
//	PRINT("\nbooting success");
//	PRINT_MSG_AND_HEX("\nAFTER BOOTING ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
//	PRINT_MSG_AND_NUM("\nstate reclamation=", IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" new slot=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" obs count=", GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" free counter=", GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nrec page obsolete=", is_page_marked_obsolete(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT_MSG_AND_NUM("\n********** cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
//	PRINT_MSG_AND_NUM("\nafter booting cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
	/* verify free pages count*/
//	PRINT_MSG_AND_NUM("\nexpected frees=", free_count);
//	PRINT_MSG_AND_NUM(" frees=", calcTotalFreePages());
	VERIFY(COMPARE(free_count, calcTotalFreePages()));

	/* verify file sizes*/
	VERIFY(COMPARE(getFileSize(f_id1), f_size1));
//	PRINT("\nfile size 1 success");
	VERIFY(COMPARE(getFileSize(f_id2), f_size2));
//	PRINT("\nfile size 2 success");
	VERIFY(COMPARE(getFileSize(f_id3), f_size3));
//	PRINT("\nfile size 3 success");
	VERIFY(COMPARE(getFileSize(f_id4), f_size4));
//	PRINT("\nfile size 4 success");
	VERIFY(COMPARE(getFileSize(f_id5), f_size5));
//	PRINT("\nfiles sizes success");
//	PRINT_MSG_AND_HEX("\nAFTER GET FILE SIZES ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
	/* verify returning to stable state (after unlink) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nvnodes transactions success");

	/* try to unlink full directory*/
//	PRINT_MSG_AND_NUM("\nb4 1st rmdir cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
//	PRINT_MSG_AND_HEX(", TID_EMPTY=", TID_EMPTY);
//	VERIFY(COMPARE(INDIRECT_CACHE_GET_TID(0), TID_EMPTY));
	VERIFY(rmdir(dir_name1));
//	PRINT("\n\n\n\nrmdir success");
//	PRINT_MSG_AND_HEX("\nAFTER RMDIR 1 ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
//	PRINT_MSG_AND_NUM("\n********** cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));
	VERIFY(!unlink(f_name5));
//	PRINT("\n\n\n\nunlink 5 success");
//	PRINT_MSG_AND_HEX("\nAFTER unlink 5 ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
	VERIFY(!unlink(f_name1));
//	PRINT("\n\n\n\nunlink 1 success");
	VERIFY(!rmdir(dir_name3));
//	PRINT("\n\nrmdir 3 success");

	/* reboot and verify file sizes*/
	free_count = calcTotalFreePages();
	VERIFY(!fsBooting());
//	PRINT("\nyet another booting success");

	/* verify file sizes*/
	VERIFY(COMPARE(getFileSize(f_id2), f_size2));
	VERIFY(COMPARE(getFileSize(f_id3), f_size3));
	VERIFY(COMPARE(getFileSize(f_id4), f_size4));
//	PRINT("\nfile sizes success");

	/* verify returning to stable state (after unlink) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nvnodes transactions success");

//	PRINT_MSG_AND_NUM("\nexpect4ed frees=", free_count);
//	PRINT_MSG_AND_NUM(" frees=", calcTotalFreePages());
	/* verify free pages count*/
	VERIFY(COMPARE(free_count, calcTotalFreePages()));

	/* verify cant open deleted files*/
	VERIFY(open(f_name1, NANDFS_O_RDONLY, 0));
	VERIFY(open(f_name5, NANDFS_O_RDONLY, 0));
	VERIFY(COMPARE(NULL, opendir(dir_name3)));

	return 1;
}

/**
 * @brief
 * create directories, files, write to them, close, re-open. re-write, unlink, rmdir etc.
 * reboot and verify we return to previous state
 * NOTICE -
 * 1. do everything with only 1 transaction active at all times
 * 2. mark several EU's as bad
 *
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest18(){
	int32_t fd1, fd2, fd3, fd4, fd5;
	int32_t f_size1 = 40000, f_size2 = 40, f_size3 = 10, f_size4 = 7530, f_size5 = 0;
	int32_t f_id1, f_id2, f_id3, f_id4, f_id5;
	int32_t i, free_count;
	uint8_t *f_name1 = "/file1.dat", *f_name2 = "/file2.dat", *f_name3 = "/dir1/file3.dat", *f_name4 = "/dir1/file4.dat", *f_name5 = "/dir1/dir3/file5.dat";
	uint8_t buf[FS_BLOCK_SIZE], byte = 'a';
	uint8_t *dir_name1 = "/dir1", *dir_name2 = "/dir2", *dir_name3 = "/dir1/dir3", *dir_name4 = "/dir1/dir4";

	//	int32_t i;
	/* mark eu's as bad*/
	for(i=0; i< 4; i++){
//		PRINT_MSG_AND_NUM("\nmark first EU as bad in slot ", i*(SEQ_N_SLOTS_COUNT/4));
		markEuAsMockBad(CALC_ADDRESS(i*(SEQ_N_SLOTS_COUNT/4), 0, 0));
	}
//	PRINT_MSG_AND_NUM("\nmark first EU as bad in slot ", SEQ_N_SLOTS_COUNT-1);
	markEuAsMockBad(CALC_ADDRESS(SEQ_N_SLOTS_COUNT-1, 0, 0));
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nb4 test flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}

//	markEuAsBad(0);
//	PRINT("\n	finished marking all bad EU's");

	/* create fs */
	VERIFY(!fsBooting());
	fsMemset(buf, byte, FS_BLOCK_SIZE);
//	PRINT("\n	initial booting success");
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 1st booting -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
	/* create 1st directory */
	VERIFY(!mkdir(dir_name1, 0));
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 1st mkdir -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
	/* create 2nd directory */
	VERIFY(!mkdir(dir_name2, 0));
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 2nd mkdire -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
//	PRINT("\n	mkdirs success");

	/* create files in root directory*/
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 1st creat -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
	fd2 = creat(f_name2, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 2nd creat -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
//	PRINT("\n	creats success");
	/* create inner directories and files*/
	VERIFY(!mkdir(dir_name3, 0));
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 3rd mkdir -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
	VERIFY(!mkdir(dir_name4, 0));
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 4th mkdir -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
//	PRINT("\n	more mkdirs success");
	fd3 = creat(f_name3, 0);
	VERIFY(!IS_NEGATIVE(fd3));
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 3rd creat -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
	fd4 = creat(f_name4, 0);
	VERIFY(!IS_NEGATIVE(fd4));
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 4th creat -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
	fd5 = creat(f_name5, 0);
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 5th creat -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
	VERIFY(!IS_NEGATIVE(fd5));
//	PRINT("\n	more creats success");
	f_id1 = GET_FILE_ID_BY_FD(fd1);
	f_id2 = GET_FILE_ID_BY_FD(fd2);
	f_id3 = GET_FILE_ID_BY_FD(fd3);
	f_id4 = GET_FILE_ID_BY_FD(fd4);
	f_id5 = GET_FILE_ID_BY_FD(fd5);

//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nb4writes -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}

	/* write to files and close them*/
	for(i = 0; i < f_size1; i++){
		VERIFY(COMPARE(1, write(fd1, buf, 1)));
	}
	VERIFY(!close(fd1));
//	PRINT("\n	1st write success");

//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 1st write -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}

	for(i = 0; i < f_size2; i++){
		VERIFY(COMPARE(1, write(fd2, buf, 1)));
	}
	VERIFY(!close(fd2));
//	PRINT("\n	2nd write success");
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\n	after 2nd write -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}

	for(i = 0; i < f_size3; i++){
		VERIFY(COMPARE(1, write(fd3, buf, 1)));
	}
	VERIFY(!close(fd3));
//	PRINT("\n	3rd write success");
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nafter 3rd write -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
//	{
//		uint8_t buf[NAND_TOTAL_SIZE];
//		page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//		nandReadPageTotal(buf, 64576);
//		PRINT_MSG_AND_NUM("\n	b4 4th write page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//	}
	for(i = 0; i < f_size4; i++){
		VERIFY(COMPARE(1, write(fd4, buf, 1)));
//		{
//	//		if(seg_map_ptr->new_slot_id == 0){
//			uint8_t buf[NAND_TOTAL_SIZE];
//			page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//			nandReadPageTotal(buf, 64576);
//			PRINT_MSG_AND_NUM("\n	after another write page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//	//		}
//		}
	}
//	{
//		uint8_t buf[NAND_TOTAL_SIZE];
//		page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//		nandReadPageTotal(buf, 64576);
//		PRINT_MSG_AND_NUM("\n	after 4th write page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//	}
//	PRINT("\n	4th write success");
	VERIFY(!close(fd4));
//	PRINT("\n	4th close success");

	/* write to file 5 and dont close*/
	free_count = calcTotalFreePages();
//	PRINT_MSG_AND_NUM("\n	save frees. state reclamation=", IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" new slot=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" obs count=", GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" free counter=", GET_FREE_COUNTER());

//	{
//		uint8_t buf[NAND_TOTAL_SIZE];
//		page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//		nandReadPageTotal(buf, 64576);
//		PRINT_MSG_AND_NUM("\n	b4 5th write page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//	}
	for(i = 0; i < f_size5; i++){
		VERIFY(COMPARE(1, write(fd5, buf, 1)));
	}
//	PRINT("\n5th write success");
//
//	PRINT("\ndo 2nd reboot");
//	PRINT_MSG_AND_NUM(" seg 2 slot=", GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, 2));
//	PRINT_MSG_AND_NUM(" seg 125 slot=", GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, 125));
//	PRINT_MSG_AND_NUM(" new slot id=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	{
//		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//		SET_LOGICAL_OFFSET(log_addr, 0);
//		SET_LOGICAL_SEGMENT(log_addr, 2);
//
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_NUM("\nb4 that - read flags of page ", CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX(" slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//		PRINT_MSG_AND_NUM("\nseg 2 replacement = ", logicalAddressToPhysical(log_addr));
//	}
//	{
//		PRINT("\nb4 rebooting segments are");
//		int32_t i;
//		for(i=0; i< SEQ_N_SLOTS_COUNT; i++){
//			PRINT_MSG_AND_NUM("\nseg #", i);
//			PRINT_MSG_AND_NUM(" slot=", GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, i));
//		}
//	}
//	{
//		uint8_t buf[NAND_TOTAL_SIZE];
//		page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//		nandReadPageTotal(buf, 64576);
//		PRINT_MSG_AND_NUM("\nb4 2nd rebooting page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//	}
	/* reboot and verify file sizes*/
	VERIFY(!fsBooting());
//	PRINT("\nbooting 2 success");
//	assert(0);
//	PRINT_MSG_AND_NUM("\nstate reclamation=", IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" new slot=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" obs count=", GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" free counter=", GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nrec page obsolete=", is_page_marked_obsolete(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	/* verify free pages count*/
//	PRINT_MSG_AND_NUM("\nexpected frees=", free_count);
//	PRINT_MSG_AND_NUM(" frees=", calcTotalFreePages());
	VERIFY(COMPARE(free_count, calcTotalFreePages()));

	/* verify file sizes*/
	VERIFY(COMPARE(getFileSize(f_id1), f_size1));
//	PRINT("\nfile size 1 success");
	VERIFY(COMPARE(getFileSize(f_id2), f_size2));
//	PRINT("\nfile size 2 success");
	VERIFY(COMPARE(getFileSize(f_id3), f_size3));
//	PRINT("\nfile size 3 success");
	VERIFY(COMPARE(getFileSize(f_id4), f_size4));
//	PRINT("\nfile size 4 success");
	VERIFY(COMPARE(getFileSize(f_id5), f_size5));
//	PRINT("\nfiles sizes success");

	/* verify returning to stable state (after unlink) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nvnodes transactions success");

	/* try to unlink full directory*/
	VERIFY(rmdir(dir_name1));

	VERIFY(!unlink(f_name5));
	VERIFY(!unlink(f_name1));
	VERIFY(!rmdir(dir_name3));

	/* reboot and verify file sizes*/
	free_count = calcTotalFreePages();
	VERIFY(!fsBooting());

	/* verify file sizes*/
	VERIFY(COMPARE(getFileSize(f_id2), f_size2));
	VERIFY(COMPARE(getFileSize(f_id3), f_size3));
	VERIFY(COMPARE(getFileSize(f_id4), f_size4));

	/* verify returning to stable state (after unlink) */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\nvnodes transactions success");

//	PRINT_MSG_AND_NUM("\nexpect4ed frees=", free_count);
//	PRINT_MSG_AND_NUM(" frees=", calcTotalFreePages());
	/* verify free pages count*/
	VERIFY(COMPARE(free_count, calcTotalFreePages()));

	/* verify cant open deleted files*/
	VERIFY(open(f_name1, NANDFS_O_RDONLY, 0));
	VERIFY(open(f_name5, NANDFS_O_RDONLY, 0));
	VERIFY(COMPARE(NULL, opendir(dir_name3)));

	return 1;
}
//#endif
