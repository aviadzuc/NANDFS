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

/** @file fsUtils.c
 * File system low-level auxiliary functions. */

#include <utils/string_lib.h>
#include <utils/memlib.h>
#include <src/sequencing/sequencingUtils.h>
#include <src/fs/fs.h>
#include <src/fs/fsUtils.h>
#include <src/fs/cache.h>
#include <src/fs/transactions.h>
#ifdef Debug
	#include <test/fs/testsHeader.h>
#endif

/* pointers to global data structures */
extern transaction_t transactions[FS_MAX_N_TRANSACTIONS];
extern vnode_t vnodes[FS_MAX_VNODES];
extern open_file_t open_files[FS_MAX_OPEN_FILES];
extern fs_t fs_ptr;
extern uint8_t fs_buffer[FS_BLOCK_SIZE];
extern fs_dirstream dirstreams[FS_MAX_OPEN_DIRESTREAMS];
static uint8_t illegal_chars[ILLEGAL_CHARS_NUM] = {2};
extern cache_lru_q lru_ptr;

/**
 * @brief
 * a mock checkpoint writer function. writes a pseudo file system checkopint data
 * and the sequencing cp data using commit()
 * @param isPartOfHeader is the checkpoint written as part of a header
 * @return 0 if successful, 1 if an error occured in commit
 */
error_t fsCheckpointWriter(bool_t isPartOfHeader){
	int32_t i;
//	uint32_t temp;
//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - fs_data_size=",MOCK_FS_DATA_SIZE);
//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - SEQ_CHECKPOINT_SIZE=",SEQ_CHECKPOINT_SIZE);
//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - fs_data_size=",MOCK_FS_DATA_SIZE);
//	temp = 	GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr)*SEQ_PAGES_PER_SLOT+GET_RECLAIMED_OFFSET();
//	PRINT_MSG_AND_NUM("\nfsCheckpointWriter() - starting. isPartOfHeader=", isPartOfHeader);
//	PRINT_MSG_AND_NUM(" write to addr", temp);
//	PRINT_MSG_AND_NUM(" seg=", GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" offset=", GET_RECLAIMED_OFFSET());
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		init_logical_address(FS_GET_TID_LAST_WRITTEN_ADDR(i));

		FS_SET_TID_LAST_WRITTEN_ADDR(i, TRANSACTION_GET_PREV_ADDR_PTR(i));
	}

	SEQ_VERIFY(!commit(fs_ptr, sizeof(filesystem_t)-sizeof(nandfs_mutex_t), isPartOfHeader));

//	PRINT("\nfsCheckpointWriter() - ");
//	PRINT_MSG_AND_HEX(" last closed tid addr=", *CAST_TO_UINT32(FS_GET_LAST_CLOSED_TID_ADDR()));
//	PRINT_MSG_AND_HEX(", tid0 last addr", *CAST_TO_UINT32(FS_GET_TID_LAST_WRITTEN_ADDR(0)));
//	if(temp == 33047 || temp == 32769){
//		uint8_t buf[NAND_TOTAL_SIZE];
//		INIT_FLAGS_POINTER_TO_BUFFER(flags, buf);
//		nandReadPageTotal(buf, temp);
//		printSeqBlock(buf);
//		PRINT_MSG_AND_NUM("\ncp flag for page ",temp);
//		PRINT_MSG_AND_NUM("=",GET_CHECKPOINT_FLAG(flags));
//		assert(0);
//	}
//	PRINT("\nfsCheckpointWriter() - commit was successful");
//	PRINT_MSG_AND_NUM(", frees=", GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM(", rec slot=", GET_RECLAIMED_SEGMENT_SLOT());

	return 0;
}

void init_file_entry(uint32_t fd){
	OPEN_FILE_SET_FLAGS(fd,0);
	OPEN_FILE_SET_OFFSET(fd,0);
	OPEN_FILE_SET_UID(fd,UID_EMPTY);
	OPEN_FILE_SET_VNODE(fd,VNODE_EMPTY);
	OPEN_FILE_SET_FTYPE(fd,FTYPE_EMPTY);
}

/**
 * @brief
 * init all file entries
 */
void init_file_entries(void){
	uint32_t i;

	for(i=0;i<FS_MAX_OPEN_FILES;i++){
		init_file_entry(i);
	}
}

/**
 * @brief
 * init a single vnode
 */
void init_vnode(uint32_t vnode_idx){
	VNODE_SET_INO_NUM(vnode_idx, INO_NUM_EMPTY);
	VNODE_SET_NREFS(vnode_idx,0);
}

/**
 * @brief
 * init all vnodes
 */
void init_vnodes(void){
	uint32_t i;

	for(i=0; i< FS_MAX_VNODES;i++){
		init_vnode(i);
	}
}

void initializeRamStructs() {
	initializeSequencingStructs();
	init_file_entries();
	init_vnodes();
	init_distreams();
	init_fsbuf(fs_buffer);
	init_transactions();
	cache_init_lru_q();
	init_fs();
	/* initialize mutex*/
	FS_MUTEX_INIT();
}
/**
 * @brief
 * auxiliary . verify fd is legal
 *
 * @param fd file descriptor
 * @return 0 if legal, -1 otherwise
 */
int32_t verifyFd(int32_t fd){
	/* check fd legal */
	FS_VERIFY(IS_FD_LEGAL(fd));
//	PRINT("\nverifyFd() - fd legal");

	/* verify fd refers to an open file */
	FS_VERIFY(!IS_OPEN_FILE_ENTRY_EMPTY(fd));
//	PRINT("\nverifyFd() - fentry not empty");

	/* verify user */
	FS_VERIFY(IS_USER_LEGAL(fd));
//	PRINT("\nverifyFd() - user legal");

	return 0;
}

/**
 * Get file updated inode
 *
 * @param ino_num inode number
 * @return FS_ERROR_SUCCESS on success
 */
int32_t getFileUpdatedInode(int32_t ino_num){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	int32_t tid = TID_EMPTY, i;
	init_logical_address(log_addr);
//	FENT();

//	L("ino_num=%d", ino_num);
	/* check if any transaction involves this file*/
	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		if(IS_TRANSACTION_EMPTY(i)){
			continue;
		}

//		L("check if transaction %d ino num %d is equal to ino_num %d", i, TRANSACTION_GET_INO(i), ino_num);
		if(TRANSACTION_GET_INO(i) == ino_num){
//			PRINT_MSG_AND_NUM("\n ino_num=", TRANSACTION_GET_INO(i));
//			PRINT_MSG_AND_NUM(" for transaction=", i);
			tid = i;
			break;
		}
	}
//	PRINT("\ngetFileSize() - after iterating transactions");

	/* if we have no transaction involving this file*/
	if(IS_EMPTY_TID(tid)){
//		L("tid empty!");
//		L("b4 getInode() of ino_num %d", ino_num);
		FS_VERIFY(!getInode(fs_buffer, ino_num, log_addr));
//		PRINT("\ngetFileSize() - after getInode()");
//		L("SUCCESS. ino addr %x",ADDR_PRINT(log_addr));
	}
	else{
		/* if active transaction indirect is the inode, read from it*/
		if(IS_INDIRECT_INODE(tid)){
			fsMemcpy(fs_buffer, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE);
		}
		/* read last commited inode address.
		 * NOTICE - don't read inode0*/
		else{
//			L("tid=%d. read inode from %x", tid, ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));
			FS_VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),
								   fs_buffer,
								   tid,
								   CACHE_ENTRY_OFFSET_EMPTY,
								   FLAG_CACHEABLE_READ_YES));
	//		if(fs_buffer[0]!='a' && TRANSACTION_GET_FILE_OFFSET(tid) > INDIRECT_DATA_OFFSET){
	//			PRINT(". print inode block");printBlock(fs_buffer);
	//			assert(0);
	//		}
	//		PRINT_MSG_AND_HEX("\ngetFileSize()- b4 changeFileSize() file_size=",INODE_GET_NBYTES(CAST_TO_INODE(fs_buffer)));
		}

		/* change file size in inode*/
//		L("call changeFileSize");
		changeFileSize(tid, fs_buffer);
//		PRINT_MSG_AND_HEX("\ngetFileSize()- after changeFileSize() file_size=",INODE_GET_NBYTES(CAST_TO_INODE(fs_buffer)));
	}

//	PRINT("\ngetFileSize()- finished");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * get block size of a file with inode number ino_num
 *
 * Assumptions:
 * 1. inode number is of a valid existing file
 *
 * @param ino_num inode number of file
 * @return file size
 */
int32_t getFileBlocks(int32_t ino_num){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);
	init_fsbuf(fs_buffer);
	FS_VERIFY(!getFileUpdatedInode(ino_num));

	return CAST_VAL_TO_INT32(INODE_GET_NBLOCKS(CAST_TO_INODE(fs_buffer)));
}

/**
 * @brief
 * get file size of a file with inode number ino_num
 *
 * Assumptions:
 * 1. inode number is of a valid existing file
 *
 * @param ino_num inode number of file
 * @return file size
 */
int32_t getFileSize(int32_t ino_num){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//	FENT();
	init_logical_address(log_addr);
	init_fsbuf(fs_buffer);

	FS_VERIFY(!getFileUpdatedInode(ino_num));

	return CAST_VAL_TO_INT32(INODE_GET_NBYTES(CAST_TO_INODE(fs_buffer)));
}

/**
 * @brief
 * get a an existing vnode index for a given inode
 *
 * @param ino_num the inode number
 * @return vnode index if successful, VNODE_EMPTY otherwise
 */
int32_t getVnodeByInodeNum(uint32_t ino_num){
	int32_t i;

	for(i=0;i<FS_MAX_VNODES;i++){
		if(VNODE_GET_INO_NUM(i) == ino_num)
			return i;
	}

	return VNODE_EMPTY;
}


/**
 * @brief
 * get a free open file entry
 * @return file descriptor is found one, FD_EMPTY otherwise
 */
int32_t getFreeOpenFileEntry(void){
	int32_t i;

	for(i=0;i<FS_MAX_OPEN_FILES;i++){
		if(IS_OPEN_FILE_ENTRY_EMPTY(i))
			return i;
	}
//	PRINT_MSG_AND_NUM("\n", );
	return FD_EMPTY;
}

/**
 * @brief
 * check if we have a conflict between flags of different open file entries:
 * 1. trying to write to an already open file
 * 2. trying to read from a file that's being writen
 *
 * @param vnode_idx index of a file vnode
 * @param flags flags to check conflict
 * @return 1 if a conflict exists, 0 otherwise
 */
error_t isFlagsConflict(uint32_t vnode_idx, uint32_t flags){
	uint32_t i;

	/* iterate all open file entries*/
	for(i=0; i< FS_MAX_OPEN_FILES; i++){
		if(!COMPARE(OPEN_FILE_GET_VNODE(i),vnode_idx))
			continue;

		/* if our file is opened for reading by another user , but we are already writing
		 * to it, return error */
		if(IS_RDONLY(flags)){
			if(IS_WRONLY(OPEN_FILE_GET_FLAGS(i))){
				/* if current user is different than writing user*/
				if(GET_CURRENT_USER() != OPEN_FILE_GET_UID(i)){
					return 1;
				}
			}
		}

//		PRINT_MSG_AND_NUM("\nisFlagsConflict() - found file with vnode. fd=", i);
		/* if the file is already open for writing*/
		if(IS_WRONLY(flags)){
			/* can't write to a file open by another user*/
			if(GET_CURRENT_USER() != OPEN_FILE_GET_UID(i)){
//				PRINT("\nisFlagsConflict() - can't write to a file open by another user");
				return 1;
			}

//			PRINT_MSG_AND_NUM(" is open for writing?", IS_WRONLY(OPEN_FILE_GET_FLAGS(i)));
			/* can't write to a file already open for writing by this user*/
			if(IS_WRONLY(OPEN_FILE_GET_FLAGS(i))){
				return 1;
			}
		}
	}
//	PRINT("\nisFlagsConflict() - finished successfuly");
	return 0;
}

