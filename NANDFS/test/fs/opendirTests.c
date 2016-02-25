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

/** @file opendirTests.c  */
#include <test/fs/opendirTests.h>
#include <test/fs/testsHeader.h>

void runAllOpendirTests(){
	
	RUN_TEST(opendir,1);
	RUN_TEST(opendir,2);
	RUN_TEST(opendir,5);
	RUN_TEST(opendir,6);
	RUN_TEST(opendir,7);
	RUN_TEST(opendir,8);
	RUN_TEST(opendir,9);
	
}

/**
 * @brief 
 * init opendir test
 * 
 */
void init_opendirTest(){
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
void tearDown_opendirTest(){
	init_flash();
	nandTerminate();
	initializeRamStructs();	
}

/**
 * @brief
 * open an existing directory for reading.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest1(){
	uint8_t *dir = "/";	
	NANDFS_DIR* ds;
	user_id uid = 1;
	
	SET_CURRENT_USER(uid);
	/* opendir*/
	ds = opendir(dir);
	VERIFY(ds!=NULL);
//	PRINT("\nopendir success");
	/* verify directory stream details. */
	VERIFY(IS_DIRENT_EMPTY(DS_GET_DIRENTRY_PTR(ds)));	
//	PRINT("\ndirentry verified");
		
	VERIFY(verifyOpenFileEntry(DS_GET_FD_BY_PTR(ds), NANDFS_O_RDONLY, DS_GET_DIR_OFFSET_BY_PTR(ds), uid, 0));
//	PRINT("\nopen file entry verified");
	VERIFY(verifyVnode(0, 1, 1));
//	PRINT("\nvnode verified");
	
	return 1;
}

/**
 * @brief
 * open a directory that doesn't exists for reading.
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest2(){	
	uint8_t *dir = "/dir1";	
		
	VERIFY(COMPARE(NULL, opendir(dir)));
	
	return 1;
}


/**
 * @brief
 * open existing directory twice from 2 different users
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest5(){
	user_id user1 = 1, user2 = 2;
	uint32_t i;
	int32_t fd1, fd2;
	uint8_t *dir = "/";
	NANDFS_DIR *ds1, *ds2;
	nandfs_dirent *de;
	
	/* set user1 in process*/
	SET_CURRENT_USER(user1);
	ds1 = opendir(dir);
	VERIFY(!COMPARE(NULL,ds1));
	fd1  =DS_GET_FD_BY_PTR(ds1);
//	PRINT("\nopendir 1 success");
	 
	/* verify open */	
	VERIFY(verifyOpenFileEntry(fd1 , NANDFS_O_RDONLY,DIRECTORY_FIRST_ENTRY_OFFSET,user1,OPEN_FILE_GET_VNODE(fd1)));
//	PRINT("\nverified 1 fentry ");
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd1), 1, 1));
//	PRINT("\nverified 1 vnode");
	
	/* set user1 in process*/
	SET_CURRENT_USER(user2);
	ds2 = opendir(dir);
	VERIFY(!COMPARE(NULL,ds2));
	fd2  =DS_GET_FD_BY_PTR(ds2);
