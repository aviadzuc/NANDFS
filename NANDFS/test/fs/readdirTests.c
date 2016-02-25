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

/** @file readdirTests.c  */
#include <test/fs/readdirTests.h>
#include <test/fs/testsHeader.h>

void runAllReaddirTests(){

	RUN_TEST(readdir,1);
	RUN_TEST(readdir,2);
	RUN_TEST(readdir,3);
	RUN_TEST(readdir,4);
	RUN_TEST(readdir,5);

	RUN_TEST(readdir,7);
	RUN_TEST(readdir,8);

}

/**
 * @brief
 * init opendir test
 *
 */
void init_readdirTest(){
	if(nandInit())
		return;

	init_flash();
	initializeRamStructs();
	handleNoFs();
	init_fsbuf(fs_buffer);
}

/**
 * @brief
 * tear down opendir test
 *
 */
void tearDown_readdirTest(){
	init_flash();
	nandTerminate();
	initializeRamStructs();
}

/**
 * @brief
 * open directory and read readdir. should return "." entry.
 * keep reading until finished
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest1(){
	uint8_t *dir = "/";
	NANDFS_DIR *ds;
	nandfs_dirent* de;
	dirent_flash *de_flash;
	int32_t offset;
	user_id uid = 1;

	SET_CURRENT_USER(uid);
	ds = opendir(dir);
	VERIFY(!COMPARE(NULL, ds));

	/* read "."*/
	de = readdir(ds);
//	PRINT("\nreaddir 1 success");
	VERIFY(!COMPARE(NULL, de));
	de_flash = CAST_TO_DIRENT(de);
	VERIFY(verifyDirenty(de_flash, 1, FTYPE_DIR, 1, "."));
//	PRINT("\ndirentry 1 success");
	offset = DIRECTORY_FIRST_ENTRY_OFFSET+DS_GET_DIRENTRY_LEN(ds);
	VERIFY(COMPARE(offset, OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds))));
//	PRINT("\noffset 1 success");

	/* read ".."*/
	de = readdir(ds);
//	PRINT("\nreaddir 2 success");
	VERIFY(!COMPARE(NULL, de));
	de_flash = CAST_TO_DIRENT(de);
	VERIFY(verifyDirenty(de_flash, 1, FTYPE_DIR, 2, ".."));
//	PRINT("\ndirentry 1 success");
	offset += DS_GET_DIRENTRY_LEN(ds);
	VERIFY(COMPARE(offset, OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds))));

	/* read ad reach EOF*/
	de = readdir(ds);
	VERIFY(COMPARE(NULL, de));
//	PRINT("\nreaddir 3 success");
	VERIFY(COMPARE(offset, OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds))));

	return 1;
}

