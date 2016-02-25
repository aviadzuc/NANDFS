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

/** @file transactions.h
 * Transaction header, function prototypes, structs etc.
 */

#ifndef TRANSACTIONS_H_
#define TRANSACTIONS_H_

#include <system.h>
#include <src/fs/fs.h>
#include <src/sequencing/sequencing.h>

/* definitions*/
#define TID_EMPTY        -1

#define FS_MAX_FILESIZE          CAST_VAL_TO_UINT32(CALC_IN_BYTES(NAND_PAGE_COUNT)) /* NOTICE - should be long (64-bit), but in 32-bit cpu this won't do us any good */
#define FS_MAX_NBLOCKS	         NAND_PAGE_COUNT
#define FS_DELETE_DIRENT_WRITES  12

#define FS_FILE_MAX_METADATA_PAGES   (TRIPLE_METADATA_PAGES+DOUBLE_METADATA_PAGES+DOUBLE_METADATA_PAGES+DIRECT_MAPPED_PAGES+1)
#define FS_FILE_MAX_DATA_PAGES       NAND_PAGE_COUNT
#define FS_MIN_FREE_PAGES            ((NAND_PAGE_COUNT >> POWER_LOG_ADDRESSES_PER_BLOCK) + FS_DELETE_DIRENT_WRITES)
#define FS_TOTAL_FREE_PAGES			 (calcTotalFreePages()-CACHE_GET_DIRTY_COUNT())
#define IS_MIN_FREE_PAGES_REACHED()  (FS_TOTAL_FREE_PAGES <= FS_MIN_FREE_PAGES)

/* we perform a transaction temporary commit if there are at least 3 vots,
 * since in worst case the transaction adds 3 new blocks to file (triple, double, indirect).
 * otherwise the temporary commit may result in block addition*/
#define TRANSACTION_COMMIT_MIN_VOTS  3

/* transaction macros*/
#define EMPTY_TR_VERIFY(TEST)  if(!(TEST)){ \
								  init_transaction(tid); \
								  return -1; \
							   }
#define TR_RES_VERIFY(RES_VAR, ACTION)  RES_VAR = ACTION; \
									    if(RES_VAR){ \
										   FS_VERIFY(!abortTransaction(tid)); \
										   L("ERROR res %d", RES_VAR); \
										   return RES_VAR; \
									    }
#define SYS_TR_VERIFY(RES_VAR, ACTION)  RES_VAR = ACTION; \
												   if(RES_VAR){ \
													  SYS_VERIFY(!abortTransaction(tid)); \
													  FS_MUTEX_UNLOCK(); \
													  L("ERROR res %d", RES_VAR); \
													  return RES_VAR; \
												   }
/* verify actions in write(). on failure return numbre of bytes written so far*/
#define WRITE_VERIFY(COMMITED_BYTES, ACTION)   if(ACTION){ \
												  FS_VERIFY(!abortTransaction(tid)); \
												  L("ERROR"); \
												  return COMMITED_BYTES; \
											   }

#define T_TYPE_EMPTY   0xffffffff
#define T_TYPE_WRITE   1
#define T_TYPE_UNLINK  2
#define T_TYPE_TEMP    3

#define TRANSACTION_GET_PTR(TID)  (&(transactions[TID]))
#define IS_TRANSACTION_EMPTY(TID) (!IS_EMPTY_TID(TID) && TRANSACTION_GET_INO(TID) == INO_NUM_EMPTY)
#define IS_EMPTY_TID(TID)         (TID == TID_EMPTY)
#define IS_EMPTY_TTYPE(TID)       (TRANSACTION_GET_TYPE(TID) == TTYPE_EMPTY)
#define IS_TTYPE_UNLINK(TID)      (TRANSACTION_GET_TYPE(TID) == T_TYPE_UNLINK)
#define IS_TTYPE_WRITE(TID)       (TRANSACTION_GET_TYPE(TID) == T_TYPE_WRITE)
#define IS_TTYPE_TEMP(TID)        (TRANSACTION_GET_TYPE(TID) == T_TYPE_TEMP)
#define IS_WRITE_PEMITTED(TID)    (IS_TTYPE_UNLINK(TID) || IS_TTYPE_TEMP(TID))

