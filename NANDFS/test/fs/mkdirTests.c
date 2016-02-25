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

/** @file mkdirTests.c  */
#include <test/fs/mkdirTests.h>
#include <test/fs/testsHeader.h>

void runAllMkdirTests(){

	RUN_TEST(mkdir,1);
	RUN_TEST(mkdir,2);
	RUN_TEST(mkdir,3);
	RUN_TEST(mkdir,4);
	RUN_TEST(mkdir,6);
	RUN_TEST(mkdir,7);
	RUN_TEST(mkdir,8);
	RUN_TEST(mkdir,9);
	RUN_TEST(mkdir,10);
	RUN_TEST(mkdir,11);
	RUN_TEST(mkdir,12);
	RUN_TEST(mkdir,13);
	RUN_TEST(mkdir,14);

}

/**
 * @brief
 * init mkdir test
 *
 */
void init_mkdirTest(){
	if(nandInit())
		return;

	init_flash();
	initializeRamStructs();
	handleNoFs();
}

/**
 * @brief
 * tear down mkdir test
 *
 */
void tearDown_mkdirTest(){
	init_flash();
	nandTerminate();
	initializeRamStructs();
}

/**
 * @brief
 * create a directory in root node, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest1(){
	uint8_t *d_name = "/dir1";
	int32_t i;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_addr);

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(dir_addr);
	init_logical_address(block_addr);

//	PRINT_MSG_AND_NUM("\nb4 creat obs=", GET_OBS_COUNT());
	VERIFY(!mkdir(d_name,0));
//	PRINT("\nmkdir success");
//	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
//	VERIFY(!getInode(fs_buffer, 2, ino_log_addr));
//	PRINT_MSG_AND_HEX("\nfile size=", INODE_GET_NBYTES(ino_ptr));
//	assert(0);

	/* verify file entries and vnodes */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified fetries vnodes");

	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	INODE_GET_DIRECT(ino_ptr, 1, dir_addr);
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
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
	VERIFY(IS_ADDR_FREE(log_addr));
	VERIFY(!fsReadBlock(log_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(log_addr)));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	verifyDirenty(dirent_ptr, 2, FTYPE_DIR, 4, &(d_name[1]));
//	PRINT("\nverified root direntry");

	/* verify directory inode sizes*/
	VERIFY(!fsReadBlock(dir_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(dir_addr)));
	VERIFY(verifyInode(ino_ptr, 2, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nfile type success");

	/* verify entries */
	INODE_GET_DIRECT(ino_ptr, 0, block_addr);
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
//	PRINT("\nfile sizes verified");
//	PRINT_MSG_AND_NUM("\nobs count=",GET_OBS_COUNT());
	/* 1 old ino0, 1 for old dirent page, 1 for old directory inode page, 1 for old inode0, 1 for vots page*/
	VERIFY(COMPARE(5, GET_OBS_COUNT()));

	/* verify empty direntries*/
	VERIFY(!fsReadBlock(block_addr,
						fs_buffer,
						TID_EMPTY,
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						IS_CACHED_ADDR(block_addr)));
	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	VERIFY(verifyDirenty(dirent_ptr, 2, FTYPE_DIR, 1, "."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, 1, FTYPE_DIR, 2, ".."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(IS_DIRENT_EMPTY(dirent_ptr));

	return 1;
}

/**
 * @brief
 * create a directory in root node that already exists, and verify failure
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest2(){
	uint8_t *d_name = "/dir1";
	int32_t frees, rec_address;

	/* create directory */
	VERIFY(!mkdir(d_name,0));

	frees = calcTotalFreePages();
	rec_address = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* verify re-mkdir failure*/
	VERIFY(mkdir(d_name,0));

	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_address, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	return 1;
}

/**
 * @brief
 * create a directory not in root inode, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest3(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir2);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	uint8_t *d_name = "/directory1/directory2/dir3";
	user_id user = 1;
	int32_t i;

	init_logical_address(prev_log_addr);
	init_logical_address(dir_addr);
	init_logical_address(log_addr_dir2);
	init_logical_address(log_addr_dir1);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);
	init_logical_address(block_addr);

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
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES+2,
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
						DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES+1,
						IS_CACHED_ADDR(log_addr_dir1)));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_dir1, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 direntry in root*/
	VERIFY(!readFileBlock(fs_buffer, 1, INODE_FILE_DATA_SIZE, log_addr_root, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	while(!IS_DIRENT_EMPTY(dirent_ptr)){
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}

	setNewDirentry(dirent_ptr, 2, FTYPE_DIR, "directory1");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_root,
						fs_buffer,
						TID_EMPTY,
						0+1,
						IS_CACHED_ADDR(log_addr_root)));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write inode0 with pointers to the new root address*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));

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
//
	/* create directory */
	VERIFY(!mkdir(d_name, 0));
