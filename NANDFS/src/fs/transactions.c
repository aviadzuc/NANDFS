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

/** @file transactions.c
 * Transaction functions, used for atomicity of file system functions
 */

#include <utils/string_lib.h>
#include <utils/memlib.h>
#include <src/fs/transactions.h>
#include <src/sequencing/sequencingUtils.h>
#include <src/sequencing/sequencing.h>
#include <src/fs/fs.h>
#include <src/fs/fsUtils.h>
#include <src/fs/cache.h>
#ifdef Debug
	#include <test/fs/testsHeader.h>
#endif
extern transaction_t transactions[FS_MAX_N_TRANSACTIONS];
extern obs_pages_per_seg_counters *obs_counters_map_ptr;
extern uint8_t fs_buffer[FS_BLOCK_SIZE];
extern fs_t fs_ptr;
extern cache_lru_q lru_ptr;

/**
 * @brief
 * find a transaction that handles file with ino_num file id
 *
 * @param ino_num file inode number
 * @return transaction id if successful, TID_EMPTY if no matching transaction found
 */
int32_t findTidByFileId(int32_t ino_num){
	int32_t i;

	for(i=0;i<FS_MAX_N_TRANSACTIONS;i++){
//		PRINT_MSG_AND_HEX("\ngetFreeTransactionId() - ino=", TRANSACTION_GET_INO(i));
		if(TRANSACTION_GET_INO(i) == ino_num){
			return i;
		}
	}

	return TID_EMPTY;
}

/**
 * @brief
 * get transaction writing to file identified by ino_num.
 * if none found, allocate a new one
 * @param ino_num file id
 * @return transaction id if successful, TID_EMPTY if none found
 */
int32_t getTidByFileId(int32_t ino_num){
	int32_t tid;

	tid = findTidByFileId(ino_num);
	if(!IS_EMPTY_TID(tid)){
		return tid;
	}

	/* no transaction exists, allocate one*/
	tid = getFreeTransactionId();

	if(IS_EMPTY_TID(tid)){
		return TID_EMPTY;
	}

	/* set various transaction fields.
	 * NOTICE - first we get inode address, so as not to have mixup with reading data
	 * when we have a transaction related to this inode number*/
//	PRINT("\nwrite() - get inode address");
	FS_VERIFY(!getInode(fs_buffer, ino_num, TRANSACTION_GET_INO_ADDR_PTR(tid)));
//	L("set ino num %d", ino_num);
	TRANSACTION_SET_INO(tid, ino_num);

	return tid;
}

/**
 * @brief
 * find an empty transaction to use.
 * when an empty transaction is found, allocate buffers for it, and return it's id.
 * NOTICE - a transaction is empty if it's inode num field is INO_NUM_EMPTY
 */
int32_t getFreeTransactionId(void){
	int32_t i;
	for(i=0;i<FS_MAX_N_TRANSACTIONS;i++){
		/* if we have a free transaction, then we have enough free cache buffers.
		 * allocate them for this transaction.
		 * NOTICE - this means that write cache has priority over read cache*/
//		PRINT_MSG_AND_HEX("\ngetFreeTransactionId() - ino=", TRANSACTION_GET_INO(i));
		if(IS_TRANSACTION_EMPTY(i)){
////			PRINT("\ngetFreeTransactionId() - transaction is empty. set buffers");
////			PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
//			TRANSACTION_SET_DATA_BUF_PTR(i, getFreeCacheTid(i));
////			PRINT("\ngetFreeTransactionId() - set 1st buffer");
////			PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
//			TRANSACTION_SET_VOTS_BUF_PTR(i, getFreeCacheTid(i));
////			PRINT("\ngetFreeTransactionId() - set 2nd buffer");
////			PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
//			TRANSACTION_SET_INDIRECT_PTR(i, getFreeCacheTid(i));
////			PRINT("\ngetFreeTransactionId() - set buffers done");
////			PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
////			PRINT_MSG_AND_NUM(". return ", i)
			return i;
		}
	}

	return TID_EMPTY;
}


/**
 * @brief
 * initialize transaction with tid
 * @param tid transaction id
 */
void
init_transaction(int32_t tid){
//	FENT();
	TRANSACTION_SET_TYPE(tid, T_TYPE_EMPTY);
	TRANSACTION_SET_FTYPE(tid, FTYPE_EMPTY);
	init_logical_address(TRANSACTION_GET_PREV_ADDR_PTR(tid));
	init_logical_address(TRANSACTION_GET_INO_ADDR_PTR(tid));
	TRANSACTION_SET_FILE_SIZE(tid, -1);
	TRANSACTION_SET_FILE_OFFSET(tid, -1);
	TRANSACTION_SET_VOTS_COUNT(tid,0);
	TRANSACTION_SET_VOTS_OFFSET(tid,0);
	TRANSACTION_SET_MAX_WRITTEN_OFFSET(tid,0);
	TRANSACTION_SET_BLOCKS_ALLOCS(tid,0);
	TRANSACTION_SET_INO(tid, INO_NUM_EMPTY);
	TRANSACTION_SET_WAS_COMMITED(tid, 0);
	TRANSACTION_SET_IS_COMMITING(tid, 0);

	/* init buffers*/
	init_fsbuf(TRANSACTION_GET_VOTS_BUF_PTR(tid));
	init_fsbuf(TRANSACTION_GET_DATA_BUF_PTR(tid));
	init_fsbuf(TRANSACTION_GET_INDIRECT_PTR(tid));
}

/**
 * @brief
 * init all transactions
 */
void init_transactions(void){
	int32_t i;

	for(i=0; i< FS_MAX_N_TRANSACTIONS;i++){
		init_transaction(i);
	}
}

/**
 * Mark a page as obsolete, as part of a transaction.
 * This mean also releasing it's cache.
 *
 * @param log_addr logical address to mark as obsolete
 * @param is_from_reboot is called as part of a reboot
 * @return FS_ERROR_CRITICAL if a critical error occurred
 * 		   FS_ERROR_SUCCESS if successful
 * 		   FS_ERROR_IO if a program error occurred
 */
