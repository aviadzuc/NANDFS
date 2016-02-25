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

/** @file testFsUtils.c  */
#include <system.h>
#include <src/fs/fsUtils.h>
#include <src/fs/fs.h>
#include <src/fs/cache.h>
#include <src/sequencing/sequencing.h>
#include <test/fs/testsHeader.h>
#include <test/fs/testFsUtils.h>

int32_t
verify_no_dirty_caches(){
	int32_t i;

	/* verify no dirty cache entries left*/
	VERIFY(COMPARE(CACHE_GET_DIRTY_COUNT(), 0));
	for(i=0; i< FS_CACHE_BUFFERS;i++){
		VERIFY(!CACHE_IS_DIRTY(i));
		VERIFY(IS_EMPTY_TID(CACHE_GET_TID(i)));
	}

	return 1;
}

/**
 * Verify given cache details
 *
 */
int32_t
verify_cache(int32_t cid, /* cid*/
			 int32_t exp_is_voted,
			 int32_t exp_ref_count,
			 int32_t exp_parent_cid,
			 int32_t exp_parent_offset,
			 int32_t exp_prev_in_lru,
			 int32_t exp_next_in_lru,
			 int32_t exp_tid,
			 int32_t exp_is_dirty,
			 logical_addr_t exp_log_addr,
			 uint8_t *exp_buf,
			 int32_t exp_active_indirect_leaf){

//	L("exp_is_voted %d, CACHE_GET_IS_VOTED(cid) %d", exp_is_voted, CACHE_GET_IS_VOTED(cid));
	VERIFY(COMPARE(exp_is_voted, CACHE_GET_IS_VOTED(cid)));
	VERIFY(COMPARE(exp_ref_count, CACHE_GET_REF_COUNT(cid)));
//	L("exp_parent_cid %d CACHE_GET_PARENT_CACHE_ID(cid) %d", exp_parent_cid, CACHE_GET_PARENT_CACHE_ID(cid));
	VERIFY(COMPARE(exp_parent_cid, CACHE_GET_PARENT_CACHE_ID(cid)));
//	L("exp_parent_offset %d CACHE_GET_PARENT_OFFSET(cid) %d", exp_parent_offset, CACHE_GET_PARENT_OFFSET(cid));
	VERIFY(COMPARE(exp_parent_offset, CACHE_GET_PARENT_OFFSET(cid)));
//	L("exp_prev_in_lru %d CACHE_GET_LESS_RECENTLY_USED(cid) %d", exp_prev_in_lru, CACHE_GET_LESS_RECENTLY_USED(cid));
	VERIFY(COMPARE(exp_prev_in_lru, CACHE_GET_LESS_RECENTLY_USED(cid)));
//	L("exp_next_in_lru %d actual one %d", exp_next_in_lru, CACHE_GET_MORE_RECENTLY_USED(cid));
	VERIFY(COMPARE(exp_next_in_lru, CACHE_GET_MORE_RECENTLY_USED(cid)));
	VERIFY(COMPARE(exp_tid, CACHE_GET_TID(cid)));
	VERIFY(COMPARE(exp_is_dirty, CACHE_GET_IS_DIRTY(cid)));
//	L("exp_log_addr %x CACHE_GET_LOG_ADDR_PTR(cid) %x", *((uint32_t*)exp_log_addr), *((uint32_t*)CACHE_GET_LOG_ADDR_PTR(cid)));
	VERIFY(COMPARE_ADDR(exp_log_addr, CACHE_GET_LOG_ADDR_PTR(cid)));

	VERIFY(compare_bufs(exp_buf, CACHE_GET_BUF_PTR(cid), FS_BLOCK_SIZE));
	VERIFY(COMPARE(exp_active_indirect_leaf, CACHE_GET_IS_INDIRECT_LEAF(cid)));

	return 1;
}

error_t
verifyFsBlock(uint8_t *buf, uint8_t byte){
	int32_t i;

	for(i=0; i<FS_BLOCK_SIZE;i++){
		if(!COMPARE(byte, buf[i])){
			L("i=%d, expected %x, got %x ", i,  byte, buf[i]);
			VERIFY(COMPARE(byte, buf[i]));
		}
	}

	return 1;
}