//	PRINT("\nmkdir success");

	/* verify file entries and vnodes */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified fentries vnodes");

	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));
	INODE_GET_DIRECT(ino_ptr, 3, dir_addr);
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 5, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*4));
//	PRINT("\nino0 verified");

	/* verify parent inode sizes */
	init_logical_address(log_addr_dir2);
	VERIFY(!getInode(fs_buffer, 3, log_addr_dir2));
	VERIFY(verifyInode(ino_ptr, 3, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nparent verified");

	/* verify direntry - read first roor address block,
	 * read 3rd direntry, and verify details*/
	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(IS_ADDR_FREE(log_addr));
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
//	PRINT("\nread block success");
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
//	PRINT("\nmoveToNextDirentry success");
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
//	PRINT("\nmoveToNextDirentry success");
//	printBlock(fs_buffer);
	VERIFY(verifyDirenty(dirent_ptr, 4, FTYPE_DIR, 4, "dir3"));
//	PRINT("\nverified parent direntry");

	/* verify directory inode */
	VERIFY(!fsReadBlockSimple(dir_addr, fs_buffer));
	VERIFY(verifyInode(ino_ptr, 4, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));

	/* verify entries */
	INODE_GET_DIRECT(ino_ptr, 0, block_addr);
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
//	PRINT("\nfile sizes verified");
//	PRINT_MSG_AND_NUM("\nobs count=",GET_OBS_COUNT());
	/* 1 old ino0, 1 for old dirent page, 1 for old directory inode page, 1 for old inode0, 1 for vots page*/
	VERIFY(COMPARE(5, GET_OBS_COUNT()));

	/* verify empty direntries*/
	VERIFY(!fsReadBlockSimple(block_addr, fs_buffer));
	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	VERIFY(verifyDirenty(dirent_ptr, 4, FTYPE_DIR, 1, "."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, 3, FTYPE_DIR, 2, ".."));

	return 1;
}

/**
 * @brief
 * create a directory not in root node that already exists, and verify afilure
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest4(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir2);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	uint8_t *d_name = "/directory1/directory2/dir3";
	int32_t frees, rec_address;
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
	while(!IS_DIRENT_EMPTY(dirent_ptr)){
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}

	setNewDirentry(dirent_ptr, 2, FTYPE_DIR, "directory1");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!fsReadBlock(log_addr_root,
						fs_buffer,
						TID_EMPTY,
						0+1,
						IS_CACHED_ADDR(log_addr_root)));
	SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));


	/* write inode0 with pointers to the new root address*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));

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

	/* create directory */
	VERIFY(!mkdir(d_name, 0));

	frees = calcTotalFreePages();
	rec_address = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* verify re-mkdir failure*/
	VERIFY(mkdir(d_name,0));

	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_address, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	return 1;
}

/**
 * @brief
 * create a directory in root node, when we have no free direentries to use and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest6(){
	uint8_t *d_name = "/dir1";
	int32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);

	init_logical_address(root_addr);
	init_logical_address(log_addr);
	init_logical_address(prev_log_addr);

	/* mark first direntry block in root as not free*/
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	MARK_BLOCK_NO_HOLE(log_addr);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr);

	VERIFY(!allocAndWriteBlock(root_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* re-write inode0*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));
	MARK_BLOCK_NO_HOLE(root_addr);
	INODE_SET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* try to create directory*/
	VERIFY(!mkdir(d_name, 0));
