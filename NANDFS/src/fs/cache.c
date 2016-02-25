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

/** @file cache.c
 *
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
 * Init a cache buffer
 *
 * @param cid cache number
 */
static void
cache_init_by_cid(int32_t cid){
//	FENT();
//	PRINT_MSG_AND_NUM("init cid ", cid);
	init_struct(CACHE_GET_PTR(cid), sizeof(cache_block));
	CACHE_SET_REF_COUNT(cid, 0);
	CACHE_SET_IS_VOTED(cid, CACHE_IS_VOTED_NO);
	CACHE_SET_IS_INDIRECT_LEAF(cid, 0);
}

/**
 * init lru queue
 *
 */
void
cache_init_lru_q(){
	int32_t i;

	for(i=0; i< FS_CACHE_BUFFERS;i++){
		cache_init_by_cid(i);
	}

	CACHE_LRU_Q_SET_MRU(CACHE_ID_EMPTY);
	CACHE_LRU_Q_SET_LRU(CACHE_ID_EMPTY);
	CACHE_LRU_Q_SET_DIRTY_COUNT(0);
}

/**
 * Auxiliary function (also to readIndirectToBuffer()).
 * Find cache that relates to a real logical address
 *
 * @param log_addr the real logical address
 * @return cache id if found one.
 * 		   CACHE_ID_EMPTY otherwise
 */
error_t
cache_get_cid_by_real_addr(logical_addr_t log_addr){
	int32_t cid;
//	FENT();
//	L("trying to get real addr %x", ADDR_PRINT(log_addr));

	/* can't find cache by empty addr*/
	if(IS_ADDRESS_EMPTY(log_addr)){
		return CACHE_ID_EMPTY;
	}

	cid = CACHE_LRU_Q_GET_MRU();
	while(!IS_CACHE_ID_EMPTY(cid)){
		if(COMPARE_ADDR(CACHE_GET_LOG_ADDR_PTR(cid), log_addr)){
//			L("found read cache for addr %x (belongs to transaction %d)",
//					*((uint32_t*)(log_addr)),
//					(IS_EMPTY_TID(CACHE_GET_TID(cid)))?-1:CACHE_GET_TID(cid));
			return cid;
		}

		cid = CACHE_GET_LESS_RECENTLY_USED(cid);
	}

	return CACHE_ID_EMPTY;
}

//static int32_t;
//verify_no_contradictions(){
//	int32_t i, j;
//
//	for(i=0; i< FS_CACHE_BUFFERS;i++){
//		if(!IS_CACHE_ID_EMPTY(CACHE_GET_LESS_RECENTLY_USED(i))){
//			if(!COMPARE(i, CACHE_GET_MORE_RECENTLY_USED(CACHE_GET_LESS_RECENTLY_USED(i)))){
//				L("ERROR cid %d less recently used is %d. it's more used is %d", i, CACHE_GET_LESS_RECENTLY_USED(i), CACHE_GET_MORE_RECENTLY_USED(CACHE_GET_LESS_RECENTLY_USED(i)));
//				VERIFY(0);
//			}
//			VERIFY(COMPARE(i, CACHE_GET_MORE_RECENTLY_USED(CACHE_GET_LESS_RECENTLY_USED(i))))
//		}
//
//		if(!IS_CACHE_ID_EMPTY(CACHE_GET_MORE_RECENTLY_USED(i))){
//			if(!COMPARE(i, CACHE_GET_LESS_RECENTLY_USED(CACHE_GET_MORE_RECENTLY_USED(i)))){
//				L("ERROR cid %d more recently used is %d. it's less used is %d", i, CACHE_GET_MORE_RECENTLY_USED(i), CACHE_GET_LESS_RECENTLY_USED(CACHE_GET_MORE_RECENTLY_USED(i)));
//				VERIFY(0);
//			}
//		}
//		for(j=0; j< FS_CACHE_BUFFERS;j++){
//			if(!IS_CACHE_ID_EMPTY(CACHE_GET_LESS_RECENTLY_USED(i)) &&
//			   !IS_CACHE_ID_EMPTY(CACHE_GET_LESS_RECENTLY_USED(j))){
//				if(CACHE_GET_LESS_RECENTLY_USED(i)==CACHE_GET_LESS_RECENTLY_USED(j)){
//					L("caches %d, %d have identical less used %d", i,j,CACHE_GET_LESS_RECENTLY_USED(j));
//					VERIFY(0);
//				}
//			}
//
//			if(!IS_CACHE_ID_EMPTY(CACHE_GET_MORE_RECENTLY_USED(i)) &&
//			   !IS_CACHE_ID_EMPTY(CACHE_GET_MORE_RECENTLY_USED(j))){
//				if(CACHE_GET_MORE_RECENTLY_USED(i)==CACHE_GET_MORE_RECENTLY_USED(j)){
//					L("caches %d, %d have identical more used %d", i,j,CACHE_GET_MORE_RECENTLY_USED(j));
//					VERIFY(0);
//				}
//			}
//		}
//	}
//
//	return 1;
//}