/**
 * @brief
 * move to next directory entry in a buffer
 * @param dirent_ptr
 * @param offset
 */
void moveToNextDirentry(dirent_flash **dirent_ptr, uint32_t offset){
	uint8_t *ptr = CAST_TO_UINT8(*dirent_ptr);

	ptr = &(ptr[offset]);
//	ptr +=  offset;

	*dirent_ptr = CAST_TO_DIRENT(ptr);
}

/**
 * @brief
 * set new directory entry according to given details, in the buffer pointer
 *
 * Assupmtions:
 * 1. name length is legal
 *
 * @param dirent_ptr pointer to a directory entry
 * @param f_id file id
 * @param f_type file type
 * @param name file name
 */
void setNewDirentry(dirent_flash *dirent_ptr, int32_t f_id, uint32_t f_type, uint8_t *name){
//	PRINT_MSG_AND_STR("\nsetNewDirentry() - starting. name=", name);
//	PRINT_MSG_AND_HEX(" f_id=",f_id);

	DIRENT_SET_INO_NUM(dirent_ptr, f_id);
//	PRINT("\nsetNewDirentry() - set ino num");
//	PRINT_MSG_AND_HEX("\nsetNewDirentry() - ino num is now=",DIRENT_GET_INO_NUM(dirent_ptr));
//	assert(0);
	DIRENT_SET_TYPE(dirent_ptr, f_type);
//	PRINT("\nsetNewDirentry() - set f_type");
//	PRINT_MSG_AND_HEX("\nsetNewDirentry() - about to set len=",calcNameLen(name)+DIRENT_FLASH_FIELDS_SIZE+1);
//	PRINT_MSG_AND_HEX(" name len=",calcNameLen(name));
//	PRINT_MSG_AND_HEX(" b4 setting, direntry len=",DIRENT_GET_LEN(dirent_ptr));
	DIRENT_SET_LEN(dirent_ptr,calcNameLen(name));
//	PRINT_MSG_AND_HEX("\nsetNewDirentry() - after setting direntry len=",DIRENT_GET_LEN(dirent_ptr));
//	PRINT_MSG_AND_HEX(" de name len=",DIRENT_GET_NAME_LEN(dirent_ptr));
	DIRENT_SET_NAME(dirent_ptr,name);
//	PRINT_MSG_AND_HEX("\nsetNewDirentry() - after setting name len=",calcNameLen(name)+DIRENT_FLASH_FIELDS_SIZE+1);
//	PRINT_MSG_AND_HEX(" de name len=",DIRENT_GET_NAME_LEN(dirent_ptr));

//	PRINT("\nsetNewDirentry() - finished");
}

/**
 * @brief
 * auxiliary to verifyLegalPathname.
 * check that the character c is not in the illegal characters array
 * @return 1 if legal, 0 if illegal.
 */
error_t isLegalChar(uint8_t c){
	int32_t i;

	for(i=0; i<ILLEGAL_CHARS_NUM; i++){
		if(illegal_chars[i] == c){
			return 0;
		}
	}

	return 1;
}

/**
 * @brief
 * calc name len
 * @param name the name
 * @return name length
 */
uint32_t calcNameLen(uint8_t *name){
	uint32_t i=0;

//	PRINT("\ncalcNameLen() - name=");
//	PRINT(name);
	while(!IS_PATH_END(*name)){
		i++;
		name++;
	}

//	PRINT_MSG_AND_NUM("\ncalcNameLen() - len=", i);

	return i;
}

/**
 * @brief
 * verify that a given pathname is composed only from legal characters -
 * 1. 0x00 - 0x1f not accepted, except a configurable set of characters
 * 2. 0x80 - 0xff not accepted unless specified in exclude list for supporting unicode by treating a char as single UTF-8 octet
 * @param pathname pah name to check
 * @return 1 if legal, 0 otherwise
 */
error_t verifyLegalPathname(uint8_t *pathname){
//	PRINT("\nverifyLegalPathname() - starting");
	/* TODO: legal sets etc.*/
	while(!IS_PATH_END(*pathname)){
		/* check legal characters*/
		if(*pathname < 0x80 && *pathname > 0x1f &&
			isLegalChar(*pathname)){
			pathname++;
			continue;
		}

		return 0;
	}
//	PRINT("\nverifyLegalPathname() - finished");
	return 1;
}

/**
 * @brief
 * find next '/' in pathname, and return offset
 * @return offset in pathname of next '/'
 */
uint32_t findNextSeperator(uint8_t *pathname){
	uint32_t offset = 0;

//	PRINT_MSG_AND_NUM("\nfindNextSeperator() - looking for next '/'=", '/');
//	PRINT("\npathname=");PRINT(pathname);
	while(!IS_SLASH(*pathname) && !IS_PATH_END(*pathname)){
//		PRINT_MSG_AND_NUM("\nbyte=", *pathname);
		pathname++;
		offset++;
	}

	return offset;
}

/**
 * @brief
 * initialize file system with inode0, and root directory.
 * use mock transaction
 * @return 0 if successful, -1 otherwise
 */
int32_t handleNoFs(void){
	bool_t cpWritten, pendingVOTs;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	int32_t res, tid;
	init_logical_address(prev_log_addr);
	init_logical_address(log_addr);
//	FENT();

	tid = getFreeTransactionId();

	TRANSACTION_SET_TYPE(tid, T_TYPE_WRITE);
	TRANSACTION_SET_INO(tid, 0); /* this prevents caching of writes*/

	/* write initial checkpoint*/
	FS_VERIFY(!sequencingBooting(fs_ptr,
								 sizeof(filesystem_t)-sizeof(nandfs_mutex_t),
								 &pendingVOTs,
								 fsCheckpointWriter));

//	L("sequencingBooting success. rec offset %d", GET_RECLAIMED_OFFSET());

	/* write root directory inode, and save it's address*/
//	PRINT("\nhandleNoFs() - call setNewInode ");
	TR_RES_VERIFY(res, setNewInode(1, 1, FTYPE_DIR, tid));
//	L("\nhandleNoFs() - setNewInode success, write root inode to flash. ino id %d", 1);
	TR_RES_VERIFY(res, allocAndWriteBlockTid(log_addr,
											 TRANSACTION_GET_INDIRECT_PTR(tid),
											 DATA_TYPE_REGULAR,
											 CACHE_ENTRY_OFFSET_EMPTY,
											 tid));

	MARK_BLOCK_NO_HOLE(log_addr);
//	L("writeNewInode of root success. rec offset %d", GET_RECLAIMED_OFFSET());
	/* write inode0  */
	init_fsbuf(fs_buffer);

	INODE_SET_FILE_ID(ino_ptr, 0);
	INODE_SET_FILE_TYPE(ino_ptr, FTYPE_FILE);
	INODE_SET_NBLOCKS(ino_ptr, 2);

	INODE_SET_NBYTES(ino_ptr,CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
	INODE_SET_DIRECT(ino_ptr,0,log_addr);

	init_logical_address(log_addr);
//	L("\nhandleNoFs() - setNewInode success, write inode0 to flash. ino id %d", 0);
	TR_RES_VERIFY(res, allocAndWriteBlockTid(log_addr,
											 fs_buffer,
											 DATA_TYPE_REGULAR,
											 CACHE_ENTRY_OFFSET_EMPTY,
											 tid));

//	PRINT("\nhandleNoFs() - allocAndWriteBlock inode0 success");
	/* save address in file system data */
	FS_SET_INO0_ADDR(log_addr);
//	PRINT_MSG_AND_NUM("\nhandleNoFs() - set ino0 address to ", logicalAddressToPhysical(log_addr));
	init_transaction(tid);
	FS_VERIFY(!fsCheckpointWriter(0));
	RES_VERIFY(res, findNextFreePage(&cpWritten, fsCheckpointWriter, 0));

	/* initialize RAM structures that were used during this operation*/
	init_fsbuf(fs_buffer);
	init_transaction(tid);
//	PRINT("\nhandleNoFs() - done");
	return 0;
}

/**
 * @brief
 * auxiliary to readFileBlock().
 * read to buf inode block of file with inode number ino_num
 * and save inode address to ino_log_addr.
 * if we read inode for a file involved in a transaction,
 * make sure to read inode according to transaction data
 *
 * @param buf buffer to save inode to
 * @param ino_num file inode number
 * @param ino_log_addr logical address of inode. if 0, find it from inode0
 * @param tid transaction id
 * @return FS_ERROR_SUCCESS if successful
 * 		   -1 otherwise
 */
int32_t readInodeBlock(uint8_t *buf,
					   uint32_t ino_num,
					   logical_addr_t ino_log_addr,
					   int32_t tid){
	int32_t res;
//	PRINT_MSG_AND_NUM("\nreadInodeBlock() - reading inode for ino_num=", ino_num);
//	PRINT_MSG_AND_NUM("\nreadInodeBlock() - is inode address given empty?=", IS_ADDR_EMPTY(ino_log_addr));
//	PRINT_MSG_AND_HEX("\nreadInodeBlock() - ino addr=", *((uint32_t*)(ino_log_addr)));
//	{
//		int32_t i;
//		for(i=0; i< FS_MAX_VNODES;i++){
//			PRINT_MSG_AND_HEX("\nfentry vnode=", OPEN_FILE_GET_VNODE(i));
//		}
//	}

	/* if we read an inode for file being transactioned */
	if(!IS_EMPTY_TID(tid)){
//		L("is indirect inode?=%d", IS_INDIRECT_INODE(tid));
		/* if the indirec is the inode, read it*/
		if(IS_INDIRECT_INODE(tid)){
//			L("current transaction indirect is the inode. memcpy from it to buf");
			fsMemcpy(buf, TRANSACTION_GET_INDIRECT_PTR(tid), FS_BLOCK_SIZE);
		}
		/* indirect is not inode. read last commited version of the inode*/
		else{
			/* inode offset is of no importance*/
//			L("read inode from transaction ino addr %x", ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));
			RES_VERIFY(res, fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),
										buf,
										tid,
										CACHE_ENTRY_OFFSET_EMPTY,
										FLAG_CACHEABLE_READ_YES));
			copyLogicalAddress(ino_log_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));
		}

//		L("done getting inode from transaction");
		return FS_ERROR_SUCCESS;
	}