//	PRINT("\nmkdir success");

	/* verify directory was created in second root direntries block*/
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 3, INODE_FILE_DATA_SIZE+2*FS_BLOCK_SIZE));
//	PRINT("\nroot inode success");

	/* verify entries */
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

	/* verify block 0*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
	VERIFY(!IS_ADDR_FREE(log_addr));
//	PRINT("\nblock0 success");

	/* verify (new) block 1*/
	INODE_GET_DIRECT(ino_ptr, 1, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
	VERIFY(IS_ADDR_FREE(log_addr));

	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	VERIFY(verifyDirenty(de_ptr, 2, FTYPE_DIR, calcNameLen(&(d_name[1])), &(d_name[1])));
//	PRINT("\n1st de success");
	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	return 1;
}



/**
 * @brief
 * create a directory with length 0 ("/", "directory1/directory2/"). should fail
 *
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest7(){
	uint8_t *d_name = "/dir1/";
	int32_t frees, rec_address;

	frees = calcTotalFreePages();
	rec_address = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* verify mkdir failure*/
	VERIFY(mkdir(d_name,0));

	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_address, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	return 1;
}

/**
 * @brief
 * create a directory in a directory open for reading. should fail
 *
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest8(){
	int32_t fd = 0, vnode_idx = 0, frees, rec_address;
	user_id uid = 0;
	uint8_t *d_name = "/dir1";

	/* set open file entry for root directory */
	OPEN_FILE_SET_FLAGS(fd, NANDFS_O_RDONLY);
	OPEN_FILE_SET_FTYPE(fd, FTYPE_DIR);
	OPEN_FILE_SET_OFFSET(fd, INODE_FILE_DATA_SIZE);
	OPEN_FILE_SET_UID(fd, uid);
	OPEN_FILE_SET_VNODE(fd, vnode_idx);

	VNODE_SET_INO_NUM(vnode_idx, 1);
	VNODE_SET_NREFS(vnode_idx, 1);

	frees = calcTotalFreePages();
	rec_address = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* verify mkdir failure*/
	VERIFY(mkdir(d_name,0));
//	PRINT("\nmkdir success");
	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_address, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\nfrees, rec addr success");
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyOpenFileEntry(fd, NANDFS_O_RDONLY, INODE_FILE_DATA_SIZE, uid, vnode_idx));
	VERIFY(verifyVnode(vnode_idx, 1, 1));

	return 1;
}

/**
 * @brief
 * try to create a directory in a non-existent directory
 *
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest9(){
	int32_t frees, rec_address;
	uint8_t *d_name = "/dir1/dir2";

	frees = calcTotalFreePages();
	rec_address = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* verify mkdir failure*/
	VERIFY(mkdir(d_name,0));
//	PRINT("\nmkdir success");
	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_address, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	return 1;
}

/**
 * @brief
 * create 2 directories in root node, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest10(){
	int32_t i ,j;
	uint8_t *d_name1 = "/dir1", *d_name2 = "/dir2";
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);

	init_logical_address(root_addr);
	init_logical_address(log_addr);

	/* create directories */
	VERIFY(!mkdir(d_name1, 0));
	VERIFY(!mkdir(d_name2, 0));
//	PRINT("\nmkdirs success");

	/* verify root directory */
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));

	/* verify entries */
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
//	PRINT("\nroot success");

	/* verify direntries */
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	VERIFY(verifyDirenty(de_ptr, 1, FTYPE_DIR, 1, "."));
	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	VERIFY(verifyDirenty(de_ptr, 1, FTYPE_DIR, 2, ".."));
	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	VERIFY(verifyDirenty(de_ptr, 2, FTYPE_DIR, calcNameLen(&(d_name1[1])), &(d_name1[1])));
	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	VERIFY(verifyDirenty(de_ptr, 3, FTYPE_DIR, calcNameLen(&(d_name2[1])), &(d_name2[1])));
