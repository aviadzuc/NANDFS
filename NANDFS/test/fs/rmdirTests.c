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

/** @file rmdirTests.c  */
#include <test/fs/rmdirTests.h>
#include <test/fs/testsHeader.h>

void runAllRmdirTests(){

	RUN_TEST(rmdir,1);
	RUN_TEST(rmdir,2);
	RUN_TEST(rmdir,3);
	RUN_TEST(rmdir,4);
	RUN_TEST(rmdir,5);
	RUN_TEST(rmdir,6);
	RUN_TEST(rmdir,7);
	RUN_TEST(rmdir,8);
	RUN_TEST(rmdir,9);
	RUN_TEST(rmdir,10);
	RUN_TEST(rmdir,11);
	RUN_TEST(rmdir,12);
	RUN_TEST(rmdir,13);
	RUN_TEST(rmdir,14);
	RUN_TEST(rmdir,15);

}

/**
 * @brief
 * init rmdir test
 */
void init_rmdirTest(){
	if(nandInit())
		return;

	init_flash();
	initializeRamStructs();
	handleNoFs();
	init_fsbuf(fs_buffer);
	SET_CURRENT_USER(-1);
}

/**
 * @brief
 * tear down rmdir test
 */
void tearDown_rmdirTest(){
	init_flash();

	nandTerminate();
	initializeRamStructs();
}

/**
 * @brief
 * rmdir an existing directory in root directory
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest1(){
	int32_t free_pages, i;
	uint8_t *d_name = "/dir1";
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_ino0);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_dir);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_root_entries);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(old_ino0);
	init_logical_address(old_dir);
	init_logical_address(old_root);
	init_logical_address(old_root_entries);

	/* create directory */
	free_pages = calcTotalFreePages();
	VERIFY(!mkdir(d_name, 0));

	copyLogicalAddress(old_ino0, FS_GET_INO0_ADDR_PTR());
	VERIFY(!getInode(fs_buffer, 2, old_dir));
	VERIFY(!getInode(fs_buffer, 1, old_root));
	VERIFY(!fsReadBlockSimple(old_root, fs_buffer));
	INODE_GET_DIRECT(ino_ptr, 0, old_root_entries);
	init_fsbuf(fs_buffer);
//	PRINT("\nabout to unlink");
//	PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	VERIFY(!rmdir(d_name));
//	PRINT("\nrmdir success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify inode0 */
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
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
	VERIFY(!fsReadBlockSimple(root_addr, fs_buffer));
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
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));

	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));
	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",calcTotalFreePages());
	VERIFY(COMPARE(free_pages, calcTotalFreePages()));

	/* verify old pages were marked obsolete */
//	PRINT("\ncheck if page is obsolete");
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(old_ino0)));
//	PRINT("\nold_ino0 success");
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(old_root)));
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(old_dir)));
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(old_root_entries)));

	return 1;
}

/**
 * @brief
 * rmdir a non-existant directory in root directory. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest2(){
	int32_t frees;
	uint8_t *d_name = "/dir1";

	frees = calcTotalFreePages();

	/* verify rmdir failure */
	VERIFY(rmdir(d_name));
	VERIFY(COMPARE(frees, calcTotalFreePages()));

	return 1;
}

/**
 * @brief
 * rmdir an existing directory not in root directory. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest3(){
	int32_t free_pages, i;
	uint8_t *d_name = "/dir1/dir2", *d_name0 = "/dir1";
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir1_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);

	init_logical_address(log_addr);
	init_logical_address(root_addr);
	init_logical_address(dir1_addr);

	VERIFY(!mkdir(d_name0, 0));

	/* create directory */
	free_pages = calcTotalFreePages();
	VERIFY(!mkdir(d_name, 0));
//	PRINT("\nmkdir success");
//
//	PRINT_MSG_AND_NUM("\n inode0 file id=", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nread inode0 from address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
	VERIFY(!rmdir(d_name));