/**
 * @brief
 * verify that a given file entry has given values
 * @param fd open file entry index (file descriptor)
 * @return 1 is successful, 0 otherwise
 */
error_t
verifyOpenFileEntry(uint32_t fd, uint32_t flags, uint32_t offset, user_id uid, uint32_t vnode){
//	PRINT_MSG_AND_NUM("\nverifyOpenFileEntry() - fd=",fd);
//	PRINT_MSG_AND_HEX(" expected flags=",flags);
//	PRINT_MSG_AND_NUM(" offset=",offset);
//	PRINT_MSG_AND_NUM(" uid=",uid);
//	PRINT_MSG_AND_NUM(" vnode=",vnode);
//
//	PRINT("\nactual values - ");
//	PRINT_MSG_AND_HEX(" flags=",OPEN_FILE_GET_FLAGS(fd));
//	PRINT_MSG_AND_NUM(" offset=",OPEN_FILE_GET_OFFSET(fd));
//	PRINT_MSG_AND_NUM(" uid=",OPEN_FILE_GET_UID(fd));
//	PRINT_MSG_AND_NUM(" vnode=",OPEN_FILE_GET_VNODE(fd));

	VERIFY(COMPARE(OPEN_FILE_GET_FLAGS(fd),flags));
	VERIFY(COMPARE(OPEN_FILE_GET_OFFSET(fd),offset));
	VERIFY(COMPARE(OPEN_FILE_GET_UID(fd),uid));
	VERIFY(COMPARE(OPEN_FILE_GET_VNODE(fd),vnode));

	return 1;
}

/**
 * @brief
 * verify that a given file entry s empty
 * @param fd open file entry index (file descriptor)
 * @return 1 is successful, 0 otherwise
 */
#define verifyOpenFileEntryEmpty(FD) verifyOpenFileEntry(FD,0,0,0,VNODE_EMPTY)

/**
 * @brief
 * verify that a given vnode entry s empty
 * @param vnode_idx vnode index
 * @return 1 is successful, 0 otherwise
 */
error_t verifyVnode(uint32_t vnode_idx, uint32_t ino_num, uint32_t nrefs){
	VERIFY(COMPARE(VNODE_GET_INO_NUM(vnode_idx), ino_num));
	VERIFY(COMPARE(VNODE_GET_NREFS(vnode_idx),nrefs));

	return 1;
}

#define verifyVnodeEmpty(VNODE_IDX) verifyVnode(VNODE_IDX,INO_NUM_EMPTY,0)

/**
 * @brief
 * auxiliary. verify inode details.
 *
 * @return 1 if successful, 0 otherwise
 */
error_t verifyInode(inode_t *ino_ptr, int32_t f_id, int32_t f_type, int32_t nblocks, int32_t nbytes){
//	PRINT_MSG_AND_NUM("\nfile id=",INODE_GET_FILE_ID(ino_ptr));
	VERIFY(COMPARE(INODE_GET_FILE_ID(ino_ptr), f_id));
//	PRINT_MSG_AND_NUM("\nfile type=",INODE_GET_FILE_TYPE(ino_ptr));
	VERIFY(COMPARE(INODE_GET_FILE_TYPE(ino_ptr), f_type));
//	PRINT_MSG_AND_NUM("\nexpected nblocks=",nblocks);
//	PRINT_MSG_AND_NUM("\nnblocks=",INODE_GET_NBLOCKS(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBLOCKS(ino_ptr), nblocks));
//	PRINT_MSG_AND_NUM("\nexpected nbytes=",nbytes);
//	PRINT_MSG_AND_NUM("\nnbytes=",INODE_GET_NBYTES(ino_ptr));
	VERIFY(COMPARE(INODE_GET_NBYTES(ino_ptr), nbytes));

	return 1;
}

/**
 * @brief
 *
 * auxiliary. verify file is empty after first direct entry.
 * @param ino_ptr pointer to file inode
 * @return 1 if successful, 0 otherwise
 */
