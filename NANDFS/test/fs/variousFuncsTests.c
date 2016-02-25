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

/** @file variousFuncsTests.c  */
#include <test/fs/variousFuncsTests.h>
#include <test/fs/testsHeader.h>

void runAllVariousFuncsTests(){

	RUN_TEST(lseek,1);
	RUN_TEST(lseek,2);
	RUN_TEST(lseek,3);
	RUN_TEST(lseek,4);

	RUN_TEST(telldir,1);
	RUN_TEST(telldir,2);
	RUN_TEST(telldir,3);
	RUN_TEST(telldir,4);
	RUN_TEST(telldir,5);

	RUN_TEST(dirfd,1);
	RUN_TEST(dirfd,2);
	RUN_TEST(dirfd,3);

	RUN_TEST(seekdir,1);
	RUN_TEST(seekdir,2);

	RUN_TEST(rewinddir,1);
	RUN_TEST(rewinddir,2);

	RUN_TEST(closedir,1);
	RUN_TEST(closedir,2);
	RUN_TEST(closedir,3);

	RUN_TEST(stat,1);
	RUN_TEST(stat,2);
	RUN_TEST(stat,3);
	RUN_TEST(stat,4);

	RUN_TEST(dup,1);
	RUN_TEST(dup,2);
	RUN_TEST(dup,3);

	RUN_TEST(dup2,1);
	RUN_TEST(dup2,2);
	RUN_TEST(dup2,3);
	RUN_TEST(dup2,4);
	RUN_TEST(dup2,5);
	RUN_TEST(dup2,6);

}

/**
 * @brief
 * init lseek test
 *
 */
void init_lseekTest(){
	if(nandInit())
		return;

	initializeRamStructs();
	handleNoFs();
	init_fsbuf(fs_buffer);

}

/**
 * @brief
 * tear down lseek test
 *
 */
void tearDown_lseekTest(){
	init_flash();

	nandTerminate();
	initializeRamStructs();
}

/**
 * @brief
 * seek file to various legal offset. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t lseekTest1(){
	int32_t fd;
	int32_t offset = 100;

	fd = open("/", NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nfd success");
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), offset))
//	PRINT("\n SEEK_SET success");
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_CUR), offset*2));
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), offset*2))
//	PRINT("\n SEEK_CUR success");
	VERIFY(IS_NEGATIVE(lseek(fd, offset, FS_SEEK_END)));
//	PRINT("\n SEEK_END success");
	VERIFY(COMPARE(lseek(fd, 100, FS_SEEK_SET), 100));
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), 100))

	return 1;
}

/**
 * @brief
 * seek file to offset that exceeds file size. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t lseekTest2(){
	int32_t fd;
	int32_t offset = FS_MAX_FILESIZE*2;

	/* in case we test for a msx file size >= 2^31*/
	if(FS_MAX_FILESIZE*2 < FS_MAX_FILESIZE){
		return 1;
	}

	fd = open("/", NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nfd success");
	VERIFY(IS_NEGATIVE(lseek(fd, offset, FS_SEEK_SET)));
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd), 0))

	return 1;
}

/**
 * @brief
 * seek file with illegal whence, and illegal offset. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t lseekTest3(){
	int32_t fd;
	int32_t offset = -1;

	fd = open("/", NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nfd success");
	VERIFY(IS_NEGATIVE(lseek(fd, offset, FS_SEEK_SET)));
	VERIFY(IS_NEGATIVE(lseek(fd, 0, 400)));

	return 1;
}

/**
 * @brief
 * seek file that is written by a transaction that extended the file's offset, and hasn't
 * commited inode yet to flash. seek is done to newly written section. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t lseekTest4(){
	int32_t fd, tid =0, offset = 300;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);

	/* create and open file*/
//	PRINT("\ncreat");
	fd = creat("/file1.dat", 0);
	VERIFY(!IS_NEGATIVE(fd));

