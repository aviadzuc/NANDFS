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

/** @file cache.h
 *
 */

#ifndef CACHE_H_
#define CACHE_H_

#include <system.h>
#include <src/fs/fs.h>

/* cache macros and definitions*/
#define CID_EMPTY -1
#define IS_EMPTY_CID(CID) (CID_EMPTY == CID)
#define IS_CACHE_EMPTY()  (FS_CACHE_BUFFERS == 0)
//typedef struct
//{
//	  int32_t is_tid_indirect_leaf; /* is the tid indirect a leaf of this buffer*/
//    int32_t is_voted; /* was the logical address of this cache voted*/
//    int32_t ref_count; /* number of blocks in cache refering to this block as "parent"*/
//    int32_t parent_cid; /* logical address IN CACHE of this block parent*/
//    int32_t parent_offset; /* offset of entry pointing to this block in parent*/
//    int32_t next_in_turn; /* id of cache AFTER this one in turn (less recently used)*/
//    int32_t prev_in_turn; /* id of cache BEFORE this one in turn (more recently used)*/
//	  int32_t tid; /* id of a transaction the buffer is related to*/
//    uint8_t is_dirty; /* is this block dirty*/
//    logical_addr_t log_addr; /* logical address this block was read from*/
//    uint8_t indirect_buf[FS_BLOCK_SIZE]; /* indirect buffer cache */
//} cache_block;

typedef struct{
	uint8_t bytes[8*sizeof(int32_t)+sizeof(uint8_t)+sizeof(logical_addr_struct)+FS_BLOCK_SIZE];
} cache_block;

typedef struct _cache_lru_q{
	cache_block caches[FS_CACHE_BUFFERS];
	int32_t lru_cid;
	int32_t mru_cid;
	int32_t dirty_count;
}cache_lru_q;

/* cache dirty indicators*/
#define CACHE_DIRTY 0
#define CACHE_CLEAN 0xff

#define CACHE_ACCESS_WRITE  1
#define CACHE_ACCESS_READ   0

#define CACHE_ENTRY_OFFSET_EMPTY 0xffffffff
#define CACHE_ID_EMPTY 			 0xffffffff

#define IS_CACHE_ID_EMPTY(SOME_CID)   ((SOME_CID)==CACHE_ID_EMPTY)

/* cache container macros*/
#define CACHE_LRU_Q_GET_PTR_BY_CID(CID)   (&((&lru_ptr)->caches[(CID)]))
#define CACHE_LRU_Q_GET_MRU()  			  ((&lru_ptr)->mru_cid)
#define CACHE_LRU_Q_GET_LRU()  			  ((&lru_ptr)->lru_cid)
#define CACHE_LRU_Q_GET_DIRTY_COUNT()  	  ((&lru_ptr)->dirty_count)

#define CACHE_LRU_Q_SET_MRU(CID)         (&lru_ptr)->mru_cid     = (CID)
#define CACHE_LRU_Q_SET_LRU(CID)         (&lru_ptr)->lru_cid     = (CID)
#define CACHE_LRU_Q_SET_DIRTY_COUNT(VAL) (&lru_ptr)->dirty_count = (VAL)

#define CACHE_GET_DIRTY_COUNT()          CACHE_LRU_Q_GET_DIRTY_COUNT()
/* cache struct field locations */
#define CACHE_IS_INDIRECT_LEAF_LOCATION  0
#define CACHE_IS_VOTED_LOCATION          (CACHE_IS_INDIRECT_LEAF_LOCATION+sizeof(int32_t))
#define CACHE_REF_COUNT_LOCATION         (CACHE_IS_VOTED_LOCATION+sizeof(int32_t))
#define CACHE_PARENT_CACHE_ID_LOCATION   (CACHE_REF_COUNT_LOCATION+sizeof(int32_t))
#define CACHE_PARENT_OFFSET_LOCATION     (CACHE_PARENT_CACHE_ID_LOCATION+sizeof(int32_t))
#define CACHE_NEXT_IN_TURN_LOCATION      (CACHE_PARENT_OFFSET_LOCATION+sizeof(int32_t))
#define CACHE_PREV_IN_TURN_LOCATION      (CACHE_NEXT_IN_TURN_LOCATION+sizeof(int32_t))
#define CACHE_TID_LOCATION               (CACHE_PREV_IN_TURN_LOCATION+sizeof(int32_t))
#define CACHE_DIRTY_LOCATION             (CACHE_TID_LOCATION+sizeof(int32_t))
#define CACHE_LOG_ADDR_LOCATION 		 (CACHE_DIRTY_LOCATION+sizeof(uint8_t))
#define CACHE_BUF_LOCATION      		 (CACHE_LOG_ADDR_LOCATION+sizeof(logical_addr_struct))

