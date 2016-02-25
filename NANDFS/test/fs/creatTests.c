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

/** @file creatTests.c  */
#include <test/fs/creatTests.h>
#include <test/fs/testsHeader.h>

void runAllCreatTests(){

	RUN_TEST(creat,1);
	RUN_TEST(creat,2);
	RUN_TEST(creat,3);
	RUN_TEST(creat,4);
	RUN_TEST(creat,6);
	RUN_TEST(creat,8);
	RUN_TEST(creat,9);
	RUN_TEST(creat,7);
	RUN_TEST(creat,10);
	RUN_TEST(creat,11);
	RUN_TEST(creat,20);
	RUN_TEST(creat,21);
	RUN_TEST(creat,22);

}


/**
 * @brief
 * init creat test
 *
 */
void init_creatTest(){
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
 * tear down creat test
 *
 */
void tearDown_creatTest(){
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
 * create a file in root directory, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest1(){
	uint8_t *f_name = "/file1.dat";
	int32_t i, fd;
	user_id user = 1;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(file_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);
//	PRINT_MSG_AND_NUM("\nb4 creat obs=", GET_OBS_COUNT());

	/******* call create() **********/
	fd = creat(f_name,0);
	VERIFY(!IS_NEGATIVE(fd));
//	L("after creat INO0_ADDR from %x", *((uint32_t*)FS_GET_INO0_ADDR_PTR()));
//	PRINT("\ncreat success");
//	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
//	VERIFY(!getInode(fs_buffer, 2, ino_log_addr));
//	PRINT_MSG_AND_HEX("\nfile size=", INODE_GET_NBYTES(ino_ptr));
//	assert(0);

//	PRINT("\ncreat success");
	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd){
			VERIFY(verifyOpenFileEntry(fd, CREAT_FLAGS, 0, user, 0));
			continue;
		}

		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nfile is open success");
	for(i=0; i<FS_MAX_VNODES;i++){
		if(i==OPEN_FILE_GET_VNODE(fd)){
			VERIFY(verifyVnode(i, 2, 1));
			continue;
		}

		VERIFY(verifyVnodeEmpty(i));
	}

	/* verify cache is empty of all transaction realted blocks*/
	for(i=0; i< FS_CACHE_BUFFERS; i++){
		VERIFY(IS_EMPTY_TID(CACHE_GET_TID(i)));
	}

	/* verify inode0 sizes changed*/
//	L("read INO0_ADDR from %x", ADDR_PRINT(FS_GET_INO0_ADDR_PTR()));
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
//	L("after creat root_addr %x", *((uint32_t*)root_addr));
	INODE_GET_DIRECT(ino_ptr, 1, file_addr);

//	PRINT_MSG_AND_NUM("\nino0 nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM(" nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 3));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 0));
//	PRINT("\nino0 sizes verified");
	/* verify root inode sizes */
//	L("");
	VERIFY(!fsReadBlockSimple(root_addr, fs_buffer));
//	PRINT_MSG_AND_NUM("\nroot nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM(" nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 2));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 1));
//	PRINT("\nroot sizes verified");

	/* verify direntry - read first root address block,
	 * read 3rd direntry, and verify details*/
//	L("");
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
//	L("log_addr %x",*((uint32_t*)log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));
	VERIFY(!IS_CACHED_ADDR(log_addr));
//	PRINT("\nlog_addr free");
//	L("");
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
//	L("");
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
//	L("expected dirent ino num %d, actual one %d", 2, DIRENT_GET_INO_NUM(dirent_ptr));
	VERIFY(COMPARE(DIRENT_GET_INO_NUM(dirent_ptr), 2));
//	PRINT("\ndirent ino num verified");
	VERIFY(COMPARE(DIRENT_GET_TYPE(dirent_ptr), FTYPE_FILE));
//	PRINT("\ndirent type verified");
	VERIFY(COMPARE(DIRENT_GET_NAME_LEN(dirent_ptr), 9));
//	PRINT("\ndirent name len");
	VERIFY(!fsStrcmp(DIRENT_GET_NAME(dirent_ptr), f_name+1));

	/* verify file inode sizes*/
//	L("");
	VERIFY(!fsReadBlockSimple(file_addr, fs_buffer));
//	PRINT_MSG_AND_NUM("\nnblocks=",INODE_GET_NBLOCKS(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1));
//	PRINT("\nnblocks success");
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), 0));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 2));
//	PRINT("\nfile id success");
	VERIFY(COMPARE(INODE_GET_FILE_TYPE(ino_ptr), FTYPE_FILE));
//	PRINT("\nfile type success");
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
//	PRINT("\nfile sizes verified");
//	PRINT_MSG_AND_NUM("\nobs count=",GET_OBS_COUNT());
	/* 1 old ino0, 1 for old dirent page, 1 for old directory inode page, 1 for old inode0, 1 for vots page*/
	VERIFY(COMPARE(5, GET_OBS_COUNT()));
	return 1;
}

