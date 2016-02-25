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

/** @file auxiliaryFuncsTests.c
 *  Various file system auxiliary functions tests
 */
#include <test/fs/testsHeader.h>
#include <system.h>
#include <src/sequencing/sequencingUtils.h>
#include <src/sequencing/sequencing.h>
#include <src/sequencing/lfsr.h>
#include <src/fs/fsUtils.h>
#include <src/fs/fs.h>
#include <src/fs/transactions.h>
#include <lpc2000/uart.h>
#include <lpc2000/clocks.h>
#include <lpc2000/busywait.h>
#include <utils/print.h>
#include <utils/memlib.h>
#include <utils/string_lib.h>
#include <test/macroTest.h>
#include <test/fs/auxiliaryFuncsTests.h>
#include <test/fs/testsHeader.h>

void runAllAuxiliaryFuncsTests(){

	RUN_TEST(readFileBlock,1);
	RUN_TEST(readFileBlock,2);
	RUN_TEST(readFileBlock,21);
	RUN_TEST(readFileBlock,3);
	RUN_TEST(readFileBlock,31);
	RUN_TEST(readFileBlock,32);
	RUN_TEST(readFileBlock,4);
	RUN_TEST(readFileBlock,41);
	RUN_TEST(readFileBlock,42);
	RUN_TEST(readFileBlock,5);
	RUN_TEST(readFileBlock,51);
	RUN_TEST(readFileBlock,52);
	RUN_TEST(readFileBlock,6);
	RUN_TEST(readFileBlock,7);

	RUN_TEST(namei,0);
	RUN_TEST(namei,1);
	RUN_TEST(namei,2);
	RUN_TEST(namei,3);
	RUN_TEST(namei,4);
	RUN_TEST(namei,5);
	RUN_TEST(namei,6);

	RUN_TEST(findEmptySparseBlock,1);
	RUN_TEST(findEmptySparseBlock,2);
	RUN_TEST(findEmptySparseBlock,3);
	RUN_TEST(findEmptySparseBlock,4);
	RUN_TEST(findEmptySparseBlock,5);
	RUN_TEST(findEmptySparseBlock,6);
	RUN_TEST(findEmptySparseBlock,7);

	RUN_TEST(findIndirectEntriesBlock,1);
	RUN_TEST(findIndirectEntriesBlock,2);
	RUN_TEST(findIndirectEntriesBlock,3);
	RUN_TEST(findIndirectEntriesBlock,4);
	RUN_TEST(findIndirectEntriesBlock,5);
	RUN_TEST(findIndirectEntriesBlock,6);
	/* cache integration tests*/
	if(!IS_CACHE_EMPTY()){
	RUN_TEST(findIndirectEntriesBlock,7);
	}
	RUN_TEST(findIndirectEntriesBlock,8);


	RUN_TEST(commitInode,1);
	RUN_TEST(commitInode,2);
	RUN_TEST(commitInode,3);
	RUN_TEST(commitInode,4);

}

/**
 * @brief
 * init namei test
 *
 */
void init_nameiTest(){
	if(nandInit())
		return;

	init_fsbuf(fs_buffer);
//	PRINT("\ninit_nameiTest() - about to init_flash()");
	init_flash();
//	PRINT("\ninit_nameiTest() - about to init_file_entries()");
	init_file_entries();
	init_vnodes();
	init_transactions();
//	PRINT("\ninit_nameiTest() - about to handleNoFs()");
	initializeRamStructs();
	handleNoFs();
	init_fsbuf(fs_buffer);
}

/**
 * @brief
 * init readFileBlock test
 *
 */
void init_readFileBlockTest(){
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
 * tear down readFileBlock test
 *
 */
void tearDown_readFileBlockTest(){
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
 * read file data in inode offset, verify read and that no inode data was read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest1(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c', *f_full_name = "/file1.dat";
	int32_t i;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill inode data*/
	readBlock(log_addr_file, fs_buffer);
	for(i=0; i< INODE_FILE_DATA_SIZE; i++){
		fs_buffer[i] = byte;
	}

	INODE_SET_NBYTES(ino_ptr, INODE_FILE_DATA_SIZE);
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
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 2, FTYPE_FILE, f_full_name+1);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(log_addr_root, fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	L("about to read block 0 of file");
	/* read block 0 of file*/
	init_logical_address(log_addr_file);
	VERIFY(!readFileBlock(fs_buffer, 2, 0, log_addr_file, TID_EMPTY));

	/* verify only INODE_FILE_DATA_SIZE are not empty*/
	for(i=0; i< INODE_FILE_DATA_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], byte));
//	PRINT("\nverified byte");

	for(i=INODE_FILE_DATA_SIZE; i< FS_BLOCK_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], FS_EMPTY_FILE_BYTE));
//	PRINT("\nverified empty data");


	return 1;
}

/**
 * @brief
 * read file data in direct index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest2(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c', *f_full_name = "/file1.dat";
	int32_t i;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill buffer data*/
	for(i=0; i< FS_BLOCK_SIZE ; i++)
		fs_buffer[i] = byte;

	/* write buffer*/
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT_MSG_AND_NUM("\nwrote direct entry to ", logicalAddressToPhysical(log_addr));

	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
//	PRINT_MSG_AND_NUM("\nfile inode size field=", CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	INODE_SET_NBLOCKS(ino_ptr, 2);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr);
	/* write inode, and link it to root directory and inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT_MSG_AND_NUM("\nwrote inode to ", logicalAddressToPhysical(log_addr_file));
	/* read inode0 for root dir inode, and read it for first entries blocks, and read first direct entry block*/
	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_GET_DIRECT(ino_ptr, 0, log_addr_root);
	readBlock(log_addr_root, fs_buffer);
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	readBlock(log_addr, fs_buffer);

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 2, FTYPE_FILE, f_full_name+1);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(log_addr_root, fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT_MSG_AND_NUM("\nwrote inode0 to ", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//	PRINT("\nabout to read block 0 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);
	VERIFY(!readFileBlock(fs_buffer, 2, INODE_FILE_DATA_SIZE, log_addr_file, TID_EMPTY));
//	PRINT("\nread succss");
	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
//		if(!COMPARE(fs_buffer[i], byte)){
//			PRINT_MSG_AND_NUM("\nfailed at byte ", i);
//			PRINT_MSG_AND_HEX(", should be ", byte);
//			PRINT_MSG_AND_HEX(", is ", fs_buffer[i]);
//		}
		VERIFY(COMPARE(fs_buffer[i], byte));
	}
//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data in empty direct index, verify read empty
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest21(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t i;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));
	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, 2000);
	INODE_SET_NBLOCKS(ino_ptr, 4);
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 1 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);
	VERIFY(!readFileBlock(fs_buffer, 2, INODE_FILE_DATA_SIZE, log_addr_file, TID_EMPTY));

	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], FS_EMPTY_FILE_BYTE));
//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data in indirect index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest3(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_indirect);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_data);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c';
	uint32_t i, indirect_offset = 1;
	uint32_t read_offset = INODE_FILE_DATA_SIZE+INODE_DIRECT_DATA_SIZE+(FS_BLOCK_SIZE*indirect_offset);
	uint32_t f_size = read_offset+2*FS_BLOCK_SIZE;
//	FENT();
	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_indirect);
	init_logical_address(log_addr_data);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill buffer data*/
	for(i=0; i< FS_BLOCK_SIZE ; i++)
		fs_buffer[i] = byte;

	/* write buffer*/
//	PRINT("\nwrite page")
	VERIFY(!allocAndWriteBlock(log_addr_data, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT_MSG_AND_NUM(", written to addr ", logicalAddressToPhysical(log_addr_data));

	/* write indirect block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, indirect_offset, log_addr_data);
//	PRINT("\nwrite indirect page");
	VERIFY(!allocAndWriteBlock(log_addr_indirect, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT_MSG_AND_NUM(", written to addr ", logicalAddressToPhysical(log_addr_indirect));
	/* set indirect pointer*/
	VERIFY(!readBlock(log_addr_file, fs_buffer));
	INODE_SET_NBYTES(ino_ptr, f_size);
	INODE_SET_NBLOCKS(ino_ptr, 2);
	INODE_SET_INDIRECT(ino_ptr, log_addr_indirect);
//	PRINT("\nwrite inode again");
	/* write inode, and link it to inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));
//	L("file written to addr %x", *((uint32_t*)log_addr_file));
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
//	PRINT("\nwrite inode0 again");
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 0 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);

	VERIFY(!readFileBlock(fs_buffer, 2, read_offset, log_addr_file, TID_EMPTY));

//	PRINT("\nverify block");
	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
		VERIFY(COMPARE(fs_buffer[i], byte));
	}
//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data in empty indirect index, verify read empty
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest31(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint32_t i, indirect_offset = 1;
	uint32_t read_offset = INODE_FILE_DATA_SIZE+INODE_DIRECT_DATA_SIZE+(FS_BLOCK_SIZE*(indirect_offset+1)); /* NOTICE - indirect_offset+1*/
	uint32_t f_size = read_offset+2*FS_BLOCK_SIZE;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* set indirect pointer*/
	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, f_size);
	INODE_SET_NBLOCKS(ino_ptr, 2);
//	PRINT("\nwrite inode again");
	/* write inode, and link it to inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
//	PRINT("\nwrite inode0 again");
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 0 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);

	VERIFY(!readFileBlock(fs_buffer, 2, read_offset, log_addr_file, TID_EMPTY));

	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], FS_EMPTY_FILE_BYTE));

//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data in indirect index, where the direct index is empty.
 * verify read empty
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest32(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_indirect);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_data);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c';
	uint32_t i, indirect_offset = 1;
	uint32_t read_offset = INODE_FILE_DATA_SIZE+INODE_DIRECT_DATA_SIZE+(FS_BLOCK_SIZE*(indirect_offset+1)); /* NOTICE - indirect_offset+1*/
	uint32_t f_size = read_offset+2*FS_BLOCK_SIZE;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_indirect);
	init_logical_address(log_addr_data);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill buffer data*/
	for(i=0; i< FS_BLOCK_SIZE ; i++)
		fs_buffer[i] = byte;

	/* write buffer*/
	VERIFY(!allocAndWriteBlock(log_addr_data, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));


	/* write indirect block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, indirect_offset, log_addr_data);
//	PRINT("\nwrite indirect page");
	VERIFY(!allocAndWriteBlock(log_addr_indirect, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* set indirect pointer*/
	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, f_size);
	INODE_SET_NBLOCKS(ino_ptr, 2);
	INODE_SET_INDIRECT(ino_ptr, log_addr_indirect);
//	PRINT("\nwrite inode again");
	/* write inode, and link it to inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
//	PRINT("\nwrite inode0 again");
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 0 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);

	VERIFY(!readFileBlock(fs_buffer, 2, read_offset, log_addr_file, TID_EMPTY));

	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], FS_EMPTY_FILE_BYTE));

//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data in double index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest4(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_double);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_indirect);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_data);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c';
	uint32_t i, indirect_offset = 2, double_offset = 1;
	uint32_t read_offset = INODE_FILE_DATA_SIZE+INODE_DIRECT_DATA_SIZE+INODE_INDIRECT_DATA_SIZE*(1+double_offset)+(FS_BLOCK_SIZE*indirect_offset);
	uint32_t f_size = read_offset+2*FS_BLOCK_SIZE;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_double);
	init_logical_address(log_addr_indirect);
	init_logical_address(log_addr_data);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill buffer data*/
	for(i=0; i< FS_BLOCK_SIZE ; i++)
		fs_buffer[i] = byte;
