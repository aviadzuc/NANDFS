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

#ifndef TESTFSUTILS_H_
#define TESTFSUTILS_H_

#include <system.h>

#define verifyOpenFileEntryEmpty(FD) verifyOpenFileEntry(FD,0,0,0,VNODE_EMPTY)
#define verifyVnodeEmpty(VNODE_IDX)  verifyVnode(VNODE_IDX,INO_NUM_EMPTY,0)
#define CALC_FREE_PAGES()            ((SEQ_SEGMENTS_COUNT-(GET_RECLAIMED_SEGMENT()+1))*(SEQ_PAGES_PER_SLOT-FS_CHECKPOINT_PAGE_COUNT) + SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET())
#define compare_fsbufs(BUF1, BUF2)   compare_bufs(BUF1, BUF2, FS_BLOCK_SIZE)

#define fsReadCache(CACHED_ADDR, BUF)        fsMemcpy(BUF, CACHE_GET_BUF_PTR(CACHE_GET_CID_FROM_ADDR(CACHED_ADDR)), FS_BLOCK_SIZE)

int32_t
verify_no_dirty_caches();

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
			 int32_t exp_active_indirect_leaf);

/**
 * Verify a buffer pointed by buf contains only bytes of type byte
 *
 */
error_t
verifyFsBlock(uint8_t *buf, uint8_t byte);

/**
 * @brief
 * verify that a given file entry has given values
 * @param fd open file entry index (file descriptor)
 * @return 1 is successful, 0 otherwise
 */
error_t verifyOpenFileEntry(uint32_t fd, uint32_t flags, uint32_t offset, user_id uid, uint32_t vnode);


/**
 * @brief
 * verify that a given vnode entry s empty
 * @param vnode_idx vnode index
 * @return 1 is successful, 0 otherwise
 */
error_t verifyVnode(uint32_t vnode_idx, uint32_t ino_num, uint32_t nrefs);

/**
 * @brief
 * auxiliary. verify inode details.
 *
 * @return 1 if successful, 0 otherwise
 */
error_t verifyInode(inode_t *ino_ptr, int32_t f_id, int32_t f_type, int32_t nblocks, int32_t nbytes);

/**
 * @brief
 *
 * auxiliary. verify file is empty after first direct entry.
 * @param ino_ptr pointer to file inode
 * @return 1 if successful, 0 otherwise
 */
error_t verifyFileSuffixlyEmpty(inode_t *ino_ptr);

/**
 * @brief
 * allocate a new inode on flash and return it's address
 * @param f_id file id
 * @param parent_f_id file id of parent of this file (if it is a directory)
 * @param f_type file type
 * @param log_addr address to which the inode will be written to
 * @return 0 if successful. if an allocAndWriteBlock() error occurs, it's error code is returned
 */
error_t writeNewInode(int32_t f_id, int32_t parent_f_id, uint32_t f_type, logical_addr_t log_addr);

/**
 * @brief
 * auxiliary. verify vot entry is as expected in a vot buffer of transaction tid
 * @return 1 is verified, 0 otherwise
 */
error_t verifyVotEntry(int32_t tid, int32_t entry_offset, logical_addr_t expected_addr);

error_t verifyFentriesVnodesEmpty(int32_t fd);

error_t verifyIndirectBlock(uint8_t *indirect_buf, uint8_t byte, int32_t entry_offset);

error_t verifyTransactionData(int32_t tid, int32_t offset, int32_t nblocks, int32_t maxWrittenOffset);

/**
 * @brief
 * verify all transactions are empty (except a specified transaction)
 * @return 1 if successful, 0 otherwise
 */
error_t verifyTransactionsEmpty(int32_t tid);

/**
 *@brief
 * verify directory entry contains expected data
 *
 * @return 1 if successful, 1 otherwise
 */
error_t verifyDirenty(dirent_flash *dirent_ptr, int32_t ino_num, int32_t d_type, int32_t d_len, uint8_t *d_name);

error_t verifyStat(file_stat_t *stat_p, int32_t ino_num, int32_t dev_id, int32_t blk_size, int32_t nbytes, int32_t nblocks);

int32_t mock_alphasort(nandfs_dirent ** a, nandfs_dirent **b);

#ifdef SIM
	#include <stdlib.h>
#endif

void mock_free(void *ptr);

void* mock_realloc(void *ptr, int32_t size);

void* limited_realloc(void *ptr, int32_t size);

#endif /*TESTFSUTILS_H_*/