/**
 * @brief
 * create a file in root node that already exists, and verify failure
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest2(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c', *f_full_name = "/file1.dat";
	uint32_t old_offset;
	int32_t i, fd;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill buffer data*/
	for(i=0; i< FS_BLOCK_SIZE ; i++){
		fs_buffer[i] = byte;
	}

	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE));
	INODE_SET_NBLOCKS(ino_ptr, 1);

	/* write inode, and link it to root directory and inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* read inode0 for root dir inode, and read it for first entries blocks, and read first direct entry block*/
	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_GET_DIRECT(ino_ptr, 0, log_addr_root);
	readBlock(log_addr_root, fs_buffer);
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	readBlock(log_addr, fs_buffer);

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr)){
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}

	/* set file direntry
	 * NOTICE - f_full_name+1 to skip first '/'*/
	setNewDirentry(dirent_ptr, 2, FTYPE_FILE, f_full_name+1);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write and mark first direntries block in directory as taken*/
	readBlock(log_addr_root, fs_buffer);
	SET_ADDR_FLAG(log_addr_file, ADDR_FLAG_TAKEN);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);

	SET_ADDR_FLAG(log_addr_root, ADDR_FLAG_TAKEN);
	SET_ADDR_FLAG(log_addr_file, ADDR_FLAG_TAKEN);

	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	old_offset = GET_RECLAIMED_OFFSET();
//	PRINT("\ncreate file");
	fd = creat(f_full_name,0);
	VERIFY(IS_NEGATIVE(fd));
//	PRINT("\ncreat successful");
	VERIFY(COMPARE(old_offset, GET_RECLAIMED_OFFSET()));
//	PRINT("\noffset successful");

	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nopenfile entries successful");
	for(i=0; i<FS_MAX_VNODES;i++){
		VERIFY(verifyVnodeEmpty(i));
	}

	return 1;
}

/**
 * @brief
 * create a file not in root, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest3(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
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
	init_logical_address(log_addr_file);
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
	VERIFY(!fsReadBlockSimple(log_addr_dir2, fs_buffer));
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
	VERIFY(!fsReadBlockSimple(log_addr_dir1, fs_buffer));
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
	VERIFY(!fsReadBlockSimple(log_addr_root, fs_buffer));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));


	/* write inode0 with pointers to the new root address*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));

	MARK_BLOCK_NO_HOLE(log_addr_root);
	MARK_BLOCK_NO_HOLE(log_addr_dir1);
	MARK_BLOCK_NO_HOLE(log_addr_dir2);

	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	INODE_SET_DIRECT(ino_ptr,1,log_addr_dir1);
	INODE_SET_DIRECT(ino_ptr,2,log_addr_dir2);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+3*FS_BLOCK_SIZE));
	INODE_SET_NBLOCKS(ino_ptr, 4);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	/* initialize all indirect caches (necessary, since we did not allocate blocks in an orderly manner)*/
//	init_indirect_caches();

	/* do creat*/
	fd = creat(f_full_name,0);

	VERIFY(!IS_NEGATIVE(fd));

	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd){
			verifyOpenFileEntry(fd, CREAT_FLAGS,0,user,0);
			continue;
		}

		VERIFY(verifyOpenFileEntryEmpty(i));
	}

	for(i=0; i<FS_MAX_VNODES;i++){
		if(i==OPEN_FILE_GET_VNODE(fd)){
			verifyVnode(0,4,1);
			continue;
		}

		VERIFY(verifyVnodeEmpty(i));
	}
//	PRINT("\nopen entries and vnodes success");
	/* verify inode0 details*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
//	PRINT_MSG_AND_NUM("\nnbytes=",INODE_GET_NBYTES(ino_ptr));
//	PRINT_MSG_AND_NUM(" nblocks=",INODE_GET_NBLOCKS(ino_ptr));
	INODE_GET_DIRECT(ino_ptr,3,log_addr_file);
	INODE_GET_DIRECT(ino_ptr,2,log_addr_dir2);
	VERIFY(!IS_ADDR_EMPTY(log_addr_file));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr),INODE_FILE_DATA_SIZE+4*FS_BLOCK_SIZE));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr),5));
//	PRINT("\nino0 success");
	/* verify file details */
	VERIFY(!fsReadBlockSimple(log_addr_file, fs_buffer));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr),4));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr),1));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr),0));
//	PRINT("\nfile success");
	/* verify direntry - read first roor address block,
	 * read 3rd direntry, and verify details*/
	VERIFY(!fsReadBlockSimple(log_addr_dir2, fs_buffer));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 2));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\n dir2 verified");

	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
//	PRINT("\n got first block address");
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
//	PRINT_MSG_AND_NUM("\naddress=", logicalAddressToPhysical(log_addr));
//	{
//		int32_t i;
//		for(i=0; i< FS_BLOCK_SIZE; i++){
//			if(fs_buffer[i] !=0xff){
//				PRINT_MSG_AND_NUM("\n", i);
//				PRINT_MSG_AND_NUM(". ", fs_buffer[i]);
//			}
//		}
//	}
	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	for(i=0;i<2;i++){
//		PRINT_MSG_AND_STR("\n dirent name=", DIRENT_GET_NAME(dirent_ptr));
//		PRINT_MSG_AND_NUM(" dirent len=", DIRENT_GET_LEN(dirent_ptr));
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}
//	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
//	PRINT("\n moved to 2nd dirent");
//		PRINT_MSG_AND_NUM(" at location ", CAST_TO_UINT8(dirent_ptr)-fs_buffer);
//	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
//	PRINT("\n moved to 3rd direntry");
//	PRINT_MSG_AND_NUM(" at location ", CAST_TO_UINT8(dirent_ptr)-fs_buffer);
//	PRINT_MSG_AND_NUM("\n ino num=", DIRENT_GET_INO_NUM(dirent_ptr));
	VERIFY(COMPARE(DIRENT_GET_INO_NUM(dirent_ptr), 4));