static error_t
cache_remove_from_lru(int32_t cid){
//	FENT();

//	assert(!IS_CACHE_ID_EMPTY(cid));
//	assert(cid >=0 && cid <FS_CACHE_BUFFERS);
//	L("remove cid %d (less %d more %d). global lru %d, global mru %d", cid,
//																	   CACHE_GET_LESS_RECENTLY_USED(cid),
//																	   CACHE_GET_MORE_RECENTLY_USED(cid),
//																	   CACHE_LRU_Q_GET_LRU(),
//																	   CACHE_LRU_Q_GET_MRU());
//
//	/* assert no contradictions in case of at least two caches in queue*/
//	if(CACHE_LRU_Q_GET_LRU() != CACHE_LRU_Q_GET_MRU()){
//		/* assert lru has more recently used*/
//		assert(!IS_CACHE_ID_EMPTY(CACHE_GET_MORE_RECENTLY_USED(CACHE_LRU_Q_GET_LRU())));
//
//		/* assert mru has less recently used*/
//		assert(!IS_CACHE_ID_EMPTY(CACHE_GET_LESS_RECENTLY_USED(CACHE_LRU_Q_GET_MRU())));
//	}

//	VERIFY(verify_no_contradictions());
	/* first eliminate refernces to cid from it's neighbours*/
//	assert(!IS_CACHE_ID_EMPTY(cid));
	if(!IS_CACHE_ID_EMPTY(CACHE_GET_MORE_RECENTLY_USED(cid))){
//		L("more used %d is not empty. set it's less used to cid %d", CACHE_GET_MORE_RECENTLY_USED(cid), cid);
		CACHE_SET_LESS_RECENTLY_USED(CACHE_GET_MORE_RECENTLY_USED(cid), CACHE_GET_LESS_RECENTLY_USED(cid));
	}
	if(!IS_CACHE_ID_EMPTY(CACHE_GET_LESS_RECENTLY_USED(cid))){
//		L("less used %d is not empty. set it's less used to cid %d", CACHE_GET_LESS_RECENTLY_USED(cid), cid);
		CACHE_SET_MORE_RECENTLY_USED(CACHE_GET_LESS_RECENTLY_USED(cid), CACHE_GET_MORE_RECENTLY_USED(cid));
	}

	/* now change global mru and lru*/
	if(CACHE_LRU_Q_GET_LRU() == cid){
//		L("cache %d is global cache lru", cid);
		/* if there isn't an lru pointed by cid*/
		if(IS_CACHE_ID_EMPTY(CACHE_GET_LESS_RECENTLY_USED(cid))){
//			L("cache %d has no less used. set global lru to it's more used %d", cid, CACHE_GET_MORE_RECENTLY_USED(cid));
			CACHE_LRU_Q_SET_LRU(CACHE_GET_MORE_RECENTLY_USED(cid));
		}
		else{
//			L("cache %d has less used. set global lru to %d", cid, CACHE_GET_LESS_RECENTLY_USED(cid));
			CACHE_LRU_Q_SET_LRU(CACHE_GET_LESS_RECENTLY_USED(cid));
		}
	}

	if(CACHE_LRU_Q_GET_MRU() == cid){
//		L("cache %d is global cache mru", cid);
		/* if there isn't an lru pointed by cid*/
		if(IS_CACHE_ID_EMPTY(CACHE_GET_MORE_RECENTLY_USED(cid))){
//			L("cache %d has no more used. set global mru to it's less used %d", cid, CACHE_GET_LESS_RECENTLY_USED(cid));
			CACHE_LRU_Q_SET_MRU(CACHE_GET_LESS_RECENTLY_USED(cid));
		}
		else{
//			L("cache %d has more used. set global lru to %d", cid, CACHE_GET_MORE_RECENTLY_USED(cid));
			CACHE_LRU_Q_SET_MRU(CACHE_GET_MORE_RECENTLY_USED(cid));
		}
	}

	/* finally initialize pointers in cid*/
	CACHE_SET_MORE_RECENTLY_USED(cid, CACHE_ID_EMPTY);
	CACHE_SET_LESS_RECENTLY_USED(cid, CACHE_ID_EMPTY);
//	L("done. global lru is %d, global mru is %d", CACHE_LRU_Q_GET_LRU(), CACHE_LRU_Q_GET_MRU());
//	assert(!(IS_CACHE_ID_EMPTY(CACHE_LRU_Q_GET_LRU()) && !IS_CACHE_ID_EMPTY(CACHE_LRU_Q_GET_MRU())));
//	assert(!(IS_CACHE_ID_EMPTY(CACHE_LRU_Q_GET_MRU()) && !IS_CACHE_ID_EMPTY(CACHE_LRU_Q_GET_LRU())));
//	VERIFY(verify_no_contradictions());
	return 1;
}

/**
 * Insert new buffer to lru queue.
 * It is the new head of line
 *
 * @param cid new mru cid
 * @return FS_ERROR_SUCCESS if successful
 * 		   FS_ERROR_CRITICAL if sanity chekc failed
 */