error_t verifyFileSuffixlyEmpty(inode_t *ino_ptr){
	int32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);

	for(i=1; i<DIRECT_INDEX_ENTRIES; i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	INODE_GET_INDIRECT(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));
	INODE_GET_TRIPLE(ino_ptr, log_addr);
	VERIFY(IS_ADDR_EMPTY(log_addr));

	return 1;
}

/**
 * @brief
 * allocate a new inode on flash and return it's address
 * @param f_id file id
 * @param parent_f_id file id of parent of this file (if it is a directory)
 * @param f_type file type
 * @param log_addr address to which the inode will be written to
 * @return 0 if successful. if an allocAndWriteBlock() error occurs, it's error code is returned
 */
error_t writeNewInode(int32_t f_id, int32_t parent_f_id, uint32_t f_type, logical_addr_t log_addr){
	bool_t cpWritten;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	error_t res;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);

	init_logical_address(prev_log_addr);
	init_fsbuf(fs_buffer);
//	PRINT("\nwriteNewInode() - starting");
	/* if we are writing a directory, we should write ./.. directory entries first*/
	if(IS_DIRECTORY(f_type)){
		/* set "." */
//		PRINT_MSG_AND_NUM("\ndirent_ptr=",dirent_ptr);
		setNewDirentry(dirent_ptr, f_id, f_type, ".");
//		PRINT("\nwriteNewInode() - set \".\"");
//		PRINT_MSG_AND_HEX("\nwriteNewInode() - after setting direntry len=",DIRENT_GET_LEN(dirent_ptr));
//		PRINT_MSG_AND_HEX(" name len=",DIRENT_GET_NAME_LEN(dirent_ptr));
		/* set "..".
		 * advance after previous entry */
//		PRINT_MSG_AND_NUM("\nnwriteNewInode() - former dirent len=",DIRENT_GET_LEN(dirent_ptr));
//		PRINT_MSG_AND_NUM("\nDIRENT_FLASH_FIELDS_SIZE=",DIRENT_FLASH_FIELDS_SIZE);
//		PRINT_MSG_AND_NUM("\n((uint32_t)(DIRENT_FLASH_FIELDS_SIZE))=",((uint32_t)(DIRENT_FLASH_FIELDS_SIZE)));
//		PRINT_MSG_AND_NUM("\nnwriteNewInode() - len-DIRENT_FLASH_FIELDS_SIZE=",DIRENT_GET_LEN(dirent_ptr)-DIRENT_FLASH_FIELDS_SIZE);
//		PRINT_MSG_AND_NUM("\nnwriteNewInode() - len-DIRENT_FLASH_FIELDS_SIZE-1",DIRENT_GET_LEN(dirent_ptr)-DIRENT_FLASH_FIELDS_SIZE-1);
//		PRINT_MSG_AND_NUM("\nnwriteNewInode() - name len=",DIRENT_GET_NAME_LEN(dirent_ptr));

//		{
//		uint32_t i;
//		for(i=0; i < 40; i++){
//			PRINT_MSG_AND_NUM("\n",i );
//			PRINT_MSG_AND_NUM(". ", fs_buffer[i]);
//		}
//		assert(0);
//		}
//		dirent_ptr = CAST_TO_DIRENT(fs_buffer+DIRENT_GET_LEN(dirent_ptr));
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
//		PRINT_MSG_AND_NUM("\ndirent_ptr=",dirent_ptr);

		setNewDirentry(dirent_ptr, parent_f_id, f_type, "..");
//		PRINT("\nwriteNewInode() - set \"..\"");

		/* write block*/
		res = allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0);

//		PRINT_MSG_AND_NUM("\nwriteNewInode() - allocAndWriteBlock result=",res);
		if(res)
			return res;
//		PRINT("\nwriteNewInode() - finished first allocAndWriteBlock()");
		/* mark it as containing space*/
		MARK_ADDR_NEGATIVE(log_addr);
	}
