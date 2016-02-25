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

/** @file unlinkTests.c  */
#include <test/fs/unlinkTests.h>
#include <test/fs/testsHeader.h>

void runAllunlinkTests(){

	RUN_TEST(unlink,1);
	RUN_TEST(unlink,2);
	RUN_TEST(unlink,3);
	RUN_TEST(unlink,4);
	RUN_TEST(unlink,5);
	RUN_TEST(unlink,6);
	RUN_TEST(unlink,7);
	RUN_TEST(unlink,8);
	RUN_TEST(unlink,9);
	RUN_TEST(unlink,10);
	RUN_TEST(unlink,11);
	RUN_TEST(unlink,12);
	RUN_TEST(unlink,13);
	RUN_TEST(unlink,14);
	RUN_TEST(unlink,15);
	RUN_TEST(unlink,16);
	RUN_TEST(unlink,17);
	RUN_TEST(unlink,18);
	RUN_TEST(unlink,19);

}

/**
 * @brief
 * init unlink test
 */
void init_unlinkTest(){
	if(nandInit())
		return;

	init_flash();
	init_file_entries();
	init_vnodes();
	init_transactions();
	initializeRamStructs();
	handleNoFs();
	init_fsbuf(fs_buffer);
}

/**
 * @brief
 * tear down unlink test
 */
void tearDown_unlinkTest(){
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
 * unlink an existing  file in root directory
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest1(){
	int32_t fd, free_pages, i;
	uint8_t *f_name = "/file1.dat";
	user_id uid = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);

	init_logical_address(log_addr);
	init_logical_address(root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	free_pages = calcTotalFreePages();
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	VERIFY(!close(fd));

	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
//	PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//	PRINT("\nabout to unlink");
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

//	{
//		uint8_t buf[FS_BLOCK_SIZE];
//		inode_t *ino_ptr = CAST_TO_INODE(buf);
//		VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(), buf));
//		PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//		PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//	}
	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
//	PRINT("\nverified inode0 direct 0");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
		VERIFY(IS_ADDR_FREE(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
					    fs_buffer,
					    TID_EMPTY,
					    0,
					    IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
					    fs_buffer,
					    TID_EMPTY,
					    DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
					    IS_CACHED_ADDR(log_addr)));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}
//	printBlock(fs_buffer);
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));

	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",calcTotalFreePages());
	VERIFY(COMPARE(free_pages, calcTotalFreePages()));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink a non-existant file in root directory. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest2(){
	uint8_t *f_name = "/file1.dat";
	user_id uid = 1;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	VERIFY(IS_NEGATIVE(unlink(f_name)));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink an existing file not in root directory. shoudl succeed
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest3(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir2);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	uint8_t *f_full_name = "/directory1/directory2/file1.dat";
	int32_t i, fd;
	user_id user = 1;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_dir2);
	init_logical_address(log_addr_dir1);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* write directory2 inode*/
	VERIFY(!writeNewInode(3, 2, FTYPE_DIR, log_addr_dir2));

	/* write directory1 inode*/
	VERIFY(!writeNewInode(2, 1, FTYPE_DIR, log_addr_dir1));

	/* write directory2 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_dir2,
					    fs_buffer,
					    TID_EMPTY,
					    0,
					    IS_CACHED_ADDR(log_addr_dir2)));
	VERIFY(!allocAndWriteBlock(log_addr_dir2, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory2 direntry in directory1*/
	VERIFY(!readFileBlock(fs_buffer, 2, INODE_FILE_DATA_SIZE, log_addr_dir1, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr)){
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}

	setNewDirentry(dirent_ptr, 3, FTYPE_DIR, "directory2");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_dir1,
					    fs_buffer,
					    TID_EMPTY,
					    0,
					    IS_CACHED_ADDR(log_addr_dir1)));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_dir1, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 direntry in root*/
	VERIFY(!readFileBlock(fs_buffer, 1, INODE_FILE_DATA_SIZE, log_addr_root, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 2, FTYPE_DIR, "directory1");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_root,
					    fs_buffer,
					    TID_EMPTY,
					    0,
					    IS_CACHED_ADDR(log_addr_root)));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));


	/* write inode0 with pointers to the new root address*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));

	MARK_BLOCK_NO_HOLE(log_addr_root);
	MARK_BLOCK_NO_HOLE(log_addr_dir1);
	MARK_BLOCK_NO_HOLE(log_addr_dir2);

	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	INODE_SET_DIRECT(ino_ptr,1,log_addr_dir1);
	INODE_SET_DIRECT(ino_ptr,2,log_addr_dir2);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+3*FS_BLOCK_SIZE));
	INODE_SET_NBLOCKS(ino_ptr, 4);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	/* initialize all indirect caches (necessay - blocks allocated abnormally)*/
//	init_indirect_caches();

	/* create file*/