//	PRINT("\ndirent ino num verified");
	VERIFY(COMPARE(DIRENT_GET_TYPE(dirent_ptr), FTYPE_FILE));
//	PRINT("\ndirent type verified");
	VERIFY(COMPARE(DIRENT_GET_NAME_LEN(dirent_ptr), 9));
//	PRINT("\ndirent name len");
	VERIFY(!fsStrcmp(DIRENT_GET_NAME(dirent_ptr), "file1.dat"));

	return 1;
}

/**
 * @brief
 * create a file not in root node that already exists, and verify afilure
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest4(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir2);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t *f_full_name = "/directory1/directory2/file1.dat";
	int32_t i, fd;
	int32_t old_offset;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_dir2);
	init_logical_address(log_addr_dir1);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);

	/* write file inode */
	VERIFY(!writeNewInode(4, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* write directory2 inode*/
	VERIFY(!writeNewInode(3, 2, FTYPE_DIR, log_addr_dir2));

	/* write directory1 inode*/
	VERIFY(!writeNewInode(2, 1, FTYPE_DIR, log_addr_dir1));


	/* write file1.dat direntry in directory2*/
	VERIFY(!readFileBlock(fs_buffer, 3, INODE_FILE_DATA_SIZE, log_addr_dir2, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 4, FTYPE_FILE, "file1.dat");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory2 inode with new pointer to the above block*/
	VERIFY(!fsReadBlockSimple(log_addr_dir2, fs_buffer));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_dir2, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory2 direntry in directory1*/
	VERIFY(!readFileBlock(fs_buffer, 2, INODE_FILE_DATA_SIZE, log_addr_dir1, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 3, FTYPE_DIR, "directory2");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!fsReadBlockSimple(log_addr_dir1, fs_buffer));
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

	/* write directory1 inode */
	VERIFY(!fsReadBlockSimple(log_addr_root, fs_buffer));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write inode0 with pointers to the new root address*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));

	SET_ADDR_FLAG(log_addr_root, ADDR_FLAG_TAKEN);
	SET_ADDR_FLAG(log_addr_dir1, ADDR_FLAG_TAKEN);
	SET_ADDR_FLAG(log_addr_dir2, ADDR_FLAG_TAKEN);
	SET_ADDR_FLAG(log_addr_file, ADDR_FLAG_TAKEN);

	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	INODE_SET_DIRECT(ino_ptr,1,log_addr_dir1);
	INODE_SET_DIRECT(ino_ptr,2,log_addr_dir2);
	INODE_SET_DIRECT(ino_ptr,3,log_addr_file);

	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+4*FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	old_offset = GET_RECLAIMED_OFFSET();
	fd = creat(f_full_name, 0);
	VERIFY(COMPARE(old_offset, GET_RECLAIMED_OFFSET()));
	VERIFY(IS_NEGATIVE(fd));

	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		VERIFY(verifyOpenFileEntryEmpty(i));
	}

	for(i=0; i<FS_MAX_VNODES;i++){
		VERIFY(verifyVnodeEmpty(i));
	}

	return 1;
}

/**
 * @brief
 * create a file in root node, when we have no free direentries to use and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest6(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_old_ino0);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint32_t long_dir_name_size = DIV_BY_2(FS_BLOCK_SIZE);
	uint8_t long_dir_name[DIV_BY_2(FS_BLOCK_SIZE)+1];
	uint8_t *f_full_name = "/file1.dat";
	int32_t i, fd;
	user_id user = 1;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_old_ino0);
	init_logical_address(log_addr_dir1);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);

	/* set user in process*/
	SET_CURRENT_USER(user);

	/* set directory and file name*/
	for(i=0;i<long_dir_name_size;i++){
		long_dir_name[i]= 'c';
	}
	long_dir_name[i] = '\0';

	/* write directory inode*/
	VERIFY(!writeNewInode(2, 1, FTYPE_DIR, log_addr_dir1));

	/* read first directory direntry block in root*/
	VERIFY(!readFileBlock(fs_buffer, 1, INODE_FILE_DATA_SIZE, log_addr_root, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr)){
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}

	setNewDirentry(dirent_ptr, 2, FTYPE_DIR, long_dir_name);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write root inode with new pointer to the above block*/
	VERIFY(!fsReadBlockSimple(log_addr_root, fs_buffer));
	MARK_BLOCK_NO_HOLE(log_addr);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE))
	INODE_SET_NBLOCKS(ino_ptr, 2);
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write inode0 with pointers to the new root address*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));

	MARK_BLOCK_NO_HOLE(log_addr_root);
	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));
	copyLogicalAddress(log_addr_old_ino0, FS_GET_INO0_ADDR_PTR());

	fd = creat(f_full_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd){
			verifyOpenFileEntry(fd,CREAT_FLAGS,0,user,0);
			continue;
		}

		VERIFY(verifyOpenFileEntryEmpty(i));
	}

	for(i=0; i<FS_MAX_VNODES;i++){
		if(i==OPEN_FILE_GET_VNODE(fd)){
			verifyVnode(0,2,1);
			continue;
		}

		VERIFY(verifyVnodeEmpty(i));
	}

	/* verify ino0 address changed*/
	VERIFY(!COMPARE(GET_LOGICAL_OFFSET(log_addr_old_ino0), GET_LOGICAL_OFFSET(FS_GET_INO0_ADDR_PTR())));
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(log_addr_old_ino0),GET_LOGICAL_SEGMENT(FS_GET_INO0_ADDR_PTR())));

	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_GET_DIRECT(ino_ptr, 1, log_addr_file);