//	PRINT("\nget inode");
	tid = getFreeTransactionId();
	TRANSACTION_SET_INO(tid, 2);
	TRANSACTION_SET_FILE_OFFSET(tid, -1);

	VERIFY(!getInode(TRANSACTION_GET_INDIRECT_PTR(tid), 2,  TRANSACTION_GET_INO_ADDR_PTR(tid)));
	VERIFY(!IS_ADDR_EMPTY(TRANSACTION_GET_INO_ADDR_PTR(tid)));
//	PRINT_MSG_AND_NUM("\nfile inode addr=",logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	/* write data block*/
	fsMemset(TRANSACTION_GET_DATA_BUF_PTR(tid), 'c', FS_BLOCK_SIZE);
	VERIFY(!writeFileBlock(INODE_FILE_DATA_SIZE, FS_BLOCK_SIZE, DATA_TYPE_REGULAR, tid, log_addr, 1));
	init_fsbuf(TRANSACTION_GET_DATA_BUF_PTR(tid));

//	PRINT("\ntry to lseek");
	/* try to lseek*/
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET), offset));
//	PRINT("\nlseek success");
	return 1;
}

/**
 * @brief
 * init telldir test
 *
 */
void init_telldirTest(){
	init_lseekTest();
}

/**
 * @brief
 * tear down telldir test
 *
 */
void tearDown_telldirTest(){
	tearDown_lseekTest();
}

/**
 * @brief
 * tell dir directory. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t telldirTest1(){
	uint8_t *dir = "/";
	int32_t offset;
	NANDFS_DIR* ds;
	user_id uid = 1;

	SET_CURRENT_USER(uid);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	offset = telldir(ds);
	VERIFY(COMPARE(DIRECTORY_FIRST_ENTRY_OFFSET, DS_GET_DIR_OFFSET_BY_PTR(ds)));

	return 1;
}

/**
 * @brief
 * telldir empty dirstream. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t telldirTest2(){
	NANDFS_DIR* ds = NULL;
	VERIFY(COMPARE(-1, telldir(ds)));

	return 1;
}

/**
 * @brief
 * telldir directory in some offset. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t telldirTest3(){
	uint8_t *dir = "/";
	int32_t offset;
	NANDFS_DIR* ds;
	user_id uid = 1;

	SET_CURRENT_USER(uid);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));
	VERIFY(!IS_NULL(readdir(ds)));

	offset = telldir(ds);
	VERIFY(COMPARE(DIRECTORY_FIRST_ENTRY_OFFSET+DIRENT_GET_LEN(DS_GET_DIRENTRY_PTR_BY_PTR(ds)), DS_GET_DIR_OFFSET_BY_PTR(ds)));

	return 1;
}

/**
 * @brief
 * telldir directory opened by another user. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t telldirTest4(){
	uint8_t *dir = "/";
	int32_t offset;
	NANDFS_DIR* ds;
	user_id uid1 = 1, uid2 = 2;

	SET_CURRENT_USER(uid1);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	SET_CURRENT_USER(uid2);
	offset = telldir(ds);
	VERIFY(COMPARE(-1, offset));

	return 1;
}

/**
 * @brief
 * telldir directory after reaching EOF with readdir.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t telldirTest5(){
	uint8_t *dir = "/";
	NANDFS_DIR* ds;
	user_id uid = 1;
	int32_t offset = DIRECTORY_FIRST_ENTRY_OFFSET;

	SET_CURRENT_USER(uid);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	/*readdir ",", ".."*/
	VERIFY(!IS_NULL(readdir(ds)));
	offset += DIRENT_GET_LEN(DS_GET_DIRENTRY_PTR_BY_PTR(ds));
	VERIFY(!IS_NULL(readdir(ds)));
	offset += DIRENT_GET_LEN(DS_GET_DIRENTRY_PTR_BY_PTR(ds));
	VERIFY(IS_NULL(readdir(ds)));

//	PRINT_MSG_AND_NUM("\ntell dir res=", telldir(ds));
//	PRINT_MSG_AND_NUM(" expected=", getFileSize(1));
	VERIFY(COMPARE(offset, telldir(ds)));

	return 1;
}