//	PRINT("\nwrite data page")
	/* write buffer*/
	VERIFY(!allocAndWriteBlock(log_addr_data, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));


	/* write indirect block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, indirect_offset, log_addr_data);
//	PRINT("\nwrite indirect page");
	VERIFY(!allocAndWriteBlock(log_addr_indirect, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write double block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, double_offset, log_addr_indirect);
//	PRINT("\nwrite double page");
	VERIFY(!allocAndWriteBlock(log_addr_double, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* set indirect pointer*/
	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, f_size);
	INODE_SET_NBLOCKS(ino_ptr, 2);
	INODE_SET_DOUBLE(ino_ptr, log_addr_double);
//	PRINT("\nwrite inode again");
	/* write inode, and link it to inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
//	PRINT("\nwrite inode0 again");
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 0 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);

	VERIFY(!readFileBlock(fs_buffer, 2, read_offset, log_addr_file, TID_EMPTY));

	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], byte));

//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data in empty double index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest41(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint32_t i, indirect_offset = 1;
	uint32_t read_offset = INODE_FILE_DATA_SIZE+INODE_DIRECT_DATA_SIZE+INODE_INDIRECT_DATA_SIZE+(FS_BLOCK_SIZE*(indirect_offset+1)); /* NOTICE - indirect_offset+1*/
	uint32_t f_size = read_offset+2*FS_BLOCK_SIZE;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* set inode size and rewrite*/
	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, f_size);
	INODE_SET_NBLOCKS(ino_ptr, 2);
//	PRINT("\nwrite inode again");
	/* write inode, and link it to inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
//	PRINT("\nwrite inode0 again");
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 0 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);

	VERIFY(!readFileBlock(fs_buffer, 2, read_offset, log_addr_file, TID_EMPTY));

	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], FS_EMPTY_FILE_BYTE));

//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data in double index, where the direct index is empty.
 * verify read empty
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest42(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_double);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_indirect);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_data);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c';
	uint32_t i, indirect_offset = 2, double_offset = 1;
	uint32_t read_offset = INODE_FILE_DATA_SIZE+INODE_DIRECT_DATA_SIZE+INODE_INDIRECT_DATA_SIZE*(1+double_offset)+(FS_BLOCK_SIZE*(indirect_offset+1)); /* NOTICE - indirect_offset+1*/
	uint32_t f_size = read_offset+2*FS_BLOCK_SIZE;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_double);
	init_logical_address(log_addr_indirect);
	init_logical_address(log_addr_data);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill buffer data*/
	for(i=0; i< FS_BLOCK_SIZE ; i++)
		fs_buffer[i] = byte;
//	PRINT("\nwrite data page")
	/* write buffer*/
	VERIFY(!allocAndWriteBlock(log_addr_data, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));


	/* write indirect block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, indirect_offset, log_addr_data);
//	PRINT("\nwrite indirect page");
	VERIFY(!allocAndWriteBlock(log_addr_indirect, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write double block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, double_offset, log_addr_indirect);
//	PRINT("\nwrite double page");
	VERIFY(!allocAndWriteBlock(log_addr_double, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* set indirect pointer*/
	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, f_size);
	INODE_SET_NBLOCKS(ino_ptr, 2);
	INODE_SET_DOUBLE(ino_ptr, log_addr_double);
//	PRINT("\nwrite inode again");
	/* write inode, and link it to inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
//	PRINT("\nwrite inode0 again");
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 0 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);

	VERIFY(!readFileBlock(fs_buffer, 2, read_offset, log_addr_file, TID_EMPTY));

	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], FS_EMPTY_FILE_BYTE));

//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data in triple index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest5(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_triple);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_double);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_indirect);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_data);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c';
	uint32_t i, indirect_offset = 2, double_offset = 1, triple_offset = 1;
	uint32_t read_offset = INODE_FILE_DATA_SIZE+INODE_DIRECT_DATA_SIZE+
						   INODE_DOUBLE_DATA_SIZE*(1+triple_offset)   +
						   INODE_INDIRECT_DATA_SIZE*(1+double_offset) +
						   (FS_BLOCK_SIZE*indirect_offset);
	uint32_t f_size = read_offset+2*FS_BLOCK_SIZE;

	/* verify triple offset is used*/
	if(read_offset > FS_MAX_FILESIZE){
		return 1;
	}
//	PRINT_MSG_AND_NUM("\nINODE_DOUBLE_DATA_SIZE=", INODE_DOUBLE_DATA_SIZE);
//	PRINT_MSG_AND_NUM("\nINODE_INDIRECT_DATA_SIZE=", INODE_INDIRECT_DATA_SIZE);

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_triple);
	init_logical_address(log_addr_double);
	init_logical_address(log_addr_indirect);
	init_logical_address(log_addr_data);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill buffer data*/
	for(i=0; i< FS_BLOCK_SIZE ; i++)
		fs_buffer[i] = byte;
//	PRINT("\nwrite data page")
	/* write buffer*/
	VERIFY(!allocAndWriteBlock(log_addr_data, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));


	/* write indirect block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, indirect_offset, log_addr_data);
//	PRINT("\nwrite indirect page");
	VERIFY(!allocAndWriteBlock(log_addr_indirect, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write double block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, double_offset, log_addr_indirect);
//	PRINT("\nwrite double page");
	VERIFY(!allocAndWriteBlock(log_addr_double, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write triple block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, triple_offset, log_addr_double);
//	PRINT("\nwrite double page");
	VERIFY(!allocAndWriteBlock(log_addr_triple, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* set triple pointer*/
	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, f_size);
	INODE_SET_NBLOCKS(ino_ptr, 2);
	INODE_SET_TRIPLE(ino_ptr, log_addr_triple);
//	PRINT("\nwrite inode again");
	/* write inode, and link it to inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
//	PRINT("\nwrite inode0 again");
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 0 of file");
	/* read block from triple offset*/
	init_logical_address(log_addr_file);

	VERIFY(!readFileBlock(fs_buffer, 2, read_offset, log_addr_file, TID_EMPTY));
//	PRINT("\nfinished readFileBlock()");

	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], byte));

//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data in empty triple index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest51(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint32_t i, indirect_offset = 1, double_offset = 1, triple_offset = 1;
	uint32_t read_offset = INODE_FILE_DATA_SIZE+INODE_DIRECT_DATA_SIZE+
						   INODE_DOUBLE_DATA_SIZE*(1+triple_offset)   +
						   INODE_INDIRECT_DATA_SIZE*(1+double_offset) +
						   (FS_BLOCK_SIZE*(indirect_offset+1)); /* NOTICE - indirect_offset+1*/
	uint32_t f_size = read_offset+2*FS_BLOCK_SIZE;

	/* verify triple offset is used*/
	if(read_offset > FS_MAX_FILESIZE){
		return 1;
	}

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* set inode size and rewrite*/
	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, f_size);
	INODE_SET_NBLOCKS(ino_ptr, 2);
//	PRINT("\nwrite inode again");
	/* write inode, and link it to inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
//	PRINT("\nwrite inode0 again");
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 0 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);

	VERIFY(!readFileBlock(fs_buffer, 2, read_offset, log_addr_file, TID_EMPTY));

	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], FS_EMPTY_FILE_BYTE));

//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data in triple index, where the direct index is empty.
 * verify read empty
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest52(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_triple);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_double);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_indirect);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_data);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c';
	uint32_t i, indirect_offset = 2, double_offset = 1, triple_offset = 1;
	uint32_t read_offset = INODE_FILE_DATA_SIZE+INODE_DIRECT_DATA_SIZE+
						   INODE_DOUBLE_DATA_SIZE*(1+triple_offset)   +
						   INODE_INDIRECT_DATA_SIZE*(1+double_offset) +
						   (FS_BLOCK_SIZE*(indirect_offset+1)); /* NOTICE - indirect_offset+1*/
	uint32_t f_size = read_offset+2*FS_BLOCK_SIZE;

	/* verify triple offset is used*/
	if(read_offset > FS_MAX_FILESIZE){
		return 1;
	}

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_triple);
	init_logical_address(log_addr_double);
	init_logical_address(log_addr_indirect);
	init_logical_address(log_addr_data);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill buffer data*/
	for(i=0; i< FS_BLOCK_SIZE ; i++)
		fs_buffer[i] = byte;
//	PRINT("\nwrite data page")
	/* write buffer*/
	VERIFY(!allocAndWriteBlock(log_addr_data, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));


	/* write indirect block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, indirect_offset, log_addr_data);
//	PRINT("\nwrite indirect page");
	VERIFY(!allocAndWriteBlock(log_addr_indirect, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write double block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, double_offset, log_addr_indirect);
//	PRINT("\nwrite double page");
	VERIFY(!allocAndWriteBlock(log_addr_double, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write triple block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer, triple_offset, log_addr_double);
//	PRINT("\nwrite double page");
	VERIFY(!allocAndWriteBlock(log_addr_triple, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* set triple pointer*/
	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, f_size);
	INODE_SET_NBLOCKS(ino_ptr, 2);
	INODE_SET_TRIPLE(ino_ptr, log_addr_triple);
//	PRINT("\nwrite inode again");
	/* write inode, and link it to inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
//	PRINT("\nwrite inode0 again");
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 0 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);

	VERIFY(!readFileBlock(fs_buffer, 2, read_offset, log_addr_file, TID_EMPTY));

	/* verify byte*/
	for(i=0; i< FS_BLOCK_SIZE; i++)
		VERIFY(COMPARE(fs_buffer[i], FS_EMPTY_FILE_BYTE));

//	PRINT("\nverified byte");

	return 1;
}

/**
 * @brief
 * read file data from offset larger than file size. verify failure
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest6(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t byte = 'c', *f_full_name = "/file1.dat";
	uint32_t i;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr_file);
	init_logical_address(log_addr_root);
	init_logical_address(log_addr);

	/* write file inode */
	VERIFY(!writeNewInode(2, INO_NUM_EMPTY, FTYPE_FILE, log_addr_file));

	/* fill buffer data*/
	for(i=0; i< FS_BLOCK_SIZE ; i++)
		fs_buffer[i] = byte;

	/* write buffer*/
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(log_addr_file, fs_buffer);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	INODE_SET_NBLOCKS(ino_ptr, 2);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr);
	/* write inode, and link it to root directory and inode0*/
	VERIFY(!allocAndWriteBlock(log_addr_file, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* read inode0 for root dir inode, and read it for first entries blocks, and read first direct entry block*/
	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_GET_DIRECT(ino_ptr, 0, log_addr_root);
	readBlock(log_addr_root, fs_buffer);
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	readBlock(log_addr, fs_buffer);

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 2, FTYPE_FILE, f_full_name+1);
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(log_addr_root, fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer);
	INODE_SET_DIRECT(ino_ptr, 0, log_addr_root);
	INODE_SET_DIRECT(ino_ptr, 1, log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE*2));
	INODE_SET_NBLOCKS(ino_ptr, 3);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT("\nabout to read block 0 of file");
	/* read block 1 of file, from INODE_FILE_DATA_SIZE offset*/
	init_logical_address(log_addr_file);

	VERIFY(IS_NEGATIVE(readFileBlock(fs_buffer, 2, FS_MAX_FILESIZE + INODE_FILE_DATA_SIZE, log_addr_file, TID_EMPTY)));

	return 1;
}