//	PRINT("\ncreat ");
	fd = creat(f_full_name,0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");

	/* close file*/
	VERIFY(!close(fd));
	/* unlink file*/
	VERIFY(!unlink(f_full_name));
//	PRINT("\nunlink success");
	/* verify inode was indeed erased */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	INODE_GET_DIRECT(ino_ptr, 3, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

	/* verify direntry was indeed erased */
	INODE_GET_DIRECT(ino_ptr, 2, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
	VERIFY(!fsReadBlock(log_addr,
					    fs_buffer,
					    TID_EMPTY,
					    DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES+2,
					    IS_CACHED_ADDR(log_addr)));
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
					    fs_buffer,
					    TID_EMPTY,
					    DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
					    IS_CACHED_ADDR(log_addr)));

	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(dirent_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(dirent_ptr));
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}
//	printBlock(fs_buffer);
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(dirent_ptr));

	VERIFY(IS_DIRENT_EMPTY(dirent_ptr));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink a non-existant file not in root directory. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest4(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir2);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	uint8_t *f_full_name = "/directory1/directory2/file1.dat";
	user_id user = 1;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_dir2);
	init_logical_address(log_addr_dir1);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* write directory2 inode*/
	VERIFY(!writeNewInode(3, 2, FTYPE_DIR, log_addr_dir2));

	/* write directory1 inode*/
	VERIFY(!writeNewInode(2, 1, FTYPE_DIR, log_addr_dir1));

	/* write directory2 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_dir2,
					    fs_buffer,
					    TID_EMPTY,
					    0,
					    IS_CACHED_ADDR(log_addr_dir2)));
	VERIFY(!allocAndWriteBlock(log_addr_dir2, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory2 direntry in directory1*/
	VERIFY(!readFileBlock(fs_buffer, 2, INODE_FILE_DATA_SIZE, log_addr_dir1, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr)){
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}

	setNewDirentry(dirent_ptr, 3, FTYPE_DIR, "directory2");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_dir1,
					    fs_buffer,
					    TID_EMPTY,
					    0,
					    IS_CACHED_ADDR(log_addr_dir1)));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_dir1, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 direntry in root*/
	VERIFY(!readFileBlock(fs_buffer, 1, INODE_FILE_DATA_SIZE, log_addr_root, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

//	setNewDirentry(dirent_ptr, 2, FTYPE_DIR, "directory1");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_root,
					    fs_buffer,
					    TID_EMPTY,
					    0,
					    IS_CACHED_ADDR(log_addr_root)));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write inode0 with pointers to the new root address*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));

	MARK_BLOCK_NO_HOLE(log_addr_root);
	MARK_BLOCK_NO_HOLE(log_addr_dir1);
	MARK_BLOCK_NO_HOLE(log_addr_dir2);

	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	INODE_SET_DIRECT(ino_ptr,1,log_addr_dir1);
	INODE_SET_DIRECT(ino_ptr,2,log_addr_dir2);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+3*FS_BLOCK_SIZE));
	INODE_SET_NBLOCKS(ino_ptr, 4);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* unlink file*/
	VERIFY(IS_NEGATIVE(unlink(f_full_name)));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink a file open for reading/writing. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest5(){
	int32_t fd, rec_address;
	uint8_t *f_name = "/file1.dat";
	user_id uid = 1;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	/* create file*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	rec_address = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* unlink*/
	VERIFY(IS_NEGATIVE(unlink(f_name)));

	VERIFY(COMPARE(rec_address, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	VERIFY(!close(fd));

	fd = open(f_name, NANDFS_O_RDWR, 0);
	VERIFY(!IS_NEGATIVE(fd));

	rec_address = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* unlink*/
	VERIFY(IS_NEGATIVE(unlink(f_name)));

	VERIFY(COMPARE(rec_address, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink a file whose parent directory is open for reading/writing
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest6(){
	int32_t fd, rec_address;
	uint8_t *f_name = "/file1.dat";
	user_id uid = 1;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	/* create file*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	/* mock open root directory (file id=1)*/
	OPEN_FILE_SET_FLAGS(0, NANDFS_O_RDONLY);
	OPEN_FILE_SET_FTYPE(0, FTYPE_DIR);
	OPEN_FILE_SET_OFFSET(0, 0);
	OPEN_FILE_SET_UID(0, uid);
	OPEN_FILE_SET_VNODE(0, 0);

	VNODE_SET_INO_NUM(0, 1);
	VNODE_SET_NREFS(0, 1);

	rec_address = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	/* unlink*/
	VERIFY(IS_NEGATIVE(unlink(f_name)));
	VERIFY(COMPARE(rec_address, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink a file where one of it's parent directories is open for reading/writing. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest7(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir2);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	uint8_t *f_full_name = "/directory1/directory2/file1.dat";
	int32_t fd;
	user_id uid = 1;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_dir2);
	init_logical_address(log_addr_dir1);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	/* write directory2 inode*/
	VERIFY(!writeNewInode(3, 2, FTYPE_DIR, log_addr_dir2));

	/* write directory1 inode*/
	VERIFY(!writeNewInode(2, 1, FTYPE_DIR, log_addr_dir1));

	/* write directory2 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_dir2,
					    fs_buffer,
					    TID_EMPTY,
					    0,
					    IS_CACHED_ADDR(log_addr_dir2)));
	VERIFY(!allocAndWriteBlock(log_addr_dir2, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory2 direntry in directory1*/
	VERIFY(!readFileBlock(fs_buffer, 2, INODE_FILE_DATA_SIZE, log_addr_dir1, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr)){
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}

	setNewDirentry(dirent_ptr, 3, FTYPE_DIR, "directory2");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_dir1,
					    fs_buffer,
					    TID_EMPTY,
					    0,
					    IS_CACHED_ADDR(log_addr_dir1)));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_dir1, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 direntry in root*/
	VERIFY(!readFileBlock(fs_buffer, 1, INODE_FILE_DATA_SIZE, log_addr_root, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 2, FTYPE_DIR, "directory1");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_root,
					    fs_buffer,
					    TID_EMPTY,
					    0,
					    IS_CACHED_ADDR(log_addr_root)));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write inode0 with pointers to the new root address*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));

	MARK_BLOCK_NO_HOLE(log_addr_root);
	MARK_BLOCK_NO_HOLE(log_addr_dir1);
	MARK_BLOCK_NO_HOLE(log_addr_dir2);

	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	INODE_SET_DIRECT(ino_ptr,1,log_addr_dir1);
	INODE_SET_DIRECT(ino_ptr,2,log_addr_dir2);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+3*FS_BLOCK_SIZE));
	INODE_SET_NBLOCKS(ino_ptr, 4);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	/* initialize all indirect caches (necessay - blocks allocated abnormally)*/
