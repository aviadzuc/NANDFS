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

/** @file readTests.c  */
#include <test/fs/readTests.h>
#include <test/fs/testsHeader.h>

void runAllreadTests(){
	RUN_TEST(read,1);
	RUN_TEST(read,2);
	RUN_TEST(read,3);
	RUN_TEST(read,4);
	RUN_TEST(read,5);
	RUN_TEST(read,6);
	RUN_TEST(read,7);
	RUN_TEST(read,8);
	RUN_TEST(read,9);
	RUN_TEST(read,10);
	RUN_TEST(read,11);
	RUN_TEST(read,12);
	RUN_TEST(read,13);
	RUN_TEST(read,14);
	RUN_TEST(read,15);
	RUN_TEST(read,16);
	RUN_TEST(read,17);
}


/**
 * @brief
 * init read test
 */
void init_readTest(){
	if(nandInit())
		return;

	init_flash();
	initializeRamStructs();
	handleNoFs();
	init_fsbuf(fs_buffer);
}

/**
 * @brief
 * tear down read test
 */
void tearDown_readTest(){
	init_flash();

	nandTerminate();
	init_fsbuf(fs_buffer);
	initializeRamStructs();
}

/**
 * @brief
 * try reading empty file. should return 0
 * @return 1 if successful, 0 otherwise
 */
error_t readTest1(){
	int32_t res, fd, vnode_idx, f_id;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE];
	user_id uid = 1;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\ncreat success");

	/* verify read failed (file not open for writing*/
	VERIFY(COMPARE(-1, read(fd, buf, 100)));
	VERIFY(!close(fd));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\ncreat close success");

	/* open again for reading*/
	fd = open(f_name, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd));

//	PRINT("\nre-open success");
	f_id = GET_FILE_ID_BY_FD(fd);
	vnode_idx = OPEN_FILE_GET_VNODE(fd);
	res = read(fd, buf, 100);
//	PRINT_MSG_AND_NUM("\nread res=", res);
	VERIFY(COMPARE(0, res));

//	PRINT("\nread success");
	VERIFY(verifyFentriesVnodesEmpty(fd));
//	PRINT("\nempty fentries success");
	VERIFY(verifyOpenFileEntry(fd, NANDFS_O_RDONLY, 0, uid, vnode_idx));
//	PRINT("\nfentry success");
	VERIFY(verifyVnode(vnode_idx, f_id,1));

	VERIFY(!close(fd));

	return 1;
}

/**
 * @brief
 * read data from inode file data.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t readTest2(){
	int32_t fd, vnode_idx, f_id;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'a', count = 100;
	user_id uid = 1;
	int32_t i;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(write(fd, buf, INODE_FILE_DATA_SIZE),INODE_FILE_DATA_SIZE));

	VERIFY(!close(fd));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\ncreat close success");
	/* open again for reading*/
	fd = open(f_name, NANDFS_O_RDONLY,0);
	f_id = GET_FILE_ID_BY_FD(fd);
	vnode_idx = OPEN_FILE_GET_VNODE(fd);
	VERIFY(!IS_NEGATIVE(fd));

	init_fsbuf(buf);
	VERIFY(COMPARE(count, read(fd, buf, count)));
//	PRINT("\nread success");
	for(i= 0; i< count;i++){
		VERIFY(COMPARE(buf[i], byte));
	}

	for(; i< FS_BLOCK_SIZE;i++){
		VERIFY(COMPARE(buf[i], 0xff));
	}
//	PRINT("\nread data success");
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyOpenFileEntry(fd, NANDFS_O_RDONLY, count, uid, vnode_idx));
//	PRINT("\nfentry success");
	VERIFY(verifyVnode(vnode_idx, f_id,1));
//	PRINT("\nverifyVnode success");
	VERIFY(!close(fd));

	return 1;
}

/**
 * @brief
 * read data from direct entry.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t readTest3(){
	int32_t fd, vnode_idx, f_id;
	int32_t i, count = FS_BLOCK_SIZE*7, offset = INODE_FILE_DATA_SIZE+300;
	uint8_t read_buf[FS_BLOCK_SIZE*7], *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'a', new_byte = byte+5;
	user_id uid = 1;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	fsMemset(buf, byte, FS_BLOCK_SIZE);

	/* write DIRECT_INDEX_ENTRIES+5 times block size*/
	for(i=0;i<DIRECT_INDEX_ENTRIES+5;i+=1){
		VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}