#define TRANSACTION_GET_TYPE(TID)	          (TRANSACTION_GET_PTR(TID)->t_type)
#define TRANSACTION_GET_VOTS_OFFSET(TID)      (TRANSACTION_GET_PTR(TID)->vots_buf_offset)
#define TRANSACTION_GET_VOTS_BUF_PTR(TID)     (TRANSACTION_GET_PTR(TID)->vots_buf)
#define TRANSACTION_GET_DATA_BUF_PTR(TID)     (TRANSACTION_GET_PTR(TID)->data_buf)
#define TRANSACTION_GET_INDIRECT_PTR(TID)     (TRANSACTION_GET_PTR(TID)->indirect_buf)
//#define TRANSACTION_GET_INDIRECT_PTR(TID)     CACHE_GET_BUF_PTR(TRANSACTION_GET_INDIRECT_CID(TID))

#define TRANSACTION_SET_VOTS_BUF_PTR(TID, BUF_PTR)     TRANSACTION_GET_PTR(TID)->vots_buf     = BUF_PTR
#define TRANSACTION_SET_DATA_BUF_PTR(TID, BUF_PTR)     TRANSACTION_GET_PTR(TID)->data_buf     = BUF_PTR
#define TRANSACTION_SET_INDIRECT_PTR(TID, BUF_PTR)     TRANSACTION_GET_PTR(TID)->indirect_buf = BUF_PTR

#define TRANSACTION_GET_INO(TID)	            (TRANSACTION_GET_PTR(TID)->ino_num)
#define TRANSACTION_GET_VOTS_COUNT(TID)         (TRANSACTION_GET_PTR(TID)->vots_count)
#define TRANSACTION_GET_PREV_ADDR_PTR(TID)      (&(TRANSACTION_GET_PTR(TID)->prev_log_addr))
#define TRANSACTION_GET_INO_ADDR_PTR(TID)       (&(TRANSACTION_GET_PTR(TID)->ino_log_addr))
#define TRANSACTION_GET_IS_INDIRECT_INODE(TID)  (TRANSACTION_GET_PTR(TID)->isIndirectInode)
#define TRANSACTION_GET_IS_INDIRECT_SPARSE(TID) (TRANSACTION_GET_PTR(TID)->isIndirectSparse)
#define TRANSACTION_GET_FILE_OFFSET(TID)        (TRANSACTION_GET_PTR(TID)->fileOffset)
#define TRANSACTION_GET_MAX_WRITTEN_OFFSET(TID) (TRANSACTION_GET_PTR(TID)->maxWrittenOffset)
#define TRANSACTION_GET_BLOCKS_ALLOCS(TID)      (TRANSACTION_GET_PTR(TID)->newBlocksallocated)
#define TRANSACTION_GET_FILE_SIZE(TID)          (TRANSACTION_GET_PTR(TID)->fileSize)
#define TRANSACTION_GET_FTYPE(TID)              (TRANSACTION_GET_PTR(TID)->f_type)
#define TRANSACTION_GET_WAS_COMMITED(TID)       (TRANSACTION_GET_PTR(TID)->wasCommited)
#define TRANSACTION_GET_IS_COMMITING(TID)       (TRANSACTION_GET_PTR(TID)->isCommiting)
#define TRANSACTION_GET_PARENT_CID(TID)         (TRANSACTION_GET_PTR(TID)->parent_cid)
#define TRANSACTION_GET_PARENT_OFFSET(TID)      (TRANSACTION_GET_PTR(TID)->parent_offset)
#define TRANSACTION_GET_IS_VOTED(TID)	        (TRANSACTION_GET_PTR(TID)->is_voted)