//	init_indirect_caches();

	/* create file*/
//	PRINT("\ncreat ");
	fd = creat(f_full_name,0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");

	/* close file*/
	VERIFY(!close(fd));

	/* mock open directory1 (file id=2)*/
	OPEN_FILE_SET_FLAGS(0, NANDFS_O_RDONLY);
	OPEN_FILE_SET_FTYPE(0, FTYPE_DIR);
	OPEN_FILE_SET_OFFSET(0, 0);
	OPEN_FILE_SET_UID(0, uid);
	OPEN_FILE_SET_VNODE(0, 0);

	VNODE_SET_INO_NUM(0, 2);
	VNODE_SET_NREFS(0, 1);

	/* unlink*/
	VERIFY(!unlink(f_full_name));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink a file, and re-creat it. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest8(){
	int32_t fd, i;
	uint8_t *f_name = "/file1.dat";
	user_id uid = 1;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(file_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	/* create file*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	/* unlink*/
	VERIFY(!unlink(f_name));

	/* create file*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));
//	PRINT("\nre-creat success");

	/* verify empty file was created with same id and offset in inode0*/
	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!IS_ADDR_EMPTY(root_addr));
//	PRINT("\nverified root netry ");
	INODE_GET_DIRECT(ino_ptr, 1, file_addr);
	VERIFY(!IS_ADDR_EMPTY(file_addr));
//	PRINT("\nverified file entry");


	for(i=2; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}
//	printBlock(fs_buffer);
//	PRINT_MSG_AND_NUM("\nlast direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));
	VERIFY(COMPARE(DIRENT_GET_INO_NUM(de_ptr), 2));

	/* verify file inode */
	VERIFY(!fsReadBlock(file_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(file_addr)));
	VERIFY(verifyInode(ino_ptr, 2, FTYPE_FILE, 1, 0));
//	PRINT("\nverified file inode");

	for(i=0; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified file directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink file with inode file data, and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest9(){
	int32_t fd, free_pages, i;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'v';
	user_id uid = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);

	init_logical_address(log_addr);
	init_logical_address(root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	free_pages = FS_TOTAL_FREE_PAGES;
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	fsMemset(buf, byte, FS_BLOCK_SIZE);

	/* write to file and close */
	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, buf, INODE_FILE_DATA_SIZE)));
	VERIFY(!close(fd));

//	PRINT("\nabout to unlink");
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
//	PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

//	{
//		uint8_t buf[FS_BLOCK_SIZE];
//		inode_t *ino_ptr = CAST_TO_INODE(buf);
//		VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(), buf));
//		PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//		PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//	}
	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
//	PRINT("\nverified inode0 direct 0");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
		VERIFY(IS_ADDR_FREE(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}
//	printBlock(fs_buffer);
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));

	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",calcTotalFreePages());
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink file with indirect entries, and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest10(){
	int32_t fd, free_pages, i, offset = 11*FS_BLOCK_SIZE;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'd';
	user_id uid = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);
	int32_t after_creat_frees;

	init_logical_address(log_addr);
	init_logical_address(root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	free_pages = FS_TOTAL_FREE_PAGES;
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	fsMemset(buf, byte, FS_BLOCK_SIZE);

	after_creat_frees = FS_TOTAL_FREE_PAGES;
//	L("after creat() free pages %d (obs %d, frees %d, dirtys %d)", FS_TOTAL_FREE_PAGES,
//																   GET_OBS_COUNT(),
//																   GET_FREE_COUNTER(),
//																   CACHE_GET_DIRTY_COUNT());
	/* only wrote inode, so that's all tha should have changed*/
	VERIFY(COMPARE(free_pages-1, after_creat_frees));

	/* write to file and close */
	//	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, buf, INODE_FILE_DATA_SIZE)));
//	L("nstart writes");
//	PRINT("\n\n");
	for(i=0; i < offset; i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	}

//	L("b4 close() free pages %d (obs %d, frees %d, dirtys %d)", FS_TOTAL_FREE_PAGES,
//															    GET_OBS_COUNT(),
//															    GET_FREE_COUNTER(),
//															    CACHE_GET_DIRTY_COUNT());
//	PRINT("\n\n");
	VERIFY(!close(fd));
//	PRINT("\n\n");
//	L("after close() free pages %d (obs %d, frees %d, dirtys %d)", FS_TOTAL_FREE_PAGES,
//																   GET_OBS_COUNT(),
//																   GET_FREE_COUNTER(),
//																   CACHE_GET_DIRTY_COUNT());
//	L("expected %d", after_creat_frees - 2);
	/* verify only free pages difference is the new blocks written*/
	VERIFY(COMPARE(after_creat_frees - 12, FS_TOTAL_FREE_PAGES));

//	PRINT("\nabout to unlink");
//	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
//	PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	VERIFY(!unlink(f_name));
//	L("after unlink() free pages %d (obs %d, frees %d, dirtys %d)", FS_TOTAL_FREE_PAGES,
//																	   GET_OBS_COUNT(),
//																	   GET_FREE_COUNTER(),
//																	   CACHE_GET_DIRTY_COUNT());
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
//	PRINT("\nverified inode0 direct 0");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
		VERIFY(IS_ADDR_FREE(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}
//	printBlock(fs_buffer);
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));
	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	/* verify free pages count is as before file creation*/
	L("b4 comparison free pages %d (obs %d, frees %d, dirtys %d)", FS_TOTAL_FREE_PAGES,
																	   GET_OBS_COUNT(),
																	   GET_FREE_COUNTER(),
																	   CACHE_GET_DIRTY_COUNT());
	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
	PRINT_MSG_AND_NUM("\nfree_pages=",FS_TOTAL_FREE_PAGES);
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));

	return 1;
}