//	PRINT("\nregular write success");

	/* write 7 * block size special data in direct offset*/
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
	fsMemset(buf, new_byte, FS_BLOCK_SIZE);
	for(i=0;i<count;i+=FS_BLOCK_SIZE){
//		PRINT("\nspecial write");
		VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}
//	PRINT("\nspecial write success");

	VERIFY(!close(fd));

	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\ncreat close success");
	/* open again for reading*/
	fd = open(f_name, NANDFS_O_RDONLY,0);
	f_id = GET_FILE_ID_BY_FD(fd);
	vnode_idx = OPEN_FILE_GET_VNODE(fd);
	VERIFY(!IS_NEGATIVE(fd));

	init_fsbuf(buf);
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
//	PRINT("\nlseek success");
	VERIFY(COMPARE(count, read(fd, read_buf, count)));
//	PRINT("\nread success");
	for(i= 0; i< count;i++){
//		if(!COMPARE(read_buf[i], new_byte)){
//			PRINT_MSG_AND_NUM("\n", i);
//			PRINT_MSG_AND_HEX(". ", buf[i]);
//			PRINT_MSG_AND_NUM(". expected ", new_byte);
//		}
		VERIFY(COMPARE(read_buf[i], new_byte));
	}

//	PRINT("\nread data success");
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyOpenFileEntry(fd, NANDFS_O_RDONLY, offset+count, uid, vnode_idx));
//	PRINT("\nfentry success");
	VERIFY(verifyVnode(vnode_idx, f_id,1));
//	PRINT("\nverifyVnode success");
	VERIFY(!close(fd));

	return 1;
}

/**
 * @brief
 * read data from indirect entry.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t readTest4(){
	int32_t fd, vnode_idx, f_id;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'a', new_byte = byte+5;
	user_id uid = 1;
	int32_t i, count = 300, offset = INDIRECT_DATA_OFFSET+FS_BLOCK_SIZE*5+100;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	fsMemset(buf, byte, FS_BLOCK_SIZE);

	for(i=0;i<offset+FS_BLOCK_SIZE*LOG_ADDRESSES_PER_BLOCK;i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));

	/* write special data in direct offset*/
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
	fsMemset(buf, new_byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	VERIFY(!close(fd));

	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\ncreat close success");
	/* open again for reading*/
	fd = open(f_name, NANDFS_O_RDONLY,0);
	f_id = GET_FILE_ID_BY_FD(fd);
	vnode_idx = OPEN_FILE_GET_VNODE(fd);
	VERIFY(!IS_NEGATIVE(fd));

	init_fsbuf(buf);
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
	VERIFY(COMPARE(count, read(fd, buf, count)));
//	PRINT("\nread success");
	for(i= 0; i< count;i++){
//		PRINT_MSG_AND_NUM("\n", i);
//		PRINT_MSG_AND_HEX(". ", buf[i]);
//		PRINT_MSG_AND_NUM(". expected ", 0xff);
		VERIFY(COMPARE(buf[i], new_byte));
	}
//	PRINT("\nread actual data success");
	for(; i< FS_BLOCK_SIZE;i++){
		VERIFY(COMPARE(buf[i], 0xff));
	}
//	PRINT("\nread data success");
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyOpenFileEntry(fd, NANDFS_O_RDONLY, offset+count, uid, vnode_idx));
//	PRINT("\nfentry success");
	VERIFY(verifyVnode(vnode_idx, f_id,1));
//	PRINT("\nverifyVnode success");
	VERIFY(!close(fd));

	return 1;
}

/**
 * @brief
 * read data from double entry.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t readTest5(){
	int32_t fd, vnode_idx, f_id;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'a', new_byte = byte+5;
	user_id uid = 1;
	int32_t i, count = 500, offset = DOUBLE_DATA_OFFSET+FS_BLOCK_SIZE*5+100;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	fsMemset(buf, byte, FS_BLOCK_SIZE);

	for(i=0;i<offset+FS_BLOCK_SIZE*LOG_ADDRESSES_PER_BLOCK;i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}

	/* write special data in direct offset*/
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
	fsMemset(buf, new_byte, FS_BLOCK_SIZE);
//	PRINT("\n\ndo special write");
	VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	VERIFY(!close(fd));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\ncreat close success");

	/* open again for reading*/
	fd = open(f_name, NANDFS_O_RDONLY,0);
	f_id = GET_FILE_ID_BY_FD(fd);
	vnode_idx = OPEN_FILE_GET_VNODE(fd);
	VERIFY(!IS_NEGATIVE(fd));

	init_fsbuf(buf);
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
//	PRINT("\nlseek success");
	VERIFY(COMPARE(count, read(fd, buf, count)));