//	PRINT_MSG_AND_NUM("\nino0 nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nino0 nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 3));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 0));
//	PRINT("\nino0 sizes verified");
	/* verify root inode sizes */
	VERIFY(!fsReadBlockSimple(log_addr_root, fs_buffer));
	VERIFY(!IS_ADDR_EMPTY(log_addr_root));
//	PRINT("\nroot addr not empty");
//	PRINT_MSG_AND_NUM("\nnew root address=",logicalAddressToPhysical(log_addr_root));
//	PRINT_MSG_AND_NUM("\nroot nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nroot nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 3));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 1));
//	PRINT("\nroot sizes verified");
	/* verify direntry - read first roor address block,
	 * read 1st direntry, and verify details*/
	INODE_GET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
//	PRINT_MSG_AND_NUM("\nread direntries from address=",logicalAddressToPhysical(log_addr));
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	VERIFY(COMPARE(DIRENT_GET_INO_NUM(dirent_ptr), 2));
//	PRINT("\ndirent ino num verified");
	VERIFY(COMPARE(DIRENT_GET_TYPE(dirent_ptr), FTYPE_FILE));
//	PRINT("\ndirent type verified");
	VERIFY(COMPARE(DIRENT_GET_NAME_LEN(dirent_ptr), 9));
//	PRINT("\ndirent name len");
	VERIFY(!fsStrcmp(DIRENT_GET_NAME(dirent_ptr), f_full_name+1));

	/* verify file inode sizes*/
	VERIFY(!fsReadBlockSimple(log_addr_file, fs_buffer));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), 0));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 2));

//	PRINT("\nfile sizes verified");

	return 1;
}



/**
 * @brief
 * create a file not in root directory "directory1/directory2/file.dat/". should fail
 *
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest7(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir2);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t *f_full_name = "/directory1/directory2/file1.dat/";
	int32_t fd, old_offset;
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
	VERIFY(!fsReadBlockSimple(log_addr_dir2, fs_buffer));
	VERIFY(!allocAndWriteBlock(log_addr_dir2, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory2 direntry in directory1*/
	VERIFY(!readFileBlock(fs_buffer, 2, INODE_FILE_DATA_SIZE, log_addr_dir1, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 3, FTYPE_DIR, "directory2");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!fsReadBlockSimple(log_addr_dir1, fs_buffer));
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
	VERIFY(!fsReadBlockSimple(log_addr_root, fs_buffer));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write inode0 with pointers to the new root address*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));

	SET_ADDR_FLAG(log_addr_root, ADDR_FLAG_TAKEN);
	SET_ADDR_FLAG(log_addr_dir1, ADDR_FLAG_TAKEN);
	SET_ADDR_FLAG(log_addr_dir2, ADDR_FLAG_TAKEN);

	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	INODE_SET_DIRECT(ino_ptr,1,log_addr_dir1);
	INODE_SET_DIRECT(ino_ptr,2,log_addr_dir2);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+3*FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	old_offset = GET_RECLAIMED_OFFSET();
	fd = creat(f_full_name, 0);
	VERIFY(COMPARE(old_offset, GET_RECLAIMED_OFFSET()));
	VERIFY(IS_NEGATIVE(fd));


	VERIFY(verifyFentriesVnodesEmpty(-1));

	return 1;
}

/**
 * @brief
 * create a file in a directory open for reading. should fail
 *
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest8(){
	int32_t old_offset, fd1, fd2, i;

	/* open rrot for reading */
	fd1 = open("/", NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd1));
	/* try creat() in root */
//	PRINT("\nopened root directory for reading");
	old_offset = GET_RECLAIMED_OFFSET();
	fd2 = creat("/file1.dat", 0);
	VERIFY(COMPARE(old_offset, GET_RECLAIMED_OFFSET()));
	VERIFY(IS_NEGATIVE(fd2));
//	PRINT("\nsecond creat failed as expected");
	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i != fd1)
			VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nopen files entries verified");
	for(i=0; i<FS_MAX_VNODES;i++){
		if(VNODE_GET_INO_NUM(i) != 1)
			VERIFY(verifyVnodeEmpty(i));
	}

	return 1;
}