//
//	PRINT("\nwriteNewInode() - finished directory entries");
	/* set inode fields */
	init_fsbuf(fs_buffer);
	INODE_SET_FILE_ID(ino_ptr, f_id);
	INODE_SET_FILE_TYPE(ino_ptr, f_type);
	INODE_SET_NBLOCKS(ino_ptr, 1);
	INODE_SET_NBYTES(ino_ptr,0);

	/* if we're writing a directory, mark address*/
	if(IS_DIRECTORY(f_type)){
//		PRINT_MSG_AND_NUM("\nwriteNewInode() - setting first direct entry in directory f_id=",f_id);
//		PRINT_MSG_AND_NUM(" to address=",logicalAddressToPhysical(log_addr));
		SET_ADDR_FLAG(log_addr, ADDR_FLAG_FREE);
		INODE_SET_DIRECT(ino_ptr, 0, log_addr);
		INODE_SET_NBYTES(ino_ptr,CAST_VAL_TO_UINT32(INODE_FILE_DATA_SIZE+FS_BLOCK_SIZE));
		INODE_SET_NBLOCKS(ino_ptr, 2);
	}
//	PRINT("\nwriteNewInode() - set inode details. about to continue allocAndWriteBlock");
	init_logical_address(log_addr);
	return allocAndWriteBlock(log_addr, fs_buffer, 0, prev_log_addr, &cpWritten, fsCheckpointWriter, 0);
}

/**
 * @brief
 * auxiliary. verify vot entry is as expected in a vot buffer of transaction tid
 * @return 1 is verified, 0 otherwise
 */
error_t verifyVotEntry(int32_t tid, int32_t entry_offset, logical_addr_t expected_addr){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);

	BLOCK_GET_INDEX(TRANSACTION_GET_VOTS_BUF_PTR(tid),entry_offset,log_addr);
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(log_addr), GET_LOGICAL_OFFSET(expected_addr)));
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(log_addr), GET_LOGICAL_SEGMENT(expected_addr)));

	return 1;
}

error_t verifyFentriesVnodesEmpty(int32_t fd){
	int32_t i;

	/* verify file entries and vnodes */
	for(i=0; i< FS_MAX_OPEN_FILES;i++){
		if(i==fd) continue;
//		PRINT_MSG_AND_NUM("\nverify fentry ", i);
		VERIFY(verifyOpenFileEntryEmpty(i));
	}
//	PRINT("\nopen files entries verified");
	for(i=0; i<FS_MAX_VNODES;i++){
//		PRINT_MSG_AND_NUM("\nverify vnode ", i);
		if(i==OPEN_FILE_GET_VNODE(fd)) continue;
		VERIFY(verifyVnodeEmpty(i));
	}

	return 1;
}

error_t verifyIndirectBlock(uint8_t *indirect_buf, uint8_t byte, int32_t entry_offset){
	uint8_t buf[FS_BLOCK_SIZE];
	int32_t j, k;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);

	for(j=0; j<entry_offset;j++){
		BLOCK_GET_INDEX(indirect_buf, j, log_addr);
//		L("read address %x", ADDR_PRINT(log_addr));
		if(!IS_CACHED_ADDR(log_addr)){
//			L("address is real");
			VERIFY(!fsReadBlockSimple(log_addr, buf));
		}
		else{
//			L("address is cached");
			fsReadCache(log_addr, buf);
		}

		for(k=0; k< FS_BLOCK_SIZE; k++){
			VERIFY(COMPARE(buf[k], byte));
		}
	}

	/* verify other addresses in indirect block are empty*/
	for(; j< LOG_ADDRESSES_PER_BLOCK; j++){
		BLOCK_GET_INDEX(indirect_buf, j, log_addr);
		VERIFY(IS_ADDR_EMPTY(log_addr));
	}

	return 1;
}

error_t verifyTransactionData(int32_t tid, int32_t offset, int32_t nblocks, int32_t maxWrittenOffset){
	/* verify data offset, and blocks written dor this indirect block*/
//	PRINT_MSG_AND_NUM("\ntransaction offset=",TRANSACTION_GET_FILE_OFFSET(tid));
	VERIFY(COMPARE(TRANSACTION_GET_FILE_OFFSET(tid), offset));
//	PRINT_MSG_AND_NUM("\ntransaction blocks=",TRANSACTION_GET_BLOCKS_ALLOCS(tid));
	VERIFY(COMPARE(TRANSACTION_GET_BLOCKS_ALLOCS(tid), nblocks));
//	PRINT_MSG_AND_NUM("\ntransaction offset=",TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid));
	VERIFY(COMPARE(TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid), maxWrittenOffset));
	return 1;
}