//	PRINT("\nread success");

	for(i= 0; i< count;i++){
//		if(!COMPARE(buf[i], new_byte)){
//			PRINT_MSG_AND_NUM("\n", i);
//			PRINT_MSG_AND_HEX(". ", buf[i]);
//			PRINT_MSG_AND_HEX(". expected ", new_byte);
//		}
		VERIFY(COMPARE(buf[i], new_byte));
	}
//	PRINT("\nread actual data success");
	for(; i< FS_BLOCK_SIZE;i++){
		VERIFY(COMPARE(buf[i], 0xff));
	}
//	PRINT("\nread data success");
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyOpenFileEntry(fd, NANDFS_O_RDONLY, offset+count, uid, vnode_idx));
//	PRINT("\nfentry success");
	VERIFY(verifyVnode(vnode_idx, f_id,1));
//	PRINT("\nverifyVnode success");
	VERIFY(!close(fd));

	return 1;
}

/**
 * @brief
 * read data from triple entry.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t readTest6(){
	int32_t fd, vnode_idx, f_id;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'a', new_byte = byte+5;
	user_id uid = 1;
	int32_t i, count = 500, offset = TRIPLE_DATA_OFFSET+FS_BLOCK_SIZE*5+100;

	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < offset){
		return 1;
	}

	/* set user in process*/
	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	fsMemset(buf, byte, FS_BLOCK_SIZE);

	for(i=0;i<offset+FS_BLOCK_SIZE*LOG_ADDRESSES_PER_BLOCK;i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}

	/* write special data in direct offset*/
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
	fsMemset(buf, new_byte, FS_BLOCK_SIZE);
//	PRINT("\n\ndo special write");
	VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	VERIFY(!close(fd));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\ncreat close success");

	/* open again for reading*/
	fd = open(f_name, NANDFS_O_RDONLY,0);
	f_id = GET_FILE_ID_BY_FD(fd);
	vnode_idx = OPEN_FILE_GET_VNODE(fd);
	VERIFY(!IS_NEGATIVE(fd));

	init_fsbuf(buf);
	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
//	PRINT("\nlseek success");
	VERIFY(COMPARE(count, read(fd, buf, count)));
//	PRINT("\nread success");

	for(i= 0; i< count;i++){
//		if(!COMPARE(buf[i], new_byte)){
//			PRINT_MSG_AND_NUM("\n", i);
//			PRINT_MSG_AND_HEX(". ", buf[i]);
//			PRINT_MSG_AND_HEX(". expected ", new_byte);
//		}
		VERIFY(COMPARE(buf[i], new_byte));
	}
//	PRINT("\nread actual data success");
	for(; i< FS_BLOCK_SIZE;i++){
		VERIFY(COMPARE(buf[i], 0xff));
	}
//	PRINT("\nread data success");
	VERIFY(verifyFentriesVnodesEmpty(fd));
	VERIFY(verifyOpenFileEntry(fd, NANDFS_O_RDONLY, offset+count, uid, vnode_idx));
//	PRINT("\nfentry success");
	VERIFY(verifyVnode(vnode_idx, f_id,1));
//	PRINT("\nverifyVnode success");
	VERIFY(!close(fd));

	return 1;
}

/**
 * @brief
 * read data from illegal file descriptor, and illegal bytes count.
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t readTest7(){
	int32_t fd, f_id;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'a';
	user_id uid = 1;
	int32_t i, count = 500;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	fsMemset(buf, byte, FS_BLOCK_SIZE);

	for(i=0;i<FS_BLOCK_SIZE*LOG_ADDRESSES_PER_BLOCK;i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}

	VERIFY(!close(fd));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\ncreat close success");

	/* open again for reading*/
	fd = open(f_name, NANDFS_O_RDONLY,0);
	f_id = GET_FILE_ID_BY_FD(fd);
	VERIFY(!IS_NEGATIVE(fd));

	/* read illegal fd */
	VERIFY(COMPARE(-1, read(fd+1, buf, count)));
//	PRINT("\n1st read success");
	/* read illegal count */
	VERIFY(COMPARE(-1, read(fd, buf, -1)));
//	PRINT("\n2nd read success");
	/* read illegal count */
	VERIFY(COMPARE(count, read(fd, buf, count)));

	return 1;
}