/**
 * @brief
 * init dirfd test
 *
 */
void init_dirfdTest(){
	init_lseekTest();
}

/**
 * @brief
 * tear down dirfd test
 *
 */
void tearDown_dirfdTest(){
	tearDown_lseekTest();
}

/**
 * @brief
 * dirfd dirstream. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t dirfdTest1(){
	uint8_t *dir = "/";
	NANDFS_DIR* ds;
	user_id uid = 1;

	SET_CURRENT_USER(uid);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	VERIFY(COMPARE(dirfd(ds), DS_GET_FD_BY_PTR(ds)));

	return 1;
}

/**
 * @brief
 * dirfd empty dirstream. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t dirfdTest2(){
	NANDFS_DIR* ds = NULL;
	VERIFY(COMPARE(dirfd(ds), -1));

	return 1;
}

/**
 * @brief
 * dirfd dirstream opened by another user. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t dirfdTest3(){
	uint8_t *dir = "/";
	NANDFS_DIR* ds;
	user_id uid1 = 1, uid2 = 2;

	SET_CURRENT_USER(uid1);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	SET_CURRENT_USER(uid2);
	VERIFY(COMPARE(-1, dirfd(ds)));

	return 1;
}

/**
 * @brief
 * init seekdir test
 *
 */
void init_seekdirTest(){
	init_lseekTest();
}

/**
 * @brief
 * tear down seekdir test
 *
 */
void tearDown_seekdirTest(){
	tearDown_lseekTest();
}

/**
 * @brief
 * seekdir directoy. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t seekdirTest1(){
	uint8_t *dir = "/";
	int32_t offset;
	NANDFS_DIR* ds;
	user_id uid = 1;

	SET_CURRENT_USER(uid);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	VERIFY(!IS_NULL(readdir(ds)));
	offset = telldir(ds);
	VERIFY(!COMPARE(-1, offset));
	VERIFY(!IS_NULL(readdir(ds)));

	VERIFY(!COMPARE(-1, telldir(ds)));
	VERIFY(IS_NULL(readdir(ds)));

	seekdir(ds, offset);
	VERIFY(COMPARE(telldir(ds), offset));

//	PRINT("\nperform read from start");
	VERIFY(!IS_NULL(readdir(ds)));
//	PRINT("\nreaddir success");

	VERIFY(verifyDirenty(CAST_TO_DIRENT(DS_GET_DIRENTRY_PTR_BY_PTR(ds)), 1, FTYPE_DIR, 2, ".."));

	return 1;
}

/**
 * @brief
 * seekdir dirstream opened by another user. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t seekdirTest2(){
	uint8_t *dir = "/";
	int32_t offset1, offset2;
	NANDFS_DIR* ds;
	user_id uid1 = 1, uid2 = 2;

	SET_CURRENT_USER(uid1);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	VERIFY(!IS_NULL(readdir(ds)));
	offset1 = telldir(ds);
	VERIFY(!COMPARE(-1, offset1));
	VERIFY(!IS_NULL(readdir(ds)));
	offset2 = telldir(ds);
	VERIFY(!COMPARE(-1, offset2));

	VERIFY(!COMPARE(-1, telldir(ds)));

	SET_CURRENT_USER(uid2);
	seekdir(ds, offset1);

	SET_CURRENT_USER(uid1);

	/* verify seek hasn't had any outcome*/
	VERIFY(COMPARE(telldir(ds), offset2));

	return 1;
}

/**
 * @brief
 * init rewinddir test
 *
 */
void init_rewinddirTest(){
	init_lseekTest();
}

/**
 * @brief
 * tear down rewinddir test
 *
 */
void tearDown_rewinddirTest(){
	tearDown_lseekTest();
}