static error_t
cache_insert_to_lru(int32_t cid){
	int old_mru_cid;
//	FENT();

//	L("insert cid %d", cid);
//	if(!verify_no_contradictions()){
//		return FS_ERROR_CRITICAL;
//	}

	/* if no mru found, then cache is empty and we're done*/
	old_mru_cid = CACHE_LRU_Q_GET_MRU();
//	L("old_mru_cid %d", old_mru_cid);
	if(IS_CACHE_ID_EMPTY(old_mru_cid)){
//		assert(IS_CACHE_ID_EMPTY(CACHE_LRU_Q_GET_LRU()));
//		L("no global mru (CACHE LRU EMPTY!). set cid %d to global lru and mru", cid);
		CACHE_LRU_Q_SET_MRU(cid);
		CACHE_LRU_Q_SET_LRU(cid);
//		if(!verify_no_contradictions()){
//			return FS_ERROR_CRITICAL;
//		}
		return FS_ERROR_SUCCESS;
	}

	/* sanity check - mru points to nothing as more recently used*/
	if(!IS_CACHE_ID_EMPTY(CACHE_GET_MORE_RECENTLY_USED(old_mru_cid))
		|| !IS_CACHE_ID_EMPTY(CACHE_GET_MORE_RECENTLY_USED(CACHE_LRU_Q_GET_MRU()))){
		L("cache_insert_to_lru() - cache mru %d, has mru in itself %d", cid, CACHE_GET_MORE_RECENTLY_USED(old_mru_cid));
//		PRINT("\n@@@@@@@@@ cache_insert_to_lru() - cache mru, has mru in itself! @@@@@@@@@@");
		return FS_ERROR_CRITICAL;
	}

	/* set locally */
	CACHE_SET_LESS_RECENTLY_USED(cid, old_mru_cid);
	CACHE_SET_MORE_RECENTLY_USED(old_mru_cid, cid);
//	L("set cid %d less used to old global mru. it is now the new global mru", cid, old_mru_cid);

	/* set globally*/
	CACHE_LRU_Q_SET_MRU(cid);

//	if(!verify_no_contradictions()){
//		return FS_ERROR_CRITICAL;
//	}

	/*done. return*/
//	assert(!IS_CACHE_ID_EMPTY(CACHE_LRU_Q_GET_LRU()));
	return FS_ERROR_SUCCESS;
}

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
cache_free_by_real_addr(logical_addr_t log_addr){
	int32_t cid;
//	FENT();
//	L("trying to get real addr %x", *((uint32_t*)(log_addr)));

	/* get cache id*/
	cid = cache_get_cid_by_real_addr(log_addr);

	/* if cid is empty, then the address is not cache and we're done*/
	if(IS_CACHE_ID_EMPTY(cid)){
		return FS_ERROR_SUCCESS;
	}

	/* if cache is not dirty
	 * we can simply recycle it, and that's the end of it
	 * why? it is not referenced by a magic address from nowhere*/
	if(!CACHE_IS_DIRTY(cid)){
		if(!cache_remove_from_lru(cid)){
			L("ERROR");
			return FS_ERROR_CRITICAL;
		}
		cache_init_by_cid(cid);
		return FS_ERROR_SUCCESS;
	}
	else{
		L("ERROR trying to free dirty cache %d, belonging to tid %d", cid, CACHE_GET_TID(cid));
		return FS_ERROR_FREEING_DIRTY_CACHE;
	}

	return FS_ERROR_SUCCESS;
}

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
cache_force_free_cache(int32_t cid){
	int32_t i;
//	FENT();
//	L("cid %d", cid);

	/* sanity check - if cid is empty, something really wrong has happenned*/
	if(IS_CACHE_ID_EMPTY(cid)){
		return FS_ERROR_CRITICAL;
	}

	/* fix reference in parent*/
//	L("force free cid %d. parent cid % log_Addr %x", cid, CACHE_GET_PARENT_CACHE_ID(cid), ADDR_PRINT(CACHE_GET_LOG_ADDR_PTR(cid)));
	if(!IS_CACHE_ID_EMPTY(CACHE_GET_PARENT_CACHE_ID(cid))){
//		L("fix in parent cid %d in offset %d to addr %x", CACHE_GET_PARENT_CACHE_ID(cid),
//														  CACHE_GET_PARENT_OFFSET(cid),
//														  ADDR_PRINT(CACHE_GET_LOG_ADDR_PTR(cid)));
		BLOCK_SET_INDEX(CACHE_GET_BUF_PTR(CACHE_GET_PARENT_CACHE_ID(cid)),
					    CACHE_GET_PARENT_OFFSET(cid),
					    CACHE_GET_LOG_ADDR_PTR(cid));
	}

	/* edge case -
	 * if indirect is inode of it's transaction, fix the reference in
	 * the transaction struct*/
	if(IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(CACHE_GET_TID(cid))) &&
	   GET_LOGICAL_OFFSET(TRANSACTION_GET_INO_ADDR_PTR(CACHE_GET_TID(cid))) == cid){
		copyLogicalAddress(TRANSACTION_GET_INO_ADDR_PTR(CACHE_GET_TID(cid)), CACHE_GET_LOG_ADDR_PTR(cid));
	}

	/* flush cache*/
	if(!cache_remove_from_lru(cid)){
		L("ERROR");
		return FS_ERROR_CRITICAL;
	}

	/* don't forget to decrease dirty count*/
	if(CACHE_IS_DIRTY(cid)){
		CACHE_LRU_Q_SET_DIRTY_COUNT(CACHE_LRU_Q_GET_DIRTY_COUNT()-1);
	}
	cache_init_by_cid(cid);

	/* omit all parent references*/
	i = CACHE_LRU_Q_GET_MRU();
	while(!IS_CACHE_ID_EMPTY(i)){
		if(CACHE_GET_PARENT_CACHE_ID(i)==cid){
			CACHE_SET_PARENT_CACHE_ID(i, CACHE_ID_EMPTY);
		}

		i = CACHE_GET_LESS_RECENTLY_USED(i);
	}

//	L("done!");
	return FS_ERROR_SUCCESS;
}

/**
 * Iterate cache until we find lru.
 * return it.
 * after we find it, iterate upwards until we find a cache that is a leaf and doesn't reference any block
 *
 * (TBD in the future:
 * 	for a dirty cache, increment lru hit count. Only if we reached 5 hits flush hit)
 * @param except_cid a cid that shouldn't be flushed. example - parent of block we are evacuating place for
 * @return lru cache id
 *         CACHE_ID_EMPTY if cache is empty
 */