static error_t
tr_mark_as_obsolete(logical_addr_t log_addr, bool_t is_from_reboot){
	if(markAsObsolete(log_addr, is_from_reboot)){
//		L("ERROR marking log_addr as obsolete (seg %d, offset %d)", GET_LOGICAL_SEGMENT(log_addr), GET_LOGICAL_OFFSET(log_addr));
		return FS_ERROR_IO;
	}

	if(!IS_FS_SUCCESS(cache_free_by_real_addr(log_addr))){
		L("ERROR cache_free_by_real_addr on log_addr (seg %d, offset %d)", GET_LOGICAL_SEGMENT(log_addr), GET_LOGICAL_OFFSET(log_addr));
		return FS_ERROR_CRITICAL;
	}

	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * auxiliary.
 * delete all transaction pages, starting from last written page
 *
 * @param log_addr last written logical address in this transaction
 * @param tid transaction id
 * @param is_from_reboot is function called as part of a reboot
 * @return 0 if successful, -1 if a read or obsolete write error occured
 */
int32_t
deleteTransactionPages(logical_addr_t log_addr, int32_t tid, bool_t is_from_reboot){
	bool_t isVOTs;
//	FENT();

	/* clear all tid-related cache buffers
	 * NOTICE - we must do this before marking previously written as obsolete
	 *          since some of them may now be cached (dirty)*/
//	L("init cache related to tid %d", tid);
//	L("init tid related");
//	L("b4 cache_init_tid_related free count is %d", FS_TOTAL_FREE_PAGES);
	cache_init_tid_related(tid);
//	L("after cache_init_tid_related free count is %d", FS_TOTAL_FREE_PAGES);
//	PRINT("\ndeleteTransactionPages() - starting");
//	PRINT_MSG_AND_HEX("\nlog_addr=", *CAST_TO_UINT32(log_addr));
//	L("b4 marking tid writes as obsolete free count is %d. prev log_addr is %x", FS_TOTAL_FREE_PAGES, ADDR_PRINT(log_addr));
	while(!IS_ADDR_EMPTY(log_addr)){
		/* mark prev address as obsolete*/
//		L("markAsObsolete addr %x. b4 that obs count is %d", ADDR_PRINT(log_addr), GET_OBS_COUNT());
		FS_VERIFY(IS_FS_SUCCESS(tr_mark_as_obsolete(log_addr, is_from_reboot)));
//		L("after that obs count is %d", GET_OBS_COUNT());

		/* read previous*/
		FS_VERIFY(!readVOTsAndPrev(log_addr, fs_buffer, log_addr, &isVOTs));
//		PRINT_MSG_AND_HEX("\nprev=", *CAST_TO_UINT32(log_addr));
	}
//	L("after marking tid writes as obsolete free count is %d", FS_TOTAL_FREE_PAGES);

//	PRINT_MSG_AND_NUM("\nobsoleted pages ", temp);
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * abort a transaction.
 * - vot all written blocks
 * - initialize transaction struct
 * - write checkpoint
 *
 * @param tid transaction id
 * @return FS_ERROR_SUCCESS if successful
 * 		   -1 if a write/read error occured
 */
int32_t abortTransaction(int32_t tid){
	bool_t cpWritten;
	int32_t res;
//	FENT();

	/* vot all transaction pages */
//	L("call deleteTransactionPages(). b4 that free count is %d", FS_TOTAL_FREE_PAGES);
	FS_VERIFY(!deleteTransactionPages(TRANSACTION_GET_PREV_ADDR_PTR(tid), tid, FLAG_FROM_REBOOT_NO));
//	L("after deleteTransactionPages(), free count is %d", FS_TOTAL_FREE_PAGES);
	/* initialize transaction, and finish*/
	init_transaction(tid);
//	PRINT("\nabortTransaction() - finished aborting. write checkpoint");
	FS_VERIFY(!fsCheckpointWriter(0));
//	PRINT("\nabortTransaction() - findNextFreePage()");
	RES_VERIFY(res, findNextFreePage(&cpWritten, fsCheckpointWriter, 0));

//	L("done with tid %d", tid);
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * auxiliary to allocAndWriteBlockTid().
 * handle case we don't have enough free pages, and cannot complete the transaction.
 * if there aren't enough:
 * - transaction type == unlink:  allow to proceed
 * - abort other write transactions until we have enough free pages to continue
 * - still not enough: check how vots count of current transaction.
 *                     -if more than TRANSACTION_COMMIT_MIN_VOTS, perform a temporary commit
 * 					   - else return error
 *
 * @param tid current transaction id
 * @return FS_ERROR_SUCCESS if we have enough free pages.
 * 		   abortTransaction() errors in case we failed during a transaction abort
 * 		   commitTransaction() errors if we try to perform temporary commit
 * 		   FS_ERROR_WRT_THRSHLD if not enough free pages
 */
int32_t handleMinPages(int32_t tid){
	int32_t i;
//	FENT();

	/* allow unconditional writing if:
	 * a. transaction performs unlink - unlink will create free pages, at least as much as it writes
	 * b. total free pages minimum not reached*/
	if(IS_WRITE_PEMITTED(tid)){
		return FS_ERROR_SUCCESS;
	}

//	L("do we have enough pages? %d. calcTotalFreePages() %d (obs %d ,frees %d, dirty count %d) FS_MIN_FREE_PAGES %d", !IS_MIN_FREE_PAGES_REACHED(), calcTotalFreePages(), GET_OBS_COUNT() , GET_FREE_COUNTER() ,CACHE_GET_DIRTY_COUNT(), FS_MIN_FREE_PAGES);
	/* while we don't have enough obsolete and free pages*/
	while(IS_MIN_FREE_PAGES_REACHED()){
//		L("not enough free pages. free pages count is %d", calcTotalFreePages());
		/* we cannot write a page, minimum free pages reached.
		 * start aborting transactions*/
		for(i=0; i< FS_MAX_N_TRANSACTIONS; i++){
//			L("try to abort transaction %d", i);
			/* don't abort transation if it's:
			 * a. current transaction
			 * b. empty */
			if(i == tid  || IS_TRANSACTION_EMPTY(i)){
				continue;
			}

//				PRINT_MSG_AND_HEX("\ni=",i);
//				assert(0);
//			PRINT_MSG_AND_HEX("\nhandleMinPages() - b4 aborting transaction, tid1 prev addr=", *CAST_TO_UINT32(TRANSACTION_GET_PREV_ADDR_PTR(0)));
//			L("abort transaction %d", i);
			/* found a transaction to abort */
			FS_VERIFY(!abortTransaction(i));
//			PRINT_MSG_AND_HEX("\nhandleMinPages() - after aborting transaction, tid1 prev addr=", *CAST_TO_UINT32(TRANSACTION_GET_PREV_ADDR_PTR(0)));
			break;
		}
//		L("handleMinPages() - did we abort any transaction? %d", i != FS_MAX_N_TRANSACTIONS);
		/* if a transaction was aborted, check for min pages again*/
		if(i != FS_MAX_N_TRANSACTIONS){
			continue;
		}

		/* 23.4.08 no more temporary commit. Simply abort transaction*/
		return FS_ERROR_WRT_THRSHLD;
#if 0
//		PRINT_MSG_AND_NUM("\ndidn't abort any transaction. cur transaction doesnt have enough vots? ",TRANSACTION_GET_VOTS_COUNT(tid) <= TRANSACTION_COMMIT_MIN_VOTS);
		/* if no transactions were aborted, check if we have any point in performing
		 * a temporary commit of current transaction*/
		if(IS_MINIMUM_TRANSACTION_VOTS(tid)){
//			PRINT("\nhandleMinPages() - return error not enough free pages");
//			PRINT_MSG_AND_NUM("\ntransaction will gain us vots=", TRANSACTION_GET_VOTS_COUNT(tid));
//			PRINT_MSG_AND_NUM(" we need=", TRANSACTION_COMMIT_MIN_VOTS);
			return FS_ERROR_WRT_THRSHLD;
		}

		/* if we try to temporaly commit a transction, as part
		 * of it's already ongoing commit - stop, no point in nested commit*/
//		PRINT_MSG_AND_NUM("\nhandleMinPages() - did we abort any transaction? ", i != FS_MAX_N_TRANSACTIONS);
		if(TRANSACTION_GET_IS_COMMITING(tid)){
//			PRINT("\nhandleMinPages() - transaction already committing. return");
			return 0;
		}
//		PRINT("\nhandleMinPages() - perform temporary commit");
//		PRINT_MSG_AND_NUM(", will earn vots ", TRANSACTION_GET_VOTS_COUNT(tid));
//		PRINT_MSG_AND_NUM("\nhandleMinPages() - b4 temp commit, tid1 prev addr=",logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(0)));
		/* perform temporary commit*/
		RES_VERIFY(res, commitTransaction(tid, FS_GET_INO0_ADDR_PTR(), 0));
		TRANSACTION_SET_WAS_COMMITED(tid, 1);
//		PRINT_MSG_AND_NUM("\nnow free pages=",calcTotalFreePages());
//		PRINT_MSG_AND_NUM(", transaction commited flag=",TRANSACTION_GET_WAS_COMMITED(tid));
#endif
	}

//	PRINT("\nhandleMinPages() - finished");
	return FS_ERROR_SUCCESS;

}

/**
 * Auxiliary function to allocAndWriteBlockTid() and cache flushes.
 * Perform a block write as part of a given transaction.
 *
 * @param log_addr the logical address to which the block will be written
 * @param data data to be written
 * @param is_data_vots flag indicating whether data written is a vots page (always 0 for cache flush)
 * @param tid transction id in which the block is being written
 * @return FS_ERROR_SUCCESS on success
 * 	       FS_ERROR_IO if a write error has occured
 */
error_t
allocAndWriteBlockTid_actualWrite(logical_addr_t log_addr, void* data, bool_t is_data_vots, int32_t tid){
	bool_t cpWritten;
//	FENT();

	if(allocAndWriteBlock(log_addr, data, is_data_vots, TRANSACTION_GET_PREV_ADDR_PTR(tid), &cpWritten, fsCheckpointWriter, 0)){
//		L("\nallocAndWriteBlockTid() - allocAndWriteBlock() error");
		return FS_ERROR_IO;
	}
//	L("wrote to addr %x", ADDR_PRINT(log_addr));
	/* change previous address*/
	TRANSACTION_SET_PREV_ADDR(tid, log_addr);

	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * Bottommost level write call of file system.
 * write a block as part of an on-going transaction (tid).
 * get prev address from the transaction, and mark new transaction in it
 *
 * Assumptions:
 * 1. log_addr is initialized
 *
 * @param log_addr old address of block (for caching uses), and container to new block address (if there is one)
 * @param data data block to write
 * @param dataType indicator to which data are we writing (vots, inode entries etc.)
 * @param tid transaction id
 * @return FS_ERROR_SUCCESS on success
 * 		   allocAndWriteBlock() error codes
 * 		   -1 if an error occured when aborting another transaction
 */
error_t
allocAndWriteBlockTid(logical_addr_t log_addr,
					  void* data,
					  int32_t dataType,
					  int32_t entry_offset,
					  int32_t tid){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_entry);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_log_addr);
	error_t res;
	int32_t i;
	dirent_flash *de_ptr = CAST_TO_DIRENT(data);
	bool_t is_cacheable = 1; /* by default we can cache*/
	bool_t performAlloc = 1; /* by default we can allocate*/
//	FENT();

	init_logical_address(log_addr_entry);
	init_logical_address(old_log_addr);
//	if(tid==1){
//		PRINT("\nallocAndWriteBlockTid() - starting");
//		PRINT_MSG_AND_NUM(" dataType=", dataType);
//	}

	/* handle case of minimum free pages*/
//	L("about to handleMinPages()");
	RES_VERIFY(res, handleMinPages(tid));
//	L("finished handleMinPages()");

//	for(i=0;i<FS_BLOCK_SIZE; i++){
//		assert(buf[i]==TRANSACTION_GET_DATA_BUF_PTR(tid)[i]);
//	}
//	uint32_t temp = *CAST_TO_UINT32(TRANSACTION_GET_PREV_ADDR_PTR(0));
	/* write page to flash*/
//	if(temp == 0x7f1fd07b || temp == 0x7f1fe07b){
//	PRINT_MSG_AND_HEX("\n\nallocAndWriteBlockTid() - b4 writing, tid1 prev addr=",*CAST_TO_UINT32(TRANSACTION_GET_PREV_ADDR_PTR(0)));
//	}

	/* if we're unlinking a file, no writing to is actually done.*/
	if(IS_SPARSE_FILE(tid)){
		/* no caching for root inode! */
		if(TRANSACTION_GET_INO(tid) == 0){
//			L("write is not cacheable");
			is_cacheable = 0;
		}

//		L("writing to sparse file");
		/* check if direntries page is empty.
		 * either way it is not a cacheable write (and we indicate it).
		 * don't need to worry about a re-write. the old block will be voted
		 *  and therefore flushed from cache any way*/
		if(dataType == DATA_TYPE_DIRENTS){
			is_cacheable = 0;
			if(IS_DIRENT_EMPTY(de_ptr)){
				performAlloc = 0;
			}
		}

		/* check if entries page is empty*/
		if(dataType == DATA_TYPE_ENTRIES){
//			PRINT("\n. dataType entries");
			performAlloc = 0;
			for(i=0; i<LOG_ADDRESSES_PER_BLOCK;i++){
				/* for inode entries, if we have at least one entry, write block
				 * otherwise don't write an empty block, and return empty address as written-to address*/
				BLOCK_GET_INDEX(data, i, log_addr_entry);
				if(!IS_ADDR_EMPTY(log_addr_entry)){
//					PRINT_MSG_AND_NUM("\nfound non-empty entry ", i);
//					PRINT_MSG_AND_HEX(", ", *CAST_TO_UINT32(log_addr_entry));
					performAlloc = 1;
					break;
				}
			}
		}
	}

	/* finally the page is definitely not cacheable if
	 * - it is a vots page
	 * - there is no cache (dahh!)*/
	if(dataType == DATA_TYPE_VOTS ||
	   IS_CACHE_EMPTY()){
		is_cacheable = 0;
	}
	/* if required - perform actual allocation.
	 * don't cache write if -
	 * 1. performAlloc is 0 - we don't need to write anyway...
	 * 2. indicator flag orders us not to cache */
//	L("performAlloc %d, is_cacheable %d (log_addr %x)", performAlloc, is_cacheable, ADDR_PRINT(log_addr));
	if(performAlloc && !is_cacheable){
//		L("write is not cacheable, and we should perform alloc. call allocAndWriteBlockTid_actualWrite()");
		res = allocAndWriteBlockTid_actualWrite(log_addr, data, (dataType==DATA_TYPE_VOTS), tid);
//		L("allocAndWriteBlockTid_actualWrite() wrote to %x", ADDR_PRINT(log_addr));
		if(!IS_FS_SUCCESS(res)){
			return res;
		}
	}
	/* if we should perform allocation, and block is cacheable -
	 * write cache functionality kicks in HERE!*/
	else if(is_cacheable){
//		L("write is cacheable");
		/* if we should perform alloc*/
		if(performAlloc){
//			L("call cache_access_cache(), write is cacheable (ino num %d)", TRANSACTION_GET_INO(tid));
			copyLogicalAddress(old_log_addr, log_addr);
//			L("cache write. call cache_access_cache(). old_log_addr %x", ADDR_PRINT(old_log_addr));
			RES_VERIFY(res, cache_access_cache(data,
											   log_addr,
											   entry_offset,
											   tid,
											   dataType,
											   CACHE_ACCESS_WRITE));
//			L("cache_access_cache() wrote to cached addr %x", ADDR_PRINT(log_addr));
		}
		/* if write is cacheable, but should now not be allocated
		 * (sparse file empty block) dispose of the cache, and init written addr*/
		else{
//			L("write is cacheable, but should now not be allocated. log_addr =%x", ADDR_PRINT(log_addr));
			/* if the address was cached - force free it. we dont need it anymore
			 * otherwise - find the cache id*/
			int32_t cid = CACHE_ID_EMPTY;
			if(IS_CACHED_ADDR(log_addr)){
				cid = CACHE_GET_CID_FROM_ADDR(log_addr);
			}
			else{
				cid = cache_get_cid_by_real_addr(log_addr);
			}


			/*if indeed the address is cached, force free it*/
			/*now that we have a cache id, flush it and init the address*/
			if(!IS_CACHE_ID_EMPTY(cid)){
				assert(cid < FS_CACHE_BUFFERS);
				RES_VERIFY(res, cache_force_free_cache(cid));
			}
//			else{
//				RES_VERIFY(res, writeVot(log_addr, tid));
//			}

			init_logical_address(log_addr);
		}
	}
	/* we shouldn't have performed an allocation, and the write
	 * is not cacheble ~> this is an empty inode0 write. init addr*/
	else{
//		L("not cacheable, and shouldnt perform alloc. simply init logical addr");
		init_logical_address(log_addr);
	}
	/* if not cacheable*/
//	if(temp == 0x7f1fd07b || temp == 0x7f1fe07b){
//	PRINT_MSG_AND_HEX("\nallocAndWriteBlockTid() - after writing, tid1 prev addr=",*CAST_TO_UINT32(TRANSACTION_GET_PREV_ADDR_PTR(0)));
////		if(temp == 0x7f1fe07b)
////		assert(0);
//	}
//	L("wrote to address=%x",*CAST_TO_UINT32(log_addr));
//	PRINT_MSG_AND_NUM("\nallocAndWriteBlock() - wrote data to ",logicalAddressToPhysical(log_addr));

//	PRINT_MSG_AND_HEX("\nallocAndWriteBlock() - first byte=", CAST_TO_UINT8(data)[0]);

	/* mark address as negative according to data type and conditions.
	 * assume by default we don't need to mark as negative*/
	res = 0;
	switch(dataType){
		case DATA_TYPE_REGULAR:
		case DATA_TYPE_INODE:
			break;
		case DATA_TYPE_ENTRIES:
			for(i=0; i<LOG_ADDRESSES_PER_BLOCK;i++){
				/* for inode entries, if we have an entry marked as sparse, mark log_addr
				 * as sparse too*/
				BLOCK_GET_INDEX(data, i, log_addr_entry);
				if(IS_ADDR_FREE(log_addr_entry)){
					res = 1;
				}
			}
			break;
		case DATA_TYPE_VOTS:
			break;
		case DATA_TYPE_DIRENTS:
			/* for direntries, check if we overflow half block size by trying to find
			 * dirent terminator after half block size*/

			for(i=FS_BLOCK_SIZE;i>= (FS_BLOCK_SIZE-DIRENT_MAX_SIZE); i--){
				if(IS_PATH_END(CAST_TO_UINT8(data)[i-1])){
					break;
				}
			}
//			PRINT_MSG_AND_NUM("\nallocAndWriteBlockTid() - writing dirent page. file ino num=", TRANSACTION_GET_INO(tid));
//			PRINT_MSG_AND_NUM(" last direntry ends at offset=", i);
			/* if there's enough space for another dirent, mark as sparse*/
			if(i < (FS_BLOCK_SIZE-DIRENT_MAX_SIZE)){
				res = 1;
			}

//			PRINT_MSG_AND_NUM("\nallocAndWriteBlockTid() - found last entry end at buf offset=", i);
			break;
		default:
			break;
	}

	/* mark address as sparse if necessary*/
	if(res){
		MARK_BLOCK_WITH_HOLE(log_addr);
	}
	else{
		MARK_BLOCK_NO_HOLE(log_addr);
	}
//	L("allocated. returned log addr is %x", ADDR_PRINT(log_addr));
//	TRANSACTION_INC_TOTAL_BLOCK_ALLOCS(tid);
//	if(tid==1){
//		PRINT_MSG_AND_NUM("\nallocAndWriteBlockTid() - finished, tid prev addr=",logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
//	}
//	L("done!");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * mark address in log_addr as vot in transaction tid
 *
 * @param org_vot_log_addr address to vot when transaction commits
 * @param tid transaction id
 * @return FS_ERROR_SUCCESS if successful
 *         allocAndWriteBlockTid() errors in case of a write error
 */
error_t
writeVot(logical_addr_t org_vot_log_addr, int32_t tid){
	/* the actual addr that will be voted, since org_vot_log_addr may be a cached addr*/
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(vot_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	error_t res;
	int32_t cid = CACHE_ID_EMPTY;
	init_logical_address(log_addr);
	init_logical_address(vot_log_addr);
//	FENT();
//	L("try to vot %x=",ADDR_PRINT(org_vot_log_addr));
	/* can't vot empty address*/
	if(IS_ADDR_EMPTY(org_vot_log_addr)){
//		L("can't vot empty addr");
		return FS_ERROR_SUCCESS;
	}

	/* can't vot a cached address.
	 * vot it's actual address, or if it's already voted return.
	 * for a non-cached addr, simply continue */
	if(IS_CACHED_ADDR(org_vot_log_addr)){
//		L("org_vot_log_addr cached. vot cid %d", CACHE_GET_CID_FROM_ADDR(org_vot_log_addr));
		/* get real addr of cached buffer (or an error saying it was already cached)*/
		cid = CACHE_GET_CID_FROM_ADDR(org_vot_log_addr);

		/* if was already cached we're done*/
		if(CACHE_GET_IS_VOTED(cid)){
//			L("didnt vot anything since cache is alreadt voted");
			return FS_ERROR_SUCCESS;
		}

		/* can't vot empty address*/
		if(IS_ADDR_EMPTY(CACHE_GET_LOG_ADDR_PTR(cid))){
//			L("can't vot empty addr");
			return FS_ERROR_SUCCESS;
		}

		copyLogicalAddress(vot_log_addr, CACHE_GET_LOG_ADDR_PTR(cid));
	}
	else{
		/* try to get cache for the voted addr. if:
		 * 1. cache exists
		 * 2. cache wasn't already voted
		 * vot it's real address */
		cid = cache_get_cid_by_real_addr(org_vot_log_addr);
//		L("org_vot_log_addr NOT cached. vot cid %d", cid);
		if(!IS_CACHE_ID_EMPTY(cid) &&
		   !CACHE_GET_IS_VOTED(cid)){
			/* can't vot empty address*/
			if(IS_ADDR_EMPTY(CACHE_GET_LOG_ADDR_PTR(cid))){
				return FS_ERROR_SUCCESS;
			}

//			L("org_vot_log_addr is cached but not voted. vot it's real address");
			copyLogicalAddress(vot_log_addr, CACHE_GET_LOG_ADDR_PTR(cid));
		}
		else{
			/* for consistency, use vot_log_addr from now on */
//			L("org_vot_log_addr is real and not cached. vot it");
			copyLogicalAddress(vot_log_addr, org_vot_log_addr);
		}
	}

	/* before adding to vots buffer, check address isn't already in buffer*/
	uint32_t i;
	for(i=0; i<TRANSACTION_GET_VOTS_OFFSET(tid); i++){
		BLOCK_GET_INDEX(TRANSACTION_GET_VOTS_BUF_PTR(tid), i, log_addr);
		if(COMPARE_ADDR(log_addr, vot_log_addr)){
			return FS_ERROR_SUCCESS;
		}
	}
	init_logical_address(log_addr);

	/* set vots buffer, write it if necessary, and reset etc.*/
//	L("add vot of addr=%d vots offset=%d", logicalAddressToPhysical(vot_log_addr), TRANSACTION_GET_VOTS_OFFSET(tid));
//	L("add addr to vot buffer %x", ADDR_PRINT(vot_log_addr));
	TRANSACTION_ADD_VOT(tid, vot_log_addr);
//	PRINT("\nwriteVot() - inc vot offset");
	TRANSACTION_INC_VOTS_OFFSET(tid);
	TRANSACTION_INC_VOTS_COUNT(tid);

	/* if we filled the vots block*/
	if(TRANSACTION_GET_VOTS_OFFSET(tid) == LOG_ADDRESSES_PER_BLOCK){
//		L("write vot block - call allocAndWriteBlockTid()");
		RES_VERIFY(res, allocAndWriteBlockTid(log_addr, TRANSACTION_GET_VOTS_BUF_PTR(tid), DATA_TYPE_VOTS, CACHE_ENTRY_OFFSET_EMPTY, tid));
//		L("VOT BLOCK WRITTEN TO %x", ADDR_PRINT(log_addr));
		init_fsbuf(TRANSACTION_GET_VOTS_BUF_PTR(tid));
		TRANSACTION_SET_VOTS_OFFSET(tid, 0);
	}

	/* if address is cached, mark it's cache that it is voted*/
	if(!IS_CACHE_ID_EMPTY(cid) &&
	   !IS_ADDR_EMPTY(CACHE_GET_LOG_ADDR_PTR(cid))){
		CACHE_SET_IS_VOTED(cid, CACHE_IS_VOTED_YES);
	}

//	L("finished");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * auxiliary to commitInode().
 * initialize fields realted to writes during a transaction,.
 * after commiting inode.
 * @param tid transaction id
 */
void initWriteFields(int32_t tid){
	/* initialize write fields*/
//	TRANSACTION_SET_MAX_WRITTEN_OFFSET(tid,0);
	TRANSACTION_SET_BLOCKS_ALLOCS(tid,0);
}

/**
 * @brief
 * auxiliary to handleTransactionVOTs
 *
 * @param vots buffer filled with vot records, size of FS_BLOCK_SIZE
 * @param is_from_reboot is this function called as part of a boot
 * @return FS_ERROR_SUCCESS if successful
 * 		   -1 if a write error occurred
 */
error_t
performVOTs(uint8_t *vots, bool_t is_from_reboot){
	int32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//	FENT();
	init_logical_address(log_addr);
//	for(i=0; i<FS_BLOCK_SIZE; i++){
//		if(vots[i] == 0xff)
//			continue;
//
//		PRINT_MSG_AND_NUM("\nvots byte ", i);
//		PRINT_MSG_AND_HEX(". ",vots[i]);
//	}

	for(i=0; i<LOG_ADDRESSES_PER_BLOCK; i++){
		BLOCK_GET_INDEX(vots, i, log_addr);
		if(IS_ADDR_EMPTY(log_addr)){
//			L("STOPPING. address empty at offset=",i);
			break;
		}

//		PRINT_MSG_AND_HEX("\nperformVOTs() - make obsolete vot address=", *CAST_TO_UINT32(log_addr));
//		PRINT_MSG_AND_NUM("\nperformVOTs() - make obsolete vot address=", logicalAddressToPhysical(log_addr));
//		L("markAsObsolete addr with seg %d, offset %d", GET_LOGICAL_SEGMENT(log_addr), GET_LOGICAL_OFFSET(log_addr));
		FS_VERIFY(!tr_mark_as_obsolete(log_addr, is_from_reboot));
//		L("continue to next address");
//		PRINT("\nperformVOTs() - decrement vots ocunt");
//		TRANSACTION_SET_VOTS_COUNT(0, TRANSACTION_GET_VOTS_COUNT(0)-1);
	}
//	L("done");
	return FS_ERROR_SUCCESS;
}


/**
 * @brief
 * do vots for all vot pages in transaction tid
 *
 * @param tid_last_addr last address written in a transaction
 * @return FS_ERROR_SUCCESS if successful
 */
int32_t handleTransactionVOTs(logical_addr_t tid_last_addr, bool_t is_from_reboot){
	bool_t isVOTs;
	int32_t res;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
//	FENT();
//	L("tid last addr is %x", ADDR_PRINT(tid_last_addr));

	init_logical_address(log_addr);
	init_logical_address(prev_log_addr);

	copyLogicalAddress(prev_log_addr, tid_last_addr);

	while(!IS_ADDR_EMPTY(prev_log_addr)){
		copyLogicalAddress(log_addr, prev_log_addr);
		/* read previous*/
//		L("log addr seg %d, offset %d, read from %d", GET_LOGICAL_SEGMENT(log_addr), GET_LOGICAL_OFFSET(log_addr), logicalAddressToPhysical(log_addr));
		FS_VERIFY(!readVOTsAndPrev(log_addr, fs_buffer, prev_log_addr, &isVOTs));
//		L("is it VOTs? %s", (isVOTs)?"YES":"NO");

		/* if is vots delete all, otherwise continue...*/
		if(!isVOTs){
			continue;
		}

		/* vot all */
//		L("call performVOTs() on fs_buffer");
		RES_VERIFY(res, performVOTs(fs_buffer, is_from_reboot));
//		PRINT_MSG_AND_NUM("\nhandleTransactionVOTs() - make obsolete vots page=", logicalAddressToPhysical(log_addr));
//		L("call markAsObsolete on log_addr of vots page itself. seg %d, offset %d", GET_LOGICAL_SEGMENT(log_addr), GET_LOGICAL_OFFSET(log_addr));
		FS_VERIFY(!tr_mark_as_obsolete(log_addr, is_from_reboot));
	}

//	L("return succesfuly");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * auxiliary to commitInode(), readFileBlock(). change file size in inode, according to indirect block location
 * in file, and max offset written to in this block.
 * if the file is not involved in a transaction, no chnages are made
 *
 * @param tid transaction id
 * @param ino_buf buffer containing a file inode
 */
void changeFileSize(int32_t tid, uint8_t *ino_buf){
	inode_t *ino_ptr;
	uint32_t indirect_max_offset;
	uint32_t old_nblocks;

	/* if the file is not involved in any transaction, no chnages will be made*/
	if(IS_EMPTY_TID(tid)){
		return;
	}

	ino_ptr = CAST_TO_INODE(ino_buf);
	indirect_max_offset = TRANSACTION_GET_FILE_OFFSET(tid)+TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid);
	old_nblocks = INODE_GET_NBLOCKS(ino_ptr);

	/* no point in setting it, if no blocks written */
	if(!IS_WRITES_PERFORMED(tid)){
		return;
	}

//	PRINT_MSG_AND_NUM("\nchangeFileSize() - ino= ", INODE_GET_FILE_ID(ino_ptr));
//	PRINT_MSG_AND_NUM("\nchangeFileSize() - old nblocks is ", old_nblocks);
//	PRINT_MSG_AND_NUM(" increase by ", TRANSACTION_GET_BLOCKS_ALLOCS(tid));
//	PRINT_MSG_AND_NUM(", new nblocks would be ",old_nblocks +TRANSACTION_GET_BLOCKS_ALLOCS(tid));
//	PRINT_MSG_AND_NUM("\nchangeFileSize() - old nbytes ", INODE_GET_NBYTES(ino_ptr));
//	PRINT_MSG_AND_NUM(" indirect_max_offset=", indirect_max_offset);

	INODE_SET_NBLOCKS(ino_ptr, old_nblocks+TRANSACTION_GET_BLOCKS_ALLOCS(tid));

	if(indirect_max_offset > INODE_GET_NBYTES(ino_ptr)){
//		L("change size from %d to %d", INODE_GET_NBYTES(ino_ptr), indirect_max_offset);
		INODE_SET_NBYTES(ino_ptr, indirect_max_offset);
	}
//	PRINT("\nchangeFileSize() - finished");
}

/**
 * @brief
 * commit inode of of given transaction
 *
 * Assumptions:
 * 1. If the active indirect is cached, it is the caller's responsibility
 *    to remove it from cache.
 * 2. when commitInode() returns, the indirect active,if cached , is marked in parent
 *    as such, and not as active
 *
 * @param tid transaction id
 * @return FS_ERROR_SUCCESS if successful
 * 	       allocAndWriteBlockTid() errors if it failed
 */
error_t
commitInode(int32_t tid){
	error_t res;
	int32_t entry_offset;
	int32_t offset_indirect = TRANSACTION_GET_FILE_OFFSET(tid);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(triple_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(new_indirect_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_indirect_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_double_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_triple_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_inode_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_log_addr);
	int32_t old_isCommiting;
	init_logical_address(triple_log_addr);
	init_logical_address(double_log_addr);
	init_logical_address(new_indirect_log_addr);
	init_logical_address(old_indirect_log_addr);
	init_logical_address(old_double_log_addr);
	init_logical_address(old_triple_log_addr);
	init_logical_address(old_inode_log_addr);
	init_logical_address(old_log_addr);
//	FENT();

	/* if no writes were performed we don't have to write anything*/
	if(IS_INDIRECT_EMPTY(tid)){
		return FS_ERROR_SUCCESS;
	}

	old_isCommiting = TRANSACTION_GET_IS_COMMITING(tid);
	TRANSACTION_SET_IS_COMMITING(tid, 1);
//	L("commit inode num=%d", TRANSACTION_GET_INO(tid));
//	PRINT_MSG_AND_NUM(" tid=", tid);
//	PRINT_MSG_AND_NUM(" indirect offset=", TRANSACTION_GET_FILE_OFFSET(tid));
//	PRINT_MSG_AND_NUM("\ncommitInode() - is indirect inode?=",IS_INDIRECT_INODE(tid));

	/* if the indirect block is actually the inode, we should simply write it */
	if(IS_INDIRECT_INODE(tid)){
//		L("inode is indirect");
		/* vot old inode (if temporary commit). */
//		 /* NOTICE - vot is done if address not empty.writeVot() does it too, however this
//		 * helps us avoid incrementing nblocks count twice in case of creat,
//		 * since setNewInode already sets nblocks to 1 */
//		L("\ncommitInode() - vot old inode (indirect is inode)");
		RES_VERIFY(res, writeVot(TRANSACTION_GET_INO_ADDR_PTR(tid), tid));
		TRANSACTION_SET_IS_VOTED(tid, CACHE_IS_VOTED_YES);
//		PRINT("\ncommitInode() - writeVot success")

		/* change file size (if necessary) and write inode*/
		changeFileSize(tid, TRANSACTION_GET_INDIRECT_PTR(tid));
//		L("commiting inode - call allocAndWriteBlockTid()");
		RES_VERIFY(res, allocAndWriteBlockTid(TRANSACTION_GET_INO_ADDR_PTR(tid), TRANSACTION_GET_INDIRECT_PTR(tid), DATA_TYPE_INODE, CACHE_ENTRY_OFFSET_EMPTY, tid));
//		L("DONE commiting inode to %x", ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));
		/* write cache related fields*/
		if(IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid)) &&
		   !IS_ADDR_EMPTY(CACHE_GET_LOG_ADDR_PTR(CACHE_GET_CID_FROM_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid))))){
			CACHE_SET_IS_VOTED(CACHE_GET_CID_FROM_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid)), CACHE_IS_VOTED_YES);
		}

//		PRINT_MSG_AND_NUM("\ncommitInode() - committed inode to ", logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));
//		PRINT_MSG_AND_NUM(", commited inode whose file size=", INODE_GET_NBYTES(ino_ptr));
		initWriteFields(tid);

		/*return to normal state (not commiting) */
//		PRINT_MSG_AND_NUM("\ncommitInode() - set is commiting back to ", old_isCommiting);
		TRANSACTION_SET_IS_COMMITING(tid, old_isCommiting);

//		L("done!");
		return FS_ERROR_SUCCESS;
	}

//	L("indirect is not inode");
	/* read inode to fs_buffer */
//	L("read inode from %x", ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	FS_VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),
						   fs_buffer,
						   tid,
						   CACHE_ENTRY_OFFSET_EMPTY,
						   FLAG_CACHEABLE_READ_YES));

	//	PRINT_MSG_AND_NUM("\ncommitInode() - offset_indirect=",offset_indirect);
	/* if it is the inode indirect data offset
	 * write indirect*/
	if(offset_indirect <DOUBLE_DATA_OFFSET){
//		L("offset_indirect <DOUBLE_DATA_OFFSET");
		/* vot old address*/
		INODE_GET_INDIRECT(CAST_TO_INODE(fs_buffer), old_log_addr);

		/* write indirect index to flash*/
		copyLogicalAddress(new_indirect_log_addr, old_log_addr);
//		L("write indirect index to flash. old_log_addr %x", ADDR_PRINT(old_log_addr));
		RES_VERIFY(res, allocAndWriteBlockTid(new_indirect_log_addr, TRANSACTION_GET_INDIRECT_PTR(tid), DATA_TYPE_ENTRIES, CALC_IN_LOG_ADDRESSES(INDIRECT_INDEX_LOCATION), tid));
//		L("new_indirect_log_addr %x", *((uint32_t*)new_indirect_log_addr));
		//	PRINT_MSG_AND_NUM("\n wrote indirect to logical  offset=", GET_LOGICAL_OFFSET(new_indirect_log_addr));
		//	PRINT_MSG_AND_NUM(" phy addr=", logicalAddressToPhysical(new_indirect_log_addr)	);

		/* vot old indirect
		 * NOTICE - do it after read/write, so we mark the vot in cache if necessary*/
//		L("vot old indirect");
		RES_VERIFY(res, writeVot(old_log_addr, tid));

		/* read inode to fs_buffer,
		 * mark indirect in inode*/
//		L("read inode from address %x", (TRANSACTION_GET_INO_ADDR_PTR(tid)));
		FS_VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),
							   fs_buffer,
							   tid,
							   CACHE_ENTRY_OFFSET_EMPTY,
							   FLAG_CACHEABLE_READ_YES));
		copyLogicalAddress(old_inode_log_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));