#define CACHE_GET_PTR(CID)       CACHE_LRU_Q_GET_PTR_BY_CID(CID)

/* cache getters*/
#define CACHE_GET_IS_INDIRECT_LEAF(CID) 	GET_INT32(CACHE_GET_PTR(CID), CACHE_IS_INDIRECT_LEAF_LOCATION)
#define CACHE_GET_IS_VOTED(CID) 		    GET_INT32(CACHE_GET_PTR(CID), CACHE_IS_VOTED_LOCATION)
#define CACHE_GET_REF_COUNT(CID)		    GET_INT32(CACHE_GET_PTR(CID), CACHE_REF_COUNT_LOCATION)
#define CACHE_GET_PARENT_CACHE_ID(CID)      GET_INT32(CACHE_GET_PTR(CID), CACHE_PARENT_CACHE_ID_LOCATION)
#define CACHE_GET_PARENT_OFFSET(CID)  	    GET_INT32(CACHE_GET_PTR(CID), CACHE_PARENT_OFFSET_LOCATION)
#define CACHE_GET_LESS_RECENTLY_USED(CID)   GET_INT32(CACHE_GET_PTR(CID), CACHE_NEXT_IN_TURN_LOCATION)
#define CACHE_GET_MORE_RECENTLY_USED(CID)   GET_INT32(CACHE_GET_PTR(CID), CACHE_PREV_IN_TURN_LOCATION)
#define CACHE_GET_TID(CID)                  GET_INT32(CACHE_GET_PTR(CID), CACHE_TID_LOCATION)
#define CACHE_GET_IS_DIRTY(CID)      		GET_BYTE(CACHE_GET_PTR(CID), CACHE_DIRTY_LOCATION)
#define CACHE_GET_LOG_ADDR_PTR(CID)  		CAST_TO_LOG_ADDR(CAST_TO_UINT32(&(CACHE_GET_PTR(CID)->bytes[CACHE_LOG_ADDR_LOCATION])))
#define CACHE_GET_LOG_ADDR(CID, ADDR) 		copyLogicalAddress(ADDR, CAST_TO_LOG_ADDR(CACHE_GET_LOG_ADDR_PTR(CID)))
#define CACHE_GET_BUF_PTR(CID)   			GET_BYTE_PTR(CACHE_GET_PTR(CID), CACHE_BUF_LOCATION)