/**
 * @brief
 * read data from inode file data to triple entry, and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t readTest8(){
	int32_t fd, vnode_idx, f_id;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE], byte = 'a', new_byte = byte+5;
	user_id uid = 1;
	int32_t j, i, count = 500, offset = TRIPLE_DATA_OFFSET+FS_BLOCK_SIZE*5+100;

	/* verify we can write to triple offset*/
	if(FS_MAX_FILESIZE < offset){
		return 1;
	}

	/* set user in process*/
	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	fsMemset(buf, byte, FS_BLOCK_SIZE);

	for(i=0;i<offset+FS_BLOCK_SIZE*LOG_ADDRESSES_PER_BLOCK;i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}
//
//
//
//	/* write special data in direct offset*/
//	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
//	fsMemset(buf, new_byte, FS_BLOCK_SIZE);
////	PRINT("\n\ndo special write");
//	VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	VERIFY(!close(fd));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\ncreat close success");

	/* open again for reading*/
	fd = open(f_name, NANDFS_O_RDONLY,0);
	f_id = GET_FILE_ID_BY_FD(fd);
	vnode_idx = OPEN_FILE_GET_VNODE(fd);
	VERIFY(!IS_NEGATIVE(fd));

	init_fsbuf(buf);
//	VERIFY(COMPARE(lseek(fd, offset, FS_SEEK_SET),offset));
//	PRINT("\nlseek success");

	for(i=0;i<offset+FS_BLOCK_SIZE*LOG_ADDRESSES_PER_BLOCK;i+=FS_BLOCK_SIZE){
		VERIFY(COMPARE(FS_BLOCK_SIZE, read(fd, buf, FS_BLOCK_SIZE)));

		for(j= 0; j< count;j++){
	//		if(!COMPARE(buf[j], new_byte)){
	//			PRINT_MSG_AND_NUM("\n", j);
	//			PRINT_MSG_AND_HEX(". ", buf[j]);
	//			PRINT_MSG_AND_HEX(". expected ", new_byte);
	//		}
			VERIFY(COMPARE(buf[j], byte));
		}
	}
	VERIFY(!close(fd));

	/* now try again writing and reading*/
	for(i=0;i<offset+FS_BLOCK_SIZE*LOG_ADDRESSES_PER_BLOCK;i+=2*FS_BLOCK_SIZE){
		/* write block*/
		fd = open(f_name, NANDFS_O_WRONLY,0);
		VERIFY(!IS_NEGATIVE(fd));
		fsMemset(buf, new_byte, FS_BLOCK_SIZE);
		VERIFY(COMPARE(lseek(fd, i, FS_SEEK_SET),i));
		VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
		VERIFY(!close(fd));
//		PRINT("\nanother write success");
		init_fsbuf(buf);
		fd = open(f_name, NANDFS_O_RDONLY,0);
		VERIFY(!IS_NEGATIVE(fd));
		VERIFY(COMPARE(lseek(fd, i, FS_SEEK_SET),i));
		VERIFY(COMPARE(FS_BLOCK_SIZE, read(fd, buf, FS_BLOCK_SIZE)));

		for(j= 0; j< FS_BLOCK_SIZE;j++){
//			if(!COMPARE(buf[j], new_byte)){
//				PRINT_MSG_AND_NUM("\n", j);
//				PRINT_MSG_AND_HEX(". ", buf[j]);
//				PRINT_MSG_AND_HEX(". expected ", new_byte);
//			}
			VERIFY(COMPARE(buf[j], new_byte));
		}
		VERIFY(!close(fd));
//		PRINT("\nanother read success");
	}

	return 1;
}

/**
 * @brief
 * read from two files concurrently, that belong to two different users. should succeed.
 * also test reading a file using a file descriptor that was opened by an other user. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t readTest9(){
	int32_t fd1, fd2;
	uint8_t *f_name1 = "/file1.dat", *f_name2 = "/file2.dat", buf[FS_BLOCK_SIZE], byte1 = 'a', byte2 = byte1+5;
	user_id uid1 = 1, uid2 = 2;
	int32_t j, i, offset = INDIRECT_DATA_OFFSET+FS_BLOCK_SIZE*5+100;

	if(FS_MAX_N_TRANSACTIONS < 2){
		return 1;
	}

	/* set user in process*/
	SET_CURRENT_USER(uid1);

	/* create 1st file */
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));

	/* set user in process*/
	SET_CURRENT_USER(uid2);

	/* create 1st file */
	fd2 = creat(f_name2, 0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT("\ncreats success");

	/* try to write to first file. should fail, user2 is still set*/
	VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),-1));