//	PRINT("\nrmdir success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify inode0 */
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 4, INODE_FILE_DATA_SIZE+3*FS_BLOCK_SIZE));

	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	INODE_GET_DIRECT(ino_ptr, 1, dir1_addr);
//	PRINT("\nverified inode0 direct 0");

	for(i=2; i<DIRECT_INDEX_ENTRIES; i++){
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
	VERIFY(!fsReadBlockSimple(root_addr, fs_buffer));
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
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));

	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	VERIFY(verifyDirenty(de_ptr, 2, FTYPE_DIR, calcNameLen(&(d_name0[1])), &(d_name0[1])));
	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	VERIFY(IS_DIRENT_EMPTY(de_ptr));
//	PRINT("\nverified root direntries")

	/* verify dir2 in dir1*/
	VERIFY(!fsReadBlockSimple(dir1_addr, fs_buffer));
	VERIFY(verifyInode(ino_ptr, 2, FTYPE_DIR, 2, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT("\nverified dir1 inode");

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\nverified dir1 directs");

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
//	PRINT("\nverified dir1 dir");

	/* verify direntry was deleted -
	 * check root directory now contains only first two direntries (".", "..")*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));

	de_ptr = CAST_TO_DIRENT(fs_buffer);
	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));
	VERIFY(IS_DIRENT_EMPTY(de_ptr));

	/* verify free pages count is as before file creation*/
//	PRINT_MSG_AND_NUM("\nexpected free_pages=",free_pages);
//	PRINT_MSG_AND_NUM("\nfree_pages=",calcTotalFreePages());
	VERIFY(COMPARE(free_pages, calcTotalFreePages()));

	return 1;
}

/**
 * @brief
 * rmdir a non-existant directory not in root directory. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest4(){
	int32_t free_pages, rec_addr;
	uint8_t *d_name = "/dir1/dir2", *d_name0 = "/dir1";

	VERIFY(!mkdir(d_name0, 0));

	/* create directory */
	free_pages = calcTotalFreePages();
	rec_addr   = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	VERIFY(rmdir(d_name));

	VERIFY(COMPARE(free_pages, calcTotalFreePages()));
	VERIFY(COMPARE(rec_addr, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	return 1;
}

/**
 * @brief
 * rmdir a directory open for reading. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest5(){
	int32_t fd = 0, vnode_idx = 0, frees, rec_address;
	user_id uid = 0;
	uint8_t *d_name = "/dir1";

	VERIFY(!mkdir(d_name, 0));

	/* set open file entry for root directory */
	OPEN_FILE_SET_FLAGS(fd, NANDFS_O_RDONLY);
	OPEN_FILE_SET_FTYPE(fd, FTYPE_DIR);
	OPEN_FILE_SET_OFFSET(fd, INODE_FILE_DATA_SIZE);
	OPEN_FILE_SET_UID(fd, uid);
	OPEN_FILE_SET_VNODE(fd, vnode_idx);

	VNODE_SET_INO_NUM(vnode_idx, 2);
	VNODE_SET_NREFS(vnode_idx, 1);

	frees = calcTotalFreePages();
	rec_address = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* verify rmdir failure*/
	VERIFY(rmdir(d_name));
//	PRINT("\nrmdir success");
	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_address, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\nfrees, rec addr success");
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyOpenFileEntry(fd, NANDFS_O_RDONLY, INODE_FILE_DATA_SIZE, uid, vnode_idx));
	VERIFY(verifyVnode(vnode_idx, 2, 1));

	return 1;
}

/**
 * @brief
 * rmdir a directory whose parent directory is open for reading
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest6(){
	int32_t fd = 0, vnode_idx = 0, frees, rec_address;
	user_id uid = 0;
	uint8_t *d_name = "/dir1";

	VERIFY(!mkdir(d_name, 0));

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

	/* verify rmdir failure*/
	VERIFY(rmdir(d_name));
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
 * rmdir a directory where the suffix of pathname contains "." or "..". try to rmdir root directory
 * should fail.
 *
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest7(){
	int32_t frees, rec_address;
	uint8_t *d_name1 = "/..", *d_name2 = "/.", *d_name3 = "/";

	frees = calcTotalFreePages();
	rec_address = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());

	/* verify rmdir failure*/
	VERIFY(rmdir(d_name1));