/**
 * @brief
 * rewinddir directoy. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t rewinddirTest1(){
	uint8_t *dir = "/";
	NANDFS_DIR* ds;
	user_id uid = 1;

	SET_CURRENT_USER(uid);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	VERIFY(!IS_NULL(readdir(ds)));
	VERIFY(!IS_NULL(readdir(ds)));

	VERIFY(!COMPARE(-1, telldir(ds)));
	VERIFY(!COMPARE(DIRECTORY_FIRST_ENTRY_OFFSET, telldir(ds)));

	rewinddir(ds);
	VERIFY(COMPARE(telldir(ds), DIRECTORY_FIRST_ENTRY_OFFSET));

	VERIFY(!IS_NULL(readdir(ds)));
	VERIFY(verifyDirenty(CAST_TO_DIRENT(DS_GET_DIRENTRY_PTR_BY_PTR(ds)), 1, FTYPE_DIR, 1, "."));
	return 1;
}

/**
 * @brief
 * rewinddir dirstream opened by another user. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rewinddirTest2(){
	uint8_t *dir = "/";
	int32_t offset2;
	NANDFS_DIR* ds;
	user_id uid1 = 1, uid2 = 2;

	SET_CURRENT_USER(uid1);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	VERIFY(!IS_NULL(readdir(ds)));
	VERIFY(!IS_NULL(readdir(ds)));
	offset2 = telldir(ds);
	VERIFY(!COMPARE(-1, offset2));

	SET_CURRENT_USER(uid2);
	rewinddir(ds);

	SET_CURRENT_USER(uid1);

	/* verify rewind hasn't had any outcome*/
	VERIFY(COMPARE(telldir(ds), offset2));

	return 1;
}

/**
 * @brief
 * init closedir test
 *
 */
void init_closedirTest(){
	init_lseekTest();
}

/**
 * @brief
 * tear down closedir test
 *
 */
void tearDown_closedirTest(){
	tearDown_lseekTest();
}

/**
 * @brief
 * close directory and verify it is now available for opening. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t closedirTest1(){
	uint8_t *dir = "/";
	NANDFS_DIR* ds;
	user_id uid = 1;
	int32_t i;

	SET_CURRENT_USER(uid);
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	/* open all available dirstreams*/
	for(i=0; i<FS_MAX_OPEN_DIRESTREAMS-1; i++){
		VERIFY(!IS_NULL(opendir(dir)));
	}

	/* demonstrate we can't open any more*/
	VERIFY(IS_NULL(opendir(dir)));

	/* close first opened*/
	VERIFY(!closedir(ds));
	VERIFY(IS_DIRSTREAM_EMPTY_BY_PTR(ds));

	/* show now we can open*/
	ds = opendir(dir);
	VERIFY(!IS_NULL(ds));

	return 1;
}

/**
 * @brief
 * close non-existsent directory stream. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t closedirTest2(){
	NANDFS_DIR* ds = NULL;
	VERIFY(closedir(ds));

	return 1;
}

/**
 * @brief
 * close directory opened by 2 users. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t closedirTest3(){
	uint8_t *dir = "/";
	NANDFS_DIR* ds1, *ds2;
	user_id uid1 = 1, uid2 = 2;

	SET_CURRENT_USER(uid1);
	ds1 = opendir(dir);
	VERIFY(!IS_NULL(ds1));

	SET_CURRENT_USER(uid2);
	ds2 = opendir(dir);
	VERIFY(!IS_NULL(ds2));

	/* try closing streams */
	VERIFY(closedir(ds1));
	VERIFY(!IS_DIRSTREAM_EMPTY_BY_PTR(ds1));
	VERIFY(!closedir(ds2));

	return 1;
}

/**
 * @brief
 * init stat test
 *
 */
void init_statTest(){
	init_lseekTest();
}

/**
 * @brief
 * tear down stat test
 *
 */
void tearDown_statTest(){
	tearDown_lseekTest();
}