//	PRINT("\nfirst write success");
	/* write to both files*/
	for(i=0; i<offset; i+=FS_BLOCK_SIZE){
		/* set user in process*/
		SET_CURRENT_USER(uid1);
		fsMemset(buf, byte1, FS_BLOCK_SIZE);
//		PRINT_MSG_AND_NUM("\nfile1. fentry uid=", OPEN_FILE_GET_UID(fd1));
//		PRINT_MSG_AND_NUM(", process uid=", GET_CURRENT_USER());
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));

		/* set user in process*/
		SET_CURRENT_USER(uid2);
//		PRINT_MSG_AND_NUM("\nfile2. fentry uid=", OPEN_FILE_GET_UID(fd2));
//		PRINT_MSG_AND_NUM(", process uid=", GET_CURRENT_USER());
		fsMemset(buf, byte2, FS_BLOCK_SIZE);
		VERIFY(COMPARE(write(fd2, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}
//	PRINT("\nwrites success");
	/* close them both */
	SET_CURRENT_USER(uid1);
	VERIFY(!close(fd1));
	SET_CURRENT_USER(uid2);
	VERIFY(!close(fd2));
//	PRINT("\nclose success");

	/* read from both files */
	SET_CURRENT_USER(uid1);
	fd1 = open(f_name1, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\nopen 1st success");

	SET_CURRENT_USER(uid2);
	fd2 = open(f_name2, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT("\nopen 2nd success");

	/* try to read from first file, when second user is still set. should fail*/
	VERIFY(COMPARE(-1, read(fd1, buf, FS_BLOCK_SIZE)));
//	PRINT("\nfirst read success");

//	{
//		PRINT("\nprint page 64");
//		nandReadPageTotal(fs_buffer, 64);
//		printBlock(buf);
//		return 0;
//	}

//	PRINT_MSG_AND_NUM("\noffset=",offset);
	/* read from both files*/
	for(i=0; i<offset; i+=FS_BLOCK_SIZE){
//		PRINT_MSG_AND_NUM("\ni=",i);
		/* set user in process*/
		SET_CURRENT_USER(uid1);
		VERIFY(COMPARE(FS_BLOCK_SIZE, read(fd1, buf, FS_BLOCK_SIZE)));
		for(j= 0; j< FS_BLOCK_SIZE;j++){
			VERIFY(COMPARE(buf[j], byte1));
		}
//		PRINT("\nread file1 success");
		/* set user in process*/
		SET_CURRENT_USER(uid2);
		VERIFY(COMPARE(FS_BLOCK_SIZE, read(fd2, buf, FS_BLOCK_SIZE)));
		for(j= 0; j< FS_BLOCK_SIZE;j++){
			VERIFY(COMPARE(buf[j], byte2));
		}
//		PRINT("\nread file2 success");
	}

//	PRINT("\nclose files");
	/* close files*/
	SET_CURRENT_USER(uid1);
	VERIFY(!close(fd1));
	SET_CURRENT_USER(uid2);
	VERIFY(!close(fd2));

	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));

	return 1;
}

/**
 * @brief
 * try reading a file by two users concurrently from different file descriptors
 * @return 1 if successful, 0 otherwise
 */
error_t readTest10(){
	int32_t fd1, fd2;
	uint8_t *f_name1 = "/file1.dat", buf[FS_BLOCK_SIZE], byte1 = 'a', byte2 = byte1+5;
	user_id uid1 = 1, uid2 = 2;
	int32_t j, i, offset = INDIRECT_DATA_OFFSET+FS_BLOCK_SIZE*5+100;

	/* set user in process*/
	SET_CURRENT_USER(uid1);

	/* create 1st file */
	fd1 = creat(f_name1, 0);
	VERIFY(!IS_NEGATIVE(fd1));

	fsMemset(buf, byte1, FS_BLOCK_SIZE);
	for(i=0; i<offset; i+=FS_BLOCK_SIZE*2){
		fsMemset(buf, byte1, FS_BLOCK_SIZE);
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
		fsMemset(buf, byte2, FS_BLOCK_SIZE);
		VERIFY(COMPARE(write(fd1, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	}
//	PRINT("\nwrite success");
	VERIFY(!close(fd1));

	/* open file by two users*/
	fd1 = open(f_name1, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd1));

	SET_CURRENT_USER(uid2);
	fd2 = open(f_name1, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT("\nopens success");
	/* read from both files*/
	for(i=0; i<offset; i+=FS_BLOCK_SIZE*2){
//		PRINT_MSG_AND_NUM("\nfile1. fentry uid=", OPEN_FILE_GET_UID(fd1));
//		PRINT_MSG_AND_NUM(", process uid=", GET_CURRENT_USER());
		SET_CURRENT_USER(uid1);
		VERIFY(COMPARE(lseek(fd1, i, FS_SEEK_SET),i));
		VERIFY(COMPARE(FS_BLOCK_SIZE, read(fd1, buf, FS_BLOCK_SIZE)));
		for(j= 0; j< FS_BLOCK_SIZE;j++){
			VERIFY(COMPARE(buf[j], byte1));
		}
//		PRINT("\nread file uid1 success");
		/* set user in process*/
		SET_CURRENT_USER(uid2);
		VERIFY(COMPARE(lseek(fd2, i+FS_BLOCK_SIZE, FS_SEEK_SET),i+FS_BLOCK_SIZE));
//		PRINT("\nlseek file uid2 success");
		VERIFY(COMPARE(FS_BLOCK_SIZE, read(fd2, buf, FS_BLOCK_SIZE)));
//		PRINT("\nread file uid2 success");
		for(j= 0; j< FS_BLOCK_SIZE;j++){
			VERIFY(COMPARE(buf[j], byte2));
		}
//		PRINT("\nread file uid2 success");
	}
//	PRINT("\nreads success");
	/* close files*/
	SET_CURRENT_USER(uid1);
	VERIFY(!close(fd1));
	SET_CURRENT_USER(uid2);
	VERIFY(!close(fd2));

	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));

	return 1;
}

/**
 * @brief
 * try reading a directory. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t readTest11(){
	int32_t fd , uid = 1;
	uint8_t *f_name = "/", buf[FS_BLOCK_SIZE];

	SET_CURRENT_USER(uid);
	fd = open(f_name, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd));

	VERIFY(COMPARE(-1, read(fd, buf, 1)));

	return 1;
}

/**
 * @brief
 * read beyond file size, so that we read less bytes than expected
 * @return 1 if successful, 0 otherwise
 */
error_t readTest12(){
	int32_t fd , uid = 1, write_size = 100, i;
	uint8_t byte = 'a', *f_name = "/file.dat", buf[FS_BLOCK_SIZE];

	SET_CURRENT_USER(uid);
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	fsMemset(buf, byte, write_size);

	VERIFY(COMPARE(write_size, write(fd, buf, write_size)));

	VERIFY(!close(fd));
	init_fsbuf(buf);

	fd = open(f_name, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd));

	VERIFY(COMPARE(write_size, read(fd, buf, write_size+100)));

	for(i=0	; i<write_size; i++){
		VERIFY(COMPARE(buf[i], byte));
	}

	for(; i<FS_BLOCK_SIZE; i++){
		VERIFY(COMPARE(buf[i], 0xff));
	}

	return 1;
}

/**
 * @brief
 * open file with O_RDWR, and read/write simultaneously from transaction indirect block. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t readTest13(){
	int32_t fd , uid = 1, write_size = 100, i;
	uint8_t byte = 'a', *f_name = "/file.dat", buf[FS_BLOCK_SIZE];

	SET_CURRENT_USER(uid);
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	fsMemset(buf, byte, write_size);
	VERIFY(!close(fd));

	/* open for read/write*/
	fd = open(f_name, NANDFS_O_RDWR,0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nopen success");

	/* write */
	VERIFY(COMPARE(write_size, write(fd, buf, write_size)));
	init_fsbuf(buf);
//	PRINT("\nwrite success");

	/* read*/
	VERIFY(COMPARE(lseek(fd, 0, FS_SEEK_SET),0));
	VERIFY(COMPARE(write_size, read(fd, buf, write_size+100)));
//	PRINT("\nread success");

	/* verify read data */
	for(i=0	; i<write_size; i++){
		VERIFY(COMPARE(buf[i], byte));
	}

	for(; i<FS_BLOCK_SIZE; i++){
		VERIFY(COMPARE(buf[i], 0xff));
	}
//	PRINT("\nread data success");

	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd),write_size));

	return 1;
}