//	PRINT("\nsuccess ..");
	VERIFY(rmdir(d_name2));
//	PRINT("\nsuccess .");
	VERIFY(rmdir(d_name3));
//	PRINT("\nsuccess /");

	VERIFY(COMPARE(frees, calcTotalFreePages()));
//	PRINT("\nsuccess frees");
	VERIFY(COMPARE(rec_address, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	return 1;
}

/**
 * @brief
 * rmdir a directory, and re-creat it. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest8(){
	uint8_t *d_name = "/dir1";
	int32_t i, frees;
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
	/* create, remove, re-create*/
	VERIFY(!mkdir(d_name,0));
//	PRINT("\nmkdir 1 success");
	frees = calcTotalFreePages();
	VERIFY(!rmdir(d_name));
//	PRINT("\nrmdir success");
	VERIFY(!mkdir(d_name,0));

//	PRINT("\nmkdir 2 success");
//	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
//	VERIFY(!getInode(fs_buffer, 2, ino_log_addr));
//	PRINT_MSG_AND_HEX("\nfile size=", INODE_GET_NBYTES(ino_ptr));
//	assert(0);

	/* verify file entries and vnodes */
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified fetries vnodes");

	/* verify inode0 sizes changed*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr, 0, root_addr);
	INODE_GET_DIRECT(ino_ptr, 1, dir_addr);
	VERIFY(verifyInode(ino_ptr, 0, FTYPE_FILE, 3, INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
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
	VERIFY(IS_ADDR_FREE(log_addr));
	VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	verifyDirenty(dirent_ptr, 2, FTYPE_DIR, 4, &(d_name[1]));
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
//	PRINT("\nfile sizes verified");
//	PRINT_MSG_AND_NUM("\nobs count=",GET_OBS_COUNT());
	/* 1 old ino0, 1 for old dirent page, 1 for old directory inode page, 1 for old inode0, 1 for vots page
	 * times 3 + 2 for ld inode and direntries page of when the directoy was deleted*/
	VERIFY(COMPARE(5*2+7, GET_OBS_COUNT()));

	/* verify empty direntries*/
	VERIFY(!fsReadBlockSimple(block_addr, fs_buffer));
	dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	VERIFY(verifyDirenty(dirent_ptr, 2, FTYPE_DIR, 1, "."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(verifyDirenty(dirent_ptr, 1, FTYPE_DIR, 2, ".."));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	VERIFY(IS_DIRENT_EMPTY(dirent_ptr));

	VERIFY(COMPARE(frees, calcTotalFreePages()));

	return 1;
}

/**
 * @brief
 * rmdir directory that contains a direntry in first inode direct entry.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest9(){
	int32_t fd, frees, rec_addr;
	uint8_t *d_name = "/dir1", *f_name = "/dir1/file1.dat";
	user_id uid = 1;

	SET_CURRENT_USER(uid);
	/* create directory */
	VERIFY(!mkdir(d_name, 0));

	/* create file in directory (direntry stored in first direntries block) */
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	/* try removind directory */
	frees = calcTotalFreePages();
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	VERIFY(rmdir(d_name));
//	PRINT("\nfirst rmdir success");
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
//	PRINT("\ntransaction fentries verified");
	VERIFY(verifyOpenFileEntry(fd, CREAT_FLAGS, 0, uid, 0));


	/* even after closing file should fail*/
	VERIFY(!close(fd));
//	PRINT("\nfile closed successfuly");
	VERIFY(rmdir(d_name));
//	PRINT("\n2nd rmdir success");

	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_addr, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	return 1;
}

/**
 * @brief
 * rmdir directory that contains a direntry in some inode direct entry.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest10(){
	int32_t frees, rec_addr;
	uint8_t *d_name = "/dir1";
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(mock_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_addr);
	bool_t cpWritten;

	init_logical_address(mock_addr);
	init_logical_address(dir_addr);

	MARK_BLOCK_NO_HOLE(mock_addr);
	SET_LOGICAL_OFFSET(mock_addr, 0);
	SET_LOGICAL_SEGMENT(mock_addr, 0);

	/* create directory */
	VERIFY(!mkdir(d_name, 0));

	/* create mock taken inode direct entry*/
	VERIFY(!getInode(fs_buffer, 2, dir_addr));
	INODE_SET_DIRECT(ino_ptr, 1, mock_addr);
	VERIFY(!allocAndWriteBlock(dir_addr, fs_buffer, 0, mock_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_NO_HOLE(dir_addr);
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, dir_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, mock_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT("\nallocs success");

	/* try removind directory */
	frees = calcTotalFreePages();
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	VERIFY(rmdir(d_name));
//	PRINT("\nrmdir success");
	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_addr, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\nfrees, rec_addr success");

	/* verify inode still exists */
	init_logical_address(dir_addr);
	VERIFY(!getInode(fs_buffer, 2, dir_addr));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 2))

	return 1;
}