/**
 * @brief
 * readFileBlock for a file involved in another transaction, should fail
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest7(){
	int32_t ino_num = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);

	TRANSACTION_SET_INO(0, ino_num);
	VERIFY(readFileBlock(fs_buffer, ino_num, INODE_FILE_DATA_SIZE, ino_log_addr, TID_EMPTY));

	return 1;
}

/*************************************************namei tests **********************************************/
/**
 * @brief
 * tear down namei test
 *
 */
void tearDown_nameiTest(){
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
 * test that various illegal path names are not recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest0(){
	uint8_t name[11] = "/file1.dat";
	uint32_t f_type = FTYPE_FILE;
	int32_t de_offset;

//	PRINT("\nnameiTest0() start");
	/* set illegal chars*/
	name[3] = 12;
//	PRINT("\ntest 1");
	VERIFY(IS_NEGATIVE(namei(name, &f_type, &de_offset, DEFAULT_CWD_INO_NUM)));
//	PRINT("\ntest no illegal char");

	/* test no '/' at start */
	VERIFY(IS_NEGATIVE(namei("file1.dat", &f_type, &de_offset, DEFAULT_CWD_INO_NUM)));
//	PRINT("\ntest no / success");

	/* double '/'*/
	VERIFY(COMPARE(1,namei("//file1.dat", &f_type, &de_offset, DEFAULT_CWD_INO_NUM)));
//	PRINT("\ntest double / success");

	return 1;
}

/**
 * @brief
 * test that an existing file in root dir is recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest1(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr2);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t *f_name = "/file1.dat";
	uint32_t f_type = FTYPE_FILE;
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	int32_t ino_num;
	int32_t de_offset;

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr1);
	init_logical_address(log_addr2);

	/* write another directory entry to root directory.*/
	/* read inode0, get root directory inode address.*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr,0, log_addr1);

	/* read root directory inode, and find first block address*/
	VERIFY(!readBlock(log_addr1, fs_buffer));
	INODE_GET_DIRECT(ino_ptr,0, log_addr2);

	/* read first block*/
	VERIFY(!readBlock(log_addr2, fs_buffer));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr)){
//		PRINT("\nmoving to next direntry");
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}

//	PRINT_MSG_AND_NUM("\noffset in fs_buffer=",fs_buffer-CAST_TO_UINT8(dirent_ptr));
	/* set direntry details
	 * and rewrite all the above*/
	setNewDirentry(dirent_ptr, 2, FTYPE_FILE, f_name+1);