//	L("finished checking transactions. inode is not here. get it from flash");
//	{
//		int32_t i;
//		for(i=0; i< FS_MAX_VNODES;i++){
//			PRINT_MSG_AND_HEX("\nfentry vnode=", OPEN_FILE_GET_VNODE(i));
//		}
//	}
	/* get inode data to buf*/
	if(ino_num == 0){
		/* read inode0*/
//		L("read inode0 to buf from %x", ADDR_PRINT(FS_GET_INO0_ADDR_PTR()));
		RES_VERIFY(res, fsReadBlock(FS_GET_INO0_ADDR_PTR(),
									buf,
									tid,
									CACHE_ENTRY_OFFSET_EMPTY,
									FLAG_CACHEABLE_READ_YES));
//		{
//			int32_t i;
//			for(i=0; i< FS_MAX_VNODES;i++){
//				PRINT_MSG_AND_HEX("\nfentry vnode=", OPEN_FILE_GET_VNODE(i));
//			}
//		}
//		L("copyLogicalAddress() of ino0=%d",logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//		PRINT_MSG_AND_HEX(", or in hex=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
		copyLogicalAddress(ino_log_addr, FS_GET_INO0_ADDR_PTR());
	}
	else if(IS_ADDRESS_EMPTY(ino_log_addr)){
//		L("read inode from inode0");
		/* as an optimiztion for sequential reading, inode address is stored in ino_log_addr*/
		RES_VERIFY(res, getInode(buf, ino_num, ino_log_addr));
	}
	else{
		/* read inode0*/
//		L("read inode from ino_log_addr");
		RES_VERIFY(res, fsReadBlock(ino_log_addr,
									buf,
									tid,
									CACHE_ENTRY_OFFSET_EMPTY,
									FLAG_CACHEABLE_READ_YES));
	}

	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * auxiliary to findIndirectEntriesBlock().
 * get file block offset, and calculate it's indirect block offset
 *
 * @return indirect block offset
 */
int32_t calcIndirectBlockOffset(int32_t fileBlockOffset){
	if(fileBlockOffset < INDIRECT_DATA_OFFSET){
		fileBlockOffset = 0;
	}
	else{
		/* set offset to start of indirect size block it is in (+the data before any indirect size)*/
//		PRINT_MSG_AND_NUM("\nsetIndirectOffset() - b4 setting, offset=",offset);
		fileBlockOffset = fileBlockOffset-CALC_OFFSET_IN_INDIRECT_SIZE(fileBlockOffset-INDIRECT_DATA_OFFSET);
	}

	return fileBlockOffset;
}



/**
 * @brief
 * aixiliary. read indirect block containing a direct entry in offset block_offset
 * of a file to buf
 *
 * @param block_offset offset of the file block we wish to read
 * @param ino_log_addr logical address of file inode (may be empty, if so exclusive to ino_log_addr)
 * @param buf buffer to hold inode block (may be already full, if so exclusive to ino_log_addr)
 * @param indirect_addr pointer to hold the indirect block address that we'll find
 * @param is_active_indirect_read are we reading the indirect buffer to the transaction active indirect (if so, remove it from cache)
 * @return FS_ERROR_SUCCESS if successful, read error otherwise
 */