static error_t
cache_get_lru_cid(int32_t except_cid)
{
	int32_t i=0;
//	FENT();

	/* if no lru cache, then all caches are free.
	 * return empty cid*/
	if(IS_CACHE_ID_EMPTY(CACHE_LRU_Q_GET_LRU())){
//		L("lru is empty! return CACHE_ID_EMPTY");
		int32_t i;
//		for(i=0; i<FS_CACHE_BUFFERS;i++){
//			L("CACHE #%d less used %d more used %d", i, CACHE_GET_LESS_RECENTLY_USED(i), CACHE_GET_MORE_RECENTLY_USED(i));
//		}
		return CACHE_ID_EMPTY;
	}
//	L("lru wasn't empty (%d)", CACHE_LRU_Q_GET_LRU());
	/* iterate upwards and find a leaf cache
	 * 1. ref count 0
	 * 2. not except_cid
	 * 3. transaction indirect is not it's leaf*/
	i = CACHE_LRU_Q_GET_LRU();
	while(CACHE_GET_REF_COUNT(i) > 0 ||
		  except_cid == i ||
		  CACHE_GET_IS_INDIRECT_LEAF(i)){
//		L("get more recently used of cache %d (ref count %d, except_cid %d, CACHE_GET_IS_INDIRECT_LEAF(i) %dq", i, CACHE_GET_REF_COUNT(i), except_cid ,CACHE_GET_IS_INDIRECT_LEAF(i));
		i = CACHE_GET_MORE_RECENTLY_USED(i);

		/* if we got to the mru, stop*/
		if(IS_CACHE_ID_EMPTY(i)){
			break;
		}
	}

//	L("done. return %d", i);
	return i;
}

/**
 * Auxiliary to cache_flush_cache();
 * check for any addresses that depend on this cache buffer
 * and amend them to reflect the new logical address
 * instead of the old virtual address
 *
 * @param cid cache id of cache that was flushed and now has real address
 * @param new_real_log_addr the new real logical address the cache was flushed to
 * @return FS_ERROR_SUCCESS if successful
 */
static error_t
cache_fix_block_dependencies(int32_t cid, logical_addr_t new_real_log_addr){
	int32_t i, parent_cid, old_ref_count;
	logical_addr_t entry_addr = NULL;
//	FENT();

//	L("cid %d", cid);
	/* fix in parent (if there is one)
	 * and decrement it's ref count.
	 * NOTICE - no need to fix leafs of this cache, since there are none.
	 * 	        we only flush a leaf!*/
	parent_cid = CACHE_GET_PARENT_CACHE_ID(cid);
	if(!IS_CACHE_ID_EMPTY(parent_cid)){
//		L("changing by parent, cid %d, parent_cid %d, parent offset #%d. is this OK?", cid, parent_cid, CACHE_GET_PARENT_OFFSET(cid));
		/* get address reference, and change it
		 * NOTICE - replace ONLY segment and offset of addr with real ones,
		 * 			since address flags may differ*/
		entry_addr = BLOCK_GET_ADDR_PTR_BY_INDEX(CACHE_GET_BUF_PTR(parent_cid), CACHE_GET_PARENT_OFFSET(cid));
//		L("copy new_real_log_addr %x to entry in offset %d", ADDR_PRINT(new_real_log_addr), CACHE_GET_PARENT_OFFSET(cid));
//		{
//			INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//			init_logical_address(log_addr);
//			INODE_GET_DIRECT(CAST_TO_INODE(CACHE_GET_BUF_PTR(parent_cid)), 0, log_addr);
//			L("b4 fixing parent first direct entry of inode is %x", ADDR_PRINT(log_addr));
//		}
//		L("set entry_addr %x to new_real_log_addr %x", ADDR_PRINT(entry_addr), ADDR_PRINT(new_real_log_addr));
		SET_LOGICAL_OFFSET(entry_addr, GET_LOGICAL_OFFSET(new_real_log_addr));
		SET_LOGICAL_SEGMENT(entry_addr, GET_LOGICAL_SEGMENT(new_real_log_addr));
//		L("now entry #%d in parent_cid %d is %x", CACHE_GET_PARENT_OFFSET(cid), parent_cid, *((uint32_t*)&(CACHE_GET_BUF_PTR(parent_cid)[CACHE_GET_PARENT_OFFSET(cid)*4])));
//		{
//			INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//			init_logical_address(log_addr);
//			INODE_GET_DIRECT(CAST_TO_INODE(CACHE_GET_BUF_PTR(parent_cid)), 0, log_addr);
//			L("after fixing parent first direct entry of inode is %x", ADDR_PRINT(log_addr));
//		}
		/* decrement parent ref count*/
		old_ref_count = CACHE_GET_REF_COUNT(parent_cid);

		/* sanity check*/
		if(old_ref_count == 0){
			return FS_ERROR_DEC_ZERO_REF_COUNT;
		}
//		L("cid %d. dec parent ref count (parent cid %d). old ref count %d", cid, parent_cid, old_ref_count);
		CACHE_SET_REF_COUNT(parent_cid, old_ref_count-1);
//		L("new ref count %d", CACHE_GET_REF_COUNT(CACHE_GET_PARENT_CACHE_ID(cid)));

		return FS_ERROR_SUCCESS;
	}

	/* parent is not in cache. therefore it is in transaction.
	 * fix in transaction -
	 * 1. First fix ino_addr if necessary -
	 *    i.e if it is a cached addr with logical offset=cid
	 * 2. fix transaction buffer - maybe the current active indirect cache
	 *    contains references to this cached block. */
	if(IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(CACHE_GET_TID(cid))) &&
	   GET_LOGICAL_OFFSET(TRANSACTION_GET_INO_ADDR_PTR(CACHE_GET_TID(cid))) == cid){
//		L("fixing dependency on cid %d in transaction %d ino addr", cid, CACHE_GET_TID(cid));
		entry_addr = TRANSACTION_GET_INO_ADDR_PTR(CACHE_GET_TID(cid));
	}
	/* if this block does depend on something (we know this since it has a parent offset)*/
	else if(CACHE_GET_PARENT_OFFSET(cid) != CACHE_ENTRY_OFFSET_EMPTY){
		/* check for parent in transaction indirect*/
		/* if the indirect really contains an indirect simply iterate it*/
		if(!IS_INDIRECT_INODE(CACHE_GET_TID(cid))){
//			L("indirect of tid %d is not inode. find cached address with offset %d", CACHE_GET_TID(cid), cid);
			for(i=0;i<LOG_ADDRESSES_PER_BLOCK;i++){
				/* get address reference*/

				entry_addr = BLOCK_GET_ADDR_PTR_BY_INDEX(TRANSACTION_GET_INDIRECT_PTR(CACHE_GET_TID(cid)), i);

//				L("IS_CACHED_ADDR(entry_addr) %d. entry_addr cid=%d, searched cid %d", IS_CACHED_ADDR(entry_addr), CACHE_GET_CID_FROM_ADDR(entry_addr), cid);
				if(IS_CACHED_ADDR(entry_addr) &&
				   CACHE_GET_CID_FROM_ADDR(entry_addr) == cid){
//					L("fixing dependency on cid %d in transaction %d active indirect inode", cid, CACHE_GET_TID(cid));
					break;
				}
			}

			/* if we haven't found anything, nullify entry_addr*/
			if(i==LOG_ADDRESSES_PER_BLOCK){
				entry_addr = NULL;
			}
		}
		else{
			for(i=CALC_IN_LOG_ADDRESSES(DIRECT_INDEX_LOCATION);
			    i<LOG_ADDRESSES_PER_BLOCK;
			    i++){
				entry_addr = BLOCK_GET_ADDR_PTR_BY_INDEX(TRANSACTION_GET_INDIRECT_PTR(CACHE_GET_TID(cid)), i);

				/* only cached address relevant*/
				if(IS_CACHED_ADDR(entry_addr) &&
				   CACHE_GET_CID_FROM_ADDR(entry_addr) == cid){
//					L("fixing dependency on cid %d in transaction %d active indirect", cid, CACHE_GET_TID(cid));
					break;
				}
			}

			/* if we haven't found anything, nullify entry_addr*/
			if(i==LOG_ADDRESSES_PER_BLOCK){
				entry_addr = NULL;
			}
		}
	}

	/* fix entry_addr (which now should point to where the referenced addr should be)
	 * NOTICE - replace ONLY segment and offset of addr with real ones,
	 * 			since address flags may differ*/
	if(entry_addr != NULL){
		SET_LOGICAL_OFFSET(entry_addr, GET_LOGICAL_OFFSET(new_real_log_addr));
		SET_LOGICAL_SEGMENT(entry_addr, GET_LOGICAL_SEGMENT(new_real_log_addr));
	}
	else{
		L("cid #%d entry_addr is empty. no changes made.", cid);
	}