/**
 * @brief
 * unlink file with double entries, and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest11(){
	int32_t fd, free_pages, i, offset = DOUBLE_DATA_OFFSET+FS_BLOCK_SIZE;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'v';
	user_id uid = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);

	init_logical_address(log_addr);
	init_logical_address(root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	free_pages = FS_TOTAL_FREE_PAGES;
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	fsMemset(buf, byte, FS_BLOCK_SIZE);

	/* write to file and close */
	for(i=0; i < offset; i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	}
	PRINT("\n\n\n")
	VERIFY(!close(fd));
	PRINT("\n\n\n")
	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

//	PRINT("\nabout to unlink");
//	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
//	PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");

	/* verify free pages count is as before file creation*/
	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
	PRINT_MSG_AND_NUM("\nfree_pages=",FS_TOTAL_FREE_PAGES);
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

//	{
//		uint8_t buf[FS_BLOCK_SIZE];
//		inode_t *ino_ptr = CAST_TO_INODE(buf);
//		VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(), buf));
//		PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//		PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//	}
	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
//	PRINT("\nverified inode0 direct 0");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
		VERIFY(IS_ADDR_FREE(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}
//	printBlock(fs_buffer);
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));

	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink file with triple entries, and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest12(){
	int32_t fd, free_pages, i, offset = TRIPLE_DATA_OFFSET+INODE_DOUBLE_DATA_SIZE*1+FS_BLOCK_SIZE*5;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'v';
	user_id uid = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);

	if(FS_MAX_FILESIZE < TRIPLE_DATA_OFFSET){
		return 1;
	}

	init_logical_address(log_addr);
	init_logical_address(root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	free_pages = FS_TOTAL_FREE_PAGES;
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	fsMemset(buf, byte, FS_BLOCK_SIZE);

	VERIFY(COMPARE(INODE_FILE_DATA_SIZE, write(fd, buf, INODE_FILE_DATA_SIZE)));
	/* write to file and close */
	for(i=INODE_FILE_DATA_SIZE; i < offset; i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	}
//	PRINT("\nwrites success");
	VERIFY(!close(fd));

//	PRINT("\nabout to unlink");
//	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
//	PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

//	{
//		uint8_t buf[FS_BLOCK_SIZE];
//		inode_t *ino_ptr = CAST_TO_INODE(buf);
//		VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(), buf));
//		PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//		PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//	}
	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
//	PRINT("\nverified inode0 direct 0");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
		VERIFY(IS_ADDR_FREE(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}
//	printBlock(fs_buffer);
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));

	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",calcTotalFreePages());
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink file whose direntry is the only directory entry in it's block.
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest13(){
	int32_t fd, free_pages, i;
	uint8_t f_name[1000] = "/file11111111111111111111111111111111111111111111111111111111111111111111111111111111.dat";
	user_id uid = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);

	init_logical_address(log_addr);
	init_logical_address(root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	/* allocate files until we allocate direntry in second direct block of root directory */
	while(1){
		fd = creat(f_name, 0);
		VERIFY(!IS_NEGATIVE(fd));
//		PRINT("\ncreat success");
		VERIFY(!close(fd));
//		PRINT("\nclose success");
		f_name[5] += 1;
//		PRINT("\nname change success");
		VERIFY(!getInode(fs_buffer, 1, root_addr));
//		PRINT("\nname change success");
		init_logical_address(root_addr);
		INODE_GET_DIRECT(ino_ptr, 0, log_addr);
		if(!IS_ADDR_FREE(log_addr)){
			break;
		}
	}
//	PRINT("\nfirst creats success");
	free_pages = FS_TOTAL_FREE_PAGES;

	/* creat last file */
//	PRINT("\n\n\n\ndo last creats");
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	/* verify write to root's second block of direntries */
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	init_logical_address(root_addr);
	INODE_GET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES+1,
						IS_CACHED_ADDR(log_addr)));
//	PRINT_MSG_AND_NUM("\n read second directory direct block from ", logicalAddressToPhysical(log_addr));
//	PRINT_MSG_AND_STR(". dirent name=", DIRENT_GET_NAME(de_ptr));
//	PRINT_MSG_AND_NUM(". ino=", DIRENT_GET_INO_NUM(de_ptr));
	VERIFY(!fsStrcmp(&(f_name[1]), DIRENT_GET_NAME(de_ptr)));

	/* unlink file */