//	PRINT("\nroot direntries success");

	/* verify directories*/
	for(j=2; j< 4; j++){
		VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
							fs_buffer,
							TID_EMPTY,
							0,
							IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));
		INODE_GET_DIRECT(ino_ptr, j-1, log_addr);
		VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
		VERIFY(verifyInode(ino_ptr, j, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//		PRINT("\ninode verified");
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
//		PRINT("\ndouble entry success");
		INODE_GET_TRIPLE(ino_ptr, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));

		INODE_GET_DIRECT(ino_ptr, 0, log_addr);
		VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));

		/* verify direntries */
		de_ptr = CAST_TO_DIRENT(fs_buffer);
		VERIFY(verifyDirenty(de_ptr, j, FTYPE_DIR, 1, "."));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
		VERIFY(verifyDirenty(de_ptr, 1, FTYPE_DIR, 2, ".."));
//		PRINT("\ndirectory success");
	}

	return 1;
}

/**
 * @brief
 * create directory, and create file in it. verify creation.
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest11(){
	uint8_t *d_name = "/dir1", *f_name = "/dir1/file1.dat";
	int32_t i, fd;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_addr);
	user_id uid = 1;

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(dir_addr);
	init_logical_address(file_addr);
	init_logical_address(block_addr);

	SET_CURRENT_USER(uid);

	/* create directory */
	VERIFY(!mkdir(d_name,0));
//	PRINT("\nmkdir success");

	/* create file in directory*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");

//	PRINT("\ncreat success");
	/* verify file entries and vnodes */
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyOpenFileEntry(fd, CREAT_FLAGS, 0, uid, 0));
//	PRINT("\nverified fentries vnodes");

	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	INODE_GET_DIRECT(ino_ptr, 1, dir_addr);
	INODE_GET_DIRECT(ino_ptr, 2, file_addr);
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 4, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*3));
//	PRINT("\nino0 sizes verified");

	/* verify file */
	VERIFY(!fsReadBlockSimple(file_addr, fs_buffer));
	VERIFY(verifyInode(ino_ptr, 3, FTYPE_FILE, 1, 0));