/**
 * @brief
 * create a file "/file.dat/". should fail
 *
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest9(){
	int32_t fd, old_offset;

	old_offset = GET_RECLAIMED_OFFSET();
	fd = creat("/file1.dat/", 0);
	VERIFY(COMPARE(old_offset, GET_RECLAIMED_OFFSET()));
	VERIFY(IS_NEGATIVE(fd));

	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));

	return 1;
}

/**
 * @brief
 * create 2 files root node, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest10(){
	uint8_t *f_name1 = "/file1.dat", *f_name2 = "/file2.dat";
	int32_t i, fd1, fd2;
	user_id user = 1;

	/* set user in process*/
	SET_CURRENT_USER(user);

	fd1 = creat(f_name1,0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\nfirst creat success");
	fd2 = creat(f_name2,0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT("\nsecond creat success");
//	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
//	VERIFY(!getInode(fs_buffer, 2, ino_log_addr));
//	PRINT_MSG_AND_HEX("\nfile size=", INODE_GET_NBYTES(ino_ptr));
//	assert(0);

//	PRINT("\ncreat success");

	VERIFY(verifyOpenFileEntry(fd1, CREAT_FLAGS, 0, user, 0));
	VERIFY(verifyOpenFileEntry(fd2, CREAT_FLAGS, 0, user, 1));
//	PRINT("\nverified open file entries");
	VERIFY(verifyVnode(0, 2, 1));
	VERIFY(verifyVnode(1, 3, 1));
//	PRINT("\nverified open vnodes");
	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd1 || i==fd2){
			continue;
		}

		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nfile is open success");
	for(i=0; i<FS_MAX_VNODES;i++){
		if(i==OPEN_FILE_GET_VNODE(fd1) || i==OPEN_FILE_GET_VNODE(fd2)){

			continue;
		}

		VERIFY(verifyVnodeEmpty(i));
	}

	return 1;
}

/**
 * @brief
 * create file with long name. verify creation, and that direntry is marked as not free
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest11(){
	uint8_t f_name[DIRENT_MAX_NAME_LENGTH] = "/";
	int32_t i, fd, name_len = DIRENT_MAX_NAME_LENGTH-3;
	user_id user = 1;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(file_addr);

	for(i=1; i < name_len+1;i++){
		f_name[i] = 'a';
	}
	f_name[i] = '\0';
	/* set user in process*/
	SET_CURRENT_USER(user);
//	PRINT_MSG_AND_NUM("\nb4 creat obs=", GET_OBS_COUNT());
	fd = creat(f_name,0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");
//	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
//	VERIFY(!getInode(fs_buffer, 2, ino_log_addr));
//	PRINT_MSG_AND_HEX("\nfile size=", INODE_GET_NBYTES(ino_ptr));
//	assert(0);

//	L("creat success");
	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd){
			VERIFY(verifyOpenFileEntry(fd, CREAT_FLAGS, 0, user, 0));
			continue;
		}

		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nfile is open success");
	for(i=0; i<FS_MAX_VNODES;i++){
		if(i==OPEN_FILE_GET_VNODE(fd)){
			VERIFY(verifyVnode(i, 2, 1));
			continue;
		}

		VERIFY(verifyVnodeEmpty(i));
	}

	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	INODE_GET_DIRECT(ino_ptr, 1, file_addr);

//	PRINT_MSG_AND_NUM("\nino0 nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nino0 nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 3));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 0));
//	PRINT("\nino0 sizes verified");
	/* verify root inode sizes */
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
//	PRINT_MSG_AND_NUM("\nroot nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nroot nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 2));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 1));
//	PRINT("\nroot sizes verified");

	/* verify direntry - read first roor address block,
	 * read 3rd direntry, and verify details*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
//	PRINT_MSG_AND_NUM("\nis direntry addr free=",IS_ADDR_FREE(log_addr));

	/* verify direntries block sparsity is marked as expected*/
//	L("first direct entry of root inode is %x", ADDR_PRINT(log_addr));
	if(FS_BLOCK_SIZE-DIRENT_MAX_SIZE <= DIRENT_MAX_SIZE){
		VERIFY(!IS_ADDR_FREE(log_addr));
	}
	else{
		VERIFY(IS_ADDR_FREE(log_addr));
	}

	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(COMPARE(DIRENT_GET_INO_NUM(dirent_ptr), 2));
//	PRINT("\ndirent ino num verified");
	VERIFY(COMPARE(DIRENT_GET_TYPE(dirent_ptr), FTYPE_FILE));
//	PRINT("\ndirent type verified");
	VERIFY(COMPARE(DIRENT_GET_NAME_LEN(dirent_ptr), name_len));
//	PRINT("\ndirent name len");
	VERIFY(!fsStrcmp(DIRENT_GET_NAME(dirent_ptr), &(f_name[1])));

	/* verify file inode sizes*/
	VERIFY(!fsReadBlockSimple(file_addr, fs_buffer));
//	PRINT_MSG_AND_NUM("\nnblocks=",INODE_GET_NBLOCKS(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1));
//	PRINT("\nnblocks success");
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), 0));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 2));
//	PRINT("\nfile id success");
	VERIFY(COMPARE(INODE_GET_FILE_TYPE(ino_ptr), FTYPE_FILE));
//	PRINT("\nfile type success");
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
//	PRINT("\nfile sizes verified");
//	PRINT_MSG_AND_NUM("\nobs count=",GET_OBS_COUNT());
	/* 1 old ino0, 1 for old dirent page, 1 for old directory inode page, 1 for old inode0, 1 for vots page*/
	VERIFY(COMPARE(5, GET_OBS_COUNT()));

	return 1;
}