//	L("done");
	return FS_ERROR_SUCCESS;
}

/**
 * flush a cache buffer
 *
 * Assumptions:
 * 1. block is a leaf in cache - i.e. it doesn't contain any references to mgic addresses
 *
 * @param cid id of cache to flush
 * @param log_addr pointer to logicla addr where the new real block address will be put after finished
 * @return FS_ERROR_SUCCESS on success
 * 		   FS_ERROR_ILLEGAL_CID if cid is illegal
 * 		   FS_ERROR_NONZERO_REF_COUNT if cid refers to a non-leaf cache
 * 		   write errors
 *         cache_fix_block_dependencies() errors
 */
static error_t
cache_flush_cache(int cid, logical_addr_t log_addr){
	int32_t res;
//	FENT();
//	L("cid %d", cid);

	/* sanity checks */
	/* check for legal cache_id*/
	if(cid >= FS_CACHE_BUFFERS){
		return FS_ERROR_ILLEGAL_CID;
	}

	/* verify ref count is 0*/
	if(CACHE_GET_REF_COUNT(cid) > 0){
		return FS_ERROR_NONZERO_REF_COUNT;
	}

	/* if cache is empty we are done */
	if(IS_CACHE_FREE(cid)){
//		L("cache is free");
		return FS_ERROR_SUCCESS;
	}

	/* two options here:
	 * A) cache is dirty -  flush dirty cache to media:
	 *    1. write block as part of transaction
	 *    2. fix dependencies on this block - fix any reference to previously cached block address
	 *       to the new logical address
	 * B) cache is not dirty -
	 *    we can simply recycle it, and that's the end of it
	 *    why? because it is not referenced by any magic address*/

//	check if this is a newly written block that was voted.
//	     * 						if so, we don't need to write it. it is redundant.
//	     *    otherwise,
//	 &&
//		   !(IS_ADDR_EMPTY(CACHE_GET_LOG_ADDR_PTR(cid)) &&
//		     CACHE_GET_IS_VOTED(cid))
	if(CACHE_IS_DIRTY(cid)){
//		L("cache %d is dirty. write,fix dependencies and return", cid);

		/* write block as part of transaction*/
		res = allocAndWriteBlockTid_actualWrite(log_addr,
												CACHE_GET_BUF_PTR(cid),
												0, /* data not vots*/
												CACHE_GET_TID(cid));
//		L("allocAndWriteBlockTid_actualWrite() wrote to %x", ADDR_PRINT(log_addr));
//		L("flushed cid %d block to %x", cid, ADDR_PRINT(log_addr));
		/* if an error occurred abort*/
		if(!IS_FS_SUCCESS(res)){
//			L("write error %d, for cid %d", res, cid);
			return res;
		}

		/* check for dependencies (and fix if necessary)*/
//		L("call cache_fix_block_dependencies() on cid %d", cid);
		RES_VERIFY(res, cache_fix_block_dependencies(cid, log_addr));
	}

	/* don't forget to decrease dirty count, after we dealt with dirty cache*/
	if(CACHE_IS_DIRTY(cid)){
		CACHE_LRU_Q_SET_DIRTY_COUNT(CACHE_LRU_Q_GET_DIRTY_COUNT()-1);
	}

	/* FINALLY - get buffer out of lru, recycle
	 * and return successfully*/
	if(!cache_remove_from_lru(cid)){
		L("ERROR");
		return FS_ERROR_CRITICAL;
	}
	cache_init_by_cid(cid);

	return FS_ERROR_SUCCESS;
}