//	PRINT("\nfile verified");

	/* verify root inode sizes */
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
//	PRINT_MSG_AND_NUM("\nroot nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nroot nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nroot verified");

	/* verify direntry - read first roor address block,
	 * read 3rd direntry, and verify details*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(IS_ADDR_FREE(log_addr));
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, 2, FTYPE_DIR, calcNameLen(&(d_name[1])), &(d_name[1])));
//	PRINT("\nverified root direntry");

	/* verify directory inode sizes*/
	VERIFY(!fsReadBlockSimple(dir_addr, fs_buffer));
	VERIFY(verifyInode(ino_ptr, 2, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\ndirectory inode success");

	/* verify entries */
	INODE_GET_DIRECT(ino_ptr, 0, block_addr);
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
//	PRINT("\ndirectory ino entries success");

	/* verify new directory direntries*/
	VERIFY(!fsReadBlockSimple(block_addr, fs_buffer));
	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	VERIFY(verifyDirenty(dirent_ptr, 2, FTYPE_DIR, 1, "."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, 1, FTYPE_DIR, 2, ".."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, 3, FTYPE_FILE, calcNameLen(&(f_name[calcNameLen(&(d_name[1]))+2])), &(f_name[calcNameLen(&(d_name[1]))+2])));

	/* 1 old ino0, 1 for old dirent page, 1 for old directory inode page, 1 for old inode0, 1 for vots page
	 * times 2*/
//	PRINT_MSG_AND_NUM("\nobs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(5*2, GET_OBS_COUNT()));

	return 1;
}

/**
 * @brief
 * create directory, create file in it, unlink file. verify unlink
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest12(){
	uint8_t *d_name = "/dir1", *f_name = "/dir1/file1.dat";
	int32_t i, fd;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_addr);
	user_id uid = 1;

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(dir_addr);
	init_logical_address(block_addr);

	SET_CURRENT_USER(uid);

	/* create directory */
	VERIFY(!mkdir(d_name,0));
//	PRINT("\nmkdir success");

	/* create file in directory*/
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	/* unlink file*/
	VERIFY(!unlink(f_name));

	/* verify file entries and vnodes */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified fetries vnodes");

	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	INODE_GET_DIRECT(ino_ptr, 1, dir_addr);
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 4, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*3));
//	PRINT("\nino0 inode verified");

	/* verify root inode sizes */
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
//	PRINT_MSG_AND_NUM("\nroot nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nroot nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nroot inode verified");

	/* verify direntry - read first roor address block,
	 * read 3rd direntry, and verify details*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(IS_ADDR_FREE(log_addr));
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, 2, FTYPE_DIR, calcNameLen(&(d_name[1])), &(d_name[1])));
//	PRINT("\nverified root direntry");

	/* verify directory inode sizes*/
	VERIFY(!fsReadBlockSimple(dir_addr, fs_buffer));
	VERIFY(verifyInode(ino_ptr, 2, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nfile type success");

	/* verify entries */
	INODE_GET_DIRECT(ino_ptr, 0, block_addr);
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
//	PRINT("\nverify root inode entries");

	/* verify empty direntries*/
	VERIFY(!fsReadBlockSimple(block_addr, fs_buffer));
	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	VERIFY(verifyDirenty(dirent_ptr, 2, FTYPE_DIR, 1, "."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, 1, FTYPE_DIR, 2, ".."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(IS_DIRENT_EMPTY(dirent_ptr));

	/* 1 old ino0, 1 for old dirent page, 1 for old directory inode page, 1 for old inode0, 1 for vots page
	 * times 2
	 * 1 old ino0, 1 for old dirent page, 1 for old directory inode page, 1 for old inode0, 1 for vots page, 1 for old inode*/
//	PRINT_MSG_AND_NUM("\nobs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(5*2+6, GET_OBS_COUNT()));

	return 1;
}

/**
 * @brief
 * create a directory whose inode is in triple offset in inode0
 *
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest13(){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(triple_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	int32_t i, expected_f_id = (TRIPLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE+1;
	bool_t cpWritten;
	uint8_t *d_name = "/dir1";

	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < TRIPLE_DATA_OFFSET){
		return 1;
	}

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(triple_addr);
	init_logical_address(double_addr);
	init_logical_address(indirect_addr);
	init_logical_address(dir_addr);

	/* mark all inodes as taken in inode0 until triple offset*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));
	MARK_BLOCK_NO_HOLE(log_addr);

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_SET_DIRECT(ino_ptr, i, log_addr);
	}
	INODE_SET_INDIRECT(ino_ptr, log_addr);
	INODE_SET_DOUBLE(ino_ptr, log_addr);
	INODE_SET_NBYTES(ino_ptr, TRIPLE_DATA_OFFSET);
	INODE_SET_NBLOCKS(ino_ptr, expected_f_id);

	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* create directory*/
	VERIFY(!mkdir(d_name, 0));

	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	for(i=1; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\ndirect entries success");
	INODE_GET_INDIRECT(ino_ptr, indirect_addr);
	VERIFY(IS_ADDR_EMPTY(indirect_addr));
	VERIFY(!IS_ADDR_FREE(indirect_addr));
//	PRINT("\nindirect entry success");
	INODE_GET_DOUBLE(ino_ptr, double_addr);
	VERIFY(IS_ADDR_EMPTY(double_addr));
	VERIFY(!IS_ADDR_FREE(double_addr));

	INODE_GET_TRIPLE(ino_ptr, triple_addr);
	VERIFY(IS_ADDR_FREE(triple_addr));
	VERIFY(!IS_ADDR_EMPTY(triple_addr));
//	PRINT_MSG_AND_NUM("\nino0 nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nino0 nbytes=",INODE_GET_NBYTES(ino_ptr));
//	PRINT_MSG_AND_NUM("\nino0 expected nblocks=",old_nblocks+1);
//	PRINT_MSG_AND_NUM("\nino0 expected nbytes=",old_nbytes+FS_BLOCK_SIZE);
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 1+(TRIPLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE+1, TRIPLE_DATA_OFFSET+FS_BLOCK_SIZE));
//	PRINT("\nino0 inode verified");

	/* verify root inode  */
	VERIFY(!fsReadBlock(root_addr,
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(root_addr)));
//	PRINT_MSG_AND_NUM("\nroot nblocks=",INODE_GET_NBLOCKS(ino_ptr));
//	PRINT_MSG_AND_NUM("\nroot nbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(verifyInode(ino_ptr, 1, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nroot inode verified");

	/* verify direntry - read first roor address block,
	 * read 3rd direntry, and verify details*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
//	PRINT("\nroot dir first block not empty");
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	VERIFY(verifyDirenty(dirent_ptr, 1, FTYPE_DIR, 1, "."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, 1, FTYPE_DIR, 2, ".."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, expected_f_id, FTYPE_DIR, calcNameLen(&(d_name[1])), &(d_name[1])));
//	PRINT("\nroot directory entries verified");

	/* verify directory*/
	VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(),
						fs_buffer,
						TID_EMPTY,
						0,
						IS_CACHED_ADDR(FS_GET_INO0_ADDR_PTR())));
//	PRINT_MSG_AND_NUM("\ndouble address sparse verified, in address ", logicalAddressToPhysical(triple_addr));
	VERIFY(!fsReadBlockSimple(triple_addr, fs_buffer));
	BLOCK_GET_INDEX(fs_buffer, 0, double_addr);

	VERIFY(IS_ADDR_FREE(double_addr));
	VERIFY(!IS_ADDR_EMPTY(double_addr));
//	PRINT_MSG_AND_NUM("\ndouble address sparse verified, in address ", logicalAddressToPhysical(double_addr));
	VERIFY(!fsReadBlockSimple(double_addr, fs_buffer));
	BLOCK_GET_INDEX(fs_buffer, 0, indirect_addr);

	VERIFY(IS_ADDR_FREE(indirect_addr));
	VERIFY(!IS_ADDR_EMPTY(indirect_addr));
//	PRINT_MSG_AND_NUM("\nindirect_addr address sparse verified, in address ", logicalAddressToPhysical(indirect_addr));
	VERIFY(!fsReadBlockSimple(indirect_addr, fs_buffer));
	BLOCK_GET_INDEX(fs_buffer, 0, dir_addr);

	VERIFY(!IS_ADDR_EMPTY(dir_addr));
	VERIFY(!fsReadBlockSimple(dir_addr, fs_buffer));
	VERIFY(verifyInode(ino_ptr, expected_f_id, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\ndirectory inode verified");

	/* verify direntry - read first roor address block,
	 * read 3rd direntry, and verify details*/

	for(i=1; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
		VERIFY(IS_ADDR_FREE(log_addr));
	}
//	PRINT("\ndirect entries success");
	INODE_GET_INDIRECT(ino_ptr, indirect_addr);
	VERIFY(IS_ADDR_EMPTY(indirect_addr));
	VERIFY(IS_ADDR_FREE(indirect_addr));
//	PRINT("\nindirect entry success");
	INODE_GET_DOUBLE(ino_ptr, double_addr);
	VERIFY(IS_ADDR_EMPTY(double_addr));
	VERIFY(IS_ADDR_FREE(double_addr));

	INODE_GET_DOUBLE(ino_ptr, triple_addr);
	VERIFY(IS_ADDR_EMPTY(triple_addr));
	VERIFY(IS_ADDR_FREE(triple_addr));

	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!IS_ADDR_EMPTY(log_addr));
//	PRINT("\nroot dir first block not empty");
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));

	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	VERIFY(verifyDirenty(dirent_ptr, expected_f_id, FTYPE_DIR, 1, "."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, 1, FTYPE_DIR, 2, ".."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(IS_DIRENT_EMPTY(dirent_ptr));

	return 1;
}

/**
 * @brief
 * create directory within directory until 256 depth. verify creation.
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest14(){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	int32_t i, ino_num, j, max_depth=256, frees;
	uint8_t *d_name = "/d", d_new_name[256*2+2];

	init_logical_address(log_addr);
	init_logical_address(ino_addr);

	frees = calcTotalFreePages();
	/* create directories recursively*/
	for(i=0; i< max_depth; i++){
		fsStrcpy(&(d_new_name[i*2]), d_name);
		d_new_name[i*2+2] = '\0';

		VERIFY(!mkdir(d_new_name,0));
//		PRINT_MSG_AND_STR("\ncreated ", d_new_name);
	}

	ino_num = 1;
	/* verify creation recursively*/
	for(i=0; i< max_depth+1; i++){
//		PRINT_MSG_AND_NUM("\nverify directory with id=", ino_num);
		VERIFY(!getInode(fs_buffer, ino_num, ino_addr));
		VERIFY(!fsReadBlockSimple(ino_addr, fs_buffer));

		/* verify inode */
		VERIFY(verifyInode(ino_ptr, ino_num, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//		PRINT("\ninode verified");
		for(j=1; j<DIRECT_INDEX_ENTRIES;j++){
			INODE_GET_DIRECT(ino_ptr, j, log_addr);
			VERIFY(IS_ADDR_EMPTY(log_addr));
			VERIFY(IS_ADDR_FREE(log_addr));
		}
//		PRINT("\ndirect entries success");
		INODE_GET_INDIRECT(ino_ptr, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
		VERIFY(IS_ADDR_FREE(log_addr));
//		PRINT("\nindirect entry success");
		INODE_GET_DOUBLE(ino_ptr, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
		VERIFY(IS_ADDR_FREE(log_addr));

		INODE_GET_TRIPLE(ino_ptr, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
		VERIFY(IS_ADDR_FREE(log_addr));

		INODE_GET_DIRECT(ino_ptr, 0, log_addr);
		VERIFY(!IS_ADDR_EMPTY(log_addr));
		VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));

		/* verify direntries */
		dirent_ptr = CAST_TO_DIRENT(fs_buffer);
		VERIFY(verifyDirenty(dirent_ptr, ino_num, FTYPE_DIR, 1, "."));
//		PRINT("\n. direntry success");
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
		VERIFY(verifyDirenty(dirent_ptr, (ino_num!=1)?ino_num-1:ino_num, FTYPE_DIR, 2, ".."));
//		PRINT("\n.. direntry success");
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
		if(i<max_depth){
			VERIFY(verifyDirenty(dirent_ptr, ino_num+1, FTYPE_DIR, 1, "d"));
		}
		else{
//			PRINT("\ni=max_depth");
			VERIFY(IS_DIRENT_EMPTY(dirent_ptr));
		}
//		PRINT("\nd direntry success");

		init_logical_address(ino_addr);
		ino_num++;
	}

	/* verify free pages - subtract inode, direntries page for every directory
	 * and also inode0 indirect(+double+double indirect in case inode0 was extended to double offset)*/
//	PRINT_MSG_AND_NUM("\ntotal frees=",calcTotalFreePages());
//	PRINT_MSG_AND_NUM(" expected=",frees-max_depth*2-3);
//	PRINT_MSG_AND_NUM("\ninode0 size=",getFileSize(1));
//	PRINT_MSG_AND_NUM("\nDOUBLE_DATA_OFFSET=", DOUBLE_DATA_OFFSET);
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	if(INODE_GET_NBYTES(CAST_TO_INODE(fs_buffer)) < DOUBLE_DATA_OFFSET){
		VERIFY(COMPARE(frees-max_depth*2-1, calcTotalFreePages()));
	}
	else{
		VERIFY(COMPARE(frees-max_depth*2-3, calcTotalFreePages()));
	}
	return 1;
}