/**
 * @brief
 * rmdir directory that contains a direntry in indirect entry.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest11(){
	int32_t frees, rec_addr;
	uint8_t *d_name = "/dir1";
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(mock_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_addr);
	bool_t cpWritten;

	init_logical_address(mock_addr);
	init_logical_address(dir_addr);

	MARK_BLOCK_NO_HOLE(mock_addr);
	SET_LOGICAL_OFFSET(mock_addr, 0);
	SET_LOGICAL_SEGMENT(mock_addr, 0);

	/* create directory */
	VERIFY(!mkdir(d_name, 0));

	/* create mock taken inode indirect entry*/
	VERIFY(!getInode(fs_buffer, 2, dir_addr));
	INODE_SET_INDIRECT(ino_ptr, mock_addr);
	VERIFY(!allocAndWriteBlock(dir_addr, fs_buffer, 0, mock_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_NO_HOLE(dir_addr);
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, dir_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, mock_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT("\nallocs success");

	/* try removind directory */
	frees = calcTotalFreePages();
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	VERIFY(rmdir(d_name));
//	PRINT("\nrmdir success");
	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_addr, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\nfrees, rec_addr success");

	/* verify inode still exists */
	init_logical_address(dir_addr);
	VERIFY(!getInode(fs_buffer, 2, dir_addr));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 2))

	return 1;
}

/**
 * @brief
 * rmdir directory that contains a direntry in double entry.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest12(){
	int32_t frees, rec_addr;
	uint8_t *d_name = "/dir1";
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(mock_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_addr);
	bool_t cpWritten;

	init_logical_address(mock_addr);
	init_logical_address(dir_addr);

	MARK_BLOCK_NO_HOLE(mock_addr);
	SET_LOGICAL_OFFSET(mock_addr, 0);
	SET_LOGICAL_SEGMENT(mock_addr, 0);

	/* create directory */
	VERIFY(!mkdir(d_name, 0));

	/* create mock taken inode double entry*/
	VERIFY(!getInode(fs_buffer, 2, dir_addr));
	INODE_SET_DOUBLE(ino_ptr, mock_addr);
	VERIFY(!allocAndWriteBlock(dir_addr, fs_buffer, 0, mock_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_NO_HOLE(dir_addr);
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, dir_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, mock_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT("\nallocs success");

	/* try removind directory */
	frees = calcTotalFreePages();
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	VERIFY(rmdir(d_name));
//	PRINT("\nrmdir success");
	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_addr, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\nfrees, rec_addr success");

	/* verify inode still exists */
	init_logical_address(dir_addr);
	VERIFY(!getInode(fs_buffer, 2, dir_addr));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 2))

	return 1;
}