/* cache setters*/
#define CACHE_SET_IS_INDIRECT_LEAF(CID, VAL) 	   SET_INT32(CACHE_GET_PTR(CID), CACHE_IS_INDIRECT_LEAF_LOCATION, VAL)
#define CACHE_SET_IS_VOTED(CID, NUM)               SET_INT32(CACHE_GET_PTR(CID), CACHE_IS_VOTED_LOCATION, NUM)
#define CACHE_SET_REF_COUNT(CID, NUM)              SET_INT32(CACHE_GET_PTR(CID), CACHE_REF_COUNT_LOCATION, NUM)
#define CACHE_SET_PARENT_CACHE_ID(CID, NUM)        SET_INT32(CACHE_GET_PTR(CID), CACHE_PARENT_CACHE_ID_LOCATION, NUM)
#define CACHE_SET_PARENT_LOG_ADDR(CID, ADDR)       copyLogicalAddress(CAST_TO_LOG_ADDR(CAST_TO_LOG_ADDR(CACHE_GET_PARENT_LOG_ADDR_PTR(CID))), ADDR)
#define CACHE_SET_PARENT_OFFSET(CID, NUM) SET_INT32(CACHE_GET_PTR(CID), CACHE_PARENT_OFFSET_LOCATION, NUM)
#define CACHE_SET_LESS_RECENTLY_USED(CID, NUM)     SET_INT32(CACHE_GET_PTR(CID), CACHE_NEXT_IN_TURN_LOCATION, NUM)
#define CACHE_SET_MORE_RECENTLY_USED(CID, NUM)     SET_INT32(CACHE_GET_PTR(CID), CACHE_PREV_IN_TURN_LOCATION, NUM)
#define CACHE_SET_TID(CID, TID)                    SET_INT32(CACHE_GET_PTR(CID), CACHE_TID_LOCATION, TID)
#define CACHE_SET_LOG_ADDR(CID, ADDR)     		   copyLogicalAddress(CAST_TO_LOG_ADDR(CAST_TO_LOG_ADDR(CACHE_GET_LOG_ADDR_PTR(CID))), ADDR)
#define CACHE_SET_DIRTY(CID, VAL)         		   SET_BYTE(CACHE_GET_PTR(CID), CACHE_DIRTY_LOCATION, VAL)

#define CACHE_INC_REF_COUNT(CID)   CACHE_SET_REF_COUNT((CID), CACHE_GET_REF_COUNT(CID)+1)
#define CACHE_DEC_REF_COUNT(CID)   CACHE_SET_REF_COUNT((CID), CACHE_GET_REF_COUNT(CID)-1)

/* cache various defines*/
#define CACHE_MAGIC_SEG          (SEQ_NO_SEGMENT_NUM-1)
#define IS_CACHED_ADDR(LOG_ADDR) (GET_LOGICAL_SEGMENT(LOG_ADDR) == CACHE_MAGIC_SEG)
#define CACHE_GET_CID_FROM_ADDR(LOG_ADDR)   GET_LOGICAL_OFFSET(LOG_ADDR)
#define CACHE_IS_DIRTY(CID)      (CACHE_GET_IS_DIRTY(CID) == CACHE_DIRTY)

#define IS_CACHE_FREE(CID)       (!CACHE_IS_DIRTY(CID) && IS_ADDR_EMPTY(CACHE_GET_LOG_ADDR_PTR(CID))) /* cache is not dirty (write cache) and not related to addr (must be read cache)*/

#define CACHE_SET_PARENT_FIELDS(CID, LOG_ADDR)    if(IS_CACHED_ADDR(LOG_ADDR)){ \
													  CACHE_INC_REF_COUNT(CID); \
													  CACHE_SET_PARENT_CACHE_ID(CACHE_GET_CID_FROM_ADDR(LOG_ADDR), CID); \
												  }

#define cache_get_read_buffer(CID, BUF)           fsMemcpy(BUF, CACHE_GET_BUF_PTR(CID), FS_BLOCK_SIZE)

#define CACHE_IS_VOTED_YES 1
#define CACHE_IS_VOTED_NO  0
#define CACHE_IS_ACTIVE_INDIRECT_LEAF_YES 1
#define CACHE_IS_ACTIVE_INDIRECT_LEAF_NO  0
/*************** API function prototypes *********************/
/**
 * init lru queue
 *
 */
void
cache_init_lru_q();

/**
 * Auxiliary function (also to readIndirectToBuffer()).
 * Find cache that relates to a real logical address
 *
 * @param log_addr the real logical address
 * @return cache id if found one.
 * 		   CACHE_ID_EMPTY otherwise
 */
error_t
cache_get_cid_by_real_addr(logical_addr_t log_addr);