/**
 * @brief
 * open file with O_RDWR, and read/write simultaneously. read not from transaction indirect block.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t readTest14(){
	int32_t fd , uid = 1, i;
	uint8_t byte = 'a', new_byte = byte+1, *f_name = "/file.dat", buf[FS_BLOCK_SIZE];

	SET_CURRENT_USER(uid);
	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(!close(fd));

	/* open for read/write*/
	fd = open(f_name, NANDFS_O_RDWR,0);
	VERIFY(!IS_NEGATIVE(fd));
//	PRINT("\nopen success");

	/* write first block*/
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	fsMemset(buf, new_byte, FS_BLOCK_SIZE);
//	PRINT("\n1st write success");
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK; i++){
		VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd, buf, FS_BLOCK_SIZE)));
	}
//	PRINT("\nwrite success");

	/* read first block*/
	VERIFY(COMPARE(lseek(fd, 0, FS_SEEK_SET),0));
//	PRINT("\nlseek success");

//	PRINT("\n@@@@@@@@ do read");
	VERIFY(COMPARE(FS_BLOCK_SIZE, read(fd, buf, FS_BLOCK_SIZE)));
//	PRINT("\nread success");

	/* verify read data from first block*/
	for(i=0	; i<FS_BLOCK_SIZE; i++){
		if(!COMPARE(buf[i], byte)){
			PRINT_MSG_AND_NUM("\n", i);
			PRINT_MSG_AND_HEX(". ", buf[i]);
			PRINT_MSG_AND_HEX(". expected ", byte);
		}
		VERIFY(COMPARE(buf[i], byte));
	}