#define TRANSACTION_SET_TYPE(TID, VAL)        TRANSACTION_GET_PTR(TID)->t_type     = VAL
#define TRANSACTION_SET_INO(TID, VAL)         TRANSACTION_GET_PTR(TID)->ino_num    = VAL
#define TRANSACTION_SET_VOTS_COUNT(TID, VAL)  TRANSACTION_GET_PTR(TID)->vots_count = VAL
#define TRANSACTION_SET_PREV_ADDR(TID, ADDR)  copyLogicalAddress(TRANSACTION_GET_PREV_ADDR_PTR(TID), ADDR)
#define TRANSACTION_SET_INO_ADDR(TID, ADDR)   copyLogicalAddress(TRANSACTION_GET_INO_ADDR_PTR(TID), ADDR)
#define TRANSACTION_INC_VOTS_COUNT(TID)       TRANSACTION_SET_VOTS_COUNT(TID, TRANSACTION_GET_VOTS_COUNT(TID)+1)
#define TRANSACTION_SET_IS_INDIRECT_INODE(TID, VAL)  TRANSACTION_GET_PTR(TID)->isIndirectInode  = VAL
#define TRANSACTION_SET_IS_INDIRECT_SPARSE(TID, VAL) TRANSACTION_GET_PTR(TID)->isIndirectSparse = VAL
#define TRANSACTION_SET_FILE_OFFSET(TID, VAL)        TRANSACTION_GET_PTR(TID)->fileOffset = VAL
#define TRANSACTION_SET_MAX_WRITTEN_OFFSET(TID,VAL)  TRANSACTION_GET_PTR(TID)->maxWrittenOffset = VAL
#define TRANSACTION_SET_FTYPE(TID, VAL)              TRANSACTION_GET_PTR(TID)->f_type   = VAL
#define TRANSACTION_SET_FILE_SIZE(TID, VAL)          TRANSACTION_GET_PTR(TID)->fileSize = VAL
#define TRANSACTION_SET_WAS_COMMITED(TID, VAL)       TRANSACTION_GET_PTR(TID)->wasCommited = VAL
#define TRANSACTION_SET_IS_COMMITING(TID, VAL)       TRANSACTION_GET_PTR(TID)->isCommiting = VAL
#define TRANSACTION_SET_PARENT_CID(TID, VAL)         TRANSACTION_GET_PTR(TID)->parent_cid    = VAL
#define TRANSACTION_SET_PARENT_OFFSET(TID, VAL)      TRANSACTION_GET_PTR(TID)->parent_offset = VAL
#define TRANSACTION_SET_IS_VOTED(TID, VAL)	         TRANSACTION_GET_PTR(TID)->is_voted = VAL

#define TRANSACTION_SET_VOTS_OFFSET(TID, VAL)         TRANSACTION_GET_PTR(TID)->vots_buf_offset = VAL
#define TRANSACTION_INC_VOTS_OFFSET(TID)              TRANSACTION_SET_VOTS_OFFSET(TID, TRANSACTION_GET_VOTS_OFFSET(TID)+1)
#define TRANSACTION_SET_BLOCKS_ALLOCS(TID, VAL)       TRANSACTION_GET_PTR(TID)->newBlocksallocated = VAL
#define TRANSACTION_SET_TOTAL_BLOCK_ALLOCS(TID, VAL)  TRANSACTION_GET_PTR(TID)->totalNewBlocksallocated  = VAL
#define TRANSACTION_INC_BLOCKS_ALLOCS(TID)            TRANSACTION_SET_BLOCKS_ALLOCS(TID, TRANSACTION_GET_BLOCKS_ALLOCS(TID)+1)
#define TRANSACTION_CALC_FILE_SIZE(TID)               if((TRANSACTION_GET_FILE_OFFSET(TID) + TRANSACTION_GET_MAX_WRITTEN_OFFSET(TID)) > TRANSACTION_GET_FILE_SIZE(TID)) TRANSACTION_GET_FILE_SIZE(TID) = TRANSACTION_GET_FILE_OFFSET(TID) + TRANSACTION_GET_MAX_WRITTEN_OFFSET(TID)
#define IS_BLOCK_REWRITE(BLOCK_OFFSET, TID)           (BLOCK_OFFSET < TRANSACTION_GET_FILE_SIZE(TID))
#define IS_MINIMUM_TRANSACTION_VOTS(TID)              (TRANSACTION_GET_VOTS_COUNT(TID) <= TRANSACTION_COMMIT_MIN_VOTS)

#define TRANSACTION_ADD_VOT(TID, VOT_LOG_ADDR)     BLOCK_SET_INDEX(TRANSACTION_GET_VOTS_BUF_PTR(TID), TRANSACTION_GET_VOTS_OFFSET(TID), VOT_LOG_ADDR)
#define IS_INDIRECT_INODE(TID)   				   (TRANSACTION_GET_FILE_OFFSET(TID) == 0)
#define IS_VOTS_BUF_EMPTY(TID)					   IS_ADDR_EMPTY(&(CAST_TO_LOG_ADDR(TRANSACTION_GET_VOTS_BUF_PTR(TID))[0]))
#define IS_WRITES_PERFORMED(TID)				   (TRANSACTION_GET_MAX_WRITTEN_OFFSET(TID) != 0)
#define IS_FILE_OFFSET_EMPTY(TID)				   (TRANSACTION_GET_FILE_OFFSET(TID) < 0)
#define IS_INDIRECT_EMPTY(TID)                     IS_NEGATIVE(TRANSACTION_GET_FILE_OFFSET(TID))
#define IS_SPARSE_FILE(TID)					       ((TRANSACTION_GET_INO(TID) == 0) || (IS_DIRECTORY(TRANSACTION_GET_FTYPE(TID))))