//		L("set indirect in inode");
//		PRINT_MSG_AND_HEX(" new_indirect_log_addr=",*((uint32_t*)(new_indirect_log_addr)));
		INODE_SET_INDIRECT(CAST_TO_INODE(fs_buffer), new_indirect_log_addr);
	}
	/* if it is the inode double data offset
	 * write indirect-double*/
	else if(offset_indirect <TRIPLE_DATA_OFFSET){
//		L("offset_indirect %d <TRIPLE_DATA_OFFSET", offset_indirect);
		INODE_GET_DOUBLE(CAST_TO_INODE(fs_buffer), double_log_addr);
		copyLogicalAddress(old_double_log_addr, double_log_addr);

		/* read double block */
//		L("read double from %x", *((uint32_t*)double_log_addr));
		FS_VERIFY(!fsReadBlock(double_log_addr,
							   fs_buffer,
							   tid,
							   DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
							   FLAG_CACHEABLE_READ_YES));
		assert(CACHE_GET_BUF_PTR(0)[527] != 0x41);
		/* vot old double address
		 * NOTICE - do it after read, so we mark the vot in cache if necessary*/
//		PRINT("\ncommitInode() - vot old double");
		RES_VERIFY(res, writeVot(old_double_log_addr, tid));
//		L("CACHE_GET_IS_VOTED(1) %d", CACHE_GET_IS_VOTED(1));
		entry_offset = CALC_OFFSET_IN_DOUBLE_BLOCK(CALC_OFFSET_FROM_DOUBLE(offset_indirect));
		BLOCK_GET_INDEX(fs_buffer,
						entry_offset,
					    old_indirect_log_addr);

		/* write indirect index to flash*/
//		L("write indirect whose old addr %x", ADDR_PRINT(old_indirect_log_addr));
		copyLogicalAddress(new_indirect_log_addr, old_indirect_log_addr);
		RES_VERIFY(res, allocAndWriteBlockTid(new_indirect_log_addr,
											  TRANSACTION_GET_INDIRECT_PTR(tid),
											  DATA_TYPE_ENTRIES,
											  entry_offset,
											  tid));
//		L("the indirect new addr %x", ADDR_PRINT(new_indirect_log_addr));

		/* vot old indirect address
		 * NOTICE - do it after read/write, so we mark the vot in cache if necessary*/
//		PRINT("\ncommitInode() - vot old indirect");
		RES_VERIFY(res, writeVot(old_indirect_log_addr, tid));
		//	PRINT_MSG_AND_NUM("\n wrote indirect to logical  offset=", GET_LOGICAL_OFFSET(new_indirect_log_addr));
		//	PRINT_MSG_AND_NUM(" phy addr=", logicalAddressToPhysical(new_indirect_log_addr)	);

		/* set new indirect entry
		/* NOTICE - first read double block again, since we may have
		 *          flushed another cached indirect for the last allocAndWriteBlockTid() call */
//		L("read double again from %x", *((uint32_t*)double_log_addr));
		FS_VERIFY(!fsReadBlock(double_log_addr,
							   fs_buffer,
							   tid,
							   DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
							   FLAG_CACHEABLE_READ_YES));

		entry_offset = CALC_OFFSET_IN_DOUBLE_BLOCK(CALC_OFFSET_FROM_DOUBLE(offset_indirect));
//		L("set indirect addr %x to double block entry #%d", ADDR_PRINT(new_indirect_log_addr), entry_offset);
		BLOCK_SET_INDEX(fs_buffer,
						entry_offset,
					    new_indirect_log_addr);
//		PRINT_MSG_AND_NUM("\n offset from indirect=", CALC_OFFSET_FROM_DOUBLE(offset_indirect));
//		PRINT_MSG_AND_NUM("\n in indirect size this is=", CALC_OFFSET_FROM_DOUBLE(offset_indirect) / INODE_INDIRECT_DATA_SIZE);
//		PRINT_MSG_AND_NUM("\ncommitInode() - setting indirect entry in double block at offset=", CALC_OFFSET_IN_DOUBLE_BLOCK(CALC_OFFSET_FROM_DOUBLE(offset_indirect)));
//		PRINT_MSG_AND_NUM(", new entry=", logicalAddressToPhysical(new_indirect_log_addr));

		/* write double block to flash, and mark in inode*/
//		L("perform allocAndWriteBlockTid() on double block");
//		init_logical_address(double_log_addr);
		RES_VERIFY(res, allocAndWriteBlockTid(double_log_addr,
											  fs_buffer,
											  DATA_TYPE_ENTRIES,
											  DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
											  tid));

		FS_VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),
							   fs_buffer,
							   tid,
							   CACHE_ENTRY_OFFSET_EMPTY,
							   FLAG_CACHEABLE_READ_YES));
		copyLogicalAddress(old_inode_log_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));