/**
 * Find a free cache buffer
 *
 * @return a cache id on success
 * 		   cache_flush_cache() error if occurred
 *         FS_ERROR_CRITICAL if a critical error occurred
 */
error_t
cache_get_free_cache_id()
{
	int i, lru_cid, res;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(new_log_addr);
//	FENT();

	/* find a free cache id*/
	for(i=0; i< FS_CACHE_BUFFERS; i++){
		if(IS_CACHE_FREE(i)){
			return i;
		}
	}
//	L("no free caches...");

	/* none found, find lru buffer
	 * which is also a leaf */
//	L("get lru cid");
	lru_cid = cache_get_lru_cid(CACHE_ID_EMPTY);
//	L("got lru cid %d. is it empty? %d", lru_cid, IS_CACHE_ID_EMPTY(lru_cid));
	/* if lru_cid is empty,a serious error occurred*/
	if(IS_CACHE_ID_EMPTY(lru_cid)){
//		L("couldn't get lru cid. return critical error");
		return FS_ERROR_CRITICAL;
	}

	/*flush lru cache to media */
//	L("call cache_flush_cache(), lru_cid %d", lru_cid);
	res = cache_flush_cache(lru_cid, new_log_addr);

	/* if an error occurred return the error code*/
	if(!IS_FS_SUCCESS(res)){
		L("ERROR %d", res);
		return res;
	}

	/* return newly created empty flash*/
	return lru_cid;
}

/**
 * Auxiliary to ?().
 * Our options:
 * 1. Extract cid from old block address
 * 2. if it is not cached then search by logical address
 * 3. Id old address empty get free cache
 *
 * @param old_addr old block addr (logical, empty, or cached)
 * @return CACHE_ID_EMPTY if matching cache not found
 * 		   a cache id otherwise
 */
static error_t
cache_get_cid_by_addr(logical_addr_t old_addr){
	int32_t  res;
//	FENT();

	/* extract cid*/
	if(IS_CACHED_ADDR(old_addr)){
		return CACHE_GET_CID_FROM_ADDR(old_addr);
	}

	/* if addr is not empty, see if maybe there's a read cache for it
	 * search by logical address - iterate all caches for now
	 * TBD - make this efficient*/
	if(!IS_ADDR_EMPTY(old_addr)){
		res = cache_get_cid_by_real_addr(old_addr);

		/* addr is not empty but we didn't find it in cache
		 * TBD: get it from flash? for now return error*/
		if(!IS_CACHE_ID_EMPTY(res)){
			return res;
		}
	}

	return CACHE_ID_EMPTY;
}

/**
 * Auxiliary to cache_access_cache()/
 * Get buffer from cache, and fix dependencies on it according to type -
 * Change parent related fields in newly accessed cache
 * Specifically change parent id in children and ref count in parent itself
 *
 * @param cid cache id, which may be parent to others
 * @param buf buffer containing new data to cache
 * @param data_type data type of buffer. according to this we know how to search for nested cached addresses
 */
static void
cache_set_write_buffer(int32_t cid,
					   uint8_t *buf,
					   int32_t data_type){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr_entry);
	int32_t i;
//	FENT();

//	L("cid %d. data_type %d", cid, data_type);
	/* copy buffer*/
	fsMemcpy(CACHE_GET_BUF_PTR(cid), buf, FS_BLOCK_SIZE);
	/* set as parent of contained cached addresses
	 * and get ref count*/
	/* first initialize ref count for cid*/
	CACHE_SET_REF_COUNT(cid, 0);

//	L("do switch");
	switch(data_type){
		/* iterate all possible inode entries*/
		case DATA_TYPE_INODE:
			for(i=0; i<DIRECT_INDEX_ENTRIES;i++){
				INODE_GET_DIRECT(CAST_TO_INODE(buf), i, log_addr_entry);

				/* only cached address relevant*/
				CACHE_SET_PARENT_FIELDS(cid, log_addr_entry);
			}

			INODE_GET_INDIRECT(CAST_TO_INODE(buf), log_addr_entry);
			CACHE_SET_PARENT_FIELDS(cid, log_addr_entry);

			INODE_GET_DOUBLE(CAST_TO_INODE(buf), log_addr_entry);
			CACHE_SET_PARENT_FIELDS(cid, log_addr_entry);

			INODE_GET_TRIPLE(CAST_TO_INODE(buf), log_addr_entry);
			CACHE_SET_PARENT_FIELDS(cid, log_addr_entry);
			break;
		/* iterate all entries*/
		case DATA_TYPE_ENTRIES:
//			L("data type entries");
			for(i=0; i<LOG_ADDRESSES_PER_BLOCK;i++){
				BLOCK_GET_INDEX(buf, i, log_addr_entry);
				CACHE_SET_PARENT_FIELDS(cid, log_addr_entry);
			}
			break;
		default:
			break;
	}

	/* if this bock doesn't relate to a real log addr
	 * (that may have been voted) un-vot it.
	 * (this is a re-write or 1st write, either way it's the right thing to do) */
	if(IS_ADDR_EMPTY(CACHE_GET_LOG_ADDR_PTR(cid))){
		CACHE_SET_IS_VOTED(cid, CACHE_IS_VOTED_NO);
	}