#define setIndirectOffset(OFFSET, TID)   TRANSACTION_SET_FILE_OFFSET(TID, calcIndirectBlockOffset(OFFSET))

typedef struct{
	uint32_t t_type; /* transaction type */
	int32_t  f_type; /* file type */
	bool_t   wasCommited;
	bool_t   isCommiting;
	int32_t  ino_num; /* inode number of file*/
	logical_addr_struct prev_log_addr;
	uint8_t  data_buf[FS_BLOCK_SIZE]; /* write data buffer*/

	uint32_t vots_count; /* number of pages VOTed during this transaction*/
	uint32_t vots_buf_offset;
	uint8_t  vots_buf[FS_BLOCK_SIZE]; /* vot entries buffer*/

	logical_addr_struct ino_log_addr;  /* logical address of the inode, as far as this transaction is concerned*/
	uint32_t maxWrittenOffset; /* max offset data was written to in this block*/
	uint32_t newBlocksallocated;/* number of direct blocks allocated in block, before indirect bock commits */
	int32_t  fileSize;          /* current file size */
	int32_t  fileOffset;       /* offset in file of indirect block(0 if indirect is actually inode)*/
	uint8_t  indirect_buf[FS_BLOCK_SIZE]; /* pointers entries buffer, with pointer to data buffer*/
	int32_t parent_cid;
	int32_t parent_offset;
	int32_t is_voted;
}transaction_t;

/* pointers to global data structures */
extern transaction_t transactions[FS_MAX_N_TRANSACTIONS];

/**
 * @brief
 * find a transaction that handles file with ino_num file id
 *
 * @param ino_num file inode number
 * @return transaction id if successful, TID_EMPTY if no matching transaction found
 */
int32_t findTidByFileId(int32_t ino_num);

/**
 * @brief
 * get transaction writing to file identified by ino_num
 * @param ino_num file id
 * @return transaction id if successful, TID_EMPTY if none found
 */
int32_t getTidByFileId(int32_t ino_num);

/**
 * @brief
 * find an empty transaction to use
 */
int32_t getFreeTransactionId(void);

/**
 * @brief
 * initialize transaction with tid.
 * If the initalization is final, free buffers as well. otherwise only initialize buffers
 *
 * @param tid transaction id
 */
void init_transaction(int32_t tid);

/**
 * @brief
 * init all transactions
 */
void init_transactions(void);

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
deleteTransactionPages(logical_addr_t log_addr, int32_t tid, bool_t is_from_reboot);

/**
 * @brief
 * abort a transaction.
 * - vot all written blocks
 * - initialize transaction struct
 * @param tid transaction id
 * @return 0 if successful, -1 if a write/read error occured
 */
int32_t abortTransaction(int32_t tid);

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
allocAndWriteBlockTid_actualWrite(logical_addr_t log_addr,
								  void* data,
								  bool_t is_data_vots,
								  int32_t tid);

/**
 * @brief
 * write a block as aprt of an on-going transaction (tid).
 * get prev address from the transaction, and mark new transaction in it
 *
 * Assumptions:
 * 1. log_addr is initialized
 *
 * @param log_addr old address of block (for caching uses), and container to new block address (if there is one)
 * @param data data block to write
 * @param dataType indicator to which data are we writing (vots, inode entries etc.)
 * @param tid transaction id
 * @return allocAndWriteBlock() error codes
 * -1 if an error occured when aborting another transaction
 */
error_t allocAndWriteBlockTid(logical_addr_t log_addr,
							  void* data,
							  int32_t dataType,
							  int32_t entry_offset,
							  int32_t tid);
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
writeVot(logical_addr_t org_vot_log_addr, int32_t tid);

/**
 * @brief
 * auxiliary to commitInode().
 * initialize fields realted to writes during a transaction,.
 * after commiting inode.
 * @param tid transaction id
 */
void initWriteFields(int32_t tid);

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
performVOTs(uint8_t *vots, bool_t is_from_reboot);

/**
 * @brief
 * do vots for all vot pages in transaction tid
 *
 * @param tid_last_addr last address written in a transaction
 * @return FS_ERROR_SUCCESS if successful
 */
int32_t handleTransactionVOTs(logical_addr_t tid_last_addr, bool_t is_from_reboot);

/**
 * @brief
 * auxiliary to commitInode(). change file size in inode, according to indirect block location
 * in file, and max offset written to in this block.
 *
 * @param tid transaction id
 * @param ino_buf buffer containing a file inode
 */