/**
 * @brief
 * verify all transactions are empty (except a specified transaction)
 * @return 1 if successful, 0 otherwise
 */
error_t verifyTransactionsEmpty(int32_t tid){
	int32_t i;
//	PRINT("\nverifyTransactionsEmpty() - starting");
	for(i=0; i<FS_MAX_N_TRANSACTIONS; i++){
//		L("tid %d", i);
//		PRINT_MSG_AND_NUM("\nverifyTransactionsEmpty() - i=", i);
		if(i==tid){
			continue;
		}

		VERIFY(verifyTransactionData(i, -1, 0, 0));
//		PRINT("\nverifyTransactionsEmpty() - verified data. verify buffers");
		/* verify no buffers related to this transaction*/
		VERIFY(verifyFsBlock(TRANSACTION_GET_DATA_BUF_PTR(i), 0xff));
		VERIFY(verifyFsBlock(TRANSACTION_GET_INDIRECT_PTR(i), 0xff));
		VERIFY(verifyFsBlock(TRANSACTION_GET_VOTS_BUF_PTR(i), 0xff));
	}

	return 1;
}

/**
 *@brief
 * verify directory entry contains expected data
 *
 * @return 1 if successful, 1 otherwise
 */
error_t verifyDirenty(dirent_flash *dirent_ptr, int32_t ino_num, int32_t d_type, int32_t d_len, uint8_t *d_name){
//	L("dirent ino num=%d, expected=%d",DIRENT_GET_INO_NUM(dirent_ptr), ino_num);
	VERIFY(COMPARE(DIRENT_GET_INO_NUM(dirent_ptr), ino_num));
//	PRINT_MSG_AND_NUM("\ndirent type=",DIRENT_GET_TYPE(dirent_ptr));
	VERIFY(COMPARE(DIRENT_GET_TYPE(dirent_ptr), d_type));
//	PRINT_MSG_AND_NUM("\ndirent len=",DIRENT_GET_NAME_LEN(dirent_ptr));
	VERIFY(COMPARE(DIRENT_GET_NAME_LEN(dirent_ptr), d_len));
//	PRINT_MSG_AND_STR("\ndirent name=",DIRENT_GET_NAME(dirent_ptr));
	VERIFY(!fsStrcmp(DIRENT_GET_NAME(dirent_ptr), d_name));

	return 1;
}

error_t verifyStat(file_stat_t *stat_p, int32_t ino_num, int32_t dev_id, int32_t blk_size, int32_t nbytes, int32_t nblocks){
//	PRINT_MSG_AND_NUM("\nexpected ino_num=", ino_num);
//	PRINT_MSG_AND_NUM(" dev_id=", dev_id);
//	PRINT_MSG_AND_NUM(" blk_size=", blk_size);
//	PRINT_MSG_AND_NUM(" nbytes=", nbytes);
//	PRINT_MSG_AND_NUM(" nblocks=", nblocks);
//
//	PRINT_MSG_AND_NUM("\nactual   ino_num=", STAT_GET_INO_NUM(stat_p));
//	PRINT_MSG_AND_NUM(" dev_id=", STAT_GET_DEV_ID(stat_p));
//	PRINT_MSG_AND_NUM(" blk_size=", STAT_GET_BLK_SIZE(stat_p));
//	PRINT_MSG_AND_NUM(" nbytes=", STAT_GET_SIZE(stat_p));
//	PRINT_MSG_AND_NUM(" nblocks=", STAT_GET_BLOCKS(stat_p));

	VERIFY(COMPARE(STAT_GET_INO_NUM(stat_p), ino_num));
	VERIFY(COMPARE(STAT_GET_DEV_ID(stat_p), dev_id));
	VERIFY(COMPARE(STAT_GET_BLK_SIZE(stat_p), blk_size));
	VERIFY(COMPARE(STAT_GET_SIZE(stat_p), nbytes));
	VERIFY(COMPARE(STAT_GET_BLOCKS(stat_p), nblocks));

	return 1;
}