/**
 * @brief
 * stat a file. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t statTest1(){
	uint8_t *file = "/file1.dat";
	int32_t fd;
	file_stat_t stat_struct, *stat_p = &stat_struct;

	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));

	VERIFY(!stat(file, stat_p));
//	PRINT("\nstat success");
	VERIFY(verifyStat(stat_p, GET_FILE_ID_BY_FD(fd), nandReadID(), NAND_PAGE_SIZE, 0, 1));

	return 1;
}

/**
 * @brief
 * stat a directory. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t statTest2(){
	uint8_t *dir = "/";
	file_stat_t stat_struct, *stat_p = &stat_struct;

	VERIFY(!stat(dir, stat_p));

	VERIFY(verifyStat(stat_p, 1, nandReadID(), NAND_PAGE_SIZE, DIRECTORY_FIRST_ENTRY_OFFSET+FS_BLOCK_SIZE, 2));

	return 1;
}

/**
 * @brief
 * stat non-existant file. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t statTest3(){
	file_stat_t stat_struct, *stat_p = &stat_struct;

	VERIFY(stat("/file11111", stat_p));

	return 1;
}

/**
 * @brief
 * stat a file in the middle of uncommited transacction and verify size is as transaction file size.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t statTest4(){
	uint8_t *file = "/file1.dat", buf[FS_BLOCK_SIZE];
	int32_t fd, i, block_count = DIRECT_INDEX_ENTRIES-2;
	file_stat_t stat_struct, *stat_p = &stat_struct;

	fsMemset(buf, 'a', FS_BLOCK_SIZE);
	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));

	for(i=0; i< block_count; i++){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	}

	VERIFY(!stat(file, stat_p));
//	PRINT("\nstat success");
	VERIFY(verifyStat(stat_p, GET_FILE_ID_BY_FD(fd), nandReadID(), NAND_PAGE_SIZE, block_count*FS_BLOCK_SIZE, 1+block_count));

	return 1;
}

/**
 * @brief
 * init dup test
 *
 */
void init_dupTest(){
	init_lseekTest();
}

/**
 * @brief
 * tear down dup test
 *
 */
void tearDown_dupTest(){
	tearDown_lseekTest();
}

/**
 * @brief
 * duplicate a file descriptor. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t dupTest1(){
	uint8_t *file = "/file.1dat";
	int32_t i, fd, new_fd, ino_num, offset, f_type, ofe_uid, flags;
	user_id uid = 1;

	SET_CURRENT_USER(uid);

	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	fd = open(file, NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd));

//	PRINT("\ncreat success");
	ino_num = GET_FILE_ID_BY_FD(fd);
	offset = OPEN_FILE_GET_OFFSET(fd);
	f_type = OPEN_FILE_GET_FTYPE(fd);
	ofe_uid = OPEN_FILE_GET_UID(fd);
	flags = OPEN_FILE_GET_FLAGS(fd);

//	PRINT("\nabout to dup");
	new_fd = dup(fd);
//	PRINT_MSG_AND_NUM("\ndup res=", new_fd);
	VERIFY(!IS_NEGATIVE(new_fd));
//	PRINT("\ndup success");

	VERIFY(verifyOpenFileEntry(new_fd, flags, offset, ofe_uid, 0));
//	PRINT("\nverifyOpenFileEntry success");

	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd || i==new_fd) continue;
//		PRINT_MSG_AND_NUM("\nverify fentry ", i);
		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nopen files entries verified");
	for(i=0; i<FS_MAX_VNODES;i++){
//		PRINT_MSG_AND_NUM("\nverify vnode ", i);
		if(i==OPEN_FILE_GET_VNODE(fd) || i==OPEN_FILE_GET_VNODE(new_fd)) continue;
		VERIFY(verifyVnodeEmpty(i));
	}

//	PRINT("\nverifyFentriesVnodesEmpty success");
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(new_fd), ino_num, 2));

	return 1;
}

/**
 * @brief
 * duplicate illegal file desciptors. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t dupTest2(){
	VERIFY(IS_NEGATIVE(dup(FS_MAX_OPEN_FILES)));
	VERIFY(IS_NEGATIVE(dup(0)));

	return 1;
}

/**
 * @brief
 * duplicate file descriptor when there are no free descriptors.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t dupTest3(){
	uint8_t *file = "/file1.dat";
	int32_t fd, i;

	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	/* use all file entries*/
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		fd = open(file, NANDFS_O_RDONLY, 0);
		VERIFY(!IS_NEGATIVE(fd));
	}

	VERIFY(IS_NEGATIVE(dup(fd)));

	return 1;
}