/**
 * @brief
 * open directory and read readdir. than do another readdir, where the next direntry is in
 * a direct entry. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest2(){
	uint8_t *dir = "/", *file = "/file1";
	NANDFS_DIR *ds;
	nandfs_dirent* de;
	dirent_flash *de_flash;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t offset, direct = 5, f_id = 2, f_type = FTYPE_FILE;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(entries_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	bool_t cpWritten;
	user_id uid = 1;
	SET_CURRENT_USER(uid);

	init_logical_address(entries_addr);
	init_logical_address(root_addr);

	/* write entries block*/
	init_fsbuf(fs_buffer);
	de_flash = CAST_TO_DIRENT(fs_buffer);
	DIRENT_SET_INO_NUM(de_flash, f_id);
	DIRENT_SET_LEN(de_flash, calcNameLen(file));
	DIRENT_SET_NAME(de_flash, file);
	DIRENT_SET_TYPE(de_flash, f_type);
	VERIFY(!allocAndWriteBlock(entries_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write to root inode */
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	INODE_SET_DIRECT(ino_ptr, direct, entries_addr);
	init_logical_address(root_addr);
	VERIFY(!allocAndWriteBlock(root_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* change inode0*/
	fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* open directory and start reading */
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	/* read "."*/
	de = readdir(ds);
	VERIFY(!IS_NULL(de));

	/* read ".."*/
	de = readdir(ds);
	VERIFY(!IS_NULL(de));

	/* read file entry */
	de = readdir(ds);
	VERIFY(!IS_NULL(de));
	de_flash = CAST_TO_DIRENT(de);
//	PRINT("\nverify 3rd readdir success");

	VERIFY(verifyDirenty(de_flash, f_id, f_type, calcNameLen(file), file));
//	PRINT("\n3rd direntry success");
//	PRINT("\ndirentry 1 success");
	offset = DIRECTORY_FIRST_ENTRY_OFFSET+direct * FS_BLOCK_SIZE+DS_GET_DIRENTRY_LEN(ds);
//	PRINT_MSG_AND_NUM("\nexpeted offset=", offset);
//	PRINT_MSG_AND_NUM(" actual offset=", OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds)));
	VERIFY(COMPARE(offset, OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds))));

	/* verify no more entries*/
	de = readdir(ds);
	VERIFY(IS_NULL(de));

	return 1;
}

/**
 * @brief
 * open directory and read readdir. than do another readdir, where the next direntry is in
 * an indirect entry. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest3(){
	uint8_t *dir = "/", *file = "/file1";
	NANDFS_DIR *ds;
	nandfs_dirent* de;
	dirent_flash *de_flash;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t offset, f_id = 2, f_type = FTYPE_FILE;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(entries_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	bool_t cpWritten;
	user_id uid = 1;
	SET_CURRENT_USER(uid);

	init_logical_address(entries_addr);
	init_logical_address(indirect_addr);
	init_logical_address(root_addr);

	/* write entries block*/
	init_fsbuf(fs_buffer);
	de_flash = CAST_TO_DIRENT(fs_buffer);
	DIRENT_SET_INO_NUM(de_flash, f_id);
	DIRENT_SET_LEN(de_flash, calcNameLen(file));
	DIRENT_SET_NAME(de_flash, file);
	DIRENT_SET_TYPE(de_flash, f_type);
	VERIFY(!allocAndWriteBlock(entries_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write indirect block */
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, 0, entries_addr);
	VERIFY(!allocAndWriteBlock(indirect_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write to root inode */
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	INODE_SET_INDIRECT(ino_ptr, indirect_addr);
	init_logical_address(root_addr);
	VERIFY(!allocAndWriteBlock(root_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* change inode0*/
	fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* open directory and start reading */
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	/* read "."*/
	de = readdir(ds);
	VERIFY(!IS_NULL(de));

	/* read ".."*/
	de = readdir(ds);
	VERIFY(!IS_NULL(de));

	/* read file entry */
	de = readdir(ds);
	VERIFY(!IS_NULL(de));
	de_flash = CAST_TO_DIRENT(de);
//	PRINT("\nverify 3rd readdir success");

	VERIFY(verifyDirenty(de_flash, f_id, f_type, calcNameLen(file), file));
//	PRINT("\n3rd direntry success");
//	PRINT("\ndirentry 1 success");
	offset = INDIRECT_DATA_OFFSET+DS_GET_DIRENTRY_LEN(ds);
//	PRINT_MSG_AND_NUM("\nexpeted offset=", offset);
//	PRINT_MSG_AND_NUM(" actual offset=", OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds)));
	VERIFY(COMPARE(offset, OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds))));

	/* verify no more entries*/
	de = readdir(ds);
	VERIFY(IS_NULL(de));

	return 1;
}

/**
 * @brief
 * open directory and read readdir. than do another readdir, where the next direntry is in
 * a double entry. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest4(){
	uint8_t *dir = "/", *file = "/file1";
	NANDFS_DIR *ds;
	nandfs_dirent* de;
	dirent_flash *de_flash;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t offset, f_id = 2, f_type = FTYPE_FILE;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(entries_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	bool_t cpWritten;
	user_id uid = 1;
	SET_CURRENT_USER(uid);

	init_logical_address(entries_addr);
	init_logical_address(indirect_addr);
	init_logical_address(double_addr);
	init_logical_address(root_addr);

	/* write entries block*/
	init_fsbuf(fs_buffer);
	de_flash = CAST_TO_DIRENT(fs_buffer);
	DIRENT_SET_INO_NUM(de_flash, f_id);
	DIRENT_SET_LEN(de_flash, calcNameLen(file));
	DIRENT_SET_NAME(de_flash, file);
	DIRENT_SET_TYPE(de_flash, f_type);
	VERIFY(!allocAndWriteBlock(entries_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write indirect block */
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, 0, entries_addr);
	VERIFY(!allocAndWriteBlock(indirect_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write double block */
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, 0, indirect_addr);
	VERIFY(!allocAndWriteBlock(double_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write to root inode */
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	INODE_SET_DOUBLE(ino_ptr, double_addr);
	init_logical_address(root_addr);
	VERIFY(!allocAndWriteBlock(root_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* change inode0*/
	fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* open directory and start reading */
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	/* read "."*/
	de = readdir(ds);
	VERIFY(!IS_NULL(de));

	/* read ".."*/
	de = readdir(ds);
	VERIFY(!IS_NULL(de));

	/* read file entry */
	de = readdir(ds);
	VERIFY(!IS_NULL(de));
	de_flash = CAST_TO_DIRENT(de);
//	PRINT("\nverify 3rd readdir success");

	VERIFY(verifyDirenty(de_flash, f_id, f_type, calcNameLen(file), file));
//	PRINT("\n3rd direntry success");
//	PRINT("\ndirentry 1 success");
	offset = DOUBLE_DATA_OFFSET+DIRENT_GET_LEN(de_flash);
//	PRINT_MSG_AND_NUM("\nexpeted offset=", offset);
//	PRINT_MSG_AND_NUM(" actual offset=", OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds)));
	VERIFY(COMPARE(offset, OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds))));

	/* verify no more entries*/
	de = readdir(ds);
	VERIFY(IS_NULL(de));

	return 1;
}

/**
 * @brief
 * open directory and read readdir. than do another readdir, where the next direntry is in
 * an triple entry. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest5(){
	uint8_t *dir = "/", *file = "/file1";
	NANDFS_DIR *ds;
	nandfs_dirent* de;
	dirent_flash *de_flash;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t offset, f_id = 2, f_type = FTYPE_FILE;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(entries_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(triple_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	bool_t cpWritten;
	user_id uid = 1;

	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < TRIPLE_DATA_OFFSET){
		return 1;
	}

	SET_CURRENT_USER(uid);

	init_logical_address(entries_addr);
	init_logical_address(indirect_addr);
	init_logical_address(double_addr);
	init_logical_address(triple_addr);
	init_logical_address(root_addr);

	/* write entries block*/
	init_fsbuf(fs_buffer);
	de_flash = CAST_TO_DIRENT(fs_buffer);
	DIRENT_SET_INO_NUM(de_flash, f_id);
	DIRENT_SET_LEN(de_flash, calcNameLen(file));
	DIRENT_SET_NAME(de_flash, file);
	DIRENT_SET_TYPE(de_flash, f_type);
	VERIFY(!allocAndWriteBlock(entries_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write indirect block */
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, 0, entries_addr);
	VERIFY(!allocAndWriteBlock(indirect_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write double block */
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, 0, indirect_addr);
	VERIFY(!allocAndWriteBlock(double_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write double block */
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, 0, double_addr);
	VERIFY(!allocAndWriteBlock(triple_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write to root inode */
	VERIFY(!getInode(fs_buffer, 1, root_addr));
	INODE_SET_TRIPLE(ino_ptr, triple_addr);
	init_logical_address(root_addr);
	VERIFY(!allocAndWriteBlock(root_addr, fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* change inode0*/
	fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, entries_addr, &cpWritten, fsCheckpointWriter, 0));

	/* open directory and start reading */
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	/* read "."*/
	de = readdir(ds);
	VERIFY(!IS_NULL(de));

	/* read ".."*/
	de = readdir(ds);
	VERIFY(!IS_NULL(de));

	/* read file entry */
	de = readdir(ds);
	VERIFY(!IS_NULL(de));
	de_flash = CAST_TO_DIRENT(de);
//	PRINT("\nverify 3rd readdir success");

	VERIFY(verifyDirenty(de_flash, f_id, f_type, calcNameLen(file), file));
//	PRINT("\n3rd direntry success");
//	PRINT("\ndirentry 1 success");
	offset = TRIPLE_DATA_OFFSET+DIRENT_GET_LEN(de_flash);
//	PRINT_MSG_AND_NUM("\nexpeted offset=", offset);
//	PRINT_MSG_AND_NUM(" actual offset=", OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds)));
	VERIFY(COMPARE(offset, OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(ds))));

	/* verify no more entries*/
	de = readdir(ds);
	VERIFY(IS_NULL(de));

	return 1;
}

/**
 * @brief
 * try to read from an illegal dirstream. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest7(){
	NANDFS_DIR* ds = NULL;

	VERIFY(IS_NULL(readdir(ds)));

	return 1;
}

/**
 * @brief
 * try to readdir a directory opened by another user. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest8(){
	user_id uid1 = 1, uid2 = 2;
	NANDFS_DIR* ds;
	uint8_t *dir = "/";

	SET_CURRENT_USER(uid1);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	SET_CURRENT_USER(uid2);
	VERIFY(IS_NULL(readdir(ds)));

	return 1;
}