/**
 * @brief
 * rmdir directory that contains a direntry in triple entry.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest13(){
	int32_t frees, rec_addr;
	uint8_t *d_name = "/dir1";
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(mock_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_addr);
	bool_t cpWritten;

	init_logical_address(mock_addr);
	init_logical_address(dir_addr);

	MARK_BLOCK_NO_HOLE(mock_addr);
	SET_LOGICAL_OFFSET(mock_addr, 0);
	SET_LOGICAL_SEGMENT(mock_addr, 0);

	/* create directory */
	VERIFY(!mkdir(d_name, 0));

	/* create mock taken inode double entry*/
	VERIFY(!getInode(fs_buffer, 2, dir_addr));
	INODE_SET_TRIPLE(ino_ptr, mock_addr);
	VERIFY(!allocAndWriteBlock(dir_addr, fs_buffer, 0, mock_addr, &cpWritten, fsCheckpointWriter, 0));
	MARK_BLOCK_NO_HOLE(dir_addr);
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 1, dir_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, mock_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT("\nallocs success");

	/* try removind directory */
	frees = calcTotalFreePages();
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	VERIFY(rmdir(d_name));
//	PRINT("\nrmdir success");
	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_addr, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\nfrees, rec_addr success");

	/* verify inode still exists */
	init_logical_address(dir_addr);
	VERIFY(!getInode(fs_buffer, 2, dir_addr));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), 2))

	return 1;
}

/**
 * @brief
 * rmdir directories recursively until 256 depth
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest14(){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_ino0);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_dir);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_parent);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_parent_entries);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *de_ptr = CAST_TO_DIRENT(fs_buffer);
	int32_t i, max_depth=256, frees;
	uint8_t *d_name = "/d", d_new_name[256*2+2];

	init_logical_address(log_addr);
	init_logical_address(ino_addr);
	init_logical_address(old_ino0);
	init_logical_address(old_dir);
	init_logical_address(old_parent);
	init_logical_address(old_parent_entries);

	frees = calcTotalFreePages();
	/* create directories recursively*/
	for(i=0; i< max_depth; i++){
		fsStrcpy(&(d_new_name[i*2]), d_name);
		d_new_name[i*2+2] = '\0';

		VERIFY(!mkdir(d_new_name,0));
//		PRINT_MSG_AND_STR("\ncreated ", d_new_name);
	}

	/* remove directories recursively*/
	for(; i>0; i--){
		d_new_name[(i-1)*2+2] = '\0';
		init_logical_address(old_dir);
		init_logical_address(old_parent);

		copyLogicalAddress(old_ino0, FS_GET_INO0_ADDR_PTR());
		VERIFY(!getInode(fs_buffer, i+1, old_dir));
		VERIFY(!getInode(fs_buffer, i, old_parent));
		VERIFY(!fsReadBlockSimple(old_parent, fs_buffer));
		INODE_GET_DIRECT(ino_ptr, 0, old_parent_entries);
		init_fsbuf(fs_buffer);

		VERIFY(!rmdir(d_new_name));
		VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(old_ino0)));
		VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(old_parent)));
		VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(old_dir)));
		VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(old_parent_entries)));

		/* verify parent directory contains no entries now */
		VERIFY(!getInode(fs_buffer, i, ino_addr));
		INODE_GET_DIRECT(ino_ptr,0 , log_addr);
		VERIFY(!fsReadBlockSimple(log_addr, fs_buffer));
		de_ptr = CAST_TO_DIRENT(fs_buffer);
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
		moveToNextDirentry(&de_ptr, DIRENT_GET_LEN(de_ptr));
	//	PRINT_MSG_AND_NUM("\nempty direntry ino num=",DIRENT_GET_INO_NUM(de_ptr));
		VERIFY(IS_DIRENT_EMPTY(de_ptr));
	}



	/* verify free pages - subtract inode, direntries page for every directory
	 * and also inode0 indirect+double+double indirect*/