//	PRINT_MSG_AND_NUM("\nf_name dirent len=",DIRENT_GET_LEN(dirent_ptr));
//	PRINT_MSG_AND_NUM("\naccording to dirent f_name len=",DIRENT_GET_NAME_LEN(dirent_ptr));

	/* write actual direntry block */
	VERIFY(!allocAndWriteBlock(log_addr2, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT_MSG_AND_NUM("\nallocAndWriteBlock() of direntry block successful. phy_addr=",logicalAddressToPhysical(log_addr2));

	/* put new address to the old directory inode block, and write it*/
	VERIFY(!readBlock(log_addr1, fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0, log_addr2);

//	init_logical_address(log_addr2);
//	INODE_GET_DIRECT(ino_ptr,0, log_addr2);
//	PRINT_MSG_AND_NUM("\nset first direct address in inode to ",logicalAddressToPhysical(log_addr2));
	VERIFY(!allocAndWriteBlock(log_addr1, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	PRINT_MSG_AND_NUM("\nallocAndWriteBlock() of root directory block successful. phy_addr=",logicalAddressToPhysical(log_addr1));
	/* and change inode0 pointer to new location of root inode*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0, log_addr1);
	VERIFY(!allocAndWriteBlock(log_addr1, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));
//	PRINT_MSG_AND_NUM("\nallocAndWriteBlock() of inode0 block successful. phy_addr=",logicalAddressToPhysical(log_addr1));
	FS_SET_INO0_ADDR(log_addr1);

	/* now try finding this file */
	ino_num = namei(f_name, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
	VERIFY(COMPARE(ino_num, 2));
//	PRINT_MSG_AND_NUM("\nde_offset=",de_offset);
	VERIFY(COMPARE(de_offset-CALC_OFFSET_IN_FILE_BLOCK(de_offset), INODE_FILE_DATA_SIZE));

	return 1;
}

/**
 * @brief
 * test that an non-existing file in root dir is not recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest2(){
	uint8_t *f_name = "/file1.dat";
	uint32_t f_type = FTYPE_FILE;
	int32_t ino_num;
	int32_t de_offset;

//	PRINT("\n\n\n\nnameiTest2() - starting");
	/* try finding non-existing file */
	ino_num = namei(f_name, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
	VERIFY(IS_NEGATIVE(ino_num));

	return 1;
}

/**
 * @brief
 * test that an existing file not in root dir. should be recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest3(){
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
	uint32_t f_type;
	int32_t ino_num;
	int32_t de_offset;

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
//	VERIFY(!readBlock(log_addr_dir2, fs_buffer));
//	INODE_GET_DIRECT(CAST_TO_INODE(fs_buffer), 0, log_addr);
//	VERIFY(!readBlock(log_addr, fs_buffer));
	VERIFY(!readFileBlock(fs_buffer, 3, INODE_FILE_DATA_SIZE, log_addr_dir2, TID_EMPTY));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 4, FTYPE_FILE, "file1.dat");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory2 inode with new pointer to the above block*/
	VERIFY(!readBlock(log_addr_dir2, fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0,log_addr);

	VERIFY(!allocAndWriteBlock(log_addr_dir2, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory2 direntry in directory1*/
//	VERIFY(!readBlock(log_addr_dir1, fs_buffer));
//	INODE_GET_DIRECT(CAST_TO_INODE(fs_buffer), 0, log_addr);
//	VERIFY(!readBlock(log_addr, fs_buffer));
//	uint8_t buf[FS_BLOCK_SIZE];
//	{
//		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//
//		L("");
//		VERIFY(!readBlock(log_addr_dir1, buf));
//		INODE_GET_DIRECT(CAST_TO_INODE(buf), 0, log_addr);
//		L("");
//		VERIFY(!readBlock(log_addr, buf));
//		L("");
//	}
//
//	{
//			uint8_t buf[FS_BLOCK_SIZE];
//			L("read dir 1 inode from addr %x", *((uint32_t*)log_addr_dir1));
//			readBlock(log_addr_dir1, buf);
//			L("for ino_num %d file size is", INODE_GET_FILE_ID(CAST_TO_INODE(buf)), INODE_GET_NBYTES(CAST_TO_INODE(buf)));
//			VERIFY(COMPARE(2, INODE_GET_FILE_ID(CAST_TO_INODE(buf))));
//		}
	VERIFY(!readFileBlock(fs_buffer, 2, INODE_FILE_DATA_SIZE, log_addr_dir1, TID_EMPTY));
//	VERIFY(compare_bufs(buf, fs_buffer, FS_BLOCK_SIZE));
//	{
//			uint8_t buf[FS_BLOCK_SIZE];
//			readBlock(log_addr_dir1, buf);
//			L("for ino_num %d file size is", INODE_GET_FILE_ID(CAST_TO_INODE(buf)), INODE_GET_NBYTES(CAST_TO_INODE(buf)));
//			VERIFY(COMPARE(2, INODE_GET_FILE_ID(CAST_TO_INODE(buf))));
//		}


	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 3, FTYPE_DIR, "directory2");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!readBlock(log_addr_dir1, fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0,log_addr);

	VERIFY(!allocAndWriteBlock(log_addr_dir1, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* check file size of inode 2*/
	readBlock(log_addr_dir1, fs_buffer);
//	L("for ino_num %d file size is", INODE_GET_FILE_ID(CAST_TO_INODE(fs_buffer)), INODE_GET_NBYTES(CAST_TO_INODE(fs_buffer)));
	VERIFY(COMPARE(2, INODE_GET_FILE_ID(CAST_TO_INODE(fs_buffer))));


	/* write directory1 direntry in root*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(CAST_TO_INODE(fs_buffer), 0, log_addr_root);
	VERIFY(!readBlock(log_addr_root, fs_buffer));
	INODE_GET_DIRECT(CAST_TO_INODE(fs_buffer), 0, log_addr);
	VERIFY(!readBlock(log_addr, fs_buffer));

	/* iterate until we find an empty directory entry*/
	while(!IS_DIRENT_EMPTY(dirent_ptr))
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

	setNewDirentry(dirent_ptr, 2, FTYPE_DIR, "directory1");
	VERIFY(!allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write directory1 inode with new pointer to the above block*/
	VERIFY(!readBlock(log_addr_root, fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
//	L("inode 1 size %d, nblocks %d", INODE_GET_NBYTES(ino_ptr), INODE_GET_NBLOCKS(ino_ptr));

	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));
//	L("root addr is %x", *((uint32_t*)log_addr_root));
	/* write inode0 with pointers to the new root address*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));

	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	INODE_SET_DIRECT(ino_ptr,1,log_addr_dir1);
	INODE_SET_DIRECT(ino_ptr,2,log_addr_dir2);
	INODE_SET_DIRECT(ino_ptr,3,log_addr_file);
//	L("set ino address to - root(1) %x dir1(2) %x dir2(3) %x #d file(4) %x", *((uint32_t*)log_addr_root), *((uint32_t*)log_addr_dir1), *((uint32_t*)log_addr_dir2), *((uint32_t*)log_addr_file));
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+4*FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* check file size of inode 2*/
	readBlock(log_addr_dir1, fs_buffer);
	VERIFY(COMPARE(2, INODE_GET_FILE_ID(CAST_TO_INODE(fs_buffer))));
//	L("for ino_num %d file size is", INODE_GET_FILE_ID(CAST_TO_INODE(fs_buffer)), INODE_GET_NBYTES(CAST_TO_INODE(fs_buffer)));
	/* and now try finding file1.dat*/
//	PRINT("\n\n\n\ndo namei()");
//	init_indirect_caches();
	ino_num = namei(f_full_name, &f_type, & de_offset, DEFAULT_CWD_INO_NUM);
//	PRINT_MSG_AND_NUM("\nnamei() done. ino num=", ino_num);
//	L("ino_num %d, expected 4", ino_num);
	VERIFY(COMPARE(ino_num, 4));
//	PRINT("\nino num done");
	VERIFY(COMPARE(f_type, FTYPE_FILE));
//	PRINT("\nftype done");
	VERIFY(COMPARE(de_offset-CALC_OFFSET_IN_FILE_BLOCK(de_offset), INODE_FILE_DATA_SIZE));

	return 1;
}

/**
 * @brief
 * test that a non-existing file, where only the last part of the path name is illegal
 * is not recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest4(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir2);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t *f_full_name = "/directory1/directory2/file1_nonexistent.dat";
	uint32_t f_type;
	int32_t ino_num;
	int32_t de_offset;

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
	VERIFY(!readBlock(log_addr_dir2, fs_buffer));
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
	VERIFY(!readBlock(log_addr_dir1, fs_buffer));
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
	VERIFY(!readBlock(log_addr_root, fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0,log_addr);
	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* write inode0 with pointers to the new root address*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));

	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	INODE_SET_DIRECT(ino_ptr,1,log_addr_dir1);
	INODE_SET_DIRECT(ino_ptr,2,log_addr_dir2);
	INODE_SET_DIRECT(ino_ptr,3,log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+4*FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* and now try finding file1.dat*/
	ino_num = namei(f_full_name, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
	VERIFY(IS_NEGATIVE(ino_num));

	return 1;
}

/**
 * @brief
 * try opening a legal pathname with "." and ".."
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest5(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir2);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t *f_full_name = "/directory1/../../../././directory1/directory2/../directory2/././file1.dat";
	uint32_t f_type;
	int32_t ino_num;
	int32_t de_offset;

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
	VERIFY(!readBlock(log_addr_dir2, fs_buffer));
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
	VERIFY(!readBlock(log_addr_dir1, fs_buffer));
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
	VERIFY(!readBlock(log_addr_root, fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0,log_addr);

	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));


	/* write inode0 with pointers to the new root address*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));

	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	INODE_SET_DIRECT(ino_ptr,1,log_addr_dir1);
	INODE_SET_DIRECT(ino_ptr,2,log_addr_dir2);
	INODE_SET_DIRECT(ino_ptr,3,log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+4*FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

//	/* initialize all indirect caches*/
//	init_indirect_caches();

	/* and now try finding file1.dat*/
	ino_num = namei(f_full_name, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
	VERIFY(COMPARE(ino_num, 4));
	VERIFY(COMPARE(f_type, FTYPE_FILE));
	VERIFY(COMPARE(de_offset-CALC_OFFSET_IN_FILE_BLOCK(de_offset), INODE_FILE_DATA_SIZE));

	return 1;
}

/**
 * @brief
 * test that an existing file not in root dir, and do namei() when cwd is not root and path is relative.
 * should be recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest6(){
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_file);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir2);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_dir1);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_root);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	uint8_t *f_full_name = "directory2/file1.dat";
	uint32_t f_type;
	int32_t ino_num;
	int32_t de_offset;

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
	VERIFY(!readBlock(log_addr_dir2, fs_buffer));
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
	VERIFY(!readBlock(log_addr_dir1, fs_buffer));
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
	VERIFY(!readBlock(log_addr_root, fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0,log_addr);

	VERIFY(!allocAndWriteBlock(log_addr_root, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));


	/* write inode0 with pointers to the new root address*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));

	INODE_SET_DIRECT(ino_ptr,0,log_addr_root);
	INODE_SET_DIRECT(ino_ptr,1,log_addr_dir1);
	INODE_SET_DIRECT(ino_ptr,2,log_addr_dir2);
	INODE_SET_DIRECT(ino_ptr,3,log_addr_file);
	INODE_SET_NBYTES(ino_ptr, CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+4*FS_BLOCK_SIZE));
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0));

	/* and now try finding file1.dat*/
//	PRINT("\n\n\n\ndo namei()");
//	init_indirect_caches();
	ino_num = namei(f_full_name, &f_type, & de_offset, 2);
//	PRINT_MSG_AND_NUM("\nnamei() done. ino num=", ino_num);
	VERIFY(COMPARE(ino_num, 4));
//	PRINT("\nino num done");
	VERIFY(COMPARE(f_type, FTYPE_FILE));
//	PRINT("\nftype done");
	VERIFY(COMPARE(de_offset-CALC_OFFSET_IN_FILE_BLOCK(de_offset), INODE_FILE_DATA_SIZE));

	return 1;
}

/**********************************************findEmptySparseBlock tests *****************************/

/**
 * @brief
 * init findEmptySparseBlock test
 *
 */
void init_findEmptySparseBlockTest(){
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
 * tear down findEmptySparseBlock test
 *
 */
void tearDown_findEmptySparseBlockTest(){
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
 * findEmptySparseBlock in root directory. should locate first block
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest1(){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	int32_t offset;
	int32_t tid = 0;
	init_logical_address(block_log_addr);

	/* find sparse block in root directory*/
	TRANSACTION_SET_INO(tid, 1);
	VERIFY(!findEmptySparseBlock(1, block_log_addr, &offset, tid));

//	PRINT_MSG_AND_NUM("\nfound sparse block at offset=",offset);
	VERIFY(COMPARE(offset, INODE_FILE_DATA_SIZE));

	return 1;
}

/**
 * @brief
 * findEmptySparseBlock in root directory, after filling all direct entries, and first indirect entry.
 * should locate second indirect block
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest2(){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	int32_t offset, i;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	int32_t tid = 0;

	init_logical_address(block_log_addr);
	init_logical_address(ino_log_addr);
	init_logical_address(prev_log_addr);

	/* write indirect block with first entry filled*/
	init_fsbuf(fs_buffer);

	MARK_BLOCK_NO_HOLE(block_log_addr);
	BLOCK_SET_INDEX(fs_buffer, 0, block_log_addr);
	VERIFY(!allocAndWriteBlock(block_log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nallocated mock indirect block to addr ",logicalAddressToPhysical(block_log_addr));
//	PRINT_MSG_AND_NUM("\nafter allocAndWriteBlock() addr is marked with hole? ",IS_ADDR_FREE(block_log_addr));
//
//	PRINT_MSG_AND_HEX("\n*CAST_TO_UINT32(block_log_addr)=",*CAST_TO_UINT32(block_log_addr));
//	PRINT_MSG_AND_HEX("\n((ADDR_FLAG_TAKEN << 31) & 0xfffffff))=",((ADDR_FLAG_TAKEN << 31) & 0xfffffff));
//	PRINT_MSG_AND_HEX("\n*CAST_TO_UINT32(block_log_addr) & ((ADDR_FLAG_TAKEN << 31) & 0xfffffff))=",*CAST_TO_UINT32(block_log_addr) & ((ADDR_FLAG_TAKEN << 31) & 0xfffffff));
	MARK_BLOCK_NO_HOLE(block_log_addr);
	/* read inode, change it, and rewrite*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
	for(i=0; i< DIRECT_INDEX_ENTRIES;i++){
		INODE_SET_DIRECT(ino_ptr, i, block_log_addr);
	}
	MARK_BLOCK_WITH_HOLE(block_log_addr);
	INODE_SET_INDIRECT(ino_ptr, block_log_addr);
//	PRINT_MSG_AND_NUM("\nsetting indirect block=",logicalAddressToPhysical(block_log_addr));
//	PRINT_MSG_AND_NUM("\nis with hole?=",IS_ADDR_FREE(block_log_addr));
//	assert(0);
	VERIFY(!allocAndWriteBlock(ino_log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

//	PRINT_MSG_AND_NUM("\nnew ino1 address=",logicalAddressToPhysical(ino_log_addr));
	/* read inode0, change it, and rewrite*/
	//	VERIFY(!getInode(fs_buffer, 1, FS_GET_INO0_ADDR_PTR()));
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 0, ino_log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT("\nallocAndWriteBlock() success");

	/* find sparse block in root directory*/
	TRANSACTION_SET_INO(tid, 1);
	VERIFY(!findEmptySparseBlock(1, block_log_addr, &offset, tid));
//	PRINT("\nfindEmptySparseBlock() success");
	VERIFY(COMPARE(offset, INDIRECT_DATA_OFFSET+FS_BLOCK_SIZE));


	return 1;
}

/**
 * @brief
 * findEmptySparseBlock in root directory, after filling all direct entries, marking indirect entries not sparse,
 * and first indirect entry as full.
 * should locate first direct block in second indirect block
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest3(){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	int32_t offset, i;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	int32_t tid = 0;
	init_logical_address(block_log_addr);
	init_logical_address(ino_log_addr);
	init_logical_address(prev_log_addr);

	/* write double block with first entry filled with no hole*/
	init_fsbuf(fs_buffer);

	MARK_BLOCK_NO_HOLE(block_log_addr);
	BLOCK_SET_INDEX(fs_buffer, 0, block_log_addr);
	VERIFY(!allocAndWriteBlock(block_log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	MARK_BLOCK_NO_HOLE(block_log_addr);
//	PRINT_MSG_AND_NUM("\nafter marking no hole is address free? ",IS_ADDR_FREE(block_log_addr));
	/* read inode, change direct and indirect entries to full, and rewrite*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
	for(i=0; i< DIRECT_INDEX_ENTRIES;i++){
		INODE_SET_DIRECT(ino_ptr, i, block_log_addr);
		INODE_GET_DIRECT(ino_ptr, i, block_log_addr);
	}
	INODE_SET_INDIRECT(ino_ptr, block_log_addr);
	MARK_BLOCK_WITH_HOLE(block_log_addr);
	INODE_SET_DOUBLE(ino_ptr, block_log_addr);

	VERIFY(!allocAndWriteBlock(ino_log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

//	PRINT_MSG_AND_NUM("\nnew ino1 address=",logicalAddressToPhysical(ino_log_addr));
	/* read inode0, change it, and rewrite*/
	//	VERIFY(!getInode(fs_buffer, 1, FS_GET_INO0_ADDR_PTR()));
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 0, ino_log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* find sparse block in root directory*/
	TRANSACTION_SET_INO(tid, 1);
	VERIFY(!findEmptySparseBlock(1, block_log_addr, &offset, tid));
	VERIFY(COMPARE(offset, DOUBLE_DATA_OFFSET+INODE_INDIRECT_DATA_SIZE));

	return 1;
}

/**
 * @brief
 * findEmptySparseBlock in root directory, after filling all direct entries, marking double, indirect entries as not sparse,
 * and first double entry as full.
 * should locate first direct block in second double block, first indirect block
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest4(){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(read_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	int32_t offset, i;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	int32_t tid = 0;
	/* verify triple offset is used*/
	if(TRIPLE_DATA_OFFSET + INODE_DOUBLE_DATA_SIZE > FS_MAX_FILESIZE){
		return 1;
	}

	init_logical_address(block_log_addr);
	init_logical_address(read_log_addr);
	init_logical_address(ino_log_addr);
	init_logical_address(prev_log_addr);

	/* write double block with first entry filled with no hole*/
	init_fsbuf(fs_buffer);

	MARK_BLOCK_NO_HOLE(block_log_addr);
	BLOCK_SET_INDEX(fs_buffer, 0, block_log_addr);
	MARK_BLOCK_WITH_HOLE(block_log_addr);
	BLOCK_SET_INDEX(fs_buffer, 1, block_log_addr);
	VERIFY(!allocAndWriteBlock(block_log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	MARK_BLOCK_NO_HOLE(block_log_addr);
//	PRINT_MSG_AND_NUM("\nafter marking no hole is address free? ",IS_ADDR_FREE(block_log_addr));
	/* read inode, change direct and indirect entries to full, and rewrite*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
	for(i=0; i< DIRECT_INDEX_ENTRIES;i++){
		INODE_SET_DIRECT(ino_ptr, i, block_log_addr);
		INODE_GET_DIRECT(ino_ptr, i, block_log_addr);
	}
	INODE_SET_INDIRECT(ino_ptr, block_log_addr);
	INODE_SET_DOUBLE(ino_ptr, block_log_addr);
	MARK_BLOCK_WITH_HOLE(block_log_addr);
	INODE_SET_TRIPLE(ino_ptr, block_log_addr);

	VERIFY(!allocAndWriteBlock(ino_log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

//	PRINT_MSG_AND_NUM("\nnew ino1 address=",logicalAddressToPhysical(ino_log_addr));
	/* read inode0, change it, and rewrite*/
	//	VERIFY(!getInode(fs_buffer, 1, FS_GET_INO0_ADDR_PTR()));
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 0, ino_log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* find sparse block in root directory*/
	TRANSACTION_SET_INO(tid, 1);
	VERIFY(!findEmptySparseBlock(1, read_log_addr, &offset, tid));
//	PRINT_MSG_AND_NUM("\nafter findEmptySparseBlock offset=",offset);
//	PRINT_MSG_AND_NUM("\nshould be=",TRIPLE_DATA_OFFSET+INODE_DOUBLE_DATA_SIZE);
	VERIFY(COMPARE(offset, TRIPLE_DATA_OFFSET+INODE_DOUBLE_DATA_SIZE));

	return 1;
}

/**
 * @brief
 * findEmptySparseBlock in root directory, when all blocks are marked as full, after FS_MAX_FILESIZE size.
 * fill all direct entries, mark double, indirect entries as not sparse, and all triple indexed blocks accordingly *
 * should return error  - can't find block
 *
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest5(){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(read_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	int32_t offset, i;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	int32_t tid = 0;
	init_logical_address(block_log_addr);
	init_logical_address(read_log_addr);
	init_logical_address(ino_log_addr);
	init_logical_address(prev_log_addr);

	/* write double block with first entry filled with no hole*/
	init_fsbuf(fs_buffer);

	MARK_BLOCK_NO_HOLE(block_log_addr);
	/* mark all */
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK-2;i++){
		BLOCK_SET_INDEX(fs_buffer, i, block_log_addr);
	}

	VERIFY(!allocAndWriteBlock(block_log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	MARK_BLOCK_NO_HOLE(block_log_addr);
//	PRINT_MSG_AND_NUM("\nafter marking no hole is address free? ",IS_ADDR_FREE(block_log_addr));
	/* read inode, change direct and indirect entries to full, and rewrite*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
	for(i=0; i< DIRECT_INDEX_ENTRIES;i++){
		INODE_SET_DIRECT(ino_ptr, i, block_log_addr);
		INODE_GET_DIRECT(ino_ptr, i, block_log_addr);
	}
	INODE_SET_INDIRECT(ino_ptr, block_log_addr);
	INODE_SET_DOUBLE(ino_ptr, block_log_addr);
	MARK_BLOCK_WITH_HOLE(block_log_addr);
	INODE_SET_TRIPLE(ino_ptr, block_log_addr);

	VERIFY(!allocAndWriteBlock(ino_log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

//	PRINT_MSG_AND_NUM("\nnew ino1 address=",logicalAddressToPhysical(ino_log_addr));
	/* read inode0, change it, and rewrite*/
//	VERIFY(!getInode(fs_buffer, 1, FS_GET_INO0_ADDR_PTR()));
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 0, ino_log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

//	PRINT_MSG_AND_NUM("\nres=",res);
//	PRINT_MSG_AND_NUM("\nfound empty block=",logicalAddressToPhysical(read_log_addr));
//	PRINT_MSG_AND_NUM("\noffset=",offset);
	/* find sparse block in root directory*/
	TRANSACTION_SET_INO(tid, 1);
	VERIFY(IS_NEGATIVE(findEmptySparseBlock(1, read_log_addr, &offset, tid)));

	return 1;
}

/**
 * @brief
 * findEmptySparseBlock in root directory, where there is a negative address followed by valid addresses
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest6(){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	int32_t offset, i, negative_direct_offset=5;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t cpWritten;
	int32_t tid = 0;
	init_logical_address(ino_log_addr);
	init_logical_address(prev_log_addr);

	/* write indirect block with first entry filled*/
	init_fsbuf(fs_buffer);

	MARK_BLOCK_NO_HOLE(block_log_addr);
	/* read inode, change it, and rewrite*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
	for(i=0; i< DIRECT_INDEX_ENTRIES;i++){
		INODE_SET_DIRECT(ino_ptr, i, block_log_addr);
	}
	MARK_BLOCK_WITH_HOLE(block_log_addr);
	INODE_SET_DIRECT(ino_ptr, negative_direct_offset, block_log_addr);

	VERIFY(!allocAndWriteBlock(ino_log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

//	PRINT_MSG_AND_NUM("\nnew ino1 address=",logicalAddressToPhysical(ino_log_addr));
	/* read inode0, change it, and rewrite*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 0, ino_log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* find sparse block in root directory*/
	TRANSACTION_SET_INO(tid, 1);
	VERIFY(!findEmptySparseBlock(1, block_log_addr, &offset, tid));
	VERIFY(COMPARE(offset, INODE_FILE_DATA_SIZE+negative_direct_offset*FS_BLOCK_SIZE));

	return 1;
}

/**
 * @brief
 * findEmptySparseBlock in root directory, where there is an empty address followed by valid addresses
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest7(){
	uint32_t seg, log_offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	int32_t offset, i, negative_direct_offset;
	bool_t cpWritten;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t tid = 0;

	seg = 5;
	log_offset = 6;
	init_logical_address(block_log_addr);
	init_logical_address(ino_log_addr);
	init_logical_address(prev_log_addr);
	negative_direct_offset=5;

	/* write indirect block with first entry filled*/
	init_fsbuf(fs_buffer);

	SET_LOGICAL_OFFSET(block_log_addr, log_offset);
	SET_LOGICAL_SEGMENT(block_log_addr, seg);
	MARK_BLOCK_NO_HOLE(block_log_addr);
	/* read inode, change it, and rewrite*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
	for(i=0; i< DIRECT_INDEX_ENTRIES;i++){
		INODE_SET_DIRECT(ino_ptr, i, block_log_addr);
	}
	MARK_BLOCK_WITH_HOLE(block_log_addr);
	INODE_SET_DIRECT(ino_ptr, negative_direct_offset, block_log_addr);

	VERIFY(!allocAndWriteBlock(ino_log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

//	PRINT_MSG_AND_NUM("\nnew ino1 address=",logicalAddressToPhysical(ino_log_addr));
	/* read inode0, change it, and rewrite*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr, 0, ino_log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* find sparse block in root directory*/
	TRANSACTION_SET_INO(tid, 1);
	VERIFY(!findEmptySparseBlock(1, block_log_addr, &offset, tid));
	VERIFY(COMPARE(offset, INODE_FILE_DATA_SIZE+negative_direct_offset*FS_BLOCK_SIZE));

	VERIFY(COMPARE(log_offset,GET_LOGICAL_OFFSET(block_log_addr)));
	VERIFY(COMPARE(seg,GET_LOGICAL_SEGMENT(block_log_addr)));
	return 1;
}

/**
 * @brief
 * init findIndirectEntriesBlock test
 *
 */
void init_findIndirectEntriesBlockTest(){
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
 * tear down findIndirectEntriesBlock test
 *
 */
void tearDown_findIndirectEntriesBlockTest(){
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
 * get indirect block of offset in file data size.
 * indirect block should be inode
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest1(){
	int32_t i, tid;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);

	tid = getFreeTransactionId();
	init_logical_address(ino_log_addr);

	/* read address of root directory inode */
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr,0, ino_log_addr);

	TRANSACTION_SET_INO(tid, 1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);

	VERIFY(!findIndirectEntriesBlock(0, tid));
//	PRINT("\nfindIndirectEntriesBlock success");

	VERIFY(!readBlock(ino_log_addr, fs_buffer));
	/* compare indirect block and inode, should be identical*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
//		PRINT_MSG_AND_NUM("\nverifying byte ",i);
//		PRINT_MSG_AND_HEX(" fs_buffer byte ",fs_buffer[i]);
//		PRINT_MSG_AND_HEX(" indirect block byte ",TRANSACTION_GET_INDIRECT_PTR(tid)[i]);
		VERIFY(COMPARE(fs_buffer[i], TRANSACTION_GET_INDIRECT_PTR(tid)[i]));
	}

	VERIFY(COMPARE(0,TRANSACTION_GET_FILE_OFFSET(tid)))

	return 1;
}

/**
 * @brief
 * get indirect block of offset in direct entry
 * indirect block should be inode
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest2(){
	int32_t i, tid;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);

	init_logical_address(ino_log_addr);
	tid = getFreeTransactionId();

	/* read address of root directory inode */
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr,0, ino_log_addr);

	TRANSACTION_SET_INO(tid, 1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);

	VERIFY(!findIndirectEntriesBlock(INODE_FILE_DATA_SIZE, tid));
//	PRINT("\nfindIndirectEntriesBlock success");

	VERIFY(!readBlock(ino_log_addr, fs_buffer));
	/* compare indirect block and inode, should be identical*/
	for(i=0; i< FS_BLOCK_SIZE; i++){

		VERIFY(COMPARE(fs_buffer[i], TRANSACTION_GET_INDIRECT_PTR(tid)[i]));
	}

	VERIFY(COMPARE(0,TRANSACTION_GET_FILE_OFFSET(tid)))

	return 1;
}

/**
 * @brief
 * get indirect block of offset in indirect entry
 * indirect bock should be the file's indirect block
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest3(){
	int32_t i, tid;
	uint32_t seg, offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	bool_t cpWritten;
	uint8_t indirect[FS_BLOCK_SIZE];
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);

	tid = getFreeTransactionId();
	seg    = 5;
	offset = 5;
	init_logical_address(ino_log_addr);
	init_logical_address(block_log_addr);
	init_logical_address(log_addr);

	SET_LOGICAL_SEGMENT(log_addr, seg);
	SET_LOGICAL_OFFSET(log_addr, offset);

	for(i=0; i< LOG_ADDRESSES_PER_BLOCK;i++){
		BLOCK_SET_INDEX(indirect, i, log_addr);
	}
	VERIFY(!allocAndWriteBlock(block_log_addr, indirect, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* read root inode, change it, and rewrite*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
	INODE_SET_INDIRECT(ino_ptr,block_log_addr);
	VERIFY(!allocAndWriteBlock(block_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* read inode0, change it, and rewrite*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0,block_log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* read address of root directory inode */
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr,0, ino_log_addr);

	TRANSACTION_SET_INO(tid, 1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);
	VERIFY(!findIndirectEntriesBlock(INODE_INDIRECT_DATA_SIZE, tid));
//	PRINT("\nfindIndirectEntriesBlock success");

	/* compare indirect block and inode, should be identical*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
//		PRINT_MSG_AND_NUM("\nverifying byte ",i);
//		PRINT_MSG_AND_HEX(". buf byte ",indirect[i]);
//		PRINT_MSG_AND_HEX(", indirect byte ",TRANSACTION_GET_INDIRECT_PTR(tid)[i]);
		VERIFY(COMPARE(indirect[i], TRANSACTION_GET_INDIRECT_PTR(tid)[i]));
	}

//	PRINT_MSG_AND_NUM("\nindirect offset=",TRANSACTION_GET_FILE_OFFSET(tid));
//	PRINT_MSG_AND_NUM("\nINDIRECT_DATA_OFFSET=",INDIRECT_DATA_OFFSET);
	VERIFY(COMPARE(INDIRECT_DATA_OFFSET,TRANSACTION_GET_FILE_OFFSET(tid)))

	return 1;
}

/**
 * @brief
 * get indirect block of offset in first double block entry
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest4(){
	int32_t i, tid, indirect_entry_offset, random_size;
	uint32_t seg, offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	bool_t cpWritten;
	uint8_t indirect[FS_BLOCK_SIZE];
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);

	tid = getFreeTransactionId();
	indirect_entry_offset = 5;
	random_size = 8*FS_BLOCK_SIZE;
	seg   = 5;
	offset= 5;
	init_logical_address(ino_log_addr);
	init_logical_address(block_log_addr);
	init_logical_address(log_addr);

	/* write indirect block*/
	SET_LOGICAL_SEGMENT(log_addr, seg);
	SET_LOGICAL_OFFSET(log_addr, offset);
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK;i++){
		BLOCK_SET_INDEX(indirect, i, log_addr);
	}
	VERIFY(!allocAndWriteBlock(block_log_addr, indirect, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nindirect written to =",logicalAddressToPhysical(block_log_addr));
	/* write double block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer,indirect_entry_offset,block_log_addr);
	VERIFY(!allocAndWriteBlock(block_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\ndouble written to =",logicalAddressToPhysical(block_log_addr));
	/* read root inode, change it, and rewrite*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
	INODE_SET_DOUBLE(ino_ptr,block_log_addr);
	VERIFY(!allocAndWriteBlock(block_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* read inode0, change it, and rewrite*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0,block_log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* read address of root directory inode */
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr,0, ino_log_addr);

	TRANSACTION_SET_INO(tid, 1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);

	VERIFY(!findIndirectEntriesBlock(DOUBLE_DATA_OFFSET+indirect_entry_offset*INODE_INDIRECT_DATA_SIZE+random_size, tid));
//	PRINT("\nfindIndirectEntriesBlock success");

	/* compare indirect block and inode, should be identical*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
//		PRINT_MSG_AND_NUM("\nverifying byte ",i);
//		PRINT_MSG_AND_HEX(". buf byte ",indirect[i]);
//		PRINT_MSG_AND_HEX(", indirect byte ",TRANSACTION_GET_INDIRECT_PTR(tid)[i]);
		VERIFY(COMPARE(indirect[i], TRANSACTION_GET_INDIRECT_PTR(tid)[i]));
	}

//	PRINT_MSG_AND_NUM("\nindirect offset=",TRANSACTION_GET_FILE_OFFSET(tid));
	VERIFY(COMPARE(DOUBLE_DATA_OFFSET+indirect_entry_offset*INODE_INDIRECT_DATA_SIZE,TRANSACTION_GET_FILE_OFFSET(tid)))

	return 1;
}

/**
 * @brief
 * get indirect block of offset in one of the triple block entries
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest5(){
	int32_t i, tid, indirect_entry_offset, double_entry_offset, random_size;
	int32_t file_offset;
	uint32_t seg, offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	bool_t cpWritten;
	uint8_t indirect[FS_BLOCK_SIZE];
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);

	indirect_entry_offset = 5;
	double_entry_offset   = 1;
	random_size = 8*FS_BLOCK_SIZE;

	/* verify triple offset is used*/
	if(TRIPLE_DATA_OFFSET + INODE_DOUBLE_DATA_SIZE*double_entry_offset > FS_MAX_FILESIZE){
		return 1;
	}

	tid = getFreeTransactionId();
	file_offset = TRIPLE_DATA_OFFSET +indirect_entry_offset*INODE_INDIRECT_DATA_SIZE + double_entry_offset*INODE_DOUBLE_DATA_SIZE+random_size;
	seg=5;
	offset=5;
	init_logical_address(ino_log_addr);
	init_logical_address(block_log_addr);
	init_logical_address(log_addr);

	/* write indirect block*/
	SET_LOGICAL_SEGMENT(log_addr, seg);
	SET_LOGICAL_OFFSET(log_addr, offset);
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK;i++){
		BLOCK_SET_INDEX(indirect, i, log_addr);
	}
	VERIFY(!allocAndWriteBlock(block_log_addr, indirect, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
//	PRINT_MSG_AND_NUM("\nindirect written to =",logicalAddressToPhysical(block_log_addr));
	/* write double block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer,indirect_entry_offset,block_log_addr);
	VERIFY(!allocAndWriteBlock(block_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* write triple block*/
	init_fsbuf(fs_buffer);
	BLOCK_SET_INDEX(fs_buffer,double_entry_offset,block_log_addr);
	VERIFY(!allocAndWriteBlock(block_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

//	PRINT_MSG_AND_NUM("\ndouble written to =",logicalAddressToPhysical(block_log_addr));
	/* read root inode, change it, and rewrite*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
	INODE_SET_TRIPLE(ino_ptr,block_log_addr);
	VERIFY(!allocAndWriteBlock(block_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* read inode0, change it, and rewrite*/
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_SET_DIRECT(ino_ptr,0,block_log_addr);
	VERIFY(!allocAndWriteBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* read address of root directory inode */
	VERIFY(!fsReadBlockSimple(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr,0, ino_log_addr);

	TRANSACTION_SET_INO(tid, 1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);

	VERIFY(!findIndirectEntriesBlock(file_offset, tid));
//	PRINT("\nfindIndirectEntriesBlock success");

	/* compare indirect block and inode, should be identical*/
	for(i=0; i< FS_BLOCK_SIZE; i++){
//		PRINT_MSG_AND_NUM("\nverifying byte ",i);
//		PRINT_MSG_AND_HEX(". buf byte ",indirect[i]);
//		PRINT_MSG_AND_HEX(", indirect byte ",TRANSACTION_GET_INDIRECT_PTR(tid)[i]);
		VERIFY(COMPARE(indirect[i], TRANSACTION_GET_INDIRECT_PTR(tid)[i]));
	}
//	PRINT_MSG_AND_NUM("\nindirect offset=",TRANSACTION_GET_FILE_OFFSET(tid));
//	PRINT_MSG_AND_NUM("\nfile_offset=",file_offset);
	VERIFY(COMPARE(file_offset-random_size,TRANSACTION_GET_FILE_OFFSET(tid)))

	return 1;
}

/**
 * @brief
 * get indirect block of offset larger than file size
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest6(){
	int32_t tid;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);

	tid = getFreeTransactionId();
	init_logical_address(ino_log_addr);

	/* read root inode, change it, and rewrite*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));

	TRANSACTION_SET_INO(tid, 1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);

//	PRINT_MSG_AND_NUM("\nindirect offset=", TRANSACTION_GET_FILE_OFFSET(tid));
	VERIFY(IS_NEGATIVE(findIndirectEntriesBlock(FS_MAX_FILESIZE, tid)));

	return 1;
}

/**
 * @brief
 * get cached indirect block, and verify it is not in cache anymore
 *
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest7(){
	int32_t tid;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t cid = 0;
	tid = getFreeTransactionId();
	init_logical_address(ino_log_addr);

	/* read address of root directory inode to cache*/
	VERIFY(!readBlock(FS_GET_INO0_ADDR_PTR(), fs_buffer));
	INODE_GET_DIRECT(ino_ptr,0, ino_log_addr);

	fsReadBlock(ino_log_addr, fs_buffer, tid, CACHE_ENTRY_OFFSET_EMPTY, FLAG_CACHEABLE_READ_YES);
	VERIFY(compare_bufs(fs_buffer, CACHE_GET_BUF_PTR(0), FS_BLOCK_SIZE));

	TRANSACTION_SET_INO(tid, 1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);

	VERIFY(!findIndirectEntriesBlock(0, tid));
//	PRINT("\nfindIndirectEntriesBlock success");
//	L("dirty cache %d , cache log addr %x", CACHE_IS_DIRTY(cid), *((uint32_t*)CACHE_GET_LOG_ADDR_PTR(cid)));
	VERIFY(IS_CACHE_FREE(cid));
	VERIFY(!readBlock(ino_log_addr, fs_buffer));

	/* compare indirect block and inode, should be identical*/
	VERIFY(compare_bufs(fs_buffer, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE));

	VERIFY(COMPARE(0,TRANSACTION_GET_FILE_OFFSET(tid)))

	return 1;
}

/**
 * @brief
 * get indirect block of offset in indirect entry
 * indirect block should be the file's indirect block.
 * verify cached inode is aware of that
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest8(){
	int32_t i, tid;
	uint32_t seg, offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	bool_t cpWritten;
	uint8_t indirect[FS_BLOCK_SIZE];

	tid = getFreeTransactionId();
	seg    = 5;
	offset = 5;
	init_logical_address(ino_log_addr);
	init_logical_address(block_log_addr);
	init_logical_address(log_addr);

	SET_LOGICAL_SEGMENT(log_addr, seg);
	SET_LOGICAL_OFFSET(log_addr, offset);

	for(i=0; i< LOG_ADDRESSES_PER_BLOCK;i++){
		BLOCK_SET_INDEX(indirect, i, log_addr);
	}
	VERIFY(!allocAndWriteBlock(block_log_addr, indirect, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));

	/* read root inode, change it, and rewrite (cached write this time)*/
	VERIFY(!getInode(TRANSACTION_GET_INDIRECT_PTR(tid), 1, TRANSACTION_GET_INO_ADDR_PTR(tid)));
	INODE_SET_INDIRECT(CAST_TO_INODE(TRANSACTION_GET_INDIRECT_PTR(tid)),block_log_addr);
	VERIFY(!allocAndWriteBlockTid(TRANSACTION_GET_INO_ADDR_PTR(tid),
								  TRANSACTION_GET_INDIRECT_PTR(tid),
								  DATA_TYPE_INODE,
								  CACHE_ENTRY_OFFSET_EMPTY,
								  tid));

	if(FS_CACHE_BUFFERS > 0){
		/* verify cache*/
	//	L("TRANSACTION_GET_INO_ADDR_PTR(tid) %x", *((uint32_t*)TRANSACTION_GET_INO_ADDR_PTR(tid)));
		VERIFY(IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid)));
		VERIFY(!IS_ADDR_EMPTY(TRANSACTION_GET_INO_ADDR_PTR(tid)));
		VERIFY(!CACHE_GET_IS_INDIRECT_LEAF(CACHE_GET_CID_FROM_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid))));
	}

	/* call findIndirectEntriesBlock()*/
	TRANSACTION_SET_INO(tid, 1);
	VERIFY(!findIndirectEntriesBlock(INODE_INDIRECT_DATA_SIZE, tid));

	if(FS_CACHE_BUFFERS > 0){
		/* verify chnages in cached inode*/
		VERIFY(IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid)));
		VERIFY(CACHE_GET_IS_INDIRECT_LEAF(CACHE_GET_CID_FROM_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid))));
	//	PRINT("\nfindIndirectEntriesBlock success");
	}

	/* compare indirect block and inode, should be identical*/
	VERIFY(compare_fsbufs(indirect, TRANSACTION_GET_INDIRECT_PTR(tid)));

//	PRINT_MSG_AND_NUM("\nindirect offset=",TRANSACTION_GET_FILE_OFFSET(tid));
//	PRINT_MSG_AND_NUM("\nINDIRECT_DATA_OFFSET=",INDIRECT_DATA_OFFSET);
	VERIFY(COMPARE(INDIRECT_DATA_OFFSET,TRANSACTION_GET_FILE_OFFSET(tid)))

	return 1;
}

/**
 * @brief
 * init commitInode test
 *
 */
void init_commitInodeTest(){
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
 * tear down commitInode test
 *
 */
void tearDown_commitInodeTest(){
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
 * commit inode, when indirect is the inode
 * @return 1 if successful, 0 otherwise
 */
error_t commitInodeTest1(){
	int32_t i, tid, old_offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	tid = getFreeTransactionId();
	init_logical_address(ino_log_addr);
	init_logical_address(log_addr);

	/* read root inode*/
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
	TRANSACTION_SET_INO(tid,1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);

	/* read inode to indirect*/
	VERIFY(!fsReadBlockSimple(ino_log_addr, TRANSACTION_GET_INDIRECT_PTR(tid)));
	TRANSACTION_SET_FILE_OFFSET(tid,0);
	old_offset = GET_RECLAIMED_OFFSET();
//	PRINT("\nabout to commitInode")
	old_offset = GET_RECLAIMED_OFFSET();
	VERIFY(!commitInode(tid));
//	PRINT("\ncommitInode success")

	if(!IS_CACHE_EMPTY()){
	/* verify no write has occurred*/
	VERIFY(COMPARE(old_offset, GET_RECLAIMED_OFFSET()));

	/* verify ino_addr cached as expected, and data hasn't changed*/
	VERIFY(!IS_CACHED_ADDR(ino_log_addr));
	VERIFY(IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	int32_t cid = CACHE_GET_CID_FROM_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid));
	VERIFY(!IS_CACHE_FREE(cid));

	/* compare indirect block and inode, should be identical*/
	VERIFY(compare_bufs(CACHE_GET_BUF_PTR(cid), TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE));
	}

	/* verify first vot entry*/
	BLOCK_GET_INDEX(TRANSACTION_GET_VOTS_BUF_PTR(tid),0,log_addr);
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(log_addr), GET_LOGICAL_OFFSET(ino_log_addr)));
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(log_addr), GET_LOGICAL_SEGMENT(ino_log_addr)));

	/* verify all the rest are empty */
	for(i=1; i<LOG_ADDRESSES_PER_BLOCK;i++){
		BLOCK_GET_INDEX(TRANSACTION_GET_VOTS_BUF_PTR(tid),i,log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	return 1;
}

/**
 * @brief
 * commit inode, when active indirect is the first indirect block
 * @return 1 if successful, 0 otherwise
 */
error_t commitInodeTest2(){
	int32_t i, tid, log_offset, offset, seg;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_ino_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(cached_addr);
	bool_t cpWritten;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t cid_indirect, cid_inode;
	uint8_t indirect_buf[FS_BLOCK_SIZE];
	uint8_t inode_buf[FS_BLOCK_SIZE];

	tid = getFreeTransactionId();
	offset = 5;
	seg    = 2;
	init_logical_address(ino_log_addr);
	init_logical_address(indirect_addr);
	init_logical_address(log_addr);
	init_logical_address(old_ino_addr);
	init_logical_address(cached_addr);

	/* set indirect block */
	SET_LOGICAL_OFFSET(log_addr, offset);
	SET_LOGICAL_SEGMENT(log_addr, seg);

	for(i=0; i <LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_SET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), i, log_addr);
	}
	fsMemcpy(indirect_buf, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE);

	/* read root inode, set mock indirect old address */
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));

	INODE_SET_INDIRECT(ino_ptr, log_addr);
	fsMemcpy(inode_buf, fs_buffer, FS_BLOCK_SIZE);
	VERIFY(!allocAndWriteBlock(ino_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
	copyLogicalAddress(old_ino_addr, ino_log_addr);
	TRANSACTION_SET_INO(tid,1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);
	TRANSACTION_SET_FILE_OFFSET(tid,INDIRECT_DATA_OFFSET);
	cache_init_lru_q();
	log_offset = GET_RECLAIMED_OFFSET();

	/******* call commitInode ******/
//	PRINT("\nabout to commitInode")
	VERIFY(!commitInode(tid));
//	PRINT("\ncommitInode success")

	/* verify indirect and inode blocks (whther in cache or not)*/
	if(!IS_CACHE_EMPTY()){
	/* verify no write has occurred*/
	VERIFY(COMPARE(log_offset, GET_RECLAIMED_OFFSET()));

	/* verify inode and indirect are cached as expected*/
	VERIFY(IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	cid_inode = cache_get_cid_by_real_addr(ino_log_addr);
	VERIFY(!IS_CACHE_ID_EMPTY(cid_inode));
	cid_indirect = CACHE_GET_LESS_RECENTLY_USED(cid_inode);

//	VERIFY(compare_bufs(CACHE_GET_BUF_PTR(cid_indirect), indirect_buf, FS_BLOCK_SIZE));
//	return 0;
	/* verify indiret*/
	VERIFY(verify_cache(cid_indirect,
						CACHE_IS_VOTED_YES, /* old indirect address should have been voted*/
						0, /* no ref count*/
						cid_inode, /* when inode is cached, indirect is recognized as child*/
						INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES, /* offset in inode*/
						CID_EMPTY, /* nothing cached previously */
						cid_inode, /* no others cached*/
						tid, /* cahced during tid*/
						CACHE_DIRTY, /* dirty block*/
						log_addr, /* old indirect addr*/
						indirect_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

	/* verify inode -
	 * first fix indirect addres so it is it's cached address
	 * in expected inode buffer*/
	SET_LOGICAL_SEGMENT(cached_addr, CACHE_MAGIC_SEG);
	SET_LOGICAL_OFFSET(cached_addr, cid_indirect);
	INODE_SET_INDIRECT(CAST_TO_INODE(inode_buf), cached_addr);
	VERIFY(verify_cache(cid_inode,
						CACHE_IS_VOTED_YES, /* old indirect address should have been voted*/
						1, /* no ref count*/
						CACHE_ID_EMPTY, /* no parent*/
						CACHE_ENTRY_OFFSET_EMPTY, /* no offset in parent*/
						cid_indirect, /* indiret cached previously*/
						CACHE_ID_EMPTY, /* no others cached afterwords*/
						tid, /* cahced during tid*/
						CACHE_DIRTY, /* dirty block*/
						ino_log_addr, /* old indirect addr*/
						inode_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));
	}

	/* verify first vot entry is old indirect */
	BLOCK_GET_INDEX(TRANSACTION_GET_VOTS_BUF_PTR(tid),0,indirect_addr);
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(indirect_addr), offset));
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(indirect_addr), seg));

	/* verify second vot entry is old inode */
	BLOCK_GET_INDEX(TRANSACTION_GET_VOTS_BUF_PTR(tid),1,log_addr);
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(old_ino_addr), GET_LOGICAL_OFFSET(log_addr)));
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(old_ino_addr), GET_LOGICAL_SEGMENT(log_addr)));

	/* verify all the rest are empty */
	for(i=2; i<LOG_ADDRESSES_PER_BLOCK;i++){
		BLOCK_GET_INDEX(TRANSACTION_GET_VOTS_BUF_PTR(tid),i,log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	return 1;
}

/**
 * @brief
 * commit inode, when indirect is an entry in double block
 * @return 1 if successful, 0 otherwise
 */
error_t commitInodeTest3(){
	int32_t i, tid, log_offset, offset, seg, indirect_entry;
	int32_t indirect_offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_ino_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(cached_addr);
	bool_t cpWritten;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t cid_double, cid_indirect, cid_inode;
	uint8_t double_buf[FS_BLOCK_SIZE];
	uint8_t indirect_buf[FS_BLOCK_SIZE];
	uint8_t inode_buf[FS_BLOCK_SIZE];

	tid = getFreeTransactionId();
	offset = 5;
	seg    = 2;
	indirect_entry = 5;
	indirect_offset = DOUBLE_DATA_OFFSET+indirect_entry*INODE_INDIRECT_DATA_SIZE;
	init_logical_address(ino_log_addr);
	init_logical_address(old_indirect_addr);
	init_logical_address(log_addr);
	init_logical_address(old_double_addr);
	init_logical_address(old_ino_addr);
	init_logical_address(cached_addr);

	init_fsbuf(indirect_buf);
	init_fsbuf(double_buf);
	init_fsbuf(inode_buf);

	/* set mock indirect addresses in double block*/
	SET_LOGICAL_OFFSET(log_addr, offset);
	SET_LOGICAL_SEGMENT(log_addr, seg);
	copyLogicalAddress(old_indirect_addr, log_addr);
	BLOCK_SET_INDEX(double_buf, indirect_entry, log_addr);
//	L("write double, when indirect entry addr is %x", *((uint32_t*)log_addr));
	VERIFY(!allocAndWriteBlock(log_addr, double_buf, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
	copyLogicalAddress(old_double_addr, log_addr);

	/* read root inode, set mock indirect old address */
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
//	L("write inode, when double entry addr is %x", *((uint32_t*)log_addr));
	INODE_SET_DOUBLE(ino_ptr, log_addr);

	VERIFY(!allocAndWriteBlock(ino_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
	copyLogicalAddress(old_ino_addr, ino_log_addr);
	fsMemcpy(inode_buf, fs_buffer, FS_BLOCK_SIZE);

	/* set transaction indirect block */
	SET_LOGICAL_OFFSET(log_addr, offset);
	SET_LOGICAL_SEGMENT(log_addr, seg);

	for(i=0; i <LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_SET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), i, log_addr);
	}
	fsMemcpy(indirect_buf, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE);

	/* set transaction*/
	TRANSACTION_SET_INO(tid,1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);
	TRANSACTION_SET_FILE_OFFSET(tid,indirect_offset);

	log_offset = GET_RECLAIMED_OFFSET();
//	PRINT("\nabout to commitInode")
	/******** CALL commitInode()***********/
	cache_init_lru_q();
	VERIFY(!commitInode(tid));

	if(!IS_CACHE_EMPTY()){
	VERIFY(COMPARE(log_offset, GET_RECLAIMED_OFFSET()));

	cid_double = cache_get_cid_by_real_addr(old_double_addr);
	VERIFY(!IS_CACHE_ID_EMPTY(cid_double));
//	L("get cid for old_indirect_addr %x", *((uint32_t*)old_indirect_addr));
	cid_indirect = cache_get_cid_by_real_addr(old_indirect_addr);
	VERIFY(!IS_CACHE_ID_EMPTY(cid_indirect));
	cid_inode = cache_get_cid_by_real_addr(old_ino_addr);
	VERIFY(!IS_CACHE_ID_EMPTY(cid_inode));

	/* verify double (including buffer with cached addr)*/
//	L("cid_double %d, cid_indirect %d, cid_inode %d", cid_double, cid_indirect, cid_inode);
	SET_LOGICAL_SEGMENT(cached_addr, CACHE_MAGIC_SEG);
	SET_LOGICAL_OFFSET(cached_addr, cid_indirect);
	MARK_BLOCK_WITH_HOLE(cached_addr);
//	L("fixing double with cached indirect addr %x", *((uint32_t*)cached_addr));
	BLOCK_SET_INDEX(double_buf, indirect_entry, cached_addr);
//	L("print double cached buffer");
//	printBlock(CACHE_GET_BUF_PTR(cid_double));
//	L("print expected double buffer");
//	printBlock(double_buf);
	VERIFY(verify_cache(cid_double,
						CACHE_IS_VOTED_YES, /* old double address should have been voted*/
						1, /* no ref count*/
						cid_inode, /* when inode is cached, indirect is recognized as child*/
						DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES, /* offset in inode*/
						cid_indirect, /* indirect written previously*/
						cid_inode, /* inode written afterwards*/
						tid, /* cahced during tid*/
						CACHE_DIRTY, /* dirty block*/
						old_double_addr, /* old indirect addr*/
						double_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

	/* verify indirect*/
	VERIFY(!IS_CACHE_ID_EMPTY(cid_indirect));
	VERIFY(verify_cache(cid_indirect,
						CACHE_IS_VOTED_YES, /* old indirect address should have been voted*/
						0, /* no ref count*/
						cid_double, /* child of double*/
						indirect_entry, /* offset in inode*/
						CACHE_ID_EMPTY, /* no prev*/
						cid_double, /* double next written*/
						tid, /* cahced during tid*/
						CACHE_DIRTY, /* dirty block*/
						log_addr, /* old indirect addr*/
						indirect_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

	/* verify inode -
	 * first fix indirect addres so it is it's cached address
	 * in expected inode buffer*/
	SET_LOGICAL_SEGMENT(cached_addr, CACHE_MAGIC_SEG);
	SET_LOGICAL_OFFSET(cached_addr, cid_double);
	INODE_SET_DOUBLE(CAST_TO_INODE(inode_buf), cached_addr);
	VERIFY(verify_cache(cid_inode,
						CACHE_IS_VOTED_YES, /* old indirect address should have been voted*/
						1, /* ref count of double*/
						CACHE_ID_EMPTY, /* no parent*/
						CACHE_ENTRY_OFFSET_EMPTY, /* no offset in parent*/
						cid_double, /* indiret cached previously*/
						CACHE_ID_EMPTY, /* this is mru*/
						tid, /* cahced during tid*/
						CACHE_DIRTY, /* dirty block*/
						ino_log_addr, /* old indirect addr*/
						inode_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));
	}

	/* verify first vot entry is empty */
	VERIFY(verifyVotEntry(tid, 0, old_double_addr));
	VERIFY(verifyVotEntry(tid, 1, old_indirect_addr));
	VERIFY(verifyVotEntry(tid, 2, old_ino_addr));

	/* verify all the rest are empty */
	for(i=3; i<LOG_ADDRESSES_PER_BLOCK;i++){
		BLOCK_GET_INDEX(TRANSACTION_GET_VOTS_BUF_PTR(tid),i,log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	return 1;
}

/**
 * @brief
 * commit inode, when indirect is mapped somewhere in triple block
 * @return 1 if successful, 0 otherwise
 */
error_t commitInodeTest4(){
	int32_t i, tid, log_offset, indirect_addr_offset, double_addr_offset, seg, indirect_entry, double_entry;
	int32_t indirect_offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_triple_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_ino_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(cached_addr);
	bool_t cpWritten;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t cid_triple, cid_double, cid_indirect, cid_inode;
	uint8_t triple_buf[FS_BLOCK_SIZE];
	uint8_t double_buf[FS_BLOCK_SIZE];
	uint8_t indirect_buf[FS_BLOCK_SIZE];
	uint8_t inode_buf[FS_BLOCK_SIZE];

	tid = getFreeTransactionId();
	indirect_addr_offset = 5;
	double_addr_offset = 20;
	seg    = 2;
	indirect_entry = 5;
	double_entry = 1;
	indirect_offset = TRIPLE_DATA_OFFSET+double_entry*INODE_DOUBLE_DATA_SIZE+indirect_entry*INODE_INDIRECT_DATA_SIZE;
	init_logical_address(ino_log_addr);
	init_logical_address(old_indirect_addr);
	init_logical_address(log_addr);
	init_logical_address(old_double_addr);
	init_logical_address(old_triple_addr);
	init_logical_address(old_ino_addr);
	init_logical_address(cached_addr);

	init_fsbuf(indirect_buf);
	init_fsbuf(double_buf);
	init_fsbuf(triple_buf);
	init_fsbuf(inode_buf);

	/* write indirect block -
	 * get triple, double ,indirect...*/
	SET_LOGICAL_OFFSET(log_addr, indirect_addr_offset);
	SET_LOGICAL_SEGMENT(log_addr, seg);
	copyLogicalAddress(old_indirect_addr, log_addr);
	BLOCK_SET_INDEX(double_buf, indirect_entry, log_addr);
//	L("write double, when indirect entry addr is %x", *((uint32_t*)log_addr));
	VERIFY(!allocAndWriteBlock(log_addr, double_buf, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
	copyLogicalAddress(old_double_addr, log_addr);

	BLOCK_SET_INDEX(triple_buf, double_entry, old_double_addr);
//	L("write triple, when double entry addr is %x", *((uint32_t*)old_double_addr));
	VERIFY(!allocAndWriteBlock(log_addr, triple_buf, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
	copyLogicalAddress(old_triple_addr, log_addr);

	/* read root inode, set mock indirect old address */
	VERIFY(!getInode(fs_buffer, 1, ino_log_addr));
//	L("write inode, when triple entry addr is %x", *((uint32_t*)old_triple_addr));
	INODE_SET_TRIPLE(ino_ptr, old_triple_addr);

	VERIFY(!allocAndWriteBlock(ino_log_addr, fs_buffer, 0, log_addr, &cpWritten, fsCheckpointWriter, IS_COMMIT_NOT));
	copyLogicalAddress(old_ino_addr, ino_log_addr);
	fsMemcpy(inode_buf, fs_buffer, FS_BLOCK_SIZE);

	/* set transaction indirect block */
	SET_LOGICAL_OFFSET(log_addr, indirect_addr_offset);
	SET_LOGICAL_SEGMENT(log_addr, seg);

	/* set mock indirect addresses in indirect block*/
	for(i=0; i <LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_SET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), i, log_addr);
	}
	fsMemcpy(indirect_buf, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE);

	/* set transaction*/
	TRANSACTION_SET_INO(tid,1);
	TRANSACTION_SET_INO_ADDR(tid, ino_log_addr);
	TRANSACTION_SET_FILE_OFFSET(tid,indirect_offset);
//	L("set transaction file offset to %d (actually it is %d)", indirect_offset, TRANSACTION_GET_FILE_OFFSET(tid));
	log_offset = GET_RECLAIMED_OFFSET();
//	L("about to commitInode");
	/******** CALL commitInode()***********/
	cache_init_lru_q();
	VERIFY(!commitInode(tid));

	if(!IS_CACHE_EMPTY()){
	VERIFY(COMPARE(log_offset, GET_RECLAIMED_OFFSET()));

	/* get cache id's*/
//	L("old_triple_addr is %x", ADDR_PRINT(old_triple_addr));
	cid_triple = cache_get_cid_by_real_addr(old_triple_addr);
	VERIFY(!IS_CACHE_ID_EMPTY(cid_triple));
	cid_double = cache_get_cid_by_real_addr(old_double_addr);
	VERIFY(!IS_CACHE_ID_EMPTY(cid_double));
	cid_indirect = cache_get_cid_by_real_addr(old_indirect_addr);
	VERIFY(!IS_CACHE_ID_EMPTY(cid_indirect));
	cid_inode = cache_get_cid_by_real_addr(old_ino_addr);
	VERIFY(!IS_CACHE_ID_EMPTY(cid_inode));
//	L("cid_triple %d, cid_double %d, cid_indirect %d, cid_inode %d", cid_triple, cid_double, cid_indirect, cid_inode);

	/* verify indirect*/
	VERIFY(!IS_CACHE_ID_EMPTY(cid_indirect));
	VERIFY(verify_cache(cid_indirect,
						CACHE_IS_VOTED_YES, /* old indirect address should have been voted*/
						0, /* no ref count*/
						cid_double, /* child of double*/
						indirect_entry, /* offset in inode*/
						CACHE_ID_EMPTY, /* no prev*/
						cid_double, /* double next written*/
						tid, /* cahced during tid*/
						CACHE_DIRTY, /* dirty block*/
						log_addr, /* old indirect addr*/
						indirect_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

	/* verify double (including buffer with cached addr)*/
	SET_LOGICAL_SEGMENT(cached_addr, CACHE_MAGIC_SEG);
	SET_LOGICAL_OFFSET(cached_addr, cid_indirect);
	MARK_BLOCK_WITH_HOLE(cached_addr);
//	L("fixing double with cached indirect addr %x", *((uint32_t*)cached_addr));

	BLOCK_SET_INDEX(double_buf, indirect_entry, cached_addr);
//	L("print double cached buffer");
//	printBlock(CACHE_GET_BUF_PTR(cid_double));
//	L("print expected double buffer");
//	printBlock(double_buf);
	VERIFY(verify_cache(cid_double,
						CACHE_IS_VOTED_YES, /* old double address should have been voted*/
						1, /* no ref count*/
						cid_triple, /* triple is parent*/
						double_entry, /* offset in inode*/
						cid_indirect, /* indirect written previously*/
						cid_triple, /* inode written afterwards*/
						tid, /* cahced during tid*/
						CACHE_DIRTY, /* dirty block*/
						old_double_addr, /* old indirect addr*/
						double_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

	/* verify triple */
//	L("cid_double %d, cid_indirect %d, cid_inode %d", cid_double, cid_indirect, cid_inode);
	SET_LOGICAL_SEGMENT(cached_addr, CACHE_MAGIC_SEG);
	SET_LOGICAL_OFFSET(cached_addr, cid_double);
	MARK_BLOCK_WITH_HOLE(cached_addr);
//	L("fixing double with cached indirect addr %x", *((uint32_t*)cached_addr));
	BLOCK_SET_INDEX(triple_buf, double_entry, cached_addr);
//	L("print double cached buffer");
//	printBlock(CACHE_GET_BUF_PTR(cid_double));
//	L("print expected double buffer");
//	printBlock(double_buf);
	VERIFY(verify_cache(cid_triple,
						CACHE_IS_VOTED_YES, /* old double address should have been voted*/
						1, /* no ref count*/
						cid_inode, /* when inode is cached, indirect is recognized as child*/
						TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES, /* offset in inode*/
						cid_double, /* indirect written previously*/
						cid_inode, /* inode written afterwards*/
						tid, /* cahced during tid*/
						CACHE_DIRTY, /* dirty block*/
						old_triple_addr, /* old indirect addr*/
						triple_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));

	/* verify inode -
	 * first fix indirect addres so it is it's cached address
	 * in expected inode buffer*/
	SET_LOGICAL_SEGMENT(cached_addr, CACHE_MAGIC_SEG);
	SET_LOGICAL_OFFSET(cached_addr, cid_triple);
	INODE_SET_TRIPLE(CAST_TO_INODE(inode_buf), cached_addr);
	VERIFY(verify_cache(cid_inode,
						CACHE_IS_VOTED_YES, /* old indirect address should have been voted*/
						1, /* ref count of double*/
						CACHE_ID_EMPTY, /* no parent*/
						CACHE_ENTRY_OFFSET_EMPTY, /* no offset in parent*/
						cid_triple, /* indiret cached previously*/
						CACHE_ID_EMPTY, /* this is mru*/
						tid, /* cahced during tid*/
						CACHE_DIRTY, /* dirty block*/
						ino_log_addr, /* old indirect addr*/
						inode_buf,
						CACHE_IS_ACTIVE_INDIRECT_LEAF_NO));
	}

	/* verify first vot entry is empty */
	VERIFY(verifyVotEntry(tid, 0, old_triple_addr));
	VERIFY(verifyVotEntry(tid, 1, old_double_addr));
	VERIFY(verifyVotEntry(tid, 2, old_indirect_addr));
	VERIFY(verifyVotEntry(tid, 3, old_ino_addr));
//	PRINT("\n vot entries success");
	/* verify all the rest are empty */
	for(i=4; i<LOG_ADDRESSES_PER_BLOCK;i++){
		BLOCK_GET_INDEX(TRANSACTION_GET_VOTS_BUF_PTR(tid),i,log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}
//	PRINT("\n empty vot entries success");
	return 1;
}