//	PRINT("\n\n\n\nabout to unlink");
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");
	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify root inode*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 3, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
//	PRINT("\nverified root inode");

	/* verify direntry block is erased (and all other direct blocks remain empty) */
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified root dir");

	/* verify free pages count is as before file creation + 1 block of direntries*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",FS_TOTAL_FREE_PAGES);
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink file which resides in indirect entry in inode0
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest14(){
	int32_t fd, free_pages, i;
	uint8_t *f_name = "/file1.dat";
	user_id uid = 1;
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(flags);

	init_logical_address(log_addr);
	init_logical_address(indirect_addr);
	init_logical_address(root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	/* make all direct entries in inode0 as taken, and change size */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	MARK_BLOCK_NO_HOLE(log_addr);
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i, log_addr);
	}

	INODE_SET_NBYTES(ino_ptr, INDIRECT_DATA_OFFSET);
	INODE_SET_NBLOCKS(ino_ptr, 1+DIRECT_INDEX_ENTRIES);
	/* re-write inode0*/
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	free_pages = FS_TOTAL_FREE_PAGES;

	/* create file. should be created in indirect offset*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	/* save inode0 indirect block addrees, so later we can verify it is obsolete*/
//	PRINT("\nabout to unlink");
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	INODE_GET_INDIRECT(ino_ptr, indirect_addr);
//	PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));

	/* unlink file. should delete inode, and indiret block leading to it*/
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 2+DIRECT_INDEX_ENTRIES, INDIRECT_DATA_OFFSET+FS_BLOCK_SIZE));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
//		PRINT_MSG_AND_NUM("\nverify direct ", i);
//		PRINT_MSG_AND_HEX("=", *CAST_TO_UINT32(log_addr));
		VERIFY(IS_ADDR_EMPTY(log_addr));
//		PRINT(". empty");
		VERIFY(!IS_ADDR_FREE(log_addr));
//		PRINT(". not free");
	}
//	PRINT("\ninode0 direct verified");

	/* verify indirect is empty and free */
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	L("indirect addr in inode0 is %x", ADDR_PRINT(log_addr));
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));

	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}
//	printBlock(fs_buffer);
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));

	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",FS_TOTAL_FREE_PAGES);
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));

//	PRINT("\nfrees success");
	/* verify old indirect block is now obsolete*/
//	PRINT_MSG_AND_HEX("\nread old indirect=", *CAST_TO_UINT32(indirect_addr));
	nandReadPageFlags(flags, logicalAddressToPhysical(indirect_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
//	printBlock(fs_buffer);

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink file whose direntry resides in indirect entry in parent directory
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest15(){
	int32_t fd, free_pages, i;
	uint8_t *f_name = "/file1.dat";
	user_id uid = 1;
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(entries_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_ino0_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(flags);

	init_logical_address(log_addr);
	init_logical_address(indirect_addr);
	init_logical_address(root_addr);
	init_logical_address(file_addr);
	init_logical_address(entries_addr);
	init_logical_address(old_ino0_addr);
	init_logical_address(old_root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	/* mark all direct entries in rootinode0 as taken, and change size */
	VERIFY(!getInode(fs_buffer, 1, root_addr));

	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	MARK_BLOCK_NO_HOLE(log_addr);
	for(i=0; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i, log_addr);
	}
	init_logical_address(log_addr);

	INODE_SET_NBYTES(ino_ptr, INDIRECT_DATA_OFFSET);
	INODE_SET_NBLOCKS(ino_ptr, 1+DIRECT_INDEX_ENTRIES);

	/* re-write root directory */
	VERIFY(!allocAndWriteBlock(root_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_NO_HOLE(root_addr);

	/* re-write inode0*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	INODE_SET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	free_pages = FS_TOTAL_FREE_PAGES;

	/* create file. should be created in indirect offset*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT_MSG_AND_NUM("\nfile id=", GET_FILE_ID_BY_FD(fd));
	VERIFY(!close(fd));
	VERIFY(!getInode(fs_buffer, 1, file_addr));

//	assert(0);

	/* save root indirect block addrees, so later we can verify it is obsolete*/
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	INODE_GET_INDIRECT(ino_ptr, indirect_addr);
	VERIFY(!fsReadBlock(indirect_addr,
						fs_buffer,
						TID_EMPTY,
						INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(indirect_addr)));
	BLOCK_GET_INDEX(fs_buffer,0,entries_addr);
	copyLogicalAddress(old_root_addr, root_addr);
	copyLogicalAddress(old_ino0_addr, FS_GET_INO0_ADDR_PTR());

//	PRINT("\n\n\nabout to unlink");
	/* unlink file. should delete inode, and indiret block leading to it*/
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
//		PRINT_MSG_AND_NUM("\nverify direct ", i);
//		PRINT_MSG_AND_HEX("=", *CAST_TO_UINT32(log_addr));
		VERIFY(IS_ADDR_EMPTY(log_addr));
//		PRINT(". empty");
		VERIFY(IS_ADDR_FREE(log_addr));
//		PRINT(". not free");
	}
//	PRINT("\ninode0 direct verified");

	/* verify indirect in empty and free */
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));

	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2+DIRECT_INDEX_ENTRIES, INDIRECT_DATA_OFFSET+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=0; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(!IS_ADDR_FREE(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));

	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));

	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));