error_t
readIndirectToBuffer(int32_t block_offset,
					 logical_addr_t ino_log_addr,
					 uint8_t *buf,
					 logical_addr_t indirect_addr,
					 int32_t tid,
					 bool_t is_active_indirect_read){
	int32_t res, entry_offset;
	inode_t *ino_ptr = CAST_TO_INODE(buf);
	int32_t org_block_offset = block_offset; /* keep real block_offset for later use (cache purposes)*/
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
//	FENT();

	init_logical_address(log_addr);
	init_logical_address(double_addr);

//	L("ino_log_addr %x, block_offset %d", ADDR_PRINT(ino_log_addr), block_offset);
	/* if we have the inode address, we need to read it (otherwise, inode is already in buf) */
	if(!IS_ADDR_EMPTY(ino_log_addr)){
	/* read inode.
	 * NOTICE - we read from temporary inode address.
	 * not the one in inode0, since we haven't commited it yet*/
//		L("read inode block");
		RES_VERIFY(res, fsReadBlock(ino_log_addr,
									buf,
									tid,
									CACHE_ENTRY_OFFSET_EMPTY,
									FLAG_CACHEABLE_READ_YES));
	}

	/* if offset is less than INODE_INDIRECT_DATA_SIZE
	 * than the indirect block is simply the inode*/
	if(block_offset < INDIRECT_DATA_OFFSET){
//		L("indirect is inode, FINISHED");
		copyLogicalAddress(indirect_addr, ino_log_addr);
		return FS_ERROR_SUCCESS;
	}
	/* if indirect entry*/
	else if(block_offset < DOUBLE_DATA_OFFSET){
//		L("offset < DOUBLE_DATA_OFFSET");
		block_offset = CALC_OFFSET_FROM_INDIRECT(block_offset);
//		/* read inode to buf. */
//		RES_VERIFY(res, fsReadBlock(ino_log_addr, buf));

		/* inode is in buf. read indirect*/
		INODE_GET_INDIRECT(ino_ptr, indirect_addr);
		entry_offset = INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES;
	}
	/* if double entry*/
	else if(block_offset < TRIPLE_DATA_OFFSET){
//		L("offset < INODE_TRIPLE_DATA_SIZE");
		block_offset -= DOUBLE_DATA_OFFSET;
//		/* read inode to buf. */
//		RES_VERIFY(res, fsReadBlock(ino_log_addr, buf));

		/* inode is in buf. read double*/
		INODE_GET_DOUBLE(ino_ptr, log_addr);
		copyLogicalAddress(double_addr, log_addr);
//		L("read double block from %x", ADDR_PRINT(log_addr));
		RES_VERIFY(res, fsReadBlock(log_addr,
									buf,
									tid,
									DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
									FLAG_CACHEABLE_READ_YES));
//		PRINT_MSG_AND_NUM("\nreadIndirectToBuffer() - double entry=",logicalAddressToPhysical(log_addr));
//		PRINT_MSG_AND_NUM("\nreadIndirectToBuffer() - read indirect entry #",CALC_OFFSET_IN_DOUBLE_BLOCK(block_offset));

		/* read indirect*/
		entry_offset = CALC_OFFSET_IN_DOUBLE_BLOCK(block_offset);
		BLOCK_GET_INDEX(buf, entry_offset, indirect_addr);
//		L("get indirect entry #%d which is %x",entry_offset, ADDR_PRINT(indirect_addr));
	}
	/* if triple entry*/
	else{
//		PRINT("\nreadIndirectToBuffer() - offset < FS_MAX_FILESIZE");
		block_offset -= TRIPLE_DATA_OFFSET;
//		/* read inode to buf. */
//		RES_VERIFY(res, fsReadBlock(ino_log_addr, buf));

		/* inode is in buf. read triple*/
		INODE_GET_TRIPLE(ino_ptr, log_addr);
		RES_VERIFY(res, fsReadBlock(log_addr,
									buf,
									tid,
									TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
									FLAG_CACHEABLE_READ_YES));

		/* read double*/
//		PRINT_MSG_AND_NUM("\nreadIndirectToBuffer() - b4 get double entry block_offset=",block_offset);
		BLOCK_GET_INDEX(buf, CALC_OFFSET_IN_TRIPLE_BLOCK(block_offset), log_addr);
//		PRINT_MSG_AND_NUM("\nreadIndirectToBuffer() - get double entry #",CALC_OFFSET_IN_TRIPLE_BLOCK(block_offset));
		RES_VERIFY(res, fsReadBlock(log_addr,
									buf,
									tid,
									CALC_OFFSET_IN_TRIPLE_BLOCK(block_offset),
									FLAG_CACHEABLE_READ_YES));
		copyLogicalAddress(double_addr, log_addr);

		/* read indirect*/
		block_offset = CALC_OFFSET_IN_DOUBLE_SIZE(block_offset);
		entry_offset = CALC_OFFSET_IN_DOUBLE_BLOCK(block_offset);
		BLOCK_GET_INDEX(buf, entry_offset, indirect_addr);
	}

	/* do the actual read*/
//	L("do actual read from %x", ADDR_PRINT(indirect_addr));
	RES_VERIFY(res, fsReadBlock(indirect_addr,
								buf,
								tid,
								entry_offset,
								FLAG_CACHEABLE_READ_YES));

	/* if we do this as part of transaction*/
	if(!IS_EMPTY_TID(tid) && is_active_indirect_read){
//		L("call cache_fix_indirect_leaf_indicator() to set 1, block_offset %d, double_addr cid %d", block_offset, CACHE_GET_CID_FROM_ADDR(double_addr));
		cache_fix_indirect_leaf_indicator(org_block_offset,
										  double_addr,
										  tid,
										  1); /* indirect is now active, not active*/
	}
//	L("FINISHED");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * read a block from file ino_num in offset to buf.
 * as an optimiztion for sequential reading, after the first read of a file block, it's inode address is stored
 * in ino_log_addr. this way. sequential calls for this function will not require reading inode0 again
 *
 * Assumptions:
 * 1. buf is FS_BLOCK_SIZE
 *
 * @param buf buffer to write data to
 * @param ino_num inode number of the file
 * @param offset offset to read from
 * @param ino_log_addr possible logical address of inode (if empty find it in inode0, or transaction)
 * @param tid transaction id during which we are reading this file
 * @return FS_ERROR_SUCCESS if successful
 * 		   read errors otherwise
 */
error_t
readFileBlock(uint8_t *buf,
			  int32_t ino_num,
			  uint32_t offset,
			  logical_addr_t ino_log_addr,
			  int32_t tid){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	int32_t i, res;
	inode_t *ino_ptr;
	uint8_t *indirect_buf = buf;
	int32_t indirect_offset;
	int32_t entry_offset;
//	FENT();

//	PRINT_MSG_AND_NUM(", ino_num=",ino_num);
//	PRINT_MSG_AND_NUM(", offset=",offset);
//	PRINT_MSG_AND_NUM(", tid=", tid);
//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
//	PRINT_MSG_AND_NUM(", transaction indirect offset=", TRANSACTION_GET_FILE_OFFSET(tid));
	indirect_offset= calcIndirectBlockOffset(offset);
	init_logical_address(log_addr);
	ino_ptr = CAST_TO_INODE(indirect_buf);

	/* verify file is not involved in any other transaction */
	for(i=0; i <FS_MAX_N_TRANSACTIONS; i++){
//		PRINT_MSG_AND_NUM("\ni=", i);
		if(i==tid){
			continue;
		}

		/* trying to read an inode involved in another transaction */
		if(!IS_TRANSACTION_EMPTY(tid) && TRANSACTION_GET_INO(i) == ino_num){
//			PRINT("\nreadFileBlock() - file involved in another transaction. error")
			return -1;
		}
	}

	/* verify offset isn't larger than max file size*/
//	PRINT("\nreadFileBlock() - check offset not overflowing file size");
	if(offset > FS_MAX_FILESIZE){
//		PRINT("\nreadFileBlock() - offset > FS_MAX_FILESIZE. return. ");
//		PRINT_MSG_AND_NUM(" offset=", offset);
//		PRINT_MSG_AND_NUM(" FS_MAX_FILESIZE=", FS_MAX_FILESIZE);
		return FS_ERROR_OFST_OVRFLW;
	}
//	PRINT("\nreadFileBlock() - get transaction indirect block");

	/* if data is located in indirect block used by the transaction, read from it.
	 * NOTICE - when we have read caching, the order is VERY important. We have to read from the transaction
	 * indirect buffer since it might be more updated than the cached one. */
	if(!IS_EMPTY_TID(tid) && isOffsetInIndirect(offset, tid)){
//		PRINT("\nreadFileBlock() - reading from indirect buffer of transaction");
		return readFromTransactionIndirectRead(buf, offset, tid);
	}

	/* do a simple read-
	 * inode, triple, double,...
	 * save indirect buffer in buffer (if there is any)*/
	/* First, read inode block.*/
//	L("read inode block to indirect_buf");
	RES_VERIFY(res, readInodeBlock(indirect_buf, ino_num, ino_log_addr, tid));
	ino_ptr = CAST_TO_INODE(indirect_buf);
//	L("inode block FINISHED");
	changeFileSize(tid, indirect_buf);
//	PRINT_MSG_AND_NUM(", after reading inode file size=", INODE_GET_NBYTES(ino_ptr));
//	PRINT_MSG_AND_HEX(", ino log addr=", logicalAddressToPhysical(ino_log_addr));

	/* verify not overflowing file size */
	if(offset > INODE_GET_NBYTES(ino_ptr)){
//		PRINT("\nreadFileBlock() - offset > f_size. return error ");
//		PRINT_MSG_AND_NUM(" ino_num=", ino_num);
//		PRINT_MSG_AND_NUM(" offset=", offset);
//		PRINT_MSG_AND_NUM(" file size=", INODE_GET_NBYTES(ino_ptr));
		return FS_ERROR_OFST_OVRFLW;
	}

	/* read indirect buffer. 3 possibilities -
	 * a. if offset is of data in inode, it is already in buf - simply read it from there*/
	//	PRINT("\nreadFileBlock() - offset doesn't overflow file size ");
	//	PRINT_MSG_AND_NUM(" ino_num=",ino_num);
	//	PRINT_MSG_AND_NUM(" nbytes=",INODE_GET_NBYTES(ino_ptr));
	if(offset < INODE_FILE_DATA_SIZE){
//		PRINT("\nreadFileBlock() - offset < INODE_FILE_DATA_SIZE");
//		PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
		fsMemset(&(buf[INODE_FILE_DATA_SIZE]), FS_EMPTY_FILE_BYTE, FS_BLOCK_SIZE-INODE_FILE_DATA_SIZE);
//		PRINT("\nreadFileBlock() - finished");
//		PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
		return FS_ERROR_SUCCESS;
	}
	/* b. if offset of directly indexed block*/
	else if(offset < INDIRECT_DATA_OFFSET){
//		L("offset < INDIRECT_DATA_OFFSET");
//		PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
		offset -= INODE_FILE_DATA_SIZE;
////		PRINT_MSG_AND_NUM(". new offset=", offset);
//		if(ino_num==0){
//			PRINT_MSG_AND_NUM("\nreadFileBlock() - offset < INODE_DIRECT_DATA_SIZE. offset=", offset);
//			PRINT_MSG_AND_NUM(" ino_num=",ino_num);
//			PRINT_MSG_AND_NUM(" read direct entry =",CALC_IN_BLOCKS(offset));
//		}
		/* read direct blocks*/
		entry_offset = CALC_IN_BLOCKS(offset);
//		L("read direct entry %d", entry_offset);
		INODE_GET_DIRECT(ino_ptr, entry_offset, log_addr);
//		PRINT_MSG_AND_HEX("\nreadFileBlock() - log_addr=", *((uint32_t*)log_addr));//printBlock(indirect_buf);
//		if(!IS_ADDR_EMPTY(log_addr)) PRINT_MSG_AND_NUM(", actual phy addr=", logicalAddressToPhysical(log_addr));
	}
	/* c. block is pointed by some indirect block - store it in indirect buf (whatever it may be, cache or not)
	 * and read from it the direct entry address*/
	else{
		/* read indirect block (inode is already in buf);
		 * NOTICE - we read inode from ino_log_addr, not from inode0 since during a transaction the inode
	 	 *          address changes*/
//		L("offset > INDIRECT_DATA_OFFSET (double offset+). readIndirectToBuffer() from offset %d", offset-CALC_OFFSET_IN_FILE_BLOCK(offset));
//		L("ino_log_addr=%x",*((uint32_t*)ino_log_addr));
		RES_VERIFY(res, readIndirectToBuffer(offset-CALC_OFFSET_IN_FILE_BLOCK(offset),
											 ino_log_addr,
											 indirect_buf,
											 indirect_addr,
											 tid,
											 FS_ACTIVE_INDIRECT_READ_NO));
		entry_offset = CALC_OFFSET_IN_INDIRECT_BLOCK(CALC_OFFSET_IN_INDIRECT_SIZE(CALC_OFFSET_FROM_INDIRECT(offset)));
		BLOCK_GET_INDEX(indirect_buf, entry_offset, log_addr);
//		L("read direct entry %d", entry_offset);
	}

	/* perform the actual block read.
	 * entry_offset should contain the offset in the parent indirect block*/
//	PRINT_MSG_AND_NUM("\nread direct from phy addr=",logicalAddressToPhysical(log_addr));
//	L("do actual read to buf from log addr=%x", ADDR_PRINT(log_addr));
	RES_VERIFY(res, fsReadBlock(log_addr,
								buf,
								tid,
								entry_offset,
								FLAG_CACHEABLE_READ_YES));
//	if(!IS_ADDR_EMPTY(log_addr)) PRINT_MSG_AND_NUM("\nreadFileBlock() - finished. read from ", logicalAddressToPhysical(log_addr));

	/* if we're looking for an inode address (inode0 read, empty inode)
	 * save it into ino_log_addr*/
	if(ino_num==0 && !IS_ADDR_EMPTY(log_addr)){
		copyLogicalAddress(ino_log_addr, log_addr);
	}
//	L("finished");
//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));

	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * compare all directory entries in directory identified by ino_num
 * to name starting in pathname until offset
 * NOTICE - reading from offset == file size will result in reading an empty block
 *
 * @param pathname name string
 * @param offset offset of name end in pathname
 * @param ino_num inode number of directory file, and where the file inode (if found) will be stored
 * @param f_type the file type we will find (if found)
 * @param directoryOffset file directory entry block offset in parent directory
 * @return 0 if successful
 * -1 in case of io error
 * FS_ERROR_OFST_OVRFLW if directory entry was not found
 */
int32_t
compareDirectoryEntries(uint8_t *pathname, uint32_t offset, int32_t *ino_num, uint32_t *f_type, int32_t *directoryOffset){
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	int32_t res =0, f_size;
//	FENT();
//	PRINT_MSG_AND_STR(", pathname=", pathname);
//	PRINT_MSG_AND_NUM(", *ino_num=",*ino_num);
	init_logical_address(ino_log_addr);

	f_size = getFileSize(*ino_num);
//	L("\ncompareDirectoryEntries() - f_size=%d",f_size);
	*directoryOffset = INODE_FILE_DATA_SIZE;

	/* read directory file blocks sequentially*/
	do{
		if(*directoryOffset >= f_size){
//			L("*directoryOffset %d f_size %d", *directoryOffset, f_size);
			res = FS_ERROR_OFST_OVRFLW;
			break;
		}

//		PRINT_MSG_AND_NUM("\ncompareDirectoryEntries() - try reading file block in offset ",*directoryOffset);
		res = readFileBlock(fs_buffer, *ino_num ,*directoryOffset, ino_log_addr, TID_EMPTY);
//		PRINT_MSG_AND_NUM(", res=",res);

		if(res){
			break;
		}
		dirent_ptr = CAST_TO_DIRENT(fs_buffer);
//		PRINT("\ncompareDirectoryEntries() - finished reading entries block");

		/* if no directory entries then advance to next block*/
		if(IS_DIRENT_EMPTY(dirent_ptr)){
//			PRINT_MSG_AND_NUM("\ncompareDirectoryEntries() - block is empty. advance to next block. directoryOffset=",*directoryOffset);
			ADVANCE_TO_NEXT_BLOCK(*directoryOffset);
//			PRINT_MSG_AND_NUM("\ncompareDirectoryEntries() - after advancing directoryOffset=",*directoryOffset);
			continue;
		}
//		PRINT("\ncompareDirectoryEntries() - iterate direntries");

		/* read directory entries from block sequentially */
		while(!IS_DIRENT_EMPTY(dirent_ptr)){
//			PRINT_MSG_AND_STR("\ncompareDirectoryEntries() - iterating direntries. compare pathname=",pathname);
//			PRINT_MSG_AND_STR(" to ",DIRENT_GET_NAME(dirent_ptr));
//			PRINT_MSG_AND_NUM(" in len ",offset);
//			PRINT_MSG_AND_NUM(" direntry len=", DIRENT_GET_NAME_LEN(dirent_ptr));
//			PRINT_MSG_AND_NUM(" ino=", DIRENT_GET_INO_NUM(dirent_ptr));
//			PRINT_MSG_AND_NUM(" fsStrncmp(de len, expected)=", fsStrncmp(DIRENT_GET_NAME(dirent_ptr), pathname, offset));
//			PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
			/* check length and name identical */
			if(offset == DIRENT_GET_NAME_LEN(dirent_ptr) &&
			   !fsStrncmp(DIRENT_GET_NAME(dirent_ptr), pathname, offset)){
//			   	PRINT("\ncompareDirectoryEntries() - comparison successful");
				*f_type = DIRENT_GET_TYPE(dirent_ptr);
				*ino_num = DIRENT_GET_INO_NUM(dirent_ptr);
			   	return 0;
			}

			/* advance pointer */
//			PRINT_MSG_AND_NUM("\ncompareDirectoryEntries() - failed comparison. advance to next direntry, incremenet offset by=",DIRENT_GET_LEN(dirent_ptr));
//			PRINT_MSG_AND_NUM("\ncompareDirectoryEntries() - DIRENT_GET_LEN(dirent_ptr)=",DIRENT_GET_LEN(dirent_ptr));
			*directoryOffset += DIRENT_GET_LEN(dirent_ptr);
//			PRINT_MSG_AND_NUM("\ncompareDirectoryEntries() - *directoryOffset=",*directoryOffset);

			if(IS_DIRENT_BLOCK_FULL(*directoryOffset)){
				break;
			}

			moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
		}

		ADVANCE_TO_NEXT_BLOCK(*directoryOffset);
//		PRINT("\ncompareDirectoryEntries() - advancing to next block");
//		PRINT_MSG_AND_NUM(" in offset ", *directoryOffset);
	}while(1);
//	PRINT("\ncompareDirectoryEntries() - finished reading blocks");

	*ino_num = INO_NUM_EMPTY;
	return res;
}

/**
 *@brief
 * translate a given pathname to the matching inode number
 * @param pathname path name to file
 * @param f_type pointer to store file type
 * @param de_offset file directory entry block offset in parent directory
 * @param ino_num current working directory inode number
 * @return inode number if the path is legal, -1 otherwise
 */
int32_t namei(uint8_t *pathname, uint32_t *f_type, int32_t *de_offset, int32_t ino_num){
	int32_t res;
	uint32_t offset;
//	FENT();
#ifdef Debug
	if(!fsStrcmp(pathname, VALID_MOCK_FILE_NAME_1)){
		*f_type = FTYPE_FILE;
		return VALID_MOCK_INO_NUM_1;
	}

	if(!fsStrcmp(pathname, VALID_MOCK_FILE_NAME_2)){
		*f_type = FTYPE_FILE;
		return VALID_MOCK_INO_NUM_2;
	}

	if(!fsStrcmp(pathname, INVALID_MOCK_FILE_NAME_1)){
		*f_type = FTYPE_FILE;
		return -1;
	}
#endif

	*f_type = FTYPE_DIR;

	/* if we don't get curernt working directory ino num */
	if(IS_INO_EMPTY(ino_num)){
		/* root (inode 1) must be the first character - we get full path*/
		if(!IS_SLASH(*pathname)){
	//		PRINT("\nnamei() -first charis not '/'");
			return -1;
		}
		/* root is the beginning*/
		else{
			ino_num = 1;
		}
	}
	/* verify characters are legal */
	FS_VERIFY(verifyLegalPathname(pathname));
//	PRINT("\nnamei() -verified legal pathname");
//	{
//	int32_t i;
//	for(i=0; i< FS_MAX_VNODES;i++){
//		PRINT_MSG_AND_HEX("\nfentry vnode=", OPEN_FILE_GET_VNODE(i));
//	}
//	}

	/* if cwd is root, we got a full path.
	 * check if there is anything after it.*/
	if(ino_num==1){
		/* if there isn't anything after '/'*/
		if(IS_PATH_END(*pathname)){
			return ino_num;
		}

		/* there is soemthing. skip '/'*/
		pathname++;
	}

//	/* skip all preceding '/' until actual name*/
//		while(IS_SLASH(*pathname)) pathname++;
	/* iterate directories until we find matching dir entry*/
	while(1){
//		PRINT_MSG_AND_STR("\nnamei() - iterating pathname. pathname=", pathname);
		/* verify we will not try reading directory entries from a regular file*/
		FS_VERIFY(IS_DIRECTORY(*f_type));

//		PRINT_MSG_AND_STR("\nnamei() - and after increment pathname=", pathname);
		/* iterate name until next '/' */
		offset = findNextSeperator(pathname);
//		PRINT_MSG_AND_NUM("\nnamei() - next seperator in offset=",offset);

		/* finished. NOTICE - this means we can't handle "//" */
		if(offset==0){
			break;
		}

		/* try finding a matching directory entry in the current directory.
		 * if found, it is stored in ino_num */
//		PRINT("\nnamei() - do compareDirectoryEntries()");
//		PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
		RES_VERIFY(res, compareDirectoryEntries(pathname, offset, &ino_num, f_type, de_offset));
//		PRINT_MSG_AND_NUM("\nnamei() - compareDirectoryEntries finished. ino_num=", ino_num);
		/* advance after next seperator*/
		pathname += offset;

		/* if we've reached the end of the pathname, break*/
		if(IS_PATH_END(*pathname))
			break;

		/*we haven't reached the end yet. skip next '/' and continue to name*/
		pathname++;
	}

	/* if not found return error*/
	if(IS_INO_EMPTY(ino_num)){
//		PRINT("\nnamei() - not found ino_num. return -1");
		return -1;
	}
//	{
//	int32_t i;
//	for(i=0; i< FS_MAX_VNODES;i++){
//		PRINT_MSG_AND_HEX("\nb4 returning from namei() fentry vnode=", OPEN_FILE_GET_VNODE(i));
//	}
//	}
	return ino_num;
}

/**
 * @brief
 * set open file entry with given details
 *
 * @param fd file descriptor
 */
void setFentry(int32_t fd, int32_t flags, int32_t offset, int32_t vnode_idx, int32_t f_type, int32_t ino_num){
	OPEN_FILE_SET_FLAGS(fd, flags);
	OPEN_FILE_SET_OFFSET(fd,offset);
//	PRINT_MSG_AND_NUM("\nsetOpenFentry() - set uid to ", GET_CURRENT_USER());
	OPEN_FILE_SET_UID(fd, GET_CURRENT_USER());
	OPEN_FILE_SET_VNODE(fd, vnode_idx);
	OPEN_FILE_SET_FTYPE(fd, f_type);

	VNODE_SET_INO_NUM(vnode_idx, ino_num);
	VNODE_INCREMENT_NREFS(vnode_idx);
}

/**
 * @brief
 * auxiliary to open and creat.
 * Given an existing file inode, try allocating an open file entry an vnode for the file.
 *
 * Assumptions:
 * 1. ino_num is not empty
 *
 * @param ino_num file inode number
 * @param flags file creation, access modes etc.
 * @param mode permissions to use in case a new file is created.
 * @param F_type file type
 * @return file descriptor if successful, or:
 * FD_EMPTY if an error occurred
 * FS_ERROR_FENTRY_MAX if no free file entries
 * FS_ERROR_VNODE_MAX if no free vnodes (for a file that is not alreday open)
 */
int32_t
setOpenFentry(int32_t ino_num, int32_t flags, uint32_t mode, uint32_t f_type){
	int32_t fd, vnode_idx, offset = 0;
//	PRINT_MSG_AND_NUM("\nsetOpenFentry() - ino_num=",ino_num);
	/* if we have an inode, and O_CREAT, and O_EXCL it's an error*/
	if(IS_CREAT_EXCL(flags)){
		return FD_EMPTY;
	}

	/* can't open a directory for writing */
	if(IS_DIRECTORY(f_type) && IS_WRONLY(flags)){
		return FD_EMPTY;
	}

	/* handle append*/
	if(IS_APPEND(flags)){
		FS_VERIFY(IS_WRONLY(flags));
	}

//	PRINT("\nsetOpenFentry() - passed flags tests");
	/********** handle vnode and file entry *********/
	/* allocate empty open file entry, and matching vnode
	 * (or empty if doesn't exist */
	vnode_idx = getVnodeByInodeNum(ino_num);

//	PRINT_MSG_AND_NUM("\nsetOpenFentry() - found vnode ", vnode_idx);
//	PRINT_MSG_AND_NUM(", is empty?=", IS_VNODE_EMPTY(vnode_idx));
	/* if a vnode already exists*/
	if(!IS_VNODE_EMPTY(vnode_idx)){
		/* check if there is any flags conflict with other file entries*/
		FS_VERIFY(!isFlagsConflict(vnode_idx, flags));
//		PRINT("\nsetOpenFentry() - no flags conflict");
	}
	else{
		vnode_idx = getFreeVnode();
//		PRINT_MSG_AND_NUM("\nsetOpenFentry() - found empty vnode_idx=", vnode_idx);

		/* if we have no free vnodes*/
		if(IS_VNODE_EMPTY(vnode_idx)){
			return FD_EMPTY;
		}
	}

	fd = getFreeOpenFileEntry();
//	PRINT_MSG_AND_NUM("\nsetOpenFentry() - found fd=", fd);
	/* if no open file entry, return error*/
	if(IS_FD_EMPTY(fd)){
		return FS_ERROR_FENTRY_MAX;
	}

	/* we have a vnode, we have open file entry.
	 * fill data, and return fd*/
	setFentry(fd, flags, offset, vnode_idx, f_type, ino_num);
//	OPEN_FILE_SET_FLAGS(fd, flags);
//	OPEN_FILE_SET_OFFSET(fd,offset);
////	PRINT_MSG_AND_NUM("\nsetOpenFentry() - set uid to ", GET_CURRENT_USER());
//	OPEN_FILE_SET_UID(fd, GET_CURRENT_USER());
//	OPEN_FILE_SET_VNODE(fd, vnode_idx);
//	OPEN_FILE_SET_FTYPE(fd, f_type);
//
//	VNODE_SET_INO_NUM(vnode_idx, ino_num);
//	VNODE_INCREMENT_NREFS(vnode_idx);

	/* if we append, change ofset accordingly*/
	if(IS_APPEND(flags)){
		offset = getFileSize(ino_num);
		OPEN_FILE_SET_OFFSET(fd,offset);
	}

	return fd;
}

/**
 * @brief
 * set a new inode in fs_buffer.
 * Write first direntries block if it is a directory inode
 *
 * @param f_id file id
 * @param parent_f_id file id of parent of this file (if it is a directory)
 * @param f_type file type
 * @param tid if of transaction associated with this write
 * @return FS_ERROR_SUCCESS if successful.
 * 		   if an allocAndWriteBlock() error occurs, it's error code is returned
 */
error_t setNewInode(int32_t f_id, int32_t parent_f_id, uint32_t f_type, int32_t tid){
	inode_t *ino_ptr = CAST_TO_INODE(TRANSACTION_GET_INDIRECT_PTR(tid));
	error_t res;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(empty_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	uint8_t *buf = TRANSACTION_GET_INDIRECT_PTR(tid);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(buf);
//	FENT();
//	PRINT_MSG_AND_NUM(" f_id=", f_id);
//	PRINT_MSG_AND_NUM(" parent_f_id=", parent_f_id);
//	PRINT_MSG_AND_NUM(" f_type=", f_type);
//	PRINT_MSG_AND_NUM(" tid=", tid);

	init_logical_address(empty_log_addr);
	init_logical_address(log_addr);

	init_fsbuf(buf);

	/* if we are writing a directory, we should write ./.. directory entries first*/
	if(IS_DIRECTORY(f_type)){
		/* set "." */
		setNewDirentry(dirent_ptr, f_id, f_type, ".");
//		L("set \".\"");
//		PRINT_MSG_AND_HEX("\nsetNewInode() - after setting direntry len=",DIRENT_GET_LEN(dirent_ptr));
//		PRINT_MSG_AND_HEX(" name len=",DIRENT_GET_NAME_LEN(dirent_ptr));
		/* set "..".
		 * advance after previous entry */
//		PRINT_MSG_AND_NUM("\nsetNewInode() - former dirent len=",DIRENT_GET_LEN(dirent_ptr));
//		PRINT_MSG_AND_NUM("\nDIRENT_FLASH_FIELDS_SIZE=",DIRENT_FLASH_FIELDS_SIZE);
//		PRINT_MSG_AND_NUM("\n((uint32_t)(DIRENT_FLASH_FIELDS_SIZE))=",((uint32_t)(DIRENT_FLASH_FIELDS_SIZE)));
//		PRINT_MSG_AND_NUM("\nsetNewInode() - len-DIRENT_FLASH_FIELDS_SIZE=",DIRENT_GET_LEN(dirent_ptr)-DIRENT_FLASH_FIELDS_SIZE);
//		PRINT_MSG_AND_NUM("\nsetNewInode() - len-DIRENT_FLASH_FIELDS_SIZE-1",DIRENT_GET_LEN(dirent_ptr)-DIRENT_FLASH_FIELDS_SIZE-1);
//		PRINT_MSG_AND_NUM("\nsetNewInode() - name len=",DIRENT_GET_NAME_LEN(dirent_ptr));

//		dirent_ptr = CAST_TO_DIRENT(fs_buffer+DIRENT_GET_LEN(dirent_ptr));
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));

		setNewDirentry(dirent_ptr, parent_f_id, f_type, "..");
//		PRINT("\nwriteNewInode() - set \"..\"");

		/* write block*/
//		L("WRITE FIRST BLOCK OF NEW DIRECTORY");
		RES_VERIFY(res, allocAndWriteBlockTid(log_addr, buf, DATA_TYPE_DIRENTS, CACHE_ENTRY_OFFSET_EMPTY, tid));
//		L("wrote first block to addr %x", ADDR_PRINT(log_addr));
		init_fsbuf(buf);
	}
//	PRINT_MSG_AND_NUM("\nsetNewInode() - is root dir first block sparse?=", IS_ADDR_FREE(log_addr));
//
//	PRINT("\nwriteNewInode() - finished directory entries");
	/* set inode fields */
	init_fsbuf(TRANSACTION_GET_INDIRECT_PTR(tid));
	INODE_SET_FILE_ID(ino_ptr, f_id);
	INODE_SET_FILE_TYPE(ino_ptr, f_type);
	INODE_SET_NBLOCKS(ino_ptr, 1);
//	PRINT_MSG_AND_NUM("\nsetNewInode() - NBYTES_LOCATION=",NBYTES_LOCATION);
//	PRINT_MSG_AND_HEX("\nsetNewInode() - b4 setting nbytes=",INODE_GET_NBYTES(ino_ptr));
//	PRINT_MSG_AND_NUM(" setting to=",INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE);
	INODE_SET_NBYTES(ino_ptr,0);

	/* if we're writing a directory, mark address*/
	if(IS_DIRECTORY(f_type)){
//		PRINT_MSG_AND_NUM("\nsetNewInode() - setting first direct entry in directory f_id=",f_id);
//		PRINT_MSG_AND_NUM(" to address=",logicalAddressToPhysical(log_addr));
		MARK_BLOCK_WITH_HOLE(log_addr);
//		PRINT_MSG_AND_NUM("\nis prev addr sparse?=",IS_ADDR_FREE(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
		INODE_SET_DIRECT(ino_ptr, 0, log_addr);
		INODE_SET_NBYTES(ino_ptr,CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
		INODE_SET_NBLOCKS(ino_ptr, 2);
	}

	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * verify directory name, or a prefix of a given filename
 *
 * @param pathname the file full pathname
 * @param temp_pathname buffer for temporary pathname storage
 * @param dir_num pointer to store the prefix file inode
 * @return 0 if successful, -1 otherwise
 */
int32_t
verifyPrefix(uint8_t *pathname, uint8_t *temp_pathname, int32_t *dir_num, uint32_t *f_offset){
	int32_t offset1 = 0, de_offset;
	uint8_t *org_pathname = pathname;
	uint32_t f_type = 0;
//	PRINT("\nverifyPrefix() - starting");
//	PRINT_MSG_AND_STR(", pathname=", pathname);
//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
	/* verify directory tree leading to our file name exists.
	 * find offset until last name with no '/' after it, and verify it's path
	 * and being a directory*/
	*f_offset = 0;

	while(!IS_PATH_END(*pathname)){
		pathname++;

		/* iterate name until next '/' */
		offset1 = findNextSeperator(pathname);
//		PRINT_MSG_AND_NUM("\nverifyPrefix() - iteration, offset1=", offset1);

		if(IS_PATH_END(pathname[offset1])){
//			PRINT("\nverifyPrefix() - pathname at offset1 is string end");
			break;
		}

//		PRINT_MSG_AND_NUM("\nverifyPrefix() - next seperator found at offset ",offset1);
		/* finished. NOTICE - this means we can't handle "//" */
		if(offset1==0){
			/* we have ".../dir_name/" */
			return -1;
		}

		/* advance after next seperator*/
		pathname  += offset1;
		*f_offset += offset1+1;
	}
	if(offset1 !=0){
		*f_offset +=1;
	}
//	PRINT_MSG_AND_NUM("\nverifyPrefix() - *f_offset=", *f_offset);
	/* - copy pathname prefix until parent directory name and verify it
	 * - verify prefix of pathname
	 * - if verified create file and direntrys*/
	fsStrcpy(temp_pathname, org_pathname);
//	PRINT("\nverifyPrefix() - fsStrcpy success");
	temp_pathname[*f_offset] = '\0';
//	PRINT_MSG_AND_STR("\nverifyPrefix() - perform namei on temp_pathname=", temp_pathname);
	*dir_num = namei(temp_pathname, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
//	PRINT_MSG_AND_NUM("\nverifyPrefix() - after namei *dir_num=", *dir_num);
//	PRINT_MSG_AND_NUM(", IS_DIRECTORY(f_type)=", IS_DIRECTORY(f_type));

	/* verify prefix is legal, and of a directory*/
	FS_VERIFY(!IS_NEGATIVE(*dir_num) && IS_DIRECTORY(f_type));
//	PRINT("\nverifyPrefix() - return 0");
	return 0;
}

/**
 * @brief
 * read direct entries block from block_addr,and find the first negative one
 *
 * @param block_addr         - address of direct entries block
 * @param offset             - pointer to save offset in block of entry found
 * @param entry_pointer_size -  total size of file blocks pointed by an entry in the block
 * @param entry_offset       - offset of indirect block (in offset) in it's parent block
 * @param tid 				 - transaction id in which we find an empty direct entry
 * @return 0 if successful, -1 otherwise
 */
int32_t
findEmptyDirectEntry(logical_addr_t block_addr,
							 int32_t *offset,
							 int32_t entry_pointer_size,
							 int32_t entry_offset,
							 int32_t tid){
	int32_t i;
//	PRINT("\nfindEmptyDirectEntry() - starting. ");
//	PRINT_MSG_AND_NUM(" entryPointerSize=", entryPointerSize);
//	PRINT_MSG_AND_NUM(" *offset=", *offset);
//	PRINT_MSG_AND_NUM(" (FS_MAX_FILESIZE=", FS_MAX_FILESIZE);

	/* make sure we did not exceed file size
	 * we can cache anyway since this is not an inode0 read*/
	FS_VERIFY(*offset < FS_MAX_FILESIZE);

	FS_VERIFY(!fsReadBlock(block_addr,
						   fs_buffer,
						   tid,
						   entry_offset,
						   FLAG_CACHEABLE_READ_YES));

	/* iterate direct pointers */
	for(i=0; i <LOG_ADDRESSES_PER_BLOCK; i++){
		/* read direct entry*/
		BLOCK_GET_INDEX(fs_buffer, i, block_addr);

		/* check if it is negative */
		if(IS_ADDR_FREE(block_addr)){
//			PRINT_MSG_AND_NUM("\nadd to offset ", i * entryPointerSize);
			*offset += i * entry_pointer_size;

			L("*offset=%d FS_MAX_FILESIZE=%d", *offset, FS_MAX_FILESIZE);
			/* make sure we did not exceed file size*/
			FS_VERIFY(*offset < FS_MAX_FILESIZE && *offset > 0);

			return 0;
		}
	}

//	PRINT("\nfindEmptyDirectEntry() - return error");
	return -1;
}

/**
 * @brief
 * find an empty block pointer in the inode of file ino_num
 *
 * @param ino_num inode number of file
 * @param block_addr address of the sparse block found (empty if a new block os allocated)
 * @param offset pointer to store the offset in file of the sparse block found
 * @return FS_ERROR_SUCCESS if successful
 *         FS_ERROR_NON_EXISTING_HOLE if there isn't a hole where there should be
 *         -1 otherwise
 */
error_t
findEmptySparseBlock(int32_t ino_num,
					 logical_addr_t block_addr,
					 int32_t *offset,
					 int32_t tid){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	int32_t i;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	int32_t entry_offset;
//	PRINT("\nfindEmptySparseBlock() - starting");
//	PRINT_MSG_AND_NUM(" ino=", ino_num);

	init_logical_address(ino_log_addr);
	*offset = 0;
	/* read inode block*/
	FS_VERIFY(!readInodeBlock(fs_buffer, ino_num, ino_log_addr, TID_EMPTY));
//	PRINT_MSG_AND_NUM("\nfindEmptySparseBlock() - read inode from address=", logicalAddressToPhysical(ino_log_addr));

	*offset += INODE_FILE_DATA_SIZE;
//	PRINT("\niterate direct entries");
	/* iterate direct pointers */
	for(i=0; i <DIRECT_INDEX_ENTRIES; i++){
		/* read direct entry*/
		INODE_GET_DIRECT(ino_ptr, i, block_addr);
//		PRINT_MSG_AND_NUM("\ndirect entry ",i);
//		PRINT_MSG_AND_NUM(" seg=",GET_LOGICAL_SEGMENT(block_addr));
//		PRINT_MSG_AND_NUM(" offset=",GET_LOGICAL_OFFSET(block_addr));
//		PRINT_MSG_AND_NUM(" is with hole=",IS_ADDR_FREE(block_addr));
		/* check if it is negative */
		if(IS_ADDR_FREE(block_addr)){
//			PRINT_MSG_AND_NUM("\n is with hole=",IS_ADDR_FREE(block_addr));
			*offset += i * FS_BLOCK_SIZE;

			return FS_ERROR_SUCCESS;
		}
	}

	*offset += INODE_DIRECT_DATA_SIZE;

	/* read indirect entry */
	INODE_GET_INDIRECT(ino_ptr, block_addr);
//	PRINT_MSG_AND_NUM("\nfindEmptySparseBlock() - read indirect entry from ", logicalAddressToPhysical(block_addr));
//	PRINT_MSG_AND_NUM("\nfindEmptySparseBlock() - is indirect address with hole? ", IS_ADDR_FREE(block_addr));

	/* check if it is free */
	if(IS_ADDR_FREE(block_addr)){
		entry_offset = INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES;
		FS_VERIFY(!findEmptyDirectEntry(block_addr, offset, FS_BLOCK_SIZE, entry_offset,tid));

		return FS_ERROR_SUCCESS;
	}

	*offset += INODE_INDIRECT_DATA_SIZE;

	/* read double indirect entry */
	INODE_GET_DOUBLE(ino_ptr, block_addr);
//	PRINT_MSG_AND_NUM("\nfindEmptySparseBlock() - read double entry from ", logicalAddressToPhysical(block_addr));
//	PRINT_MSG_AND_NUM("\nfindEmptySparseBlock() - is double address with hole? ", IS_ADDR_FREE(block_addr));
//	/* if index is empty, then the first double indirect pointer is negative */
//	if(IS_ADDR_EMPTY(block_addr)){
//		return 0;
//	}

	/* check if it is negative */
	if(IS_ADDR_FREE(block_addr)){
		entry_offset = DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES;
		FS_VERIFY(!findEmptyDirectEntry(block_addr, offset, INODE_INDIRECT_DATA_SIZE, entry_offset, tid));

		/* if found negative indirect index, dig deeper*/
		if(IS_ADDR_FREE(block_addr)){
			entry_offset = CALC_OFFSET_IN_DOUBLE_BLOCK(CALC_OFFSET_FROM_DOUBLE(*offset));
			FS_VERIFY(!findEmptyDirectEntry(block_addr, offset, FS_BLOCK_SIZE, entry_offset, tid));

			return FS_ERROR_SUCCESS;
		}

		return FS_ERROR_NON_EXISTING_HOLE;
	}

	*offset += INODE_DOUBLE_DATA_SIZE;

	/* read triple indirect entry */
	INODE_GET_TRIPLE(ino_ptr, block_addr);
//	PRINT_MSG_AND_NUM("\nfindEmptySparseBlock() - read triple entry from ", logicalAddressToPhysical(block_addr));
//	PRINT_MSG_AND_NUM("\nfindEmptySparseBlock() - is triple address with hole? ", IS_ADDR_FREE(block_addr));
//	PRINT_MSG_AND_NUM(" *offset=", *offset);

	/* check if it is negative */
	if(IS_ADDR_FREE(block_addr)){
		entry_offset = TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES;
		FS_VERIFY(!findEmptyDirectEntry(block_addr, offset, INODE_DOUBLE_DATA_SIZE, entry_offset, tid));
//		PRINT_MSG_AND_NUM("\nfindEmptySparseBlock() - triple index findEmptyDirectEntry found free double entry at offset", *offset);
//		PRINT_MSG_AND_NUM(" *offset=", *offset);
//		PRINT_MSG_AND_NUM("\nfindEmptySparseBlock() - is address with hole? ", IS_ADDR_FREE(block_addr));

		/* if found negative double index, dig deeper*/
		if(IS_ADDR_FREE(block_addr)){
			entry_offset = CALC_OFFSET_IN_TRIPLE_BLOCK(CALC_OFFSET_FROM_TRIPLE(*offset));
			FS_VERIFY(!findEmptyDirectEntry(block_addr, offset, INODE_INDIRECT_DATA_SIZE, entry_offset, tid));
//			PRINT_MSG_AND_NUM("\nafter finding double entry *offset=", *offset);
			/* if found negative indirect index, dig deeper*/
			if(IS_ADDR_FREE(block_addr)){
				entry_offset = CALC_LOG_ADDR_OFFSET_IN_BLOCK(CALC_OFFSET_IN_INDIRECT_SIZE(CALC_OFFSET_FROM_TRIPLE(*offset)));
				FS_VERIFY(!findEmptyDirectEntry(block_addr, offset, FS_BLOCK_SIZE, entry_offset, tid));

				return FS_ERROR_SUCCESS;
			}
		}

		return FS_ERROR_NON_EXISTING_HOLE;
	}

	/* none found */
	return -1;
}

/**
 * @brief
 * verify file ino_num is not open
 *
 * @param ino_num file inode number
 * @return 0 if successful, -1 otherwise
 */
int32_t
verifyFileNotOpen(int32_t ino_num){
	int32_t i;
//	PRINT_MSG_AND_NUM("\nverifyFileNotOpen() - starting. ino_num=", ino_num);

	for(i=0;i<FS_MAX_OPEN_FILES;i++){
//		PRINT_MSG_AND_HEX("\nverifyFileNotOpen() - fentry vnode=", OPEN_FILE_GET_VNODE(i));
//		PRINT_MSG_AND_HEX(" VNODE_EMPTY=", VNODE_EMPTY);
//		PRINT_MSG_AND_NUM("\nverifyFileNotOpen() - fentry ino=", GET_FILE_ID_BY_FD(i));
		if(IS_OPEN_FILE_ENTRY_EMPTY(i)){
			continue;
		}

		FS_VERIFY(GET_FILE_ID_BY_FD(i) != ino_num);
	}

	return FS_ERROR_SUCCESS;
}

void init_distreams(void){
	int32_t i;

	for(i=0; i< FS_MAX_OPEN_DIRESTREAMS; i++){
		init_dirstream(GET_DS_BY_ID(i));
	}
}

/**
 * @brief
 * Auxiliary to nandfs_scandir().
 * read direct entries block from block_addr,and find the first non-empty entry
 * from offset
 *
 * @param block_addr address of direct entries block
 * @param entry_offset initial offset to start search from in entries block
 * @param offset and pointer to save offset in block of entry found
 * @param entryPointerSize total size of file blocks pointed by an entry in the block
 * @param tid transaction id
 * @return 0 if successful, -1 otherwise
 */
int32_t findNonEmptyDirectEntry(logical_addr_t block_addr,
							    int32_t entry_offset,
							    int32_t *offset,
							    int32_t entry_pointer_size,
							    int32_t tid){
	int32_t i;
//	PRINT_MSG_AND_NUM("\nfindNonEmptyDirectEntry() - starting. entry_offset=",entry_offset);
//	PRINT_MSG_AND_NUM(". *offset=",*offset);
//	PRINT_MSG_AND_NUM(". entryPointerSize=",entryPointerSize);
	FS_VERIFY(!fsReadBlock(block_addr,
						   fs_buffer,
						   tid,
						   entry_offset,
						   FLAG_CACHEABLE_READ_YES));

	/* iterate direct pointers */
	for(i=entry_offset; i <LOG_ADDRESSES_PER_BLOCK; i++){
//		PRINT_MSG_AND_NUM("\nread direct entry ", i);
		/* read direct entry*/
		BLOCK_GET_INDEX(fs_buffer, i, block_addr);

		/* check if it is negative */
		if(!IS_ADDR_EMPTY(block_addr)){
			*offset += (i-entry_offset) * entry_pointer_size;

			/* make sure we did not exceed file size*/
			FS_VERIFY(*offset < FS_MAX_FILESIZE);

			return 0;
		}
	}

	return -1;
}

/**
 * @brief
 * Auxiliary to nandfs_scandir().
 * find next non empty block pointer in the inode of file ino_num after offset
 * NOTICE - we may check emptyness of inode entries even though they overflow file size.
 * This poses no conflict since they are empty.
 *
 * Assumptions:
 * 1. file is sparse.
 *
 * @param ino_addr file inode address
 * @param block_addr address of the non empty block found
 * @param offset file offset of last found non empty file block, and pointer to store the
 * @param tid transaction id in which we are calling this function
 * offset in file of the sparse block found
 * @return 0 if successful, -1 otherwise
 */
int32_t findNextNonEmptySparseBlock(logical_addr_t ino_addr,
									logical_addr_t block_addr,
									int32_t *offset,
									int32_t tid){
	int32_t i, block_offset, temp_offset;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
//	PRINT("\nfindNextNonEmptySparseBlock() - starting");

	/* read inode block*/
	FS_VERIFY(!fsReadBlock(ino_addr,
						   fs_buffer,
						   tid,
						   CACHE_ENTRY_OFFSET_EMPTY,
						   FLAG_CACHEABLE_READ_YES));

	/* advance to next block */
	*offset -= CALC_OFFSET_IN_FILE_BLOCK(*offset);
	*offset += FS_BLOCK_SIZE;

	/* find blook in direct entries*/
	if(*offset < INDIRECT_DATA_OFFSET){
		block_offset = CALC_IN_BLOCKS(*offset-INODE_FILE_DATA_SIZE);
//		PRINT("\niterate direct entries");
		/* iterate direct pointers */
		for(i=block_offset; i <DIRECT_INDEX_ENTRIES; i++){
			/* read direct entry*/
			INODE_GET_DIRECT(ino_ptr, i, block_addr);

			/* check if not empty */
			if(!IS_ADDR_EMPTY(block_addr)){
				*offset += (i-block_offset) * FS_BLOCK_SIZE;
				return 0;
			}
		}

		/* if all are empty, continue from INDIRECT_DATA_OFFSET*/
		*offset = INDIRECT_DATA_OFFSET;
	}

	/* read indirect entry */
	if(*offset < DOUBLE_DATA_OFFSET){
//		PRINT("\niterate indirect entries");
		INODE_GET_INDIRECT(ino_ptr, block_addr);

		/* check if empty */
		if(!IS_ADDR_EMPTY(block_addr)){
			temp_offset = CALC_OFFSET_IN_INDIRECT_BLOCK(CALC_OFFSET_FROM_INDIRECT(*offset));
			FS_VERIFY(!findNonEmptyDirectEntry(block_addr, temp_offset, offset, FS_BLOCK_SIZE, tid));

			return 0;
		}

		/* if empty, continue from DOUBLE_DATA_OFFSET*/
		*offset = DOUBLE_DATA_OFFSET;
	}

	/* read double entry */
	if(*offset < TRIPLE_DATA_OFFSET){
//		PRINT("\niterate double entries");
		/* read double indirect entry */
		INODE_GET_DOUBLE(ino_ptr, block_addr);

		temp_offset = CALC_OFFSET_FROM_DOUBLE(*offset);
		/* check if not empty */
		if(!IS_ADDR_EMPTY(block_addr)){
			FS_VERIFY(!findNonEmptyDirectEntry(block_addr,
											   CALC_OFFSET_IN_DOUBLE_BLOCK(temp_offset),
											   offset,
											   INODE_INDIRECT_DATA_SIZE,
											   tid));

			/* if found non-empty indirect index, dig deeper*/
			if(!IS_ADDR_EMPTY(block_addr)){
				temp_offset = CALC_OFFSET_IN_INDIRECT_SIZE(temp_offset);
				FS_VERIFY(!findNonEmptyDirectEntry(block_addr,
												   CALC_OFFSET_IN_INDIRECT_BLOCK(temp_offset),
												   offset,
												   FS_BLOCK_SIZE,
												   tid));
				return 0;
			}

			return -1;
		}

		/* if empty, continue from TRIPLE_DATA_OFFSET*/
		*offset = TRIPLE_DATA_OFFSET;
	}
//	PRINT("\niterate triple entries");
	/* read triple indirect entry */
	INODE_GET_TRIPLE(ino_ptr, block_addr);

	/* check if empty */
	if(!IS_ADDR_EMPTY(block_addr)){
		temp_offset = CALC_OFFSET_FROM_TRIPLE(*offset);
		FS_VERIFY(!findNonEmptyDirectEntry(block_addr,
										   CALC_OFFSET_IN_DOUBLE_SIZE(temp_offset),
										   offset,
										   INODE_DOUBLE_DATA_SIZE,
										   tid));

		/* if found non-empty double index, dig deeper*/
		if(!IS_ADDR_EMPTY(block_addr)){
			temp_offset = CALC_OFFSET_IN_DOUBLE_SIZE(temp_offset);
			FS_VERIFY(!findNonEmptyDirectEntry(block_addr,
											   CALC_OFFSET_IN_INDIRECT_SIZE(temp_offset),
											   offset,
											   INODE_INDIRECT_DATA_SIZE,
											   tid));

			/* if found negative indirect index, dig deeper*/
			if(!IS_ADDR_EMPTY(block_addr)){
				temp_offset = CALC_OFFSET_IN_INDIRECT_SIZE(temp_offset);
				FS_VERIFY(!findNonEmptyDirectEntry(block_addr,
												   CALC_OFFSET_IN_INDIRECT_BLOCK(temp_offset),
												   offset,
												   FS_BLOCK_SIZE,
												   tid));

				return 0;
			}
		}

		return -1;
	}

	/* none found */
	return FS_ERROR_OFST_OVRFLW;
}

/**
 * @brief
 * initialize a directoy stream
 *
 * @param ds_id offset of directory stream
 */
void init_dirstream(NANDFS_DIR* ds){
	if(!IS_FD_EMPTY(DS_GET_FD_BY_PTR(ds))){
		if(VNODE_GET_NREFS(OPEN_FILE_GET_VNODE(DS_GET_FD_BY_PTR(ds))) == 1){
			init_vnode(OPEN_FILE_GET_VNODE(DS_GET_FD_BY_PTR(ds)));
		}
		else{
			VNODE_DECREMENT_NREFS(OPEN_FILE_GET_VNODE(DS_GET_FD_BY_PTR(ds)));
		}
		init_file_entry(DS_GET_FD_BY_PTR(ds));
	}

	init_struct(ds, sizeof(fs_dirstream));
}

/**
 * @brief
 * find an empty transaction to use
 */
int32_t getFreeDirstreamId(void){
	int32_t i;

	for(i=0;i<FS_MAX_OPEN_DIRESTREAMS;i++){
//		PRINT_MSG_AND_HEX("\ngetFreeTransactionId() - ino=", TRANSACTION_GET_INO(i));
		if(IS_DIRSTREAM_EMPTY(i)){
			return i;
		}
	}

	return DS_ID_EMPTY;
}

void setDirstreamDirEntry(NANDFS_DIR *ds, dirent_flash *de_ptr, int32_t offset){
	DS_SET_DIRENTRY_BY_PTR(ds, de_ptr);
	DS_SET_DIR_OFFSET_BY_PTR(ds, offset+DIRENT_GET_LEN(de_ptr));
}

/**
 * Auxiliary to readDirStreamNextEntry();
 * Calc offset of direct entry in it's indirect block, according to offset in file og block
 *
 * @param offset offset in file of block pointed by direct entry
 * @return offset in indirect (or inode) block
 */
static int32_t
calc_direct_entry_offset(int32_t offset){
	/* if this is an inode direct entry*/
	if(offset<INDIRECT_DATA_OFFSET){
		offset -= INODE_FILE_DATA_SIZE;

		return (CALC_IN_LOG_ADDRESSES(offset)+DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES);
	}

	offset -= INDIRECT_DATA_OFFSET;

	offset = CALC_OFFSET_IN_INDIRECT_SIZE(offset);
	return CALC_LOG_ADDR_OFFSET_IN_BLOCK(offset);
}

/**
 * @brief
 * read to directory stream the next directory entry, and prepare it for next
 * read by setting offset after current entry
 *
 * Assumptions;
 * 1. all dirstream attributes are set
 * 2. all open file entry attributes are set
 * 3. dirstream offset is set after last known valid direntry
 *
 * @param ds_idx directory stream index
 * @return 0 if successful, if a read error occurs -1 is returned
 */
int32_t readDirStreamNextEntry(NANDFS_DIR *ds){
	dirent_flash *de_ptr;
	int32_t offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_addr);
	init_logical_address(block_addr);

//	PRINT("\nreadDirStreamNextEntry() - starting");
	/* calculate next entry location in file block */
	offset  = DS_GET_DIR_OFFSET_BY_PTR(ds);

//	PRINT_MSG_AND_NUM(" offset=",offset);

	/* check if there can be another direntry in the block*/
	if(!IS_DIRENT_BLOCK_FULL(offset)){
		/* read directory entry block */
//		PRINT("\nreadDirStreamNextEntry() - about to readFileBlock");
		FS_VERIFY(!readFileBlock(fs_buffer,
				                 GET_FILE_ID_BY_FD(DS_GET_FD_BY_PTR(ds)),
				                 offset,
				                 DS_GET_INO_ADDR_PTR_BY_PTR(ds),
				                 TID_EMPTY));
//		PRINT("\nreadDirStreamNextEntry() - readFileBlock success");
		de_ptr = CAST_TO_DIRENT(&(fs_buffer[CALC_OFFSET_IN_FILE_BLOCK(offset)] ));

//		PRINT_MSG_AND_NUM("\nreadDirStreamNextEntry() - get direntry in block from offset", CALC_OFFSET_IN_FILE_BLOCK(DS_GET_DIR_OFFSET_BY_PTR(ds)));
		/* if directory entry is valid, copy to stream and return*/
		if(!IS_DIRENT_EMPTY(de_ptr)){
			setDirstreamDirEntry(ds, de_ptr, offset);

//			PRINT_MSG_AND_STR("\nreadDirStreamNextEntry() -found entry. name=", DIRENT_GET_NAME(DS_GET_DIRENTRY_PTR_BY_PTR(ds)));
			return FS_ERROR_SUCCESS;
		}
	}

	/* directory entry is empty, keep reading directory until we find next direntry*/
	FS_VERIFY(!findNextNonEmptySparseBlock(DS_GET_INO_ADDR_PTR_BY_PTR(ds), block_addr, &offset, TID_EMPTY));
//	PRINT_MSG_AND_NUM("\nreadDirStreamNextEntry() - after findNextNonEmptySparseBlock offset=", offset);
	FS_VERIFY(!fsReadBlock(block_addr,
						   fs_buffer,
						   TID_EMPTY,
						   calc_direct_entry_offset(offset),
						   FLAG_CACHEABLE_READ_YES));
	de_ptr = CAST_TO_DIRENT(fs_buffer);
	setDirstreamDirEntry(ds, de_ptr, offset);

	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * auxiliary to dirstream functions.
 * veridy a given dirstream ds is valid.
 *
 * @param ds dirstream
 * @return 1 if verified, 0 if not
 */
error_t verifyDirstream(NANDFS_DIR *ds){
	if(!ds || IS_NULL(ds) || ds ==0){
		return 0;
	}
//	PRINT("\nverifyDirstream - ds not null");
	if(IS_DIRSTREAM_EMPTY_BY_PTR(ds)){
		return 0;
	}

	/* verify user is the one that opened the dirstream*/
//	PRINT_MSG_AND_NUM("\ncur user ", GET_CURRENT_USER());
//	PRINT_MSG_AND_NUM(" expected=", OPEN_FILE_GET_UID(DS_GET_FD_BY_PTR(ds)));
	if(!IS_USER_LEGAL(DS_GET_FD_BY_PTR(ds))){
		return 0;
	}

	return 1;
}

/**
 * @brief
 * auxiliart to scandir.
 * free array in case of an error
 *
 * @param namelist array to free
 * @param de_idx direntries count
 * @param freer function to free with
 */
void freeNamesArrary(nandfs_dirent **namelist, int32_t de_idx, void(*freer)(void *ptr)){
	int32_t i;

	for(i=0 ; i<de_idx; ++i){
		freer(namelist[i]); /* free the strings hanging off each pointer */
	}

	freer(namelist); /* lastly, free the block of pointers */
}

/**
 * @brief
 * auxiliary. this is the actual fsync fuction.
 *
 * @param fd the file descriptor.
 * @return On success, zero is returned. If the file descriptor is illegal -1 is returned.
 * If an error occurs when commiting the file to flash -1 is returned
 */
int32_t fsyncUtil(int32_t fd){
	int32_t tid;

	/* verify file descriptor*/
	if(verifyFd(fd)){
		return -1;
	}

	/* verify the file even has any transaction to commit*/
	tid = findTidByFileId(GET_FILE_ID_BY_FD(fd));

	/* if there's no open transaction return*/
	if(IS_EMPTY_TID(tid)){
		return 0;
	}

	/* commit transaction */
	if(commitTransaction(tid, FS_GET_INO0_ADDR_PTR(), 1)){
		return -1;
	}

	return 0;
}

/**
 * create a copy of the file descriptor oldfd
 * makes newfd be the copy of oldfd, closing newfd first if necessary.
 * auxiliary to dup2 and dup (which is actually a private case of dup2)
 *
 * @param oldfd the old file descriptor.
 * @param is_dup2 is this a direct call to dup2, or dup (need to get a free fd)
 * @return the new descriptor, or -1 on failure
 */
int32_t dupUtil(int32_t old_fd, int32_t new_fd, bool_t is_dup2){
	if(!is_dup2){
		/* get new fd*/
		new_fd = getFreeOpenFileEntry();
		FS_VERIFY(!IS_FD_EMPTY(new_fd));
	}

//	PRINT_MSG_AND_NUM("\ndup2() - old_fd=", old_fd);
//	PRINT_MSG_AND_NUM(", new_fd=", new_fd);
//	PRINT_MSG_AND_NUM(", is_dup2=", is_dup2);

	/* verify old fd*/
	FS_VERIFY(!verifyFd(old_fd));
//	PRINT("\ndup2() - old_fd verified");

	/* verify new fd*/
	FS_VERIFY(IS_FD_LEGAL(new_fd));
//	PRINT("\ndup2() - new_fd verified");

	/* verify no flags conflict */
	FS_VERIFY(!isFlagsConflict(OPEN_FILE_GET_VNODE(old_fd), OPEN_FILE_GET_FLAGS(old_fd)));

	/* make empty new_fd*/
	if(!IS_OPEN_FILE_ENTRY_EMPTY(new_fd)){
		/* close file (don't try to lock fs mutex, we already have it...*/
		FS_VERIFY(!closeUtil(new_fd));
	}

//	PRINT("\ndup2() - new_fd closed");
	setFentry(new_fd,
			  OPEN_FILE_GET_FLAGS(old_fd),
			  OPEN_FILE_GET_OFFSET(old_fd),
			  OPEN_FILE_GET_VNODE(old_fd),
			  OPEN_FILE_GET_FTYPE(old_fd),
			  GET_FILE_ID_BY_FD(old_fd));

//	PRINT_MSG_AND_NUM("\ndup2() - return new_fd=", new_fd);

	return new_fd;
}

/**
 * auxiliary to close() and dup2(). closes a file descriptor, so that it no longer refers to any file
 * and may be reused.
 * if the file was open for writing, close commits it's transaction.
 * if we are called from dup2(), no need to re-lock dup2()
 *
 * @param fd the file descriptor.
 * @return FS_ERROR_SUCCESS on success. On error, -1 is returned
 */
int32_t closeUtil(int32_t fd){
	int32_t tid;

//	PRINT_MSG_AND_NUM("\nclose() - starting. fd=",fd);
	FS_VERIFY(!verifyFd(fd));

//	PRINT_MSG_AND_NUM("\nclose() - is file open for writing?=",IS_WRONLY(OPEN_FILE_GET_FLAGS(fd)));
	/* commit transaction if open for writing*/
	if(IS_WRONLY(OPEN_FILE_GET_FLAGS(fd))){
		tid = findTidByFileId(GET_FILE_ID_BY_FD(fd));
//		PRINT_MSG_AND_NUM("\nfound tid=",tid);
		if(!IS_EMPTY_TID(tid)){
			/* commit transaction */
//			PRINT_MSG_AND_NUM("\nclose() - commit transaction ", tid);
			FS_VERIFY(!commitTransaction(tid, FS_GET_INO0_ADDR_PTR(), 1));
		}
	}
//	PRINT("\nclose() - init vnode and fentry");
	VNODE_DECREMENT_NREFS(OPEN_FILE_GET_VNODE(fd));

	/* if current file entry was the last one refering to it's vnode, init vnode too*/
	if(VNODE_GET_NREFS(OPEN_FILE_GET_VNODE(fd)) == 0){
		init_vnode(OPEN_FILE_GET_VNODE(fd));
	}

	init_file_entry(fd);
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * Bottommost level read call of file system.
 * read block of a file system. if we're given an empty address
 * fill buf with empty data.
 * If the read is not cacheable, request directly from sequencing layer
 * otherwise from cache layer
 *
 * @param log_addr address to read data from
 * @param buf fs buffer to read data to
 * @param tid transaction id in which we are reading this block
 * @param is_cacheable indicator whether this read is cacheable
 * @return FS_ERROR_SUCCESS if successful,
 * 		   FS_ERROR_IO in case of a read error
 */
int32_t
fsReadBlock(logical_addr_t log_addr,
			uint8_t *buf,
			int32_t tid,
			int32_t parent_offset,
			bool_t is_cacheable){
	int32_t res;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(cached_addr);
	init_logical_address(cached_addr);
//	FENT();

	/* if lru is size-0, then nothing is cacheable*/
	if(FS_CACHE_BUFFERS == 0){
		is_cacheable = FLAG_CACHEABLE_READ_NO;
	}

	/* reading an empty address means we read a sparse file part
	 * and results in an empty page*/
//	L("is addr empty?", IS_ADDRESS_EMPTY(log_addr));
	if(IS_ADDRESS_EMPTY(log_addr)){
//		L("reading from empty logical address. init buf");
		init_fsbuf(buf);
		return FS_ERROR_SUCCESS;
	}

	/* check if read call is cacheable:
	 * - useful in debugging mode*/
	/* - if cache lru is empty (size 0) don't cache */
	if(!is_cacheable){
		/* sanity check*/
//		L("not cacheable. read from ", ADDR_PRINT(log_addr));
		if(IS_CACHED_ADDR(log_addr)){
//			L("ERROR, reading uncacheable cached addr %x", ADDR_PRINT(log_addr));
			return FS_ERROR_READING_UNCACHEABLE;
		}

		if(readBlock(log_addr, buf)){
			L("read error");
			return FS_ERROR_IO;
		}

		return FS_ERROR_SUCCESS;
	}

	/* read is cacheable*/
	copyLogicalAddress(cached_addr, log_addr);
//	L("read call is cacheable. call cache_access_cache() on %x", ADDR_PRINT(cached_addr));
	res = cache_access_cache(buf,
							 cached_addr,
						     parent_offset,
						     tid,
						     DATA_TYPE_EMPTY, /* for reads it doesn't really matter what type we want to read*/
						     CACHE_ACCESS_READ);

//	L("finished");
	return res;
}