//		L("set double addr %x in inode", ADDR_PRINT(double_log_addr));
//		PRINT_MSG_AND_HEX(" new double_log_addr=",*((uint32_t*)(double_log_addr)));
		INODE_SET_DOUBLE(CAST_TO_INODE(fs_buffer), double_log_addr);
//		L("CACHE_GET_IS_VOTED(1) %d", CACHE_GET_IS_VOTED(1));
	}
	/* it is in the inode triple data offset
	 * write indirect-double-triple*/
	else{
//		L("offset is in triple size");
		INODE_GET_TRIPLE(CAST_TO_INODE(fs_buffer), triple_log_addr);
		copyLogicalAddress(old_triple_log_addr, triple_log_addr);

		/* read triple block */
		FS_VERIFY(!fsReadBlock(triple_log_addr,
							   fs_buffer,
							   tid,
							   TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
							   FLAG_CACHEABLE_READ_YES));

		/* vot old triple
		 * NOTICE - do it after read/write, so we mark the vot in cache if necessary*/
//		if(TRANSACTION_GET_VOTS_COUNT(tid) > 1300)
//		PRINT("\ncommitInode() - vot old triple");
		RES_VERIFY(res, writeVot(old_triple_log_addr, tid));

//		PRINT_MSG_AND_NUM("\n get double entry in offset=", CALC_OFFSET_IN_TRIPLE_BLOCK(CALC_OFFSET_FROM_TRIPLE(offset_indirect)));
		/* get old double address*/
		entry_offset = CALC_OFFSET_IN_TRIPLE_BLOCK(CALC_OFFSET_FROM_TRIPLE(offset_indirect));
		BLOCK_GET_INDEX(fs_buffer,
						entry_offset,
		                double_log_addr);
		copyLogicalAddress(old_double_log_addr, double_log_addr);

		/* read double block */
		FS_VERIFY(!fsReadBlock(double_log_addr,
							   fs_buffer,
							   tid,
							   entry_offset,
							   FLAG_CACHEABLE_READ_YES));

		/* vot old double
		 * NOTICE - do it after read/write, so we mark the vot in cache if necessary*/
//		if(TRANSACTION_GET_VOTS_COUNT(tid) > 1300)
//		PRINT("\ncommitInode() - vot old double");
		RES_VERIFY(res, writeVot(old_double_log_addr, tid));

		/* get old indirect */
		entry_offset = CALC_OFFSET_IN_DOUBLE_BLOCK(CALC_OFFSET_IN_DOUBLE_SIZE(CALC_OFFSET_FROM_TRIPLE(offset_indirect)));
		BLOCK_GET_INDEX(fs_buffer,
						entry_offset,
						old_indirect_log_addr);
		copyLogicalAddress(new_indirect_log_addr, old_indirect_log_addr);

		/* write indirect index to flash*/
//		L("write indirect index to flash. old_indirect_log_addr %x", ADDR_PRINT(old_indirect_log_addr));
		RES_VERIFY(res, allocAndWriteBlockTid(new_indirect_log_addr,
											  TRANSACTION_GET_INDIRECT_PTR(tid),
											  DATA_TYPE_ENTRIES,
											  entry_offset,
											  tid));
//		L("the indirect new addr %x", *((uint32_t*)new_indirect_log_addr));

		/* vot old indirect
		 * NOTICE - do it after read/write, so we mark the vot in cache if necessary*/
//		if(TRANSACTION_GET_VOTS_COUNT(tid) > 1300)
//		PRINT("\ncommitInode() - vot old indirect");
		RES_VERIFY(res, writeVot(old_indirect_log_addr, tid));

//		copyLogicalAddress(new_indirect_log_addr, old_log_addr);
		//	PRINT_MSG_AND_NUM("\n wrote indirect to logical  offset=", GET_LOGICAL_OFFSET(new_indirect_log_addr));
		//	PRINT_MSG_AND_NUM(" phy addr=", logicalAddressToPhysical(new_indirect_log_addr)	);

		/* read double block again*/
		FS_VERIFY(!fsReadBlock(double_log_addr,
							   fs_buffer,
							   tid,
							   entry_offset,
							   FLAG_CACHEABLE_READ_YES));

		/* set new indirect address*/
//		PRINT_MSG_AND_NUM("\n set indirect entry in offset=", CALC_OFFSET_IN_DOUBLE_BLOCK(CALC_OFFSET_IN_DOUBLE_SIZE(CALC_OFFSET_FROM_TRIPLE(offset_indirect))));
//		PRINT_MSG_AND_NUM("\n indirect logical offset=", GET_LOGICAL_OFFSET(new_indirect_log_addr));
		entry_offset = CALC_OFFSET_IN_DOUBLE_BLOCK(CALC_OFFSET_IN_DOUBLE_SIZE(CALC_OFFSET_FROM_TRIPLE(offset_indirect)));
		BLOCK_SET_INDEX(fs_buffer,
						entry_offset,
					    new_indirect_log_addr);
//		PRINT_MSG_AND_NUM("\n set double entry in offset=", CALC_OFFSET_IN_TRIPLE_BLOCK(CALC_OFFSET_FROM_TRIPLE(offset_indirect)));

		/* write double block to flash, and mark in triple block*/
//		init_logical_address(double_log_addr);
//		PRINT("\ncommitInode() - perform allocAndWriteBlockTid() on (triple) double block");
		RES_VERIFY(res, allocAndWriteBlockTid(double_log_addr,
											  fs_buffer,
											  DATA_TYPE_ENTRIES,
											  CALC_OFFSET_IN_TRIPLE_BLOCK(CALC_OFFSET_FROM_TRIPLE(offset_indirect)),
											  tid));
//		L("the double new addr %x", *((uint32_t*)double_log_addr));

		/* read triple block again*/
		FS_VERIFY(!fsReadBlock(triple_log_addr,
							   fs_buffer,
							   tid,
							   TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
							   FLAG_CACHEABLE_READ_YES));

//		PRINT_MSG_AND_NUM("\n wrote double to logical  offset=", GET_LOGICAL_OFFSET(double_log_addr));
		/* set new double address*/
		BLOCK_SET_INDEX(fs_buffer,
					    CALC_OFFSET_IN_TRIPLE_BLOCK(CALC_OFFSET_FROM_TRIPLE(offset_indirect)),
					    double_log_addr);

		/* write triple block to flash, and mark in inode*/
//		init_logical_address(triple_log_addr);
//		PRINT("\ncommitInode() - perform allocAndWriteBlockTid() on triple block");
		RES_VERIFY(res, allocAndWriteBlockTid(triple_log_addr,
											  fs_buffer,
											  DATA_TYPE_ENTRIES,
											  TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
											  tid));
//		L("the triple new addr %x", *((uint32_t*)triple_log_addr));

		/* set triple address in inode*/
		FS_VERIFY(!fsReadBlock(TRANSACTION_GET_INO_ADDR_PTR(tid),
							   fs_buffer,
							   tid,
							   CACHE_ENTRY_OFFSET_EMPTY,
							   FLAG_CACHEABLE_READ_YES));

		copyLogicalAddress(old_inode_log_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));
		INODE_SET_TRIPLE(CAST_TO_INODE(fs_buffer), triple_log_addr);
	}
//	PRINT_MSG_AND_NUM("\ncommitInode() - voted old inode. change file size for tid=", tid);
//	PRINT_MSG_AND_NUM(" inode=", TRANSACTION_GET_INO(tid));

//	{
//		inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
//		int32_t indirect_max_offset = TRANSACTION_GET_FILE_OFFSET(tid)+TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid);
//		PRINT_MSG_AND_NUM("\ncommitInode() - ino= ", INODE_GET_FILE_ID(ino_ptr));
//		PRINT_MSG_AND_NUM("\ncommitInode() - old nblocks is ", INODE_GET_NBLOCKS(ino_ptr));
//		PRINT_MSG_AND_NUM(" increase by ", TRANSACTION_GET_BLOCKS_ALLOCS(tid));
//			PRINT_MSG_AND_NUM("\ncommitInode() - old nbytes ", INODE_GET_NBYTES(ino_ptr));
//			PRINT_MSG_AND_NUM(" indirect_max_offset=", indirect_max_offset);
//	}
//	PRINT_MSG_AND_NUM("\ncommitInode() - b4 changeFileSize nblocks=", INODE_GET_NBLOCKS(CAST_TO_INODE(buf)));
//	PRINT_MSG_AND_NUM(" nbytes=", INODE_GET_NBYTES(CAST_TO_INODE(buf)));

	/* change file size (if necessary) and write inode*/
	changeFileSize(tid, fs_buffer);
//	L("after changeFileSize nbytes=%d", INODE_GET_NBYTES(CAST_TO_INODE(fs_buffer)));
//	L("write inode addr, old one is %x", ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));
//	L("write inode, call allocAndWriteBlockTid()");
	RES_VERIFY(res, allocAndWriteBlockTid(TRANSACTION_GET_INO_ADDR_PTR(tid),
										  fs_buffer,
										  DATA_TYPE_INODE,
										  CACHE_ENTRY_OFFSET_EMPTY,
										  tid));
	/* vot old inode address  */
//	if(TRASNACTION_GET_VOTS_COUNT(tid) > 1300)
//	L("vot old inode (final)");
	RES_VERIFY(res, writeVot(old_inode_log_addr, tid));
//	PRINT_MSG_AND_NUM("\ncommitInode() - wrote inode to address=", logicalAddressToPhysical(TRANSACTION_GET_INO_ADDR_PTR(tid)));
//	PRINT_MSG_AND_NUM("\ncommitInode()- file size=",CAST_VAL_TO_INT32(INODE_GET_NBYTES(CAST_TO_INODE(buf))));
//	L("after allocation, inode addr is %x. call cache_fix_indirect_leaf_indicator()", *((uint32_t*)TRANSACTION_GET_INO_ADDR_PTR(tid)));
	cache_fix_indirect_leaf_indicator(offset_indirect,
								      double_log_addr,
								      tid,
								      0); /* indirect is now cached, not active*/

	/* initialize write fields*/
	initWriteFields(tid);
	init_fsbuf(TRANSACTION_GET_INDIRECT_PTR(tid));
	init_fsbuf(	fs_buffer);

//	TRANSACTION_SET_IS_COMMITING(tid, 1);
//
//	/* when finished read new indirect buffer again to the transaction*/
//	entry_offset = calc_indirect_entry_offset(TRANSACTION_GET_FILE_OFFSET(tid));
//	FS_VERIFY(!fsReadBlock(new_indirect_log_addr,
//						   TRANSACTION_GET_INDIRECT_PTR(tid),
//						   tid,
//						   entry_offset,
//						   (TRANSACTION_GET_INO(tid) != 0)));
////	PRINT("\ncommitInode() - finished");
//
//	/* remove that indirect block from cache*/
//	RES_VERIFY(res, cache_remove_indirect_block(new_indirect_log_addr));

	/*return to normal state (not commiting) */
//	L("set is commiting back to %d", old_isCommiting);
	TRANSACTION_SET_IS_COMMITING(tid, old_isCommiting);
//	L("done succesfuly!");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * auxiliary to writeTid.
 * check if a given offset is in the indirect offset of the transaction tid
 * @param offset file offset
 * @param tid transaction id whose indirect offset we look at
 * @return 1 if yes, 0 if no
 */
bool_t isOffsetInIndirect(int32_t offset, int32_t tid){
	int32_t margin;

	/* if no offset is set in the transaction return negative answer*/
	if(IS_INDIRECT_EMPTY(tid)){
//		PRINT("\nisOffsetInIndirect() - indirect empty. return 0");
		return 0;
	}

	margin = offset-TRANSACTION_GET_FILE_OFFSET(tid);
//	PRINT_MSG_AND_NUM("\nisOffsetInIndirect() - offset=",offset);
//	PRINT_MSG_AND_NUM("\nisOffsetInIndirect() - TRANSACTION_GET_FILE_OFFSET(tid)=",TRANSACTION_GET_FILE_OFFSET(tid));
//	PRINT_MSG_AND_NUM(", margin=",margin);
//	PRINT_MSG_AND_NUM(", INDIRECT_DATA_OFFSET=",INDIRECT_DATA_OFFSET);
//	PRINT_MSG_AND_NUM(", INODE_INDIRECT_DATA_SIZE=",INODE_INDIRECT_DATA_SIZE);

	/* if current indirect is inode, check whther the new offset is mapped directly by it*/
	if(IS_INDIRECT_INODE(tid)){
//		PRINT_MSG_AND_NUM("\nisOffsetInIndirect() - return=",(offset < INDIRECT_DATA_OFFSET));
		return (offset < INDIRECT_DATA_OFFSET);
	}

	/* if we're dealing with offset after directly indexed data*/
//	PRINT_MSG_AND_NUM("\nisOffsetInIndirect() - return=",(margin>=0 && margin<INODE_INDIRECT_DATA_SIZE));
	return (margin>=0 && margin<INODE_INDIRECT_DATA_SIZE);
}