//	PRINT("\nverified root dir");

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",FS_TOTAL_FREE_PAGES);
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));
//	PRINT("\nfrees success");

	/* verify old addresses is now obsolete*/
//	PRINT_MSG_AND_HEX("\nread old indirect=", *CAST_TO_UINT32(indirect_addr));
	nandReadPageFlags(flags, logicalAddressToPhysical(indirect_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(entries_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(file_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(old_ino0_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(old_root_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
//	printBlock(fs_buffer);

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink file which resides in double entry in inode0
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest16(){
	int32_t fd, free_pages, i;
	uint8_t *f_name = "/file1.dat";
	user_id uid = 1;
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(flags);

	init_logical_address(log_addr);
	init_logical_address(indirect_addr);
	init_logical_address(double_addr);
	init_logical_address(root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);
//	SET_LOGICAL_OFFSET(log_addr,0);
//	SET_LOGICAL_SEGMENT(log_addr,0);
	/* makr all direct entries in inode0 as taken, and change size */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	MARK_BLOCK_NO_HOLE(log_addr);
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i, log_addr);
	}

	INODE_SET_INDIRECT(ino_ptr, log_addr);
	INODE_SET_NBYTES(ino_ptr, DOUBLE_DATA_OFFSET);
	INODE_SET_NBLOCKS(ino_ptr, (uint32_t)(1+(DOUBLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE));
//	init_logical_address(log_addr);

	/* re-write inode0*/
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	free_pages = FS_TOTAL_FREE_PAGES;

	/* create file. should be created in indirect offset*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	/* save inode0 indirect block addrees, so later we can verify it is obsolete*/
//	PRINT("\nabout to unlink");
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
//	PRINT_MSG_AND_NUM("\nafter creat, inode0 nblocks=",INODE_GET_NBLOCKS(ino_ptr));
	INODE_GET_DOUBLE(ino_ptr, double_addr);
	VERIFY(!fsReadBlock(double_addr,
						fs_buffer,
						TID_EMPTY,
						DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(double_addr)));
	BLOCK_GET_INDEX(fs_buffer,0, indirect_addr);

//	assert(0);
//	PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));

	/* unlink file. should delete inode, and indiret block leading to it*/
//	PRINT("\n\n\n\nunlink");
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 2+(DOUBLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE, DOUBLE_DATA_OFFSET+FS_BLOCK_SIZE));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
//		PRINT_MSG_AND_NUM("\nverify direct ", i);
//		PRINT_MSG_AND_HEX("=", *CAST_TO_UINT32(log_addr));
		VERIFY(IS_ADDR_EMPTY(log_addr));
//		PRINT(". empty");
		VERIFY(!IS_ADDR_FREE(log_addr));
//		PRINT(". not free");
	}
//	PRINT("\ninode0 direct verified");

	/* verify indirect in empty and free */
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\ninode0 indirect verified");

	INODE_GET_DOUBLE(ino_ptr, log_addr);
//	PRINT_MSG_AND_HEX("\ninode0 doublet=", *CAST_TO_UINT32(log_addr));
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));
//	PRINT("\ninode0 double verified");

	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}
//	printBlock(fs_buffer);
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));

	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",FS_TOTAL_FREE_PAGES);
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));

//	PRINT("\nfrees success");
	/* verify old double and indirect blocks are now obsolete*/