//	PRINT("\nread data success");

	/* read 2nd block */
	VERIFY(COMPARE(FS_BLOCK_SIZE, read(fd, buf, FS_BLOCK_SIZE)));
//	PRINT("\nread 2nd success");

	/* verify read data from second block*/
	for(i=0	; i<FS_BLOCK_SIZE; i++){
		VERIFY(COMPARE(buf[i], new_byte));
	}
//	PRINT("\nread 2nd data success");

	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd),FS_BLOCK_SIZE*2));

	return 1;
}

/**
 * @brief
 * open file by the same user, once for O_RDONLY once for O_WRONLY.
 * read and write simultaneously, shoudl succeed.
 * try reading with another user. should fail.
 * try open for reading with another user. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t readTest15(){
	int32_t i, fd1 , fd2, uid1 = 1, uid2=2;
	uint8_t byte = 'a', *f_name = "/file.dat", buf[FS_BLOCK_SIZE];

	SET_CURRENT_USER(uid1);
	fd1 = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd1));
//	PRINT("\ncreat success");

	/* open file for reading */
	fd2 = open(f_name, NANDFS_O_RDONLY, 0);
	VERIFY(!IS_NEGATIVE(fd2));
//	PRINT("\nopen read success");
//	PRINT("\n\n\n");

	/* write two blocks*/
	fsMemset(buf, byte, FS_BLOCK_SIZE);
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd1, buf, FS_BLOCK_SIZE)));
//	PRINT("\nwrite 1 success");
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd1, buf, FS_BLOCK_SIZE)));
//	PRINT("\nwrite 2 success");

	/* read first block */
	init_fsbuf(buf);
	VERIFY(COMPARE(FS_BLOCK_SIZE, read(fd2, buf, FS_BLOCK_SIZE)));
//	PRINT("\nread 1st success");

	/* verify read data from second block*/
	for(i=0	; i<FS_BLOCK_SIZE; i++){
		VERIFY(COMPARE(buf[i], byte));
	}

	/* re-write second block */
	fsMemset(buf, byte+1, FS_BLOCK_SIZE);
	VERIFY(COMPARE(lseek(fd1, FS_BLOCK_SIZE, FS_SEEK_SET),FS_BLOCK_SIZE));
	VERIFY(COMPARE(FS_BLOCK_SIZE, write(fd1, buf, FS_BLOCK_SIZE)));

	/* read second block*/
	init_fsbuf(buf);
	VERIFY(COMPARE(FS_BLOCK_SIZE, read(fd2, buf, FS_BLOCK_SIZE)));
//	PRINT("\nread success");

	/* verify read data from second block*/
	for(i=0	; i<FS_BLOCK_SIZE; i++){
		VERIFY(COMPARE(buf[i], byte+1));
	}
//	PRINT("\nread 2nd success");
	/* try reading by another user - should fail */
	SET_CURRENT_USER(uid2);
	VERIFY(COMPARE(-1, read(fd2, buf, FS_BLOCK_SIZE)));
//	PRINT("\nread 3rd success");
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd1), FS_BLOCK_SIZE*2));
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd2), FS_BLOCK_SIZE*2));

	return 1;
}

/**
 * @brief
 * read data where no complete block are read
 * @return 1 if successful, 0 otherwise
 */