/**
 * @brief
 * create a file whose inode is in indirect offset in inode0
 *
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest20(){

	uint8_t *f_name = "/file1.dat";
	int32_t i, fd, indirect_entry = 5;
	user_id user = 1;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	int32_t old_nblocks = 1+DIRECT_INDEX_ENTRIES+indirect_entry;
	int32_t old_nbytes  = INDIRECT_DATA_OFFSET+indirect_entry*FS_BLOCK_SIZE;
	bool_t cpWritten;
	int32_t expected_ino_num = old_nblocks;

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(file_addr);
	init_logical_address(indirect_addr);
	MARK_BLOCK_NO_HOLE(log_addr);

	/* set indirect block*/
	init_fsbuf(fs_buffer);
	SET_LOGICAL_OFFSET(log_addr, 5);
	SET_LOGICAL_SEGMENT(log_addr, 5);
	MARK_BLOCK_NO_HOLE(log_addr);
	for(i=0; i< indirect_entry; i++){
		BLOCK_SET_INDEX(fs_buffer, i, log_addr);
	}
	VERIFY(!allocAndWriteBlock(indirect_addr, fs_buffer, 0, indirect_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_WITH_HOLE(indirect_addr);
//	PRINT_MSG_AND_NUM("\nwrote indirect block to ", logicalAddressToPhysical(indirect_addr));
	/* write new inode0*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i,log_addr);
	}

	INODE_SET_INDIRECT(ino_ptr, indirect_addr);
	INODE_SET_NBYTES(ino_ptr, old_nbytes);
	INODE_SET_NBLOCKS(ino_ptr, old_nblocks);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* set user in process*/
	SET_CURRENT_USER(user);

	fd = creat(f_name,0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");
	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd){
			VERIFY(verifyOpenFileEntry(fd, CREAT_FLAGS, 0, user, 0));
			continue;
		}

		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nfile is open success");
	for(i=0; i<FS_MAX_VNODES;i++){
		if(i==OPEN_FILE_GET_VNODE(fd)){
			VERIFY(verifyVnode(i, expected_ino_num, 1));
			continue;
		}

		VERIFY(verifyVnodeEmpty(i));
	}
//	PRINT("\nvnode is open success");
	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	INODE_GET_INDIRECT(ino_ptr, indirect_addr);
//	PRINT_MSG_AND_NUM("\nino0 nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nino0 nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), old_nblocks+1));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), old_nbytes+FS_BLOCK_SIZE));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 0));
//	PRINT("\nino0 sizes verified");
	/* verify root inode sizes */
	VERIFY(!fsReadBlockSimple(root_addr, fs_buffer));
//	PRINT_MSG_AND_NUM("\nroot nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nroot nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 2));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 1));
//	PRINT("\nroot sizes verified");
	/* verify direntry - read first roor address block,
	 * read 3rd direntry, and verify details*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
//	PRINT("\nroot dir first block not empty");
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(COMPARE(DIRENT_GET_INO_NUM(dirent_ptr), expected_ino_num));
//	PRINT("\ndirent ino num verified");
	VERIFY(COMPARE(DIRENT_GET_TYPE(dirent_ptr), FTYPE_FILE));
//	PRINT("\ndirent type verified");
	VERIFY(COMPARE(DIRENT_GET_NAME_LEN(dirent_ptr), 9));
//	PRINT("\ndirent name len");
	VERIFY(!fsStrcmp(DIRENT_GET_NAME(dirent_ptr), f_name+1));

	/* verify file inode sizes*/
	VERIFY(IS_ADDR_FREE(indirect_addr));
	VERIFY(!fsReadBlockSimple(indirect_addr, fs_buffer));
	BLOCK_GET_INDEX(fs_buffer, indirect_entry, file_addr);
	VERIFY(!fsReadBlockSimple(file_addr, fs_buffer));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1));
//	PRINT("\nfile nblocks verified");
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), 0));
//	PRINT("\nfile nbytes verified");
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), expected_ino_num));

//	PRINT("\nfile sizes verified");
	return 1;
}