//	PRINT_MSG_AND_HEX("\nread old indirect=", *CAST_TO_UINT32(indirect_addr));
	nandReadPageFlags(flags, logicalAddressToPhysical(indirect_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(double_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
//	printBlock(fs_buffer);

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink file whose direntry which resides in double entry in parent directory
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest17(){
	int32_t fd, free_pages, i;
	uint8_t *f_name = "/file1.dat", indirect_buf[FS_BLOCK_SIZE];
	user_id uid = 1;
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(mock_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(entries_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_ino0_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(flags);

	init_logical_address(log_addr);
	init_logical_address(mock_addr);
	init_logical_address(indirect_addr);
	init_logical_address(double_addr);
	init_logical_address(root_addr);
	init_logical_address(file_addr);
	init_logical_address(entries_addr);
	init_logical_address(old_ino0_addr);
	init_logical_address(old_root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);


	/* mark all direct entries in rootinode0 as taken, and change size */
	VERIFY(!getInode(fs_buffer, 1, root_addr));

	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	MARK_BLOCK_NO_HOLE(log_addr);
	for(i=0; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i, log_addr);
	}
	init_logical_address(log_addr);

	/* write fake indirect block*/
	init_fsbuf(indirect_buf);
	VERIFY(!allocAndWriteBlock(mock_addr, indirect_buf, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_NO_HOLE(mock_addr);
	INODE_SET_INDIRECT(ino_ptr, mock_addr);

	INODE_SET_NBYTES(ino_ptr, DOUBLE_DATA_OFFSET);
	INODE_SET_NBLOCKS(ino_ptr, (uint32_t)(1+((DOUBLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE) ));

	/* re-write root directory */
	VERIFY(!allocAndWriteBlock(root_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_NO_HOLE(root_addr);

	/* re-write inode0*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	INODE_SET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	free_pages = FS_TOTAL_FREE_PAGES;

	/* create file. should be created in indirect offset*/
//	PRINT("\ncreate file");
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT_MSG_AND_NUM("\nfile id=", GET_FILE_ID_BY_FD(fd));
	VERIFY(!close(fd));
	VERIFY(!getInode(fs_buffer, 1, file_addr));

//	assert(0);

	/* save root indirect block addrees, so later we can verify it is obsolete*/
//	PRINT("\n\n\nabout to unlink");
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	INODE_GET_DOUBLE(ino_ptr, double_addr);
	VERIFY(!fsReadBlock(double_addr,
						fs_buffer,
						TID_EMPTY,
						DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(double_addr)));
	BLOCK_GET_INDEX(fs_buffer,0,indirect_addr);
	VERIFY(!fsReadBlock(indirect_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(indirect_addr)));
	BLOCK_GET_INDEX(fs_buffer,0,entries_addr);
	copyLogicalAddress(old_root_addr, root_addr);
	copyLogicalAddress(old_ino0_addr, FS_GET_INO0_ADDR_PTR());

	/* unlink file. should delete inode, and indiret block leading to it*/
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
//		PRINT_MSG_AND_NUM("\nverify direct ", i);
//		PRINT_MSG_AND_HEX("=", *CAST_TO_UINT32(log_addr));
		VERIFY(IS_ADDR_EMPTY(log_addr));
//		PRINT(". empty");
		VERIFY(IS_ADDR_FREE(log_addr));
//		PRINT(". not free");
	}
//	PRINT("\ninode0 direct verified");

	/* verify indirect in empty and free */
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));

	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2+((DOUBLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE), DOUBLE_DATA_OFFSET+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=0; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(!IS_ADDR_FREE(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(!IS_ADDR_FREE(log_addr));

	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));

	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));
//	PRINT("\nverified root dir");

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",FS_TOTAL_FREE_PAGES);
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));
//	PRINT("\nfrees success");

	/* verify old addresses is now obsolete*/
	nandReadPageFlags(flags, logicalAddressToPhysical(double_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(indirect_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(entries_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(file_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(old_ino0_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(old_root_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
//	printBlock(fs_buffer);

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink file which resides in triple entry in inode0
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest18(){
	int32_t fd, free_pages, i, nBlocks = (TRIPLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE;
	uint8_t *f_name = "/file1.dat";
	user_id uid = 1;
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(triple_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(flags);

	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < TRIPLE_DATA_OFFSET){
		return 1;
	}

	init_logical_address(log_addr);
	init_logical_address(indirect_addr);
	init_logical_address(double_addr);
	init_logical_address(triple_addr);
	init_logical_address(root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);
//	SET_LOGICAL_OFFSET(log_addr,0);
//	SET_LOGICAL_SEGMENT(log_addr,0);

	/* make all direct entries in inode0 as taken, and change size */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	MARK_BLOCK_NO_HOLE(log_addr);
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i, log_addr);
	}

	INODE_SET_INDIRECT(ino_ptr, log_addr);
	INODE_SET_DOUBLE(ino_ptr, log_addr);
	INODE_SET_NBYTES(ino_ptr, TRIPLE_DATA_OFFSET);
	INODE_SET_NBLOCKS(ino_ptr, 1+nBlocks);
//	init_logical_address(log_addr);

	/* re-write inode0*/
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	free_pages = FS_TOTAL_FREE_PAGES;

	/* create file. should be created in indirect offset*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	/* save inode0 indirect block addrees, so later we can verify it is obsolete*/
//	PRINT("\nabout to unlink");
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
//	PRINT_MSG_AND_NUM("\nafter creat, inode0 nblocks=",INODE_GET_NBLOCKS(ino_ptr));
	INODE_GET_TRIPLE(ino_ptr, triple_addr);
	VERIFY(!fsReadBlock(triple_addr,
						fs_buffer,
						TID_EMPTY,
						TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(triple_addr)));
	BLOCK_GET_INDEX(fs_buffer,0, double_addr);
	VERIFY(!fsReadBlock(double_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(double_addr)));
	BLOCK_GET_INDEX(fs_buffer,0, indirect_addr);

//	assert(0);
//	PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));

	/* unlink file. should delete inode, and indiret block leading to it*/
//	PRINT("\n\n\n\nunlink");
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 2+(TRIPLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE, TRIPLE_DATA_OFFSET+FS_BLOCK_SIZE));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
//		PRINT_MSG_AND_NUM("\nverify direct ", i);
//		PRINT_MSG_AND_HEX("=", *CAST_TO_UINT32(log_addr));
		VERIFY(IS_ADDR_EMPTY(log_addr));
//		PRINT(". empty");
		VERIFY(!IS_ADDR_FREE(log_addr));
//		PRINT(". not free");
	}
//	PRINT("\ninode0 direct verified");

	/* verify indirect in empty and free */
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(!IS_ADDR_FREE(log_addr));
//	PRINT("\ninode0 indirect verified");

	INODE_GET_DOUBLE(ino_ptr, log_addr);
//	PRINT_MSG_AND_HEX("\ninode0 doublet=", *CAST_TO_UINT32(log_addr));
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(!IS_ADDR_FREE(log_addr));
//	PRINT("\ninode0 double verified");

	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified root dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));

	for(i=0; i<2;i++){
//		PRINT_MSG_AND_STR("\nentry name=",DIRENT_GET_NAME(de_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	}
//	printBlock(fs_buffer);
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));

	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",FS_TOTAL_FREE_PAGES);
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));

//	PRINT("\nfrees success");
	/* verify old double and indirect blocks are now obsolete*/
//	PRINT_MSG_AND_HEX("\nread old indirect=", *CAST_TO_UINT32(indirect_addr));
	nandReadPageFlags(flags, logicalAddressToPhysical(indirect_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(double_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(triple_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
//	printBlock(fs_buffer);

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}

/**
 * @brief
 * unlink file whose direntry which resides in triple entry in parent directory
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest19(){
	int32_t fd, free_pages, i, nBlocks = (TRIPLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE;
	uint8_t *f_name = "/file1.dat", entries_buf[FS_BLOCK_SIZE];
	user_id uid = 1;
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(mock_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(triple_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(entries_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_ino0_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_root_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(flags);

	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < TRIPLE_DATA_OFFSET){
		return 1;
	}

	init_logical_address(log_addr);
	init_logical_address(mock_addr);
	init_logical_address(indirect_addr);
	init_logical_address(double_addr);
	init_logical_address(triple_addr);
	init_logical_address(root_addr);
	init_logical_address(file_addr);
	init_logical_address(entries_addr);
	init_logical_address(old_ino0_addr);
	init_logical_address(old_root_addr);

	/* set user in process*/
	SET_CURRENT_USER(uid);

	/* mark all direct entries in rootinode0 as taken, and change size */
	VERIFY(!getInode(fs_buffer, 1, root_addr));

	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	MARK_BLOCK_NO_HOLE(log_addr);
	for(i=0; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i, log_addr);
	}
	init_logical_address(log_addr);

	/* write fake indirect block*/
	init_fsbuf(entries_buf);
	VERIFY(!allocAndWriteBlock(mock_addr, entries_buf, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_NO_HOLE(mock_addr);
	INODE_SET_INDIRECT(ino_ptr, mock_addr);

	/* write fake double block */
	for(i=0; i < LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_SET_INDEX(entries_buf, i, mock_addr);
	}
	VERIFY(!allocAndWriteBlock(mock_addr, entries_buf, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_NO_HOLE(mock_addr);
	INODE_SET_DOUBLE(ino_ptr, mock_addr);

	INODE_SET_NBYTES(ino_ptr, TRIPLE_DATA_OFFSET);
	INODE_SET_NBLOCKS(ino_ptr, 1+nBlocks );

	/* re-write root directory */
	VERIFY(!allocAndWriteBlock(root_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_NO_HOLE(root_addr);

	/* re-write inode0*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	INODE_SET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	free_pages = FS_TOTAL_FREE_PAGES;

	/* create file. should be created in indirect offset*/
//	PRINT("\ncreate file");
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT_MSG_AND_NUM("\nfile id=", GET_FILE_ID_BY_FD(fd));
	VERIFY(!close(fd));
	VERIFY(!getInode(fs_buffer, 1, file_addr));

//	assert(0);

	/* save root indirect block addrees, so later we can verify it is obsolete*/
//	PRINT("\n\n\nabout to unlink");
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	INODE_GET_TRIPLE(ino_ptr, triple_addr);
	VERIFY(!fsReadBlock(triple_addr,
						fs_buffer,
						TID_EMPTY,
						TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(triple_addr)));
	BLOCK_GET_INDEX(fs_buffer,0,double_addr);
	VERIFY(!fsReadBlock(double_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(double_addr)));
	BLOCK_GET_INDEX(fs_buffer,0,indirect_addr);
	VERIFY(!fsReadBlock(indirect_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(indirect_addr)));
	BLOCK_GET_INDEX(fs_buffer,0,entries_addr);
	copyLogicalAddress(old_root_addr, root_addr);
	copyLogicalAddress(old_ino0_addr, FS_GET_INO0_ADDR_PTR());

	/* unlink file. should delete inode, and indiret block leading to it*/
	VERIFY(!unlink(f_name));
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify inode0 */
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						FLAG_CACHEABLE_READ_NO));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
//		PRINT_MSG_AND_NUM("\nverify direct ", i);
//		PRINT_MSG_AND_HEX("=", *CAST_TO_UINT32(log_addr));
		VERIFY(IS_ADDR_EMPTY(log_addr));
//		PRINT(". empty");
		VERIFY(IS_ADDR_FREE(log_addr));
//		PRINT(". not free");
	}
//	PRINT("\ninode0 direct verified");

	/* verify indirect in empty and free */
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified inode0");

	/* verify root inode*/
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2+((TRIPLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE), TRIPLE_DATA_OFFSET+FS_BLOCK_SIZE));
//	PRINT("\nverified root inode");

	for(i=0; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(!IS_ADDR_FREE(log_addr));
	}
//	PRINT("\nverified root directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(!IS_ADDR_FREE(log_addr));

	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(!IS_ADDR_FREE(log_addr));

	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));
//	PRINT("\nverified root dir");

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",FS_TOTAL_FREE_PAGES);
	VERIFY(COMPARE(free_pages, FS_TOTAL_FREE_PAGES));
//	PRINT("\nfrees success");

	/* verify old addresses is now obsolete*/
	nandReadPageFlags(flags, logicalAddressToPhysical(triple_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(double_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(indirect_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(entries_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(file_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(old_ino0_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
	nandReadPageFlags(flags, logicalAddressToPhysical(old_root_addr));
	VERIFY(COMPARE(GET_OBS_FLAG(flags), OBS_FLAG_TRUE));
//	printBlock(fs_buffer);

	/* verify no dirty caches*/
	VERIFY(verify_no_dirty_caches());

	return 1;
}