/**
 * @brief
 * init dup2 test
 *
 */
void init_dup2Test(){
	init_lseekTest();
}

/**
 * @brief
 * tear down dup2 test
 *
 */
void tearDown_dup2Test(){
	tearDown_lseekTest();
}

/**
 * @brief
 * duplicate a file descriptor to an empty file desceriptor.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test1(){
	uint8_t *file = "/file.1dat";
	int32_t i, fd, new_fd = 1, ino_num, offset, f_type, ofe_uid, flags, res;
	user_id uid = 1;

	SET_CURRENT_USER(uid);

	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	fd = open(file, NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd));

//	PRINT("\ncreat success");
	ino_num = GET_FILE_ID_BY_FD(fd);
	offset = OPEN_FILE_GET_OFFSET(fd);
	f_type = OPEN_FILE_GET_FTYPE(fd);
	ofe_uid = OPEN_FILE_GET_UID(fd);
	flags = OPEN_FILE_GET_FLAGS(fd);

//	PRINT("\nabout to dup");
	res = dup2(fd, new_fd);
//	PRINT_MSG_AND_NUM("\ndup res=", res);
	VERIFY(COMPARE(res, new_fd));
//	PRINT("\ndup success");

	VERIFY(verifyOpenFileEntry(new_fd, flags, offset, ofe_uid, 0));
//	PRINT("\nverifyOpenFileEntry success");

	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd || i==new_fd) continue;
//		PRINT_MSG_AND_NUM("\nverify fentry ", i);
		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nopen files entries verified");
	for(i=0; i<FS_MAX_VNODES;i++){
//		PRINT_MSG_AND_NUM("\nverify vnode ", i);
		if(i==OPEN_FILE_GET_VNODE(fd) || i==OPEN_FILE_GET_VNODE(new_fd)) continue;
		VERIFY(verifyVnodeEmpty(i));
	}

//	PRINT("\nverifyFentriesVnodesEmpty success");
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(new_fd), ino_num, 2));

	return 1;
}

/**
 * @brief
 * duplicate a file descriptor to a file open for reading
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test2(){
	uint8_t *file = "/file.1dat";
	int32_t i, fd, new_fd = 1, ino_num, offset, f_type, ofe_uid, flags, res;
	user_id uid = 1;

	SET_CURRENT_USER(uid);

	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");
	VERIFY(!close(fd));
//	PRINT("\nclose success");
	fd = open(file, NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nopen success");
	new_fd = open(file, NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(new_fd));

	ino_num = GET_FILE_ID_BY_FD(fd);
	offset  = OPEN_FILE_GET_OFFSET(fd);
	f_type  = OPEN_FILE_GET_FTYPE(fd);
	ofe_uid = OPEN_FILE_GET_UID(fd);
	flags   = OPEN_FILE_GET_FLAGS(fd);

//	PRINT("\nabout to dup");
	res = dup2(fd, new_fd);
//	PRINT_MSG_AND_NUM("\ndup res=", res);
	VERIFY(COMPARE(res, new_fd));
//	PRINT("\ndup success");

	VERIFY(verifyOpenFileEntry(new_fd, flags, offset, ofe_uid, 0));
//	PRINT("\nverifyOpenFileEntry success");

	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd || i==new_fd) continue;
//		PRINT_MSG_AND_NUM("\nverify fentry ", i);
		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nopen files entries verified");
	for(i=0; i<FS_MAX_VNODES;i++){
//		PRINT_MSG_AND_NUM("\nverify vnode ", i);
		if(i==OPEN_FILE_GET_VNODE(fd) || i==OPEN_FILE_GET_VNODE(new_fd)) continue;
		VERIFY(verifyVnodeEmpty(i));
	}

//	PRINT("\nverifyFentriesVnodesEmpty success");
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(new_fd), ino_num, 2));

	return 1;
}

/**
 * @brief
 * duplicate a file descriptor to a file open for writing
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test3(){
	uint8_t *file = "/file.1dat";
	user_id uid = 1;
	int32_t fd, new_fd;

	SET_CURRENT_USER(uid);

	new_fd = 1;
	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));

	VERIFY(IS_NEGATIVE(dup2(fd, new_fd)));

	return 1;
}

/**
 * @brief
 * duplicate a file descriptor to an illegal file descriptor
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test4(){
	uint8_t *file = "/file.1dat";
	int32_t fd;
	user_id uid = 1;

	SET_CURRENT_USER(uid);

	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	fd = open(file, NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd));

	VERIFY(IS_NEGATIVE(dup2(fd, FS_MAX_OPEN_FILES)));

	VERIFY(!close(fd));
	VERIFY(IS_NEGATIVE(dup2(fd, fd+1)));

	return 1;
}

/**
 * @brief
 * duplicate a file descriptor to a new file descriptor of a file open for writing.
 * when closing the new fd will result in failure because the current user cannot close it.
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test5(){
	uint8_t *file = "/file.1dat", *file2 = "/file.dat";
	int32_t fd, new_fd;
	user_id uid1 = 1, uid2 = 2;

	SET_CURRENT_USER(uid1);

	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));

	SET_CURRENT_USER(uid2);
	new_fd = creat(file2, 0);
	VERIFY(!IS_NEGATIVE(new_fd));

	SET_CURRENT_USER(uid1);
//	PRINT("\nabout to dup2");
	VERIFY(IS_NEGATIVE(dup2(fd, new_fd)));

	return 1;
}

/**
 * @brief
 * duplicate a file descriptor to an empty file desceriptor 3 times.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test6(){
	uint8_t *file = "/file.1dat";
	int32_t i, fd, new_fd1 = 1, new_fd2 = new_fd1+1, new_fd3 = new_fd2+1, ino_num, offset, f_type, ofe_uid, flags, res;
	user_id uid = 1;

	SET_CURRENT_USER(uid);

	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));

	fd = open(file, NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd));

//	PRINT("\ncreat success");
	ino_num = GET_FILE_ID_BY_FD(fd);
	offset = OPEN_FILE_GET_OFFSET(fd);
	f_type = OPEN_FILE_GET_FTYPE(fd);
	ofe_uid = OPEN_FILE_GET_UID(fd);
	flags = OPEN_FILE_GET_FLAGS(fd);

//	PRINT("\nabout to dup");
	res = dup2(fd, new_fd1);
	VERIFY(COMPARE(res, new_fd1));

	res = dup2(fd, new_fd2);
	VERIFY(COMPARE(res, new_fd2));

	res = dup2(fd, new_fd3);
	VERIFY(COMPARE(res, new_fd3));
//	PRINT("\ndup success");

	VERIFY(verifyOpenFileEntry(new_fd1, flags, offset, ofe_uid, 0));
	VERIFY(verifyOpenFileEntry(new_fd2, flags, offset, ofe_uid, 0));
	VERIFY(verifyOpenFileEntry(new_fd3, flags, offset, ofe_uid, 0));
//	PRINT("\nverifyOpenFileEntry success");

	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd || i==new_fd1 || i==new_fd2 || i==new_fd3) continue;
//		PRINT_MSG_AND_NUM("\nverify fentry ", i);
		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nopen files entries verified");
	for(i=0; i<FS_MAX_VNODES;i++){
//		PRINT_MSG_AND_NUM("\nverify vnode ", i);
		if(i==OPEN_FILE_GET_VNODE(fd) || i==OPEN_FILE_GET_VNODE(new_fd1)
		   || i==OPEN_FILE_GET_VNODE(new_fd2) || i==OPEN_FILE_GET_VNODE(new_fd3)) continue;
		VERIFY(verifyVnodeEmpty(i));
	}

//	PRINT("\nverifyFentriesVnodesEmpty success");
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(new_fd1), ino_num, 4));

	return 1;
}