/**
 * @brief
 * create a file whose inode is in double offset in inode0
 *
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest21(){
	uint8_t *f_name = "/file1.dat";
	int32_t i, fd, indirect_entry = 5, double_entry = 3;
	user_id user = 1;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	int32_t old_nblocks = 1+DIRECT_INDEX_ENTRIES+LOG_ADDRESSES_PER_BLOCK*(1+double_entry)+indirect_entry;
	int32_t old_nbytes  = DOUBLE_DATA_OFFSET+double_entry*INODE_INDIRECT_DATA_SIZE+indirect_entry*FS_BLOCK_SIZE;
	bool_t cpWritten;
	int32_t expected_ino_num = old_nblocks;

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(file_addr);
	init_logical_address(indirect_addr);
	init_logical_address(double_addr);
	MARK_BLOCK_NO_HOLE(log_addr);

	/* set indirect block*/
	init_fsbuf(fs_buffer);
	SET_LOGICAL_OFFSET(log_addr, 5);
	SET_LOGICAL_SEGMENT(log_addr, 5);
	MARK_BLOCK_NO_HOLE(log_addr);
	for(i=0; i< indirect_entry; i++){
		BLOCK_SET_INDEX(fs_buffer, i, log_addr);
	}
	VERIFY(!allocAndWriteBlock(indirect_addr, fs_buffer, 0, indirect_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_WITH_HOLE(indirect_addr);

	/* set double block*/
	init_fsbuf(fs_buffer);
	for(i=0; i< double_entry; i++){
		BLOCK_SET_INDEX(fs_buffer, i, log_addr);
	}
	BLOCK_SET_INDEX(fs_buffer, double_entry, indirect_addr);
	VERIFY(!allocAndWriteBlock(double_addr, fs_buffer, 0, indirect_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_WITH_HOLE(double_addr);

//	PRINT_MSG_AND_NUM("\nwrote indirect block to ", logicalAddressToPhysical(indirect_addr));
	/* write new inode0*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i,log_addr);
	}
	INODE_SET_INDIRECT(ino_ptr, log_addr);
	INODE_SET_DOUBLE(ino_ptr, double_addr);

	INODE_SET_NBYTES(ino_ptr, old_nbytes);
	INODE_SET_NBLOCKS(ino_ptr, old_nblocks);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* set user in process*/
	SET_CURRENT_USER(user);

	fd = creat(f_name,0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");
	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd){
			VERIFY(verifyOpenFileEntry(fd, CREAT_FLAGS, 0, user, 0));
			continue;
		}

		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nfile is open success");
	for(i=0; i<FS_MAX_VNODES;i++){
		if(i==OPEN_FILE_GET_VNODE(fd)){
			VERIFY(verifyVnode(i, expected_ino_num, 1));
			continue;
		}

		VERIFY(verifyVnodeEmpty(i));
	}
//	PRINT("\nvnode is open success");
	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	INODE_GET_DOUBLE(ino_ptr, double_addr);
//	PRINT_MSG_AND_NUM("\nino0 nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nino0 nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), old_nblocks+1));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), old_nbytes+FS_BLOCK_SIZE));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 0));
//	PRINT("\nino0 sizes verified");
	/* verify root inode sizes */
	VERIFY(!fsReadBlockSimple(root_addr, fs_buffer));
//	PRINT_MSG_AND_NUM("\nroot nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nroot nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 2));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 1));
//	PRINT("\nroot sizes verified");
	/* verify direntry - read first roor address block,
	 * read 3rd direntry, and verify details*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
//	PRINT("\nroot dir first block not empty");
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(COMPARE(DIRENT_GET_INO_NUM(dirent_ptr), expected_ino_num));
//	PRINT("\ndirent ino num verified");
	VERIFY(COMPARE(DIRENT_GET_TYPE(dirent_ptr), FTYPE_FILE));
//	PRINT("\ndirent type verified");
	VERIFY(COMPARE(DIRENT_GET_NAME_LEN(dirent_ptr), 9));
//	PRINT("\ndirent name len");
	VERIFY(!fsStrcmp(DIRENT_GET_NAME(dirent_ptr), f_name+1));

	/* verify file inode sizes*/
	VERIFY(IS_ADDR_FREE(double_addr));
	VERIFY(!IS_ADDR_EMPTY(double_addr));
//	PRINT_MSG_AND_NUM("\ndouble address sparse verified, in address ", logicalAddressToPhysical(double_addr));
	VERIFY(!fsReadBlockSimple(double_addr, fs_buffer));
	BLOCK_GET_INDEX(fs_buffer, double_entry, indirect_addr);

	VERIFY(IS_ADDR_FREE(indirect_addr));
	VERIFY(!IS_ADDR_EMPTY(indirect_addr));
//	PRINT_MSG_AND_NUM("\nindirect_addr address sparse verified, in address ", logicalAddressToPhysical(indirect_addr));
	VERIFY(!fsReadBlockSimple(indirect_addr, fs_buffer));
	BLOCK_GET_INDEX(fs_buffer, indirect_entry, file_addr);

	VERIFY(!IS_ADDR_EMPTY(file_addr));
//	PRINT("\nfile address verified");
	VERIFY(!fsReadBlockSimple(file_addr, fs_buffer));
//	PRINT_MSG_AND_NUM("\nfile nblocks=",INODE_GET_NBLOCKS(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1));
//	PRINT("\nfile nblocks verified");
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), 0));
//	PRINT("\nfile nbytes verified");
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), expected_ino_num));

//	PRINT("\nfile sizes verified");
	return 1;
}