void changeFileSize(int32_t tid, uint8_t *ino_buf);

/**
 * @brief
 * commit inode of of given transaction
 *
 * @param tid transaction id
 * @return FS_ERROR_SUCCESS if successful
 * 	       allocAndWriteBlockTid() errors if it failed
 */
error_t
commitInode(int32_t tid);


/**
 * @brief
 * auxiliary to writeTid.
 * check if a given offset is in the indirect offset of the transaction tid
 * @param offset file offset
 * @param tid transaction id whose indirect offset we look at
 * @return 1 if yes, 0 if no
 */
bool_t isOffsetInIndirect(int32_t offset, int32_t tid);

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
findIndirectEntriesBlock(int32_t block_offset, int32_t tid);

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
 * @return 0 if successful, -1 if a read/write error occurs
 */
int32_t writeFileBlock(int32_t block_offset, int32_t len, int32_t dataType,int32_t tid, logical_addr_t log_addr, bool_t writeFlag);

/**
 * @brief
 * commit a directory entry as part of transaction tid.
 *
 * Assumptions:
 * 1. directory inode num is set in the transaction
 *
 * @param ino_num direntry inode number
 * @param f_name direntry file name
 * @param f_type direntry file type
 * @param tid transaction id
 * @return 0 if successful, -1 if a read/write error occured
 */
int32_t writeDirEntry(int32_t ino_num, uint8_t *f_name, uint32_t f_type, int32_t tid);

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
					int32_t tid);

/**
 * @brief
 * commit transaction.
 * - commit inode to flash
 * - commit inode to inode0
 * - change inode0 address to newly written address
 * @param tid transaction id to commit
 * @param ino0_old_addr olf address of inode0
 * @param file_ino_addr file inode address
 * @param isFinal is the commit final ,or temporary
 * @return 0 if successful
 */
int32_t commitTransaction(int32_t tid, logical_addr_t ino0_old_addr, bool_t isFinal);


/**
 * @brief
 * read to buf a block mapped to offset by indirect block of transaction tid
 *
 * @param buf buffer to read data to
 * @param offset file offset to read block from
 * @param tid transaction id
 * @return 0 if successful, FS_ERROR_IO if an io error occured
 */
int32_t readFromTransactionIndirectRead(uint8_t *buf, int32_t offset, int32_t tid);

///**
// * @brief
// * auxiliary to deleteDirEntry. remove a directory entry from a given block, by writing it
// * over with all the following datas
// *
// * @param dirent_ptr directory entry pointer in data buffer
// * @param data_buf data buffer
// */
//void deleteDeFromBlock(dirent_flash *dirent_ptr, uint8_t *data_buf);

/**
 * @brief
 * vot all file blocks of file of transaction tid
 *
 * @param tid transaction id
 * @return 0 if successful,-1 if an error occured, read errors if a read error occured
 */
int32_t votFile(int32_t tid);

/**
 * @brief
 * auxiliary to unlink().
 * delete directory entry in offset de_offset from file with inode id ino_num
 * as part of transaction tid.
 *
 * @param ino_num file inode number
 * @param de_offset. offset in directory of block containig direntry to delete
 * @param tid transaction id
 * @return 0 if usccessful, -1 if errors
 */
int32_t deleteDirEntry(int32_t ino_num, int32_t de_offset, int32_t tid);

/**
 * @brief
 * auxiliary to creat() and mkdir(). creates new file according to given pathname and type
 *
 * @param new_type expected new file type
 * @param pathname pathname for a file.
 *
 * @return if file type is file return the file descriptor on success. if directory 0 is returned on success.
 * return FS_ERROR_IO if an io error occurs
 * return -1 if other error occurs
 */
int32_t createNewFile(int32_t new_type, uint8_t *pathname);

/**
 * @brief
 * auxiliary to mkdir(). check if a directory is empty before deleting it.
 * check all inode entries if they are empty. check that first entry contains only ".", ".."
 *
 * @param ino_num directory inode number
 * @param isEmpty boolean indicator to hold result
 * @return 0 if successful. -1 if a read error occured
 */
int32_t verifyDirectoryEmpty(int32_t ino_num, bool_t *isEmpty);

/**
 * auxiliary to unlink() and mkdir().
 * remove a file given by pathname of type new_type from file system
 *
 * @param new_type file type
 * @param pathname path of file to remove
 * @return 0 if successful, -1 on error
 */
int32_t removeFile(int32_t new_type, uint8_t *pathname);


#endif /*TRANSACTIONS_H_*/