error_t readTest16(){
	int32_t res, fd, vnode_idx, f_id, buf_size = FS_BLOCK_SIZE*2;
	uint8_t *f_name = "/file1.dat", buf[FS_BLOCK_SIZE*2], byte = 'a', new_byte = byte+1, count = 100;
	user_id uid = 1;
	int32_t i;

	/* set user in process*/
	SET_CURRENT_USER(uid);

	fd = creat(f_name, 0);
	VERIFY(!IS_NEGATIVE(fd));

	fsMemset(buf, byte, buf_size);
	VERIFY(COMPARE(write(fd, buf, FS_BLOCK_SIZE),FS_BLOCK_SIZE));
	fsMemset(buf, new_byte, buf_size);
	VERIFY(COMPARE(write(fd, buf, count),count));

	VERIFY(!close(fd));
	VERIFY(verifyFentriesVnodesEmpty(FD_EMPTY));
//	PRINT("\ncreat close success");
	/* open again for reading*/
	fd = open(f_name, NANDFS_O_RDONLY,0);
	VERIFY(!IS_NEGATIVE(fd));
	f_id = GET_FILE_ID_BY_FD(fd);
	vnode_idx = OPEN_FILE_GET_VNODE(fd);

	fsMemset(buf, 0xff, buf_size);
	res = read(fd, buf, buf_size);
//	PRINT_MSG_AND_NUM("\nread res=",res);
	VERIFY(COMPARE(FS_BLOCK_SIZE+count, res));
//	PRINT("\nread success");
	for(i= 0; i< FS_BLOCK_SIZE;i++){
		VERIFY(COMPARE(buf[i], byte));
	}
//	PRINT("\n1st read success");
	for(; i< FS_BLOCK_SIZE+count;i++){
		VERIFY(COMPARE(buf[i], new_byte));
	}
//	PRINT("\n2nd read success");
	for(; i< buf_size;i++){
		VERIFY(COMPARE(buf[i], 0xff));
	}

//	PRINT("\nread data success");
	VERIFY(verifyFentriesVnodesEmpty(fd));
//	PRINT("\nall fentry success");
	VERIFY(verifyOpenFileEntry(fd, NANDFS_O_RDONLY, FS_BLOCK_SIZE+count, uid, vnode_idx));
//	PRINT("\nfentry success");
	VERIFY(verifyVnode(vnode_idx, f_id,1));
//	PRINT("\nverifyVnode success");
	VERIFY(!close(fd));

	return 1;
}

/**
 * @brief
 * write and read to beginning of file in non-aligned offsets
 *
 * @return 1 if successful, 0 otherwise
 */
error_t readTest17(){
	int fd =0, fdr, i, res, j;
	char f_name[10] = "/file1", buf[FS_BLOCK_SIZE/2], byte = 'a';
	int size = FS_BLOCK_SIZE/2;

	/* read from non-open file. */
	VERIFY(read(fd, buf, 1)<0);

	/* create file*/
	fd = open(f_name, NANDFS_O_CREAT | NANDFS_O_WRONLY, 0);
	VERIFY(fd>=0);

	fdr = open(f_name, NANDFS_O_RDONLY, 0);
	VERIFY(fdr>=0);

	fsMemset(buf, byte, size);

//	PRINT_MSG_AND_NUM("\nstart writing. INODE_FILE_DATA_SIZE/2=", INODE_FILE_DATA_SIZE/2);
	/* write until inode file data*/
	for(i=0; i<2; i++){
		VERIFY(write(fd, buf, INODE_FILE_DATA_SIZE/2) == INODE_FILE_DATA_SIZE/2);
	}

	/* read file to verify data*/
	for(i=0; i<2; i++){
		fsMemset(buf, 0, size);
		VERIFY(read(fdr, buf, INODE_FILE_DATA_SIZE/2)== INODE_FILE_DATA_SIZE/2);

		for(j=0; j< INODE_FILE_DATA_SIZE/2; j++){
			VERIFY(buf[j]==byte);
		}
	}

	/* write until direct offset*/
	fsMemset(buf, byte, size);
	for(i=0; i<2; i++){
//		PRINT_MSG_AND_NUM("\n\ni=", i)
		fsMemset(buf, byte+1, size);
		VERIFY(write(fd, buf, size) == size);
		fsMemset(buf, 0, size);
//		PRINT_MSG_AND_NUM(",size=", size)
		res = read(fdr, buf, size);
//		PRINT_MSG_AND_NUM(". read res=", res)
		VERIFY(res== size);

		for(j=0; j< size; j++){
			VERIFY(buf[j]==byte+1);
		}
	}

	return 1;
}

/**
 * @brief
 * read data from a file. so that the indirect is cached.
 * than we write in the cache's ofset, and try to read from it again.
 * should read newly written data
 *
 * @return 1 if successful, 0 otherwise
 */