//	PRINT_MSG_AND_NUM("\ntotal frees=",calcTotalFreePages());
//	PRINT_MSG_AND_NUM(" expected=",frees);
	VERIFY(COMPARE(frees, calcTotalFreePages()));

	return 1;
}

/**
 * @brief
 * rmdir file whose direntry which resides in triple entry in parent directory
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest15(){
	int32_t free_pages, i, nBlocks = (TRIPLE_DATA_OFFSET-INODE_FILE_DATA_SIZE)/FS_BLOCK_SIZE;
	uint8_t *d_name = "/dir1", entries_buf[FS_BLOCK_SIZE];
	user_id uid = 1;
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(mock_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(triple_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(entries_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_ino0_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_root_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_entries_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);

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
	init_logical_address(dir_addr);
	init_logical_address(entries_addr);
	init_logical_address(old_ino0_addr);
	init_logical_address(old_root_addr);
	init_logical_address(dir_entries_addr);

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
	fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, root_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, 0));

	free_pages = calcTotalFreePages();

	/* create file. should be created in indirect offset*/
//	PRINT("\ncreate file");
	VERIFY(!mkdir(d_name,0));
	VERIFY(!getInode(fs_buffer, 2, dir_addr));
	INODE_GET_DIRECT(ino_ptr, 0, dir_entries_addr);
//	PRINT_MSG_AND_NUM("\ndir_addr=", logicalAddressToPhysical(dir_addr));

	init_logical_address(root_addr);
	VERIFY(!getInode(fs_buffer, 1, root_addr));
//	PRINT_MSG_AND_NUM("\nroot_addr=", logicalAddressToPhysical(root_addr));
//	PRINT_MSG_AND_NUM("\nroot size=", INODE_GET_NBYTES(CAST_TO_INODE(fs_buffer)));
//	assert(0);

	/* save root indirect block addrees, so later we can verify it is obsolete*/
//	PRINT("\n\n\nabout to unlink");
	init_logical_address(root_addr);
	VERIFY(!getInode(fs_buffer, 1, root_addr));
//	PRINT("\ngetInode success");
	INODE_GET_TRIPLE(ino_ptr, triple_addr);
	VERIFY(!fsReadBlockSimple(triple_addr, fs_buffer));
//	PRINT("\nread triple_addr");

	BLOCK_GET_INDEX(fs_buffer,0,double_addr);
	VERIFY(!fsReadBlockSimple(double_addr, fs_buffer));
	BLOCK_GET_INDEX(fs_buffer,0,indirect_addr);
	VERIFY(!fsReadBlockSimple(indirect_addr, fs_buffer));
//	PRINT("\n indirect read");

	BLOCK_GET_INDEX(fs_buffer,0,entries_addr);
	copyLogicalAddress(old_root_addr, root_addr);
	copyLogicalAddress(old_ino0_addr, FS_GET_INO0_ADDR_PTR());

	/* unlink file. should delete inode, and indiret block leading to it*/
	VERIFY(!rmdir(d_name));
//	PRINT("\nunlink success");

	/* verify all transactions, fentries and vnodes empty*/
	VERIFY(verifyTransactionsEmpty(TID_EMPTY));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\nverified transactions, fentries and vnodes empty");

	/* verify inode0 */
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
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
	VERIFY(!fsReadBlockSimple(root_addr, fs_buffer));
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
//	PRINT_MSG_AND_NUM("\nfree_pages=",calcTotalFreePages());
	VERIFY(COMPARE(free_pages, calcTotalFreePages()));
//	PRINT("\nfrees success");

	/* verify old addresses is now obsolete*/
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(triple_addr)));
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(double_addr)));
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(indirect_addr)));
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(entries_addr)));
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(dir_addr)));
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(old_ino0_addr)));
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(old_root_addr)));
//	PRINT("\nold_root_addr success");
//	PRINT_MSG_AND_NUM("\ncheck addr=", logicalAddressToPhysical(dir_entries_addr));
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(dir_entries_addr)));


	return 1;
}