/**
 * @brief
 * auxiliary to writeTid().
 * find indirect block pointing to the direct block in offset block_offset
 * in the file used by transaction tid. write block to indirect block buffer of tid
 *
 * @param block_offset offset in file used by tid
 * @param tid transaction id
 * @return FS_ERROR_SUCCESS if successful
 *         -1 if file offset illegal or a read error occured
 */
error_t
findIndirectEntriesBlock(int32_t block_offset, int32_t tid){
	int32_t res;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	init_logical_address(indirect_addr);
//	FENT();

//	PRINT_MSG_AND_NUM("\nfindIndirectEntriesBlock() - starting. block_offset=",block_offset);
//	PRINT_MSG_AND_NUM(", tid=",tid);
//	PRINT_MSG_AND_NUM(", tid ino=",TRANSACTION_GET_INO(tid));
//	PRINT_MSG_AND_NUM(", tid file offset=",TRANSACTION_GET_FILE_OFFSET(tid));
//	PRINT_MSG_AND_NUM(", isOffsetInIndirect?=",isOffsetInIndirect(block_offset, tid));

	/* if we already have the indirect block, return*/
	if(isOffsetInIndirect(block_offset, tid)){
		return FS_ERROR_SUCCESS;
	}

	/* make sure we did not exceed file size*/
	FS_VERIFY(block_offset < FS_MAX_FILESIZE);

	/* do the actual indirect block read */
//	L("call readIndirectToBuffer(). indirect_addr %x", *((uint32_t*)(indirect_addr)));
	RES_VERIFY(res, readIndirectToBuffer(block_offset,
										 TRANSACTION_GET_INO_ADDR_PTR(tid),
										 TRANSACTION_GET_INDIRECT_PTR(tid),
										 indirect_addr,
										 tid,
										 FS_ACTIVE_INDIRECT_READ_YES));
//	L("readIndirectToBuffer() done.");

	/* remove indirect block from cache*/
//	L("call cache_remove_indirect_block() on addr %x", ADDR_PRINT(indirect_addr));
	RES_VERIFY(res, cache_remove_indirect_block(indirect_addr));
//	L("cache_remove_indirect_block() is finished.");

//	PRINT("\nfindIndirectEntriesBlock() - readIndirectToBuffer done");
	setIndirectOffset(block_offset, tid);
	TRANSACTION_SET_MAX_WRITTEN_OFFSET(tid,0);
	TRANSACTION_SET_BLOCKS_ALLOCS(tid,0);
//	PRINT("\nfindIndirectEntriesBlock() - finished");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * auxiliary to writeFileBlock().
 * get max offset written in a block write (part of transaction tid)
 * and change transaction max offset if necessary
 *
 * @param tid transaction id
 * @param maxOffset maximum offset written during some block write
 */
void changeTransactionMaxOffset(int32_t tid, int32_t maxOffset){
	if(maxOffset > TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid)){
		TRANSACTION_SET_MAX_WRITTEN_OFFSET(tid, maxOffset);
	}
}

/**
 * @brief
 * write transaction data block (actually first len bytes) file in transaction tid at block_offset
 *
 * @param block_offset file offset in blocks (+INODE_FILE_DATA_SIZE if not in inode)
 * @param len length of new data in block *
 * @param dataType type of data to write
 * @param tid transaction id
 * @param log_addr logical address of data written (empty if not written yet)
 * @param writeFlag indictor whther to actually write the new given address
 * @return FS_ERROR_SUCCESS if successful
 * 		  -1 if a read/write error occurs
 */
int32_t writeFileBlock(int32_t block_offset, int32_t len, int32_t dataType,int32_t tid, logical_addr_t log_addr, bool_t writeFlag){
	int32_t res, offset, maxOffset, org_blk_offset = block_offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(old_log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(TRANSACTION_GET_INDIRECT_PTR(tid));
//	FENT();

//	L("writeFlag %d, log_addr %x", writeFlag, ADDR_PRINT(log_addr));
	init_logical_address(old_log_addr);
//	L("block_offset=",block_offset);
//	PRINT_MSG_AND_NUM(" ino num=",TRANSACTION_GET_INO(tid));
//	PRINT_MSG_AND_NUM(", tid=", tid);
//	PRINT_MSG_AND_NUM(", tid offset=",TRANSACTION_GET_FILE_OFFSET(tid));
//	PRINT_MSG_AND_NUM(", offset in same indirect?=",isOffsetInIndirect(block_offset, tid));

	/* check if we should read indirect block again*/
	if(!isOffsetInIndirect(block_offset, tid)){
//		L("changing indirect block to the one containing block_offset=%d for ino=%d",block_offset, TRANSACTION_GET_INO(tid));
		/* commit old indirect if necessary, and read new indirect block to tid*/
		if(!IS_INDIRECT_EMPTY(tid)){
//			L("commiting inode before changing indirect");
			RES_VERIFY(res, commitInode(tid));
//			L("commiting inode before changing indirect done. new ino addr=%x", ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));
		}
//		PRINT_MSG_AND_NUM("\nwriteFileBlock() - ino0 addr b4 findIndirectEntriesBlock()=",logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//		L("call findIndirectEntriesBlock()");
		RES_VERIFY(res, findIndirectEntriesBlock(block_offset, tid));
//		L("findIndirectEntriesBlock() done successfully");
//		PRINT_MSG_AND_NUM("\nwriteFileBlock() - ino0 addr after findIndirectEntriesBlock()=",logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//		PRINT_MSG_AND_NUM("\nwriteFileBlock() - after findIndirectEntriesBlock(), transaction offset=",TRANSACTION_GET_FILE_OFFSET(tid));
	}
//	PRINT_MSG_AND_NUM("\nwriteFileBlock() - data write is not inode write?=",IS_ADDR_EMPTY(log_addr) && block_offset >= INODE_FILE_DATA_SIZE);

	/* now we definitely have the correct indirect block in place. */
	/* if we're writing to the inode, handle:
	 * a. writing inode file data
	 * b. writing direct entries*/
	if(IS_INDIRECT_INODE(tid)){
//		L("indirect is inode");
		/* if we wrote inode file data:
		 * - change inode file size if necessary
		 * - copy new inode block to indirect buffer */
		if(block_offset<INODE_FILE_DATA_SIZE){
//			PRINT("\nwriteFileBlock() - block_offset<INODE_FILE_DATA_SIZE");
			maxOffset = len;

			changeTransactionMaxOffset(tid, maxOffset);
			changeFileSize(tid, TRANSACTION_GET_INDIRECT_PTR(tid));
			TRANSACTION_SET_FILE_SIZE(tid, INODE_GET_NBYTES(ino_ptr));

			/* copy data from data buffer to inode the file data, and vice versa*/
			fsMemcpy(TRANSACTION_GET_INDIRECT_PTR(tid), TRANSACTION_GET_DATA_BUF_PTR(tid), INODE_FILE_DATA_SIZE);
			fsMemcpy(TRANSACTION_GET_DATA_BUF_PTR(tid)+INODE_FILE_DATA_SIZE, TRANSACTION_GET_INDIRECT_PTR(tid)+INODE_FILE_DATA_SIZE, FS_BLOCK_SIZE-INODE_FILE_DATA_SIZE);
//			PRINT("\nwriteFileBlock() - finished");

			/* no need to change number of blocks allocated.
			 * simply return*/
//			 PRINT("\nwriteFileBlock() - finished");
			return FS_ERROR_SUCCESS;
		}
		else{
//			L("write to direct entry #%d",CALC_IN_BLOCKS(block_offset));
			/* we wrote a direct block*/
			block_offset -= INODE_FILE_DATA_SIZE;
			INODE_GET_DIRECT(ino_ptr, CALC_IN_BLOCKS(block_offset), old_log_addr);

			/* write the block*/
			if(writeFlag){
				copyLogicalAddress(log_addr, old_log_addr);
//				L("writeFlag is 1. Actually write block to flash. call allocAndWriteBlockTid()");
		//		PRINT_MSG_AND_NUM(", block_offset=",block_offset);
		//		PRINT_MSG_AND_HEX("\nwriteFileBlock() - write page. first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);
				RES_VERIFY(res, allocAndWriteBlockTid(log_addr, TRANSACTION_GET_DATA_BUF_PTR(tid), dataType, CALC_IN_LOG_ADDRESSES(DIRECT_INDEX_LOCATION)+CALC_IN_BLOCKS(block_offset), tid));
//				L("block written to %x", ADDR_PRINT(log_addr));
			}

//			L("set direct entry #%d to %x in indirect buffer (which is inode)",CALC_IN_BLOCKS(block_offset), ADDR_PRINT(log_addr));
			INODE_SET_DIRECT(ino_ptr, CALC_IN_BLOCKS(block_offset), log_addr);
//			PRINT_MSG_AND_NUM("\nwriteFileBlock() - in inode block, set direct entry ",CALC_IN_BLOCKS(block_offset));
//			PRINT_MSG_AND_NUM(", address= ",logicalAddressToPhysical(log_addr));

			maxOffset = INODE_FILE_DATA_SIZE+block_offset+len;
		}
	}
	/* if we're writing some direct data we can simply update indirect block accordingly*/
	else{
//		L("indirect is not inode");
		block_offset -= INDIRECT_DATA_OFFSET;
		offset = CALC_OFFSET_IN_INDIRECT_BLOCK(CALC_OFFSET_IN_INDIRECT_SIZE(block_offset));

		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), offset, old_log_addr);
//		L("get indirect entry %d",offset);
//		if(IS_ADDR_EMPTY(old_log_addr)){	PRINT("\nwriteFileBlock() - direct entry empty");}
//		else{ PRINT_MSG_AND_NUM("\nwriteFileBlock() - old addr= ",logicalAddressToPhysical(old_log_addr));}

		/* write the block*/
		if(writeFlag){
			copyLogicalAddress(log_addr, old_log_addr);
//			L("writeFlag is 1. Actually write block to flash. call allocAndWriteBlockTid()");
	//		PRINT_MSG_AND_NUM(", block_offset=",block_offset);
//			L("write page. first byte=%x", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);
			RES_VERIFY(res, allocAndWriteBlockTid(log_addr, TRANSACTION_GET_DATA_BUF_PTR(tid), dataType, offset, tid));
//			L("block written to %x (%d)", ADDR_PRINT(log_addr), logicalAddressToPhysical(log_addr));
		}

//		L("set entry %d in indirect block to %x", offset, ADDR_PRINT(log_addr));
		BLOCK_SET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), offset, log_addr);
//		L("in indirect, set direct entry %d to address=%d",offset, logicalAddressToPhysical(log_addr));
//		PRINT_MSG_AND_NUM(" ",);
		maxOffset = CALC_OFFSET_IN_INDIRECT_SIZE(block_offset)+len;
//		PRINT_MSG_AND_NUM("\nwriteFileBlock() - new maxOffset=",maxOffset);
	}
//	PRINT_MSG_AND_NUM("\nwriteFileBlock() - ino0 addr after writing block and setting indirect=",logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));

	/* if we've overflowed last written offset, change it*/
	changeTransactionMaxOffset(tid, maxOffset);

//	PRINT("\nwriteFileBlock() - changeTransactionMaxOffset() finished");
//	if(IS_NOT_REWRITE(old_log_addr)){
//		if(TRANSACTION_GET_INO(tid)==0){
//			PRINT_MSG_AND_HEX("\nwriteFileBlock() - overwritten fresh address = ",*CAST_TO_UINT32(old_log_addr));
//		}
//		PRINT("\nwriteFileBlock() - increase nblocks.");
//		TRANSACTION_INC_BLOCKS_ALLOCS(tid);
//	}
//	if(!IS_ADDR_EMPTY(old_log_addr)) PRINT_MSG_AND_NUM("\nwriteFileBlock() - old_log_addr=", logicalAddressToPhysical(old_log_addr));

	/* if we wrote a new block instead of an old one (which wasn't previously used in a spare file)
	 * increment count in transaction.
	 * NOTICE - dont vot inode in inode0 again. this was performed when the inode was commited*/
	if(!IS_ADDR_EMPTY(old_log_addr) && !IS_INODE0(TRANSACTION_GET_INO(tid))){
//		L(" vot old direct entry");
		RES_VERIFY(res, writeVot(old_log_addr,tid));
	}
//	PRINT_MSG_AND_NUM("\nwriteFileBlock() - is the block new?=", !IS_BLOCK_REWRITE(org_blk_offset, tid));
//	PRINT_MSG_AND_NUM(" t_fileSize=", TRANSACTION_GET_FILE_SIZE(tid));
//	PRINT_MSG_AND_NUM(" org_blk_offset=", org_blk_offset);

	/* increase physical data blocks case if we are not re-writing a block */
	if(!IS_BLOCK_REWRITE(org_blk_offset, tid)){
//		PRINT("\nwriteFileBlock() - inc blocks count");
		TRANSACTION_INC_BLOCKS_ALLOCS(tid);
	}

	TRANSACTION_CALC_FILE_SIZE(tid);
//	if(TRANSACTION_GET_INO(tid)==0){
//		PRINT_MSG_AND_NUM("\nwriteFileBlock() - for ino0 block count is now ", TRANSACTION_GET_BLOCKS_ALLOCS(tid));
//		PRINT_MSG_AND_NUM(", max offset=", TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid));
//	}
//	L("done");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * commit a directory entry as part of transaction tid.
 *
 * Assumptions:
 * 1. directory inode num is set in the transaction
 * 2. directroy address is set in transaction inode address
 * 3. indirect buffer is empty, and it's offset is negative
 *
 * @param ino_num direntry inode number
 * @param f_name direntry file name
 * @param f_type direntry file type
 * @param tid transaction id
 * @return FS_ERROR_SUCCESS if successful
 * 		   -1 if a read/write error occured
 */