//	L("new ref count for cid %d is %d", cid, CACHE_GET_REF_COUNT(cid));
//	L("done");
}
/**
 * perform an access to a given cache buffer
 * upon return, old_addr contains new magic address of block
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
				   int32_t is_write){
	int32_t cid;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(new_addr);
	init_logical_address(new_addr);
//	FENT();

	/* get relevant cache*/
//	L("call cache_get_cid_by_addr() on addr %x", ADDR_PRINT(old_addr));
	cid = cache_get_cid_by_addr(old_addr);
//	L("got cid by addr %d", cid);
	/* if an error occurred, return*/
	if(cid < 0 && cid != CACHE_ID_EMPTY){
		return cid;
	}

	/* set magic segment*/
	SET_LOGICAL_SEGMENT(new_addr, CACHE_MAGIC_SEG);

	/* if we got an empty cid back, then this block
	 * is not cached and we need to allocate it a cache*/
	if(IS_CACHE_ID_EMPTY(cid)){
//		L("cid empty. call cache_get_free_cache_id()");
		cid = cache_get_free_cache_id();
//		L("cid was empty. got free one %d", cid);
		/* if couldn't get one, return error*/
		if(IS_CACHE_ID_EMPTY(cid) || cid < 0){
//			L("couldn't get free cache");
			return FS_ERROR_FREE_CACHE_ACQUIREMENT;
		}

		/* we have a cid so we can set it in new_addr*/
		SET_LOGICAL_OFFSET(new_addr, cid);

		/* we have a brand new cid.
		 * if this is a read, we need to get the actual block now...*/
		if(!is_write){
//			L("call readBlock() on old_addr %x. cid %d", ADDR_PRINT(old_addr), cid);
			if(readBlock(old_addr, CACHE_GET_BUF_PTR(cid))){
				return FS_ERROR_IO;
			}
		}
	}
	/* we got the old cache of this block.
	 * change only what's necessary */
	else{
//		L("cid (%d) not empty", cid);
		/* sanity check - we know this block is cached, and we are re-writing/re-reading.
		 * if we do that but for a different cache
		 * ~> something really wrong has happened*/
		if(IS_CACHED_ADDR(old_addr) &&
		   CACHE_GET_CID_FROM_ADDR(old_addr) != cid){
//			L("rewriting a block. old cache was %d but we got %d?!", CACHE_GET_CID_FROM_ADDR(old_addr), cid);
			return FS_ERROR_CRITICAL;
		}
		/* we have a cid so we can set it in new_addr*/
		SET_LOGICAL_OFFSET(new_addr, cid);

		/* if this is an already dirty block -
		 * 1. move it up the lru,
		 * 2. if this is a re-write - do the rewrite
		 * 							  set parent fields, and fix dependencies on this block
		 * 4. return. all other fields are already set*/
		if(CACHE_IS_DIRTY(cid)){
			if(!cache_remove_from_lru(cid)){
				L("ERROR");
				return FS_ERROR_CRITICAL;
			}
			cache_insert_to_lru(cid);

			/* this might be an old read cache that doesn't have the tid
			 * so mark the transaction in it just in case*/
			CACHE_SET_TID(cid, tid);
			/* set actual buffer (according to type)*/
			if(is_write){
//				L("rewriting cid %d", cid);
				cache_set_write_buffer(cid, buf, data_type);
			}
			else{
//				L("reading to cid %d", cid);
				fsMemcpy(buf, CACHE_GET_BUF_PTR(cid), FS_BLOCK_SIZE);
			}

			copyLogicalAddress(old_addr, new_addr);
//			L("done");
			return FS_ERROR_SUCCESS;
		}
	}

	/* cache is either not dirty, or brand new.
	 * set details in cache:
	 * 1. set old logical addr (if it is not already cached)
	 * 2. insert to lru
	 * 3. save tid
	 * if this is a write block
	 * 4. mark as dirty
	 * NOTICE - we do NOT init ref count.
	 * 			If this is a new block it is initialized, otherwise, no reason to touch it...
	 *
	 * finally copy block
	 */
//	L("cache %d is either not dirty, or brand new.", cid);
	if(!IS_CACHED_ADDR(old_addr)){
//		L("first access of block. set log addr to old_addr %x", ADDR_PRINT(old_addr));
		CACHE_SET_LOG_ADDR(cid, old_addr);
	}

	if(!cache_remove_from_lru(cid)){
		L("ERROR");
		return FS_ERROR_CRITICAL;
	}
	cache_insert_to_lru(cid);
	if(is_write && !CACHE_IS_DIRTY(cid)){
		CACHE_SET_DIRTY(cid, CACHE_DIRTY);
		CACHE_LRU_Q_SET_DIRTY_COUNT(CACHE_LRU_Q_GET_DIRTY_COUNT()+1);
	}
	CACHE_SET_TID(cid, tid);

	/* set actual buffer*/
//	L(" set actual buffer for cid %d", cid);
	if(is_write){
//		L("first write to cid %d", cid);
		cache_set_write_buffer(cid, buf, data_type);
	}
	else{
//		L("call cache_get_read_buffer()");
//		L("reading to cid %d", cid);
		cache_get_read_buffer(cid, buf);
	}

	CACHE_SET_PARENT_OFFSET(cid, parent_offset);

	/* return successfully*/
	copyLogicalAddress(old_addr, new_addr);
//	L("done");
	return FS_ERROR_SUCCESS;
}

/**
 * Clear all buffers related to a specific transaction.
 * Auxiliary to abortTransaction
 *
 * @param tid transaction id
 */