//	PRINT("\nopendir 2nd success");
	
	/* verify open */
	VERIFY(verifyOpenFileEntry(fd2 , NANDFS_O_RDONLY,DIRECTORY_FIRST_ENTRY_OFFSET,user2,OPEN_FILE_GET_VNODE(fd2)));
	VERIFY(verifyVnode(OPEN_FILE_GET_VNODE(fd2), 1, 2));
	
	/* verify all other file entries are empty*/
	for(i=0;i< FS_MAX_OPEN_FILES;i++){
		if(i==fd1 || i == fd2)
			continue;
		
		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nvnode fentry 2 verified");
	/* verify all other vnodes are empty */
	for(i=0;i< FS_MAX_VNODES;i++){
		if(i==OPEN_FILE_GET_VNODE(fd1) || i==OPEN_FILE_GET_VNODE(fd2))
			continue;
		
		VERIFY(verifyVnodeEmpty(i));
	}
	
	/* verify directory entries of directory streams*/
	de = DS_GET_DIRENTRY_PTR_BY_PTR(ds1);	
	VERIFY(IS_DIRENT_EMPTY(DS_GET_DIRENTRY_PTR(ds1)));	
//	PRINT("\ndirentry 1 verified");
	de = DS_GET_DIRENTRY_PTR_BY_PTR(ds2);	
	VERIFY(IS_DIRENT_EMPTY(DS_GET_DIRENTRY_PTR(ds2)));	
	
	return 1;
}
/**
 * @brief
 * open a file for writing, then try to open it's directory for reading
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest6(){
	uint8_t *dir = "/dir1", *file = "/dir1/file1";
	int32_t fd;
	NANDFS_DIR *ds;
	nandfs_dirent *de;
	user_id uid = 1;
	
	SET_CURRENT_USER(uid);
	
	VERIFY(!mkdir(dir, 0));
	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));
	
	ds = opendir(dir);
	VERIFY(!COMPARE(NULL, ds));
//	PRINT("\nopendir success");
	de = DS_GET_DIRENTRY_PTR(ds);
	VERIFY(IS_DIRENT_EMPTY(DS_GET_DIRENTRY_PTR(ds)));	
//	PRINT("\ndirent success");
		
	VERIFY(verifyOpenFileEntry(DS_GET_FD(0), NANDFS_O_RDONLY, DS_GET_DIR_OFFSET_BY_PTR(ds), uid, 1));
//	PRINT("\nfentry success");
	VERIFY(verifyVnode(1, 2, 1));
	
	return 1;
}

/**
 * @brief
 * open a directory that exists for reading, the try to unlink a file in it
 * should fail (write exclusive)
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest7(){
	uint8_t *dir = "/dir1", *file = "/dir1/file1";
	int32_t fd, rec_addr, frees;
	NANDFS_DIR *ds;
	
	VERIFY(!mkdir(dir, 0));
	fd = creat(file, 0);
	VERIFY(!IS_NEGATIVE(fd));
	VERIFY(!close(fd));
	
	ds = opendir(dir);
	VERIFY(!COMPARE(NULL, ds));
	
	frees = calcTotalFreePages();
	rec_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	
	/* verify unlink failure*/
	VERIFY(unlink(file));
	VERIFY(COMPARE(frees, calcTotalFreePages()));
	VERIFY(COMPARE(rec_addr, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	
	return 1;
}

/**
 * @brief
 * open FS_MAX_OPEN_DIRESTREAMS different directories for reading , then another directory and making sure
 * the last open fails with the matching error 
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest8(){
	uint8_t *dir = "/";
	int32_t i;
	user_id uid =1;	
	NANDFS_DIR *ds[FS_MAX_OPEN_DIRESTREAMS];
	
	SET_CURRENT_USER(uid);		
	
	/* fill all vnodes */
	for(i=0; i<FS_MAX_OPEN_DIRESTREAMS;i++){
		ds[i] = opendir(dir);
		VERIFY(!COMPARE(NULL, ds[i]));		
//		PRINT_MSG_AND_NUM("\n", i);
//		PRINT_MSG_AND_NUM(". ds fd=", DS_GET_FD(i));
//		PRINT("\n");
//		
//		if(i) break;
	}
			
	ds[i] = opendir(dir);
	VERIFY(COMPARE(NULL, ds[i]));
	
	return 1;
}

/**
 * @brief
 * open FS_MAX_VNODES different files for reading , then try to open a directory.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest9(){
	uint8_t *dir = "/";
	user_id uid = 1;
	uint32_t i;
	NANDFS_DIR *ds;
	
	SET_CURRENT_USER(uid);
	
	/* fill all vnodes */
	for(i=0; i<FS_MAX_VNODES;i++){
		VNODE_SET_INO_NUM(i, i+100);
		VNODE_SET_NREFS(i, i);
	}
			
	/* set user in process*/
	ds = opendir(dir);
	VERIFY(COMPARE(NULL, ds));
	
	return 1;
}