int32_t
writeDirEntry(int32_t ino_num,
			  uint8_t *f_name,
			  uint32_t f_type,
			  int32_t tid){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	int32_t res, offset;
	uint8_t *data_buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(data_buf);
//	FENT();

	init_logical_address(log_addr);
//	L("starting. ino_num=%d", ino_num);
//	PRINT_MSG_AND_STR(" f_name=", f_name);
//	PRINT_MSG_AND_NUM(" f_type=", f_type);
//	PRINT_MSG_AND_NUM(" tid=", tid);
	/* - find sparse block in in the parent directory
	 * - read indirect block containing the block in that offset
	 * - read sparse block from directory file (and set inode address in transaction)
	 * - set it in the transaction data buf.
	 * - read indirect block
	 * - and set new directory entry */
	FS_VERIFY(!findEmptySparseBlock(TRANSACTION_GET_INO(tid), log_addr, &offset, tid));
//	L("found sparse block at offset=%d", offset);
//	PRINT_MSG_AND_NUM(" for ino=", TRANSACTION_GET_INO(tid));

	RES_VERIFY(res, findIndirectEntriesBlock(offset, tid));
//	L("read sparse blcok");
	RES_VERIFY(res, readFileBlock(data_buf,
								  TRANSACTION_GET_INO(tid),
								  offset,
								  TRANSACTION_GET_INO_ADDR_PTR(tid),
								  tid));

//	/* */
//	if(!IS_DIRENT_EMPTY(dirent_ptr)){
//		writeVot(log_addr, tid);
//	}

	/* set new direntry in buffers*/
	while(!IS_DIRENT_EMPTY(dirent_ptr)){
//		L("another moveToNextDirentry");
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}
//	L("call setNewDirentry()");
	setNewDirentry(dirent_ptr, ino_num, f_type, f_name);

//	{
//		int32_t i;
//		for(i=0; i< FS_BLOCK_SIZE; i++){
//			if(buf[i] !=0xff){
//				PRINT_MSG_AND_NUM("\n", i);
//				PRINT_MSG_AND_NUM(". ", buf[i]);
//			}
//		}
//	}
//
	/* write this block as a transactional write
	 * and commit the inode to flash. save new dir inode address.
	 * NOTICE - we write a block in length FS_BLOCK_SIZE. there's no point
	 * in calculating actual written length in a file with sparse blocks*/
//	L("write direntry block");
	init_logical_address(log_addr);
	TR_RES_VERIFY(res, writeFileBlock(offset, FS_BLOCK_SIZE, DATA_TYPE_DIRENTS, tid, log_addr, 1));

//	L("finished. block written to %x", ADDR_PRINT(log_addr));
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * commit to inode0 (in address ino0_log_addr) a file whose inode is in ino_log_addr, with number ino_num.
 * the transaction temporarily changes to inode0, reads indirect inode etc and writes
 * the new inode. finally we commit the inode, and save the new inode0 address.
 *
 * Assumptions:
 * 1. all transaction related cached blocks were flushed
 * 2.file inode was already written
 *
 * @param ino0_log_addr address of inode0, and where the new inode0 address will be stored.
 * @param ino_num inode number of file to be commited
 * @param ino_log_addr address of inode with ino_num id
 * @param tid id of transaction in which we commit the inode
 * @return FS_ERROR_SUCCESS if successful.
 * 		   various errors otherwise
 */
error_t
commitInodeToInode0(logical_addr_t ino0_log_addr,
					int32_t ino_num,
					logical_addr_t ino_log_addr,
					int32_t tid){
	int32_t ino_offset, res;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	init_logical_address(log_addr);
//	FENT();
//	L("@@@@@@@@@@@@@@ tid %d. committing ino %d with address %x. old ino0_log_addr %x", tid, ino_num, ADDR_PRINT(ino_log_addr), ADDR_PRINT(ino0_log_addr));

	/* set inode0 details to transaction*/
	TRANSACTION_SET_INO(tid, 0);
	TRANSACTION_SET_INO_ADDR(tid, ino0_log_addr);
	TRANSACTION_SET_FILE_OFFSET(tid,-1);
	RES_VERIFY(res, fsReadBlock(ino0_log_addr,
								fs_buffer,
								tid,
								CACHE_ENTRY_OFFSET_EMPTY,
								FLAG_CACHEABLE_READ_YES));
	TRANSACTION_SET_FILE_SIZE(tid, INODE_GET_NBYTES(ino_ptr));
	TRANSACTION_SET_FTYPE(tid, FTYPE_FILE);
//	L("set file size=%d", INODE_GET_NBYTES(ino_ptr));

	/* read file block, and indirect block */
	ino_offset = CALC_OFFSET_FROM_INODE(ino_num);
	RES_VERIFY(res, findIndirectEntriesBlock(ino_offset, tid));

//	PRINT_MSG_AND_NUM("\ncommitInodeToInode0() - ino num=", ino_num);
//	if(TRANSACTION_GET_INO(tid)==0){
//		L("for inode0: is indirect inode? %d", IS_INDIRECT_INODE(tid));
//		PRINT_MSG_AND_NUM(" transaction offset=", TRANSACTION_GET_FILE_OFFSET(tid));
//		PRINT_MSG_AND_NUM(" ino_offset=", ino_offset);
//		assert(0);
//		inode_t *ino_ptr = CAST_TO_INODE(TRANSACTION_GET_INDIRECT_PTR(tid));
//		PRINT_MSG_AND_NUM(" nblocks=", INODE_GET_NBLOCKS(ino_ptr));
//		PRINT_MSG_AND_NUM(" nbytes=", INODE_GET_NBYTES(ino_ptr));
//		PRINT_MSG_AND_NUM(" tid=", tid);
//	}
//	PRINT_MSG_AND_NUM("\ncommitInodeToInode0() - read indirect block at offset ", ino_offset);
//	PRINT_MSG_AND_NUM("\ncommitInodeToInode0() - is indirect inode?", IS_INDIRECT_INODE(tid));

	/* - vot old inode0 address
	 * - write new inode address to indirect block
	 * - commit inode0 and save address to ino0_log_addr
	 * - return*/
//	L("write to inode0 file block, whose  address %x", ADDR_PRINT(ino_log_addr));
	RES_VERIFY(res, writeFileBlock(ino_offset, FS_BLOCK_SIZE, DATA_TYPE_EMPTY, tid, ino_log_addr, 0));
//	L("write done");
//	PRINT_MSG_AND_NUM("\ncommitInodeToInode0() - after writeFileBlock() max written offset=", TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid));
//	L("FINALLY commit Inode0");
//	RES_VERIFY(res, writeVot(ino0_log_addr, tid));
	RES_VERIFY(res, commitInode(tid));
//	L("commit Inode0 done");
//	PRINT("\ncommitInode() - vot old inode0");
	copyLogicalAddress(ino0_log_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));
//	L("@@@@@@@@@@@@@@ finished. ino0_log_addr is now %x", ADDR_PRINT(ino0_log_addr));

	/* return successfuly*/
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * commit transaction.
 * - commit inode to flash
 * - commit inode to inode0
 * - change inode0 address to newly written address
 *
 * @param tid transaction id to commit
 * @param ino0_old_addr olf address of inode0
 * @param file_ino_addr file inode address
 * @param isFinal is the commit final ,or temporary
 * @return FS_ERROR_SUCCESS if successful
 */
int32_t commitTransaction(int32_t tid, logical_addr_t ino0_old_addr, bool_t isFinal){
	int32_t res, offset = 0, f_type = 0;
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(file_ino_addr);
	int32_t ino_num = TRANSACTION_GET_INO(tid), t_type = TRANSACTION_GET_TYPE(tid);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
//	FENT();
	init_logical_address(file_ino_addr);
//	L("COMMIT TRANSACTION %d, INO0_OLD_ADDR %x", tid, ADDR_PRINT(ino0_old_addr));
	/* check if we are already commiting an inode, or transaction
	 * used to prevent nested commits in temporary commit*/
	if(TRANSACTION_GET_IS_COMMITING(tid)){
		return FS_ERROR_SUCCESS;
	}

	/* indentify transaction as in the middle of commiting*/
	TRANSACTION_SET_IS_COMMITING(tid, 1);

	/* if temporary commit, mark it in transaction type so we can allocate
	 * regardless of minimum free pages status.
	 * NOTICE - we only perform temporary commit after previously verifying
	 * we will have achieved enough free pages after committing   */
	if(!isFinal){
//		PRINT("\ncommitTransaction() - starting temp commit");
//		PRINT_MSG_AND_NUM(", vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
//		PRINT_MSG_AND_NUM(", frees=", calcTotalFreePages());
		TRANSACTION_SET_TYPE(tid, T_TYPE_TEMP);
		offset = TRANSACTION_GET_FILE_OFFSET(tid);
		f_type = TRANSACTION_GET_FTYPE(tid);
	}

//	PRINT_MSG_AND_NUM("  tid=", tid);
//	PRINT_MSG_AND_NUM("  ino=", TRANSACTION_GET_INO(tid));
//	PRINT_MSG_AND_NUM("  t_type=", TRANSACTION_GET_TYPE(tid));
//	PRINT_MSG_AND_NUM("  f_type=", TRANSACTION_GET_FTYPE(tid));
//	PRINT_MSG_AND_NUM(", isFinal=", isFinal);
//	PRINT_MSG_AND_NUM(", vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
//	PRINT_MSG_AND_HEX("\ncommitTransaction() - first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);
//	PRINT_MSG_AND_NUM("\ncommitTransaction() - commiting for inode=", TRANSACTION_GET_INO(tid));

	/* commit inode */
//	L("do commitInode() for file inode");
	TR_RES_VERIFY(res, commitInode(tid));
//	L("commitInode() done");
//	PRINT_MSG_AND_NUM("\nafter commitInode() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
//	PRINT_MSG_AND_HEX("\ncommitTransaction() - 1. write page. first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);

	/* flush all transaction related cached blocks.
	 * inode0 changes are not cached, so this is ok*/
//	L("flush all transaction related cached blocks, call cache_flush_transaction_blocks()");
	RES_VERIFY(res, cache_flush_transaction_blocks(tid));
//	L("done flusing");
//	{
//		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//		SET_LOGICAL_OFFSET(log_addr, 9);
//		SET_LOGICAL_SEGMENT(log_addr, 0);
//		uint8_t buf[FS_BLOCK_SIZE];
//		fsReadBlockSimple(log_addr, buf);
//		INODE_GET_DIRECT(CAST_TO_INODE(buf), 0, log_addr);
//		L("firt direct entry of inode is %x", ADDR_PRINT(log_addr));
//		assert(!IS_CACHED_ADDR(log_addr));
//	}
	copyLogicalAddress(file_ino_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));
//	L("commited inode to addr %x",*((uint32_t*)file_ino_addr));

	/* final commit inode0 with pointer to re-written directory inode*/
//	L("final commit of inode0, call commitInodeToInode0()");
	TR_RES_VERIFY(res, commitInodeToInode0(ino0_old_addr, TRANSACTION_GET_INO(tid), file_ino_addr, tid));
//	PRINT_MSG_AND_NUM("\nafter commitInodeToInode0() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
//	PRINT("\ncommitTransaction() - commitInodeToInode0 success");
//	PRINT_MSG_AND_NUM("\ncommitTransaction() - new ino0 address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//	PRINT_MSG_AND_NUM("\ncommitTransaction() - write vots buffer?=", !IS_VOTS_BUF_EMPTY(tid));

	/* write final vots buffer (if necessary)*/
	if(!IS_VOTS_BUF_EMPTY(tid)){
//		L("write final vots buffer");
		RES_VERIFY(res, allocAndWriteBlockTid(TRANSACTION_GET_PREV_ADDR_PTR(tid),
											  TRANSACTION_GET_VOTS_BUF_PTR(tid),
											  DATA_TYPE_VOTS,
											  CACHE_ENTRY_OFFSET_EMPTY, /* we won't cache this so it doesn't matter*/
											  tid));
//		PRINT_MSG_AND_NUM("\nafter last vots block write vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
//		L("wrote VOTs to %x", ADDR_PRINT(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
	}

	/* set file system:
	 * - new ino0 address
	 * - last written block address of closed transaction
	 * - open transactions last written block addresses*/
	FS_SET_INO0_ADDR(ino0_old_addr);
	FS_SET_LAST_CLOSED_TID_ADDR(TRANSACTION_GET_PREV_ADDR_PTR(tid));

//	PRINT_MSG_AND_NUM("\ncommitTransaction() - after set file system, tid prev=", logicalAddressToPhysical(TRANSACTION_GET_PREV_ADDR_PTR(tid)));
//	PRINT_MSG_AND_NUM("\ncommitTransaction() - new inode0 address=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//
//	PRINT_MSG_AND_NUM("\ncommitTransaction() - wrote inode 0 to=", logicalAddressToPhysical(FS_GET_INO0_ADDR_PTR()));
//	PRINT_MSG_AND_NUM("\ncommitTransaction() - b4 fsCheckpointWriter() frees=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM(", obs=",GET_OBS_COUNT());
	/* write checkpoint.
	 * we temporarily change obsolete count before writing, for consistency*/
//	L("write checkpoint");
	TR_RES_VERIFY(res, fsCheckpointWriter(0));
//	PRINT_MSG_AND_NUM("\ncommitTransaction() - after fsCheckpointWriter() frees=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM(", obs=",GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM("\nafter fsCheckpointWriter() vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
//	PRINT("\ncommitTransaction() - wrote checkpoint");
//	PRINT("\ncommitTransaction() - finished fsCheckpointWriter()");
//	PRINT("\ncommitTransaction() - about to handleTransactionVOTs()");
//	PRINT_MSG_AND_HEX("\ncommitTransaction() -  write page. first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);

	/* do transaction vots*/
//	L("do transaction vots, call andleTransactionVOTs()");
//	PRINT_MSG_AND_NUM(". vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
//	PRINT_MSG_AND_NUM(", frees=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM(", obs count=", GET_OBS_COUNT());
	RES_VERIFY(res, handleTransactionVOTs(TRANSACTION_GET_PREV_ADDR_PTR(tid), FLAG_FROM_REBOOT_NO));
//	L("finished handleTransactionVOTs()");
//	PRINT_MSG_AND_NUM(", frees=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM(", obs=",GET_OBS_COUNT());

	/* and prepare sequencing layer for next write */
//	L("prepare sequencing layer for next write, call findNextFreePage()");
	RES_VERIFY(res, findNextFreePage(&cpWritten, fsCheckpointWriter, 0));

//	PRINT_MSG_AND_NUM("\ncommitTransaction() - after findNextFreePage() frees=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM(", obs=",GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM("\ncommitTransaction() - after findNextFreePage() obs count=", GET_OBS_COUNT());
//	PRINT("\ncommitTransaction() - finished findNextFreePage()");
//	PRINT_MSG_AND_HEX("\ncommitTransaction() -  after findNextFreePage first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);
	if(!isFinal){
		fsMemcpy(fs_buffer,TRANSACTION_GET_DATA_BUF_PTR(tid), FS_BLOCK_SIZE);
	}

//	PRINT_MSG_AND_NUM("\ncommitTransaction() - before init transaction, vots count=", TRANSACTION_GET_VOTS_COUNT(0));
	init_transaction(tid);
	init_logical_address(FS_GET_LAST_CLOSED_TID_ADDR());
	init_logical_address(FS_GET_TID_LAST_WRITTEN_ADDR(tid));

	/* write new checkpoint, with final free pages count and no closed transaction marked
	 * Logic - when writing first checkpoint the obsolete pages count is not consistent with the count after
	 * doing all VOTs. If we weite pages after the checkpoint, and then crash, on reboot we will lose the
	 * optimization of not re-doing VOTs if any page was written after the checkpoint.*/
//	L("write 2nd cp");
	TR_RES_VERIFY(res, fsCheckpointWriter(0));
//	L("2nd findNextFreePage() ");
	RES_VERIFY(res, findNextFreePage(&cpWritten, fsCheckpointWriter, 0));

	/* if this is a temporary commit, restore minimum relevant transaction data -
	 * - data buffer
	 * - inode number
	 * - transaction type
	 * - inode address
	 * - file size
	 * - indirect block
	 * - prev written address*/
	if(!isFinal){
//		PRINT("\ncopy transaction details");
		fsMemcpy(TRANSACTION_GET_DATA_BUF_PTR(tid), fs_buffer, FS_BLOCK_SIZE);
//		PRINT_MSG_AND_HEX("\ncommitTransaction() -  after first byte=", TRANSACTION_GET_DATA_BUF_PTR(tid)[0]);
		TRANSACTION_SET_INO(tid, ino_num);
		TRANSACTION_SET_TYPE(tid, t_type);
		TR_RES_VERIFY(res, getInode(fs_buffer, ino_num, TRANSACTION_GET_INO_ADDR_PTR(tid)));
		TRANSACTION_SET_FILE_SIZE(tid, INODE_GET_NBYTES(ino_ptr));
		TRANSACTION_SET_FTYPE(tid, f_type);
		TR_RES_VERIFY(res, findIndirectEntriesBlock(offset, tid));
//		PRINT("\ncommitTransaction() - finished temp commit");
//		PRINT_MSG_AND_NUM(", vots count=", TRANSACTION_GET_VOTS_COUNT(tid));
//		PRINT_MSG_AND_NUM(", frees=", calcTotalFreePages());
	}

//	PRINT_MSG_AND_NUM("\ncommitTransaction() - finished. max written ofset",TRANSACTION_GET_MAX_WRITTEN_OFFSET(tid));
	/* indentify transaction as not in the middle of commiting*/
	TRANSACTION_SET_IS_COMMITING(tid, 0);

//	PRINT_MSG_AND_NUM(" ino_num=", TRANSACTION_GET_INO(tid));
//	L("done!");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * read to buf a block mapped to offset by indirect block of transaction tid
 *
 * @param buf buffer to read data to
 * @param offset file offset to read block from
 * @param tid transaction id
 * @return 0 if successful, FS_ERROR_IO if an io error occured
 */
int32_t readFromTransactionIndirectRead(uint8_t *buf, int32_t offset, int32_t tid){
	int32_t block_offset, res;
	int32_t entry_offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(direct_addr);
	inode_t *ino_ptr = CAST_TO_INODE(TRANSACTION_GET_INDIRECT_PTR(tid));

	init_logical_address(direct_addr);

//	PRINT_MSG_AND_NUM("\nreadFromTransactionIndirectRead() - read from file offset=", offset);
	/* calculate block start of offset*/
	block_offset = offset-CALC_OFFSET_IN_FILE_BLOCK(offset);
//	PRINT_MSG_AND_NUM(", block_offset=", block_offset);
//	PRINT_MSG_AND_NUM(", tid ino_num=", TRANSACTION_GET_INO(tid));

//	PRINT_MSG_AND_NUM("\nreadFromTransactionIndirectRead() - indirect is inode?=", IS_INDIRECT_INODE(tid));
	if(IS_INDIRECT_INODE(tid)){
		/* if we read inode file data, copy file data from inode to buffer*/
		if(block_offset<INODE_FILE_DATA_SIZE){
//			PRINT("\nreadFromTransactionIndirectRead() - read from inode file data");
			fsMemcpy(buf, TRANSACTION_GET_INDIRECT_PTR(tid), INODE_FILE_DATA_SIZE);
			return FS_ERROR_SUCCESS;
		}

		/* otherwise we read a direct block*/
		block_offset -= INODE_FILE_DATA_SIZE;
		entry_offset = CALC_IN_BLOCKS(block_offset);
//		PRINT_MSG_AND_NUM("\nreadFromTransactionIndirectRead() - read direct entry ",CALC_IN_BLOCKS(block_offset));
		INODE_GET_DIRECT(ino_ptr, entry_offset, direct_addr);
//		PRINT_MSG_AND_NUM(", is it empty?",IS_ADDR_EMPTY(direct_addr));
	}
	/* if we're reading some direct data*/
	else{
		block_offset -= INDIRECT_DATA_OFFSET;
		entry_offset = CALC_OFFSET_IN_INDIRECT_BLOCK(CALC_OFFSET_IN_INDIRECT_SIZE(block_offset));

		BLOCK_GET_INDEX(TRANSACTION_GET_INDIRECT_PTR(tid), entry_offset, direct_addr);
	}
//	PRINT_MSG_AND_NUM("\nreadFromTransactionIndirectRead() - read from ",logicalAddressToPhysical(direct_addr) );
	RES_VERIFY(res, fsReadBlock(direct_addr,
								buf,
								tid,
								entry_offset,
								FLAG_CACHEABLE_READ_YES));

	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * vot all file blocks of file of transaction tid
 *
 * @param tid transaction id
 * @return 0 if successful,-1 if an error occured, read errors if a read error occured
 */
int32_t votFile(int32_t tid){
	int32_t f_size, i, j, k;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(direct_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(indirect_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(double_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(triple_addr);
	inode_t *ino_ptr = CAST_TO_INODE(TRANSACTION_GET_INDIRECT_PTR(tid));
	int32_t ino_num  = TRANSACTION_GET_INO(tid);
	uint8_t *temp_double_buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
//	FENT();

	init_logical_address(ino_addr);
	init_logical_address(direct_addr);
	init_logical_address(indirect_addr);
	init_logical_address(double_addr);
	init_logical_address(triple_addr);

//	f_size = getFileSize(ino_num);
//	PRINT_MSG_AND_NUM("\nvotFile() - tid=",tid);
//	PRINT_MSG_AND_NUM(", ino_num=",ino_num);
	FS_VERIFY(!getInode(TRANSACTION_GET_INDIRECT_PTR(tid), ino_num, ino_addr));
//	PRINT_MSG_AND_NUM(", ino_addr=",logicalAddressToPhysical(ino_addr));
	f_size = INODE_GET_NBYTES(ino_ptr);
	INODE_GET_INDIRECT(ino_ptr, indirect_addr);
	INODE_GET_DOUBLE(ino_ptr, double_addr);
	INODE_GET_TRIPLE(ino_ptr, triple_addr);

//	/* vot inode*/
//	if(!IS_ADDR_EMPTY(direct_addr)){ PRINT_MSG_AND_NUM("\nvotFile() - vot inode=", logicalAddressToPhysical(direct_addr));}
//	L("vot ino_addr %x", ADDR_PRINT(ino_addr));
	FS_VERIFY(!writeVot(ino_addr, tid));

	/* if file has no data besides inode file data */
	if(f_size <= INODE_FILE_DATA_SIZE){
		return 0;
	}

	/* vot direct entries*/
	for(i=0; i< DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr, i, direct_addr);
//		L("vot direct #%d %x", i, ADDR_PRINT(direct_addr));
		FS_VERIFY(!writeVot(direct_addr, tid));
	}

	/* if file has no data after direct entries */
	if(f_size <= INDIRECT_DATA_OFFSET){
		return FS_ERROR_SUCCESS;
	}

	/* vot indirect entries*/
	FS_VERIFY(!fsReadBlock(indirect_addr,
						   fs_buffer,
						   tid,
						   INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						   FLAG_CACHEABLE_READ_YES));
	for(i=0; i< LOG_ADDRESSES_PER_BLOCK;i++){
		BLOCK_GET_INDEX(fs_buffer, i, direct_addr);
//		if(!IS_ADDR_EMPTY(direct_addr)){ PRINT_MSG_AND_NUM("\nvotFile() - vot indirect address=", logicalAddressToPhysical(direct_addr));}
//		L("vot indirect-direct #%d %x", i, ADDR_PRINT(direct_addr));
		FS_VERIFY(!writeVot(direct_addr, tid));
	}
//	PRINT_MSG_AND_NUM("\nvotFile() - vot indirect block address=", logicalAddressToPhysical(indirect_addr));
//	L("vot indirect %x", ADDR_PRINT(indirect_addr));
	FS_VERIFY(!writeVot(indirect_addr, tid));

	/* if file has no data after direct entries */
	if(f_size <= DOUBLE_DATA_OFFSET){
//		PRINT("\nf_size <= DOUBLE_DATA_OFFSET. return");
		return 0;
	}
//	PRINT("\nvotFile() - continue to vot double entries");
	/* vot double entries*/
	FS_VERIFY(!fsReadBlock(double_addr,
						   temp_double_buf,
						   tid,
						   DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
						   FLAG_CACHEABLE_READ_YES));
	for(j=0; j< LOG_ADDRESSES_PER_BLOCK; j++){
		BLOCK_GET_INDEX(temp_double_buf, j, indirect_addr);

		if(IS_ADDR_EMPTY(indirect_addr)){
			continue;
		}
		FS_VERIFY(!fsReadBlock(indirect_addr,
							   fs_buffer,
							   tid,
							   j,
							   FLAG_CACHEABLE_READ_YES));
		for(i=0; i< LOG_ADDRESSES_PER_BLOCK;i++){
			BLOCK_GET_INDEX(fs_buffer, i, direct_addr);
//			L("vot double-indirect-direct #%d %x", i, ADDR_PRINT(direct_addr));
			FS_VERIFY(!writeVot(direct_addr, tid));
		}
//		L("vot double-indirect #%d %x", j, ADDR_PRINT(indirect_addr));
		FS_VERIFY(!writeVot(indirect_addr, tid));
	}
	FS_VERIFY(!writeVot(double_addr, tid));
//	L("vot double %x", ADDR_PRINT(indirect_addr));

	/* if file has no data after direct entries */
	if(f_size <= TRIPLE_DATA_OFFSET){
//		L("no more entries. done");
		return 0;
	}

	/* vot triple entries*/
	for(k=0; k<LOG_ADDRESSES_PER_BLOCK; k++){
		FS_VERIFY(!fsReadBlock(triple_addr,
							   fs_buffer,
							   tid,
							   TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES,
							   FLAG_CACHEABLE_READ_YES));
		BLOCK_GET_INDEX(fs_buffer, k, double_addr);

		if(IS_ADDR_EMPTY(double_addr)){
			continue;
		}
		FS_VERIFY(!fsReadBlock(double_addr,
							   temp_double_buf,
							   tid,
							   k,
							   FLAG_CACHEABLE_READ_YES));
		for(j=0; j< LOG_ADDRESSES_PER_BLOCK; j++){
			BLOCK_GET_INDEX(temp_double_buf, j, indirect_addr);

			if(IS_ADDR_EMPTY(indirect_addr)){
				continue;
			}
			FS_VERIFY(!fsReadBlock(indirect_addr,
							       fs_buffer,
							       tid,
								   j,
								   FLAG_CACHEABLE_READ_YES));
			for(i=0; i< LOG_ADDRESSES_PER_BLOCK;i++){
				BLOCK_GET_INDEX(fs_buffer, i, direct_addr);
				FS_VERIFY(!writeVot(direct_addr, tid));
			}
			FS_VERIFY(!writeVot(indirect_addr, tid));
		}

		FS_VERIFY(!writeVot(double_addr, tid));
	}
	FS_VERIFY(!writeVot(triple_addr, tid));
//	PRINT("\nvotFile() - finished");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * auxiliary to deleteDirEntry. remove a directory entry from a given block, by writing it
 * over with all the following datas
 *
 * @param dirent_ptr directory entry pointer in data buffer
 * @param de_offset direntry offste in block
 * @param data_buf data buffer
 */
void deleteDeFromBlock(int32_t de_len, int32_t de_offset, uint8_t *data_buf){
	int32_t past_de_data_len = FS_BLOCK_SIZE-(de_offset+de_len);
//	PRINT("\ndeleteDeFromBlock() - starting.");
//	PRINT_MSG_AND_NUM(" de_offset=",de_offset);
//	PRINT_MSG_AND_NUM(" de_len=",de_len);
//	PRINT_MSG_AND_NUM(" past_de_data_len=",past_de_data_len);

	/* copy from next directory entry over */
//	PRINT("\ndeleteDirEntry() - b4 fsMemcpy() print block");
//	printBlock(data_buf);
	fsMemcpy(&(data_buf[de_offset]), &(data_buf[de_offset+de_len]), past_de_data_len);
//	PRINT_MSG_AND_NUM("\nb4 fsMemset() vots offset=",TRANSACTION_GET_VOTS_OFFSET(0));
	/* initialize all following data*/
//	PRINT("\ndeleteDirEntry() - b4 fsMemset() print block");
//	printBlock(data_buf);
	fsMemset(&(data_buf[de_offset+past_de_data_len]), 0xff, de_len);
//	PRINT_MSG_AND_NUM("\nafter fsMemset() vots offset=",TRANSACTION_GET_VOTS_OFFSET(0));


}

/**
 * @brief
 * auxiliary to unlink().
 * delete directory entry in offset de_offset from file with inode id ino_num
 * as part of transaction tid.
 *
 * @param ino_num file inode number
 * @param de_offset. offset in directory of block containig direntry to delete
 * @param tid transaction id
 * @return FS_ERROR_SUCCESS if usccessful
 * 		   -1 if errors
 */
int32_t deleteDirEntry(int32_t ino_num, int32_t de_offset, int32_t tid){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	int32_t res;
	uint8_t *data_buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(data_buf);
//	FENT();

	init_logical_address(log_addr);
//	PRINT_MSG_AND_NUM(", ino_num=",ino_num);
//	PRINT_MSG_AND_NUM(", de_offset=",de_offset);
//	PRINT_MSG_AND_NUM(", tid=",tid);
//	PRINT_MSG_AND_NUM(", vots offset=",TRANSACTION_GET_VOTS_OFFSET(tid));

	/* find indirect block of direntry block*/
	RES_VERIFY(res, findIndirectEntriesBlock(de_offset, tid));
//	PRINT_MSG_AND_NUM("\ndeleteDirEntry() - found indirect block. transaction offset=",TRANSACTION_GET_FILE_OFFSET(tid))

	/* read the block*/
//	PRINT_MSG_AND_NUM("\ndeleteDirEntry() - read entries block from de_offset=",de_offset);
	RES_VERIFY(res, readFileBlock(data_buf, TRANSACTION_GET_INO(tid), de_offset, TRANSACTION_GET_INO_ADDR_PTR(tid), tid));
//	PRINT("\ndeleteDirEntry() - read direntry block to data buffer");

	while(!IS_DIRENT_EMPTY(dirent_ptr) && DIRENT_GET_INO_NUM(dirent_ptr) != ino_num){
//		PRINT_MSG_AND_STR("\ndeleteDirEntry() - entry name=",DIRENT_GET_NAME(dirent_ptr));
//		PRINT_MSG_AND_NUM(" len=",DIRENT_GET_LEN(dirent_ptr));
		moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	}

//	PRINT("\ndeleteDirEntry() - b4 deleteDeFromBlock() print block");printBlock(data_buf);
	/* initialize direntry */
	deleteDeFromBlock(DIRENT_GET_LEN(dirent_ptr), CALC_OFFSET_IN_FILE_BLOCK(de_offset), data_buf);
//	PRINT("\ndeleteDeFromBlock() - finished. print block"); printBlock(data_buf);

	init_logical_address(log_addr);
	/* if we still have directory entries in the block, write this block as a transactional write
	 * and commit the inode to flash. save new dir inode address.
	 * NOTICE - we write a block in length FS_BLOCK_SIZE. there's no point
	 * in calculating actual written length in a file with sparse blocks*/
	dirent_ptr = CAST_TO_DIRENT(data_buf);
	if(!IS_DIRENT_EMPTY(dirent_ptr)){
//		PRINT_MSG_AND_NUM("\ndeleteDirEntry() - entries block not empty. rewrite block to offset ", de_offset-CALC_OFFSET_IN_FILE_BLOCK(de_offset))
		TR_RES_VERIFY(res, writeFileBlock(de_offset-CALC_OFFSET_IN_FILE_BLOCK(de_offset), FS_BLOCK_SIZE, DATA_TYPE_DIRENTS, tid, log_addr, 1));
	}
	/* no directory entry in block. we write an empty address as the new block address*/
	else{
		/* write "new" (empty) address of direntries block */
		TR_RES_VERIFY(res, writeFileBlock(de_offset-CALC_OFFSET_IN_FILE_BLOCK(de_offset), FS_BLOCK_SIZE, DATA_TYPE_DIRENTS, tid, log_addr, 0));
	}

//	L("finished");
	return FS_ERROR_SUCCESS;
}

/**
 * @brief
 * auxiliary to creat() and mkdir(). creates new file according to given pathname and type
 *
 * @param new_type expected new file type
 * @param pathname pathname for a file.
 *
 * @return if file type is file return the file descriptor on success. if directory 0 is returned on success.
 * return FS_ERROR_SUCCESS if we create a directory succesfuly
 * 		  FS_ERROR_IO if an io error occurs
 * 		  -1 if other error occurs
 * 		  open file descriptor if we create a file succesfuly
 */
int32_t createNewFile(int32_t new_type, uint8_t *pathname){
	int32_t tid, ino_num, dir_num, res = 0, de_offset;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_log_addr);
	int32_t offset, nameLen;
	uint32_t name_offset, f_type;
	uint8_t *buf;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
//	FENT();

	init_logical_address(prev_log_addr);
	init_logical_address(log_addr);
	init_logical_address(ino_log_addr);
	init_logical_address(dir_log_addr);
	init_logical_address(ino0_log_addr);

//	PRINT_MSG_AND_STR(" pathanme=", pathname);

	/* try getting a free trnasaction */
	tid = getFreeTransactionId();
//	L(" tid %d", tid);
//	PRINT_MSG_AND_HEX("\nino=", TRANSACTION_GET_INO(tid));
	EMPTY_TR_VERIFY(IS_TRANSACTION_EMPTY(tid));
	buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
//	PRINT_MSG_AND_NUM("\ncreat() - tid=",tid);

	/* verify file doesn't already exist - error returned indicates no matching direntry exists*/
	ino_num = namei(pathname, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
//	L("ino_num %d", ino_num);
//	PRINT_MSG_AND_STR("\ncreat() - after namei pathname=",pathname);
	EMPTY_TR_VERIFY(IS_NEGATIVE(ino_num) && ino_num != FS_ERROR_IO);

//	PRINT("\ncreat() - file doesn't alreday exist");

	/* verify prefix */
//	L("call verifyPrefix()");
	EMPTY_TR_VERIFY(!verifyPrefix(pathname, TRANSACTION_GET_DATA_BUF_PTR(tid), &dir_num, &name_offset));
//	L("verifyPrefix() success");
	/* verify name length */
	nameLen = calcNameLen(&(pathname[name_offset]));
	EMPTY_TR_VERIFY(verifyNameLen(nameLen));

	init_fsbuf(TRANSACTION_GET_DATA_BUF_PTR(tid));
//	PRINT_MSG_AND_NUM("\ncreat() - prefix verified. parent file id=(dir_num)", dir_num);
//	assert(0);
	/* if there is any transaction/open file entry involving dir_num - abort*/
//	L("call verifyFileNotOpen()");
	EMPTY_TR_VERIFY(!verifyFileNotOpen(dir_num));
//	L("verifyFileNotOpen() success");
//	PRINT("\ncreat() - no open transaction involves parent");
	/* find offset of a sparse block in inode0, and calculate inode number
	 * according to offset*/
//	L("call findEmptySparseBlock() in inode0");
	EMPTY_TR_VERIFY(!findEmptySparseBlock(0, log_addr, &offset, tid));
//	L("findEmptySparseBlock() inode0 success");
//	PRINT_MSG_AND_NUM("\ncreat() - found sparse block at offset=", offset);
	ino_num = CALC_IN_INODES(offset);
//	PRINT_MSG_AND_NUM("\ncreat() - new file id(ino_num)=", ino_num);

	TRANSACTION_SET_TYPE(tid, T_TYPE_WRITE);
	TRANSACTION_SET_INO(tid, ino_num);
	TRANSACTION_SET_FILE_OFFSET(tid, 0);
	TRANSACTION_SET_FILE_SIZE(tid, -1);
	TRANSACTION_SET_FTYPE(tid, new_type);

	init_logical_address(ino_log_addr);
	/* set new inode in tid indirect buffer
	 * and commit the inode to flash*/
//	L("call setNewInode()");
	TR_RES_VERIFY(res, setNewInode(ino_num, dir_num, new_type, tid));
//	L("setNewInode done");
	TR_RES_VERIFY(res, commitInode(tid));
//	L("commitInode success");
//	L("new inode address %x", ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));

	/* flush all transaction related cached blocks.
	 * inode0 changes are not cached, so this is ok*/
//	L("flush all transaction related cached blocks, call cache_flush_transaction_blocks()");
	RES_VERIFY(res, cache_flush_transaction_blocks(tid));
//	L("NOW new inode address %x", ADDR_PRINT(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	/* save inode address */
	copyLogicalAddress(ino_log_addr, TRANSACTION_GET_INO_ADDR_PTR(tid));

	/* commit inode0 with pointer to newly created file inode*/
	copyLogicalAddress(ino0_log_addr, FS_GET_INO0_ADDR_PTR());
//	L("call commitInodeToInode0()");
	TR_RES_VERIFY(res, commitInodeToInode0(ino0_log_addr, ino_num, ino_log_addr, tid));
//	L("commitInodeToInode0 success");
//	PRINT_MSG_AND_NUM("\ncreat() - new ino0 address=", logicalAddressToPhysical(ino0_log_addr));

	/* add direntry to parent directory.
	 * - set directory details to transaction
	 * - write directory entry to file block */
	TRANSACTION_SET_INO(tid, dir_num);
	TRANSACTION_SET_FILE_OFFSET(tid,-1);
	TR_RES_VERIFY(res, getInode(fs_buffer, dir_num, TRANSACTION_GET_INO_ADDR_PTR(tid)));
	TRANSACTION_SET_FILE_SIZE(tid, INODE_GET_NBYTES(ino_ptr));
	TRANSACTION_SET_FTYPE(tid, FTYPE_DIR);

	init_fsbuf(fs_buffer);
//	init_logical_address(TRANSACTION_GET_INO_ADDR_PTR(tid));
//	L("call writeDirEntry() writeDirEntry success");
	TR_RES_VERIFY(res, writeDirEntry(ino_num, &(pathname[name_offset]), new_type, tid));
//	L("writeDirEntry success");

//	copyLogicalAddress(dir_log_addr,  TRANSACTION_GET_INO_ADDR_PTR(tid));
	/* commit transaction - i.e. commit changes to inode0
	 * after changing dir_num inode address*/
	TR_RES_VERIFY(res, commitTransaction(tid, ino0_log_addr, 1));
//	assert(0);
//	L("commitTransaction done. do setOpenFentry()");

	/* if called by mkdir(), we are done*/
	if(IS_DIRECTORY(new_type)){
		return FS_ERROR_SUCCESS;
	}

	return setOpenFentry(ino_num, CREAT_FLAGS, 0, FTYPE_FILE);
}

/**
 * @brief
 * auxiliary to rmdir().
 * check if a directory is empty before deleting it.
 * check all inode entries if they are empty. check that first entry contains only ".", ".."
 *
 * @param ino_num directory inode number
 * @param isEmpty boolean indicator to hold result
 * @return FS_ERROR_SUCCESS if successful.
 * 		   -1 if a read error occured
 */
int32_t verifyDirectoryEmpty(int32_t ino_num, bool_t *isEmpty){
	int32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	dirent_flash *dirent_ptr = CAST_TO_DIRENT(fs_buffer);
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);

	init_logical_address(ino_addr);
	init_logical_address(log_addr);

	/* read inode*/
	FS_VERIFY(!getInode(fs_buffer, ino_num, ino_addr));

	/* verify direct entries empty */
	for(i=1; i<DIRECT_INDEX_ENTRIES;i++){
		INODE_GET_DIRECT(ino_ptr, i, log_addr);
//		PRINT_MSG_AND_NUM("\nverifyDirectoryEmpty() - is direct entry empty?", IS_ADDR_EMPTY(log_addr));
		if(!IS_ADDR_EMPTY(log_addr)){
			return FS_ERROR_SUCCESS;
		};
	}

	/* verify indirect, double, triple*/
//	PRINT("\ndirect entries success");
	INODE_GET_INDIRECT(ino_ptr, log_addr);
	if(!IS_ADDR_EMPTY(log_addr)){
		return 0;
	};
//	PRINT("\nindirect entry success");
	INODE_GET_DOUBLE(ino_ptr, log_addr);
	if(!IS_ADDR_EMPTY(log_addr)){
		return FS_ERROR_SUCCESS;
	};

	INODE_GET_TRIPLE(ino_ptr, log_addr);
	if(!IS_ADDR_EMPTY(log_addr)){
		return FS_ERROR_SUCCESS;
	};

	/* verify 1st entry contains only ".", ".."*/
	INODE_GET_DIRECT(ino_ptr, 0, log_addr);
	FS_VERIFY(!IS_ADDR_EMPTY(log_addr));
	FS_VERIFY(!fsReadBlock(log_addr,
						   fs_buffer,
						   TID_EMPTY,
						   DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES,
						   FLAG_CACHEABLE_READ_YES));

	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	moveToNextDirentry(&dirent_ptr, DIRENT_GET_LEN(dirent_ptr));
	if(!IS_DIRENT_EMPTY(dirent_ptr)){
		return FS_ERROR_SUCCESS;
	};

	/* if we got here, all entries are empty as expected.*/
	*isEmpty = 1;

	return FS_ERROR_SUCCESS;
}

/**
 * auxiliary to unlink() and mkdir().
 * remove a file given by pathname of type new_type from file system
 *
 * @param new_type file type
 * @param pathname path of file to remove
 * @return FS_ERROR_SUCCESS if successful
 * 		   -1 on error
 */
int32_t removeFile(int32_t new_type, uint8_t *pathname){
	int32_t res, ino_num, tid, dir_num, de_offset, f_type;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(dir_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino0_log_addr);
	uint8_t *buf;
	inode_t *ino_ptr = CAST_TO_INODE(fs_buffer);
	bool_t isEmpty = 0;
	uint32_t name_offset;
//	FENT();

	init_logical_address(ino_log_addr);
	init_logical_address(dir_log_addr);
	init_logical_address(ino0_log_addr);

	/* verify file exists, and is not directorys*/
	ino_num = namei(pathname, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
//	PRINT_MSG_AND_NUM("\nremoveFile() - ino_num=", ino_num);
//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
//	FS_VERIFY(!IS_NEGATIVE(ino_num));
//	PRINT_MSG_AND_NUM("\nremoveFile() - f_type=", f_type);
//	FS_VERIFY(f_type == new_type);
//	PRINT_MSG_AND_NUM("\nremoveFile() - ino_num=", ino_num);
//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));
//	PRINT_MSG_AND_NUM("\n********** cache 0 is related to transaction=", INDIRECT_CACHE_GET_TID(0));

	/* try getting a free trnasaction */
	tid = getFreeTransactionId();
//	L("tid=%d", tid);
	EMPTY_TR_VERIFY(IS_TRANSACTION_EMPTY(tid));
	buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
//	PRINT_MSG_AND_HEX(", ino0 addr=",*((uint32_t*)(FS_GET_INO0_ADDR_PTR())));

	/* verify file prefix*/
//	PRINT("\nremoveFile() - verifyPrefix ");
	EMPTY_TR_VERIFY(!verifyPrefix(pathname, buf, &dir_num, &name_offset));
//	L("after verifyPrefix dir_num=%d", dir_num);

//	L("IS_DIRECTORY(new_type)=%d", IS_DIRECTORY(new_type));
	if(IS_DIRECTORY(new_type)){
//		L("check not erasing .. or .");
		/* verify "." or ".." are not the pathname suffix*/
		EMPTY_TR_VERIFY(fsStrcmp("..", &(pathname[name_offset])));
		EMPTY_TR_VERIFY(fsStrcmp(".", &(pathname[name_offset])));

		/* verify not erasing root directory */
//		PRINT("\nremoveFile() - directory root?");
		EMPTY_TR_VERIFY(ino_num != 1);
//		PRINT("\nremoveFile() - directory not root");

		/* verify directory is empty before deleting it*/
		EMPTY_TR_VERIFY(!verifyDirectoryEmpty(ino_num, &isEmpty));
//		PRINT_MSG_AND_NUM("\nremoveFile() - verifyDirectoryEmpty res=", isEmpty);
		EMPTY_TR_VERIFY(isEmpty);
	}

	/* verify file and parent directory are not open*/
	EMPTY_TR_VERIFY(!verifyFileNotOpen(dir_num));
//	L("parent not open");
	EMPTY_TR_VERIFY(!verifyFileNotOpen(ino_num));
//	L("file and parent dir not open");

	TRANSACTION_SET_TYPE(tid, T_TYPE_UNLINK);
	TRANSACTION_SET_INO(tid, ino_num);
	TRANSACTION_SET_FTYPE(tid, new_type);

	/* vot all file pages*/
	TR_RES_VERIFY(res, votFile(tid));
//	L("voted file succesfuly");
	/* commit empty inode address to inode0*/
	copyLogicalAddress(ino0_log_addr, FS_GET_INO0_ADDR_PTR());
//	L("commit empty inode address to inode0. call commitInodeToInode0()");
	TR_RES_VERIFY(res, commitInodeToInode0(ino0_log_addr, ino_num, ino_log_addr, tid));
//	L("commitInodeToInode0() success");

	/* delete direntry*/
	/* add direntry to parent directory.
	 * - set directory details to transaction (dir_num, dir address, file type, no file offset)
	 * - write directory entry to file block */
	TRANSACTION_SET_INO(tid, dir_num);
	TRANSACTION_SET_FILE_OFFSET(tid,-1);
	TR_RES_VERIFY(res, getInode(fs_buffer, dir_num, TRANSACTION_GET_INO_ADDR_PTR(tid)));
	TRANSACTION_SET_FILE_SIZE(tid, INODE_GET_NBYTES(ino_ptr));
	TRANSACTION_SET_FTYPE(tid, FTYPE_DIR);
	init_fsbuf(fs_buffer);

//	L("about to deleteDirEntry()");
	TR_RES_VERIFY(res, deleteDirEntry(ino_num, de_offset, tid));
//	L("deleteDirEntry() success");
	/* commit transaction - i.e. commit changes to inode0
	 * after changing dir_num inode address*/
//	 {
//	 	PRINT("\nunlink() - b4 commitTransaction()");
//		INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(root_addr);
//		uint8_t buf[FS_BLOCK_SIZE];
//		inode_t *ino_ptr = CAST_TO_INODE(buf);
//		VERIFY(!fsReadBlock(FS_GET_INO0_ADDR_PTR(), buf));
//		INODE_GET_DIRECT(ino_ptr, 0, root_addr);
//		PRINT_MSG_AND_NUM(" root addr=", logicalAddressToPhysical(root_addr));
//		VERIFY(!fsReadBlock(root_addr, buf));
//		PRINT_MSG_AND_NUM(" root file id=", INODE_GET_FILE_ID(ino_ptr));
//	}
	TR_RES_VERIFY(res, commitTransaction(tid, ino0_log_addr, 1));

//	L("commitTransaction() success\n");
	return FS_ERROR_SUCCESS;
}