void
cache_init_tid_related(int32_t tid){
	int32_t i;
	FENT();

	/* find a free cache id*/
	for(i=0; i< FS_CACHE_BUFFERS; i++){
		if(CACHE_GET_TID(i) == tid){
//			L("init tid realted cache with cid %d", i);
			/* don't forget to decrease dirty count*/
			if(CACHE_IS_DIRTY(i)){
//				L("decrement dirty count (was %d). now FS_TOTAL_FREE_PAGES %d (obs %d, frees %d)", CACHE_LRU_Q_GET_DIRTY_COUNT(), FS_TOTAL_FREE_PAGES, GET_OBS_COUNT(), GET_FREE_COUNTER());
				CACHE_LRU_Q_SET_DIRTY_COUNT(CACHE_LRU_Q_GET_DIRTY_COUNT()-1);
			}
			cache_remove_from_lru(i); /*NOTICE - must do this before init! */
			cache_init_by_cid(i);
		}
	}
}

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
cache_flush_transaction_blocks(int32_t tid){
	int32_t i, j, res;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//	FENT();
	/* TBD: iterate all blocks 5 times, and flush leafs related to tid
	 * this should be enough*/
	for(j=0; j< 5; j++){
		for(i=0; i< FS_CACHE_BUFFERS; i++){
			/* look only at tid-related blocks*/
			if(CACHE_GET_TID(i) != tid){
				continue;
			}

//			if(CACHE_IS_DIRTY(i)){
//				L("encountered dirty cache %d, ref count %d, parent id %d", i, CACHE_GET_REF_COUNT(i), CACHE_GET_PARENT_CACHE_ID(i));
//			}

			init_logical_address(log_addr);

			/* don't free ones with leafs*/
			if(CACHE_GET_REF_COUNT(i) > 0){
				continue;
			}
//			L("flushing cache %d", i);
			res = cache_flush_cache(i, log_addr);

			if(IS_FS_SUCCESS(res)){
				continue;
			}

			/* only a non-leaf error is accpeted (before last round)*/
			if(res == FS_ERROR_NONZERO_REF_COUNT && j==4){
				assert(0);
			}

//			L("cache_flush_cache() error %d. returning", res);
			return res;
		}
	}

	return FS_ERROR_SUCCESS;
}

/**
 * Auxiliary.
 * Remove a cached indirect block before making it the active indirect block of a transaction
 *
 * @param indirect_addr indirect block addr
 * @return FS_ERROR_SUCCESS on success
 */
error_t
cache_remove_indirect_block(logical_addr_t indirect_addr){
	int32_t cid, res;
//	FENT();

	/* remove that indirect block from cache*/
	if(IS_CACHED_ADDR(indirect_addr)){
		cid = CACHE_GET_CID_FROM_ADDR(indirect_addr);
//		L("indirect_addr cached. flushing indirect from cid %d", cid);
	}
	else{
		cid = cache_get_cid_by_real_addr(indirect_addr);
//		L("indirect_addr %x not cached. flushing indirect from cid %d", ADDR_PRINT(indirect_addr), cid);

		/* if the address is not cached, then we are done*/
		if(IS_CACHE_ID_EMPTY(cid)){
			return FS_ERROR_SUCCESS;
		}
	}

	/* set cache details in transaction indirect data
	 * and indicate in parent cid that we removed a leaf*/
	if(!IS_EMPTY_TID(CACHE_GET_TID(cid)) &&
	   !IS_CACHE_ID_EMPTY(CACHE_GET_PARENT_CACHE_ID(cid))){
//		TRANSACTION_SET_PARENT_CID(CACHE_GET_TID(cid), CACHE_GET_PARENT_CACHE_ID(cid));
//		TRANSACTION_SET_PARENT_OFFSET(CACHE_GET_TID(cid), CACHE_GET_PARENT_OFFSET(cid));
		TRANSACTION_SET_IS_VOTED(CACHE_GET_TID(cid), CACHE_GET_IS_VOTED(cid));
		CACHE_SET_IS_INDIRECT_LEAF(CACHE_GET_PARENT_CACHE_ID(cid), 1);
	}

	/* actually remove the block from cache*/
	if(!IS_CACHE_ID_EMPTY(cid)){
//		L("actually remove the block from cache");
		res = cache_force_free_cache(cid);
		if(!IS_FS_SUCCESS(res)){
			return res;
		}
	}

	return FS_ERROR_SUCCESS;
}

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
							      int32_t flag){
	int32_t cid, parent_cid = CACHE_ID_EMPTY;
//	FENT();

//	L("");
	/* fix parent indirect leaf indicator, if necessary) -
	 * 1. if we wrote the first indirect, ammend cached inode */
	if(indirect_offset <DOUBLE_DATA_OFFSET &&
	   IS_CACHED_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid))){
//		L("old indirect parent is the inode");
		parent_cid = CACHE_GET_CID_FROM_ADDR(TRANSACTION_GET_INO_ADDR_PTR(tid));
	}
	/* 2. we wrote some inode. ammend in cached double*/
	else if(IS_CACHED_ADDR(double_log_addr)){
//		L("old indirect parent is some double. get it's cid from its cached addr");
		parent_cid = CACHE_GET_CID_FROM_ADDR(double_log_addr);
	}
	else{
		cid = cache_get_cid_by_real_addr(double_log_addr);
//		L("old indirect parent is some double, whose cid is unknown. got double cid %d", cid);
		if(!IS_CACHE_ID_EMPTY(cid)){
			parent_cid = CACHE_GET_PARENT_CACHE_ID(cid);
		}
	}

	/* and now if we have a cached parent, indicate in it that
	 * there is a transaction indirect leaf no more!*/
	if(!IS_CACHE_ID_EMPTY(parent_cid)){
//		L("set is indirect leaf =%d in parent_cid %d", flag, parent_cid);
		CACHE_SET_IS_INDIRECT_LEAF(parent_cid, flag);
	}

//	L("done!");
}