/**
 * @brief
 * create a file whose inode is in triple offset in inode0
 *
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest22(){
	uint8_t *f_name = "/file1.dat";
	int32_t i, fd, indirect_entry = 5, double_entry = 3, triple_entry = 1;
	user_id user = 1;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(triple_addr);
	int32_t old_nbytes  = TRIPLE_DATA_OFFSET+triple_entry*INODE_DOUBLE_DATA_SIZE+double_entry*INODE_INDIRECT_DATA_SIZE+indirect_entry*FS_BLOCK_SIZE;
	int32_t old_nblocks = CALC_IN_BLOCKS(old_nbytes)+1;
	bool_t cpWritten;
	int32_t expected_ino_num = old_nblocks;

	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < old_nbytes ){
		return 1;
	}

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(file_addr);
	init_logical_address(indirect_addr);
	init_logical_address(double_addr);
	init_logical_address(triple_addr);
	MARK_BLOCK_NO_HOLE(log_addr);

	/* set indirect block*/
	init_fsbuf(fs_buffer);
	SET_LOGICAL_OFFSET(log_addr, 5);
	SET_LOGICAL_SEGMENT(log_addr, 5);
	MARK_BLOCK_NO_HOLE(log_addr);

	for(i=0; i< indirect_entry; i++){
		BLOCK_SET_INDEX(fs_buffer, i, log_addr);
	}
	VERIFY(!allocAndWriteBlock(indirect_addr, fs_buffer, 0, indirect_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_WITH_HOLE(indirect_addr);

	/* set double block*/
	init_fsbuf(fs_buffer);
	for(i=0; i< double_entry; i++){
		BLOCK_SET_INDEX(fs_buffer, i, log_addr);
	}
	BLOCK_SET_INDEX(fs_buffer, double_entry, indirect_addr);
	VERIFY(!allocAndWriteBlock(double_addr, fs_buffer, 0, indirect_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_WITH_HOLE(double_addr);

	/* set triple block*/
	init_fsbuf(fs_buffer);
	for(i=0; i< triple_entry; i++){
		BLOCK_SET_INDEX(fs_buffer, i, log_addr);
	}
	BLOCK_SET_INDEX(fs_buffer, triple_entry, double_addr);
	VERIFY(!allocAndWriteBlock(triple_addr, fs_buffer, 0, double_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_WITH_HOLE(triple_addr);

//	PRINT_MSG_AND_NUM("\nwrote indirect block to ", logicalAddressToPhysical(indirect_addr));
	/* write new inode0*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i,log_addr);
	}
	INODE_SET_INDIRECT(ino_ptr, log_addr);
	INODE_SET_DOUBLE(ino_ptr, log_addr);
	INODE_SET_TRIPLE(ino_ptr, triple_addr);

	INODE_SET_NBYTES(ino_ptr, old_nbytes);
	INODE_SET_NBLOCKS(ino_ptr, old_nblocks);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* set user in process*/
	SET_CURRENT_USER(user);

	fd = creat(f_name,0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");
	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd){
			VERIFY(verifyOpenFileEntry(fd, CREAT_FLAGS, 0, user, 0));
			continue;
		}

		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nfile is open success");
	for(i=0; i<FS_MAX_VNODES;i++){
		if(i==OPEN_FILE_GET_VNODE(fd)){
			VERIFY(verifyVnode(i, expected_ino_num, 1));
			continue;
		}

		VERIFY(verifyVnodeEmpty(i));
	}
//	PRINT("\nvnode is open success");
	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	INODE_GET_TRIPLE(ino_ptr, triple_addr);
//	PRINT_MSG_AND_NUM("\nino0 nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nino0 nbytes=",INODE_GET_NBYTES(ino_ptr));
//	PRINT_MSG_AND_NUM("\nino0 expected nblocks=",old_nblocks+1);
//	PRINT_MSG_AND_NUM("\nino0 expected nbytes=",old_nbytes+FS_BLOCK_SIZE);
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), old_nblocks+1));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), old_nbytes+FS_BLOCK_SIZE));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 0));
//	PRINT("\nino0 sizes verified");
	/* verify root inode sizes */
	VERIFY(!fsReadBlockSimple(root_addr, fs_buffer));
//	PRINT_MSG_AND_NUM("\nroot nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nroot nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 2));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 1));
//	PRINT("\nroot sizes verified");
	/* verify direntry - read first roor address block,
	 * read 3rd direntry, and verify details*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
//	PRINT("\nroot dir first block not empty");
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(COMPARE(DIRENT_GET_INO_NUM(dirent_ptr), expected_ino_num));
//	PRINT("\ndirent ino num verified");
	VERIFY(COMPARE(DIRENT_GET_TYPE(dirent_ptr), FTYPE_FILE));
//	PRINT("\ndirent type verified");
	VERIFY(COMPARE(DIRENT_GET_NAME_LEN(dirent_ptr), 9));
//	PRINT("\ndirent name len");
	VERIFY(!fsStrcmp(DIRENT_GET_NAME(dirent_ptr), f_name+1));

	/* verify file inode sizes*/
	VERIFY(IS_ADDR_FREE(triple_addr));
	VERIFY(!IS_ADDR_EMPTY(triple_addr));
//	PRINT_MSG_AND_NUM("\ndouble address sparse verified, in address ", logicalAddressToPhysical(triple_addr));
	VERIFY(!fsReadBlockSimple(triple_addr, fs_buffer));
	BLOCK_GET_INDEX(fs_buffer, triple_entry, double_addr);

	VERIFY(IS_ADDR_FREE(double_addr));
	VERIFY(!IS_ADDR_EMPTY(double_addr));
//	PRINT_MSG_AND_NUM("\ndouble address sparse verified, in address ", logicalAddressToPhysical(double_addr));
	VERIFY(!fsReadBlockSimple(double_addr, fs_buffer));
	BLOCK_GET_INDEX(fs_buffer, double_entry, indirect_addr);

	VERIFY(IS_ADDR_FREE(indirect_addr));
	VERIFY(!IS_ADDR_EMPTY(indirect_addr));
//	PRINT_MSG_AND_NUM("\nindirect_addr address sparse verified, in address ", logicalAddressToPhysical(indirect_addr));
	VERIFY(!fsReadBlockSimple(indirect_addr, fs_buffer));
	BLOCK_GET_INDEX(fs_buffer, indirect_entry, file_addr);

	VERIFY(!IS_ADDR_EMPTY(file_addr));
//	PRINT("\nfile address verified");
	VERIFY(!fsReadBlockSimple(file_addr, fs_buffer));
//	PRINT_MSG_AND_NUM("\nfile nblocks=",INODE_GET_NBLOCKS(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), 1));
//	PRINT("\nfile nblocks verified");
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), 0));
//	PRINT("\nfile nbytes verified");
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), expected_ino_num));

//	PRINT("\nfile sizes verified");
	return 1;
}