/**
 * Auxiliary to VOT transition (marking as obsolete).
 * Find cache that relates to a real logical address
 * and free it.
 *
 *
 *
 * Assumptions:
 * 1. The freed cache is not dirty. The freeing process is performed as part
 *    of VOT transition, perofrmed at the end of a transaction
 *    ~> all transaction related dirty pages were already flushed
 *       and there are no references to this cache as parent
 *
 * @param log_addr the real logical address
 * @return FS_ERROR_SUCCESS on success
 * 		   FS_ERROR_CRITICAL if we got here and we're trying to free a dirty cache by real addr
 */
error_t
cache_free_by_real_addr(logical_addr_t log_addr);

/**
 * Auxiliary to findIndirectEntriesBlock() in transactions.
 * Remove an active indirect block from cache, so we don't have two replicas in memory.
 * NOTICE - This also requires fixing metadata of cache buffer that state this cid as their parent!!!
 *
 * Assumptions:
 * 1. cid is not empty
 *
 * @param cid cache id
 */
error_t
cache_force_free_cache(int32_t cid);

/**
 * Find a free cache buffer
 *
 * @return a cache id on success
 * 		   cache_flush_cache() error if occurred
 *         FS_ERROR_CRITICAL if a critical error occurred
 */
error_t
cache_get_free_cache_id();

/**
 * perform an access to a given cache buffer
 * upon return, new_addr contains new magic address of block
 *
 * @param buf the data buffer to write
 * @param old_addr old address of block (more relevant to reads?)
 * @param parent_cid id of cached parent
 * @param parent_offset offset in parent of entry pointing to this address (offset in entries, not bytes!)
 * @param tid transaction id related to this block write
 * @param data_type data type of written block. used to extarct nested cache referecnes
 * @param flag indicating whther we're performing a write
 * @return FS_ERROR_FREE_CACHE_ACQUIREMENT if couldn't get free cache
 * 		   FS_ERROR_CRITICAL if unexplained error occurred
 * 		   FS_ERROR_SUCCESS on success
 */
error_t
cache_access_cache(uint8_t *buf,
				   logical_addr_t old_addr,
				   int32_t parent_offset,
				   int32_t tid,
				   int32_t data_type,
				   int32_t is_write);
/**
 * Clear all buffers related to a specific transaction.
 * Auxiliary to abortTransaction
 *
 * @param tid transaction id
 */
void
cache_init_tid_related(int32_t tid);

/**
 * Flush all transaction related blocks to flash.
 *
 * Assumptions:
 * 1. all blocks were commited. we only have the committing to inode0 to perform
 *    (i.e. there is not dependency between transaction indirect and cache)
 *
 * @param tid a transaction id
 * @return FS_ERROR_SUCCESS on success
 *         cache_flush_cache() errors
 */
error_t
cache_flush_transaction_blocks(int32_t tid);

/**
 * Auxiliary to writeVot().
 * Get the real address we need to vot
 *
 * Assumptions:
 * 1. org_vot_log_addr is a cached addr
 *
 * @param org_vot_log_addr the original cached logical address
 * @param vot_log_addr container to real logical address
 * @return FS_ERROR_ALREADY_VOTED if the cache buffer was already voted
 *         FS_ERROR_SUCCESS if successful
 */
error_t
cache_get_vot_addr(logical_addr_t org_vot_log_addr, logical_addr_t vot_log_addr);

/**
 * Auxiliary.
 * Remove a cached indirect block before making it the active indirect block of a transaction
 *
 * @param indirect_addr indirect block addr
 * @return FS_ERROR_SUCCESS on success
 */
error_t
cache_remove_indirect_block(logical_addr_t indirect_addr);

/**
 * Auxiliary to readIndirectToBuffer() and commitInode().
 * fix indirect leaf indicator in parent if necssary
 *
 * @param indirect_offset indirect block offset in file
 * @param double_log_addr pointer to double indirect parent block (if there is one)
 * @param tid transaction id
 * @param flag new flag value
 */
void
cache_fix_indirect_leaf_indicator(int32_t indirect_offset,
							      logical_addr_t double_log_addr,
							      int32_t tid,
							      int32_t flag);
#endif /*CACHE_H_*/
