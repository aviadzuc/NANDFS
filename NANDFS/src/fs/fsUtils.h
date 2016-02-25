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

/** @file fsutils.h
 * File System utilities header, function prototypes, structs etc.
 */

#ifndef FSUTILS_H_
#define FSUTILS_H_

#include <system.h>
#include <src/sequencing/sequencing.h>
#include <src/fs/fs.h>
#include <src/fs/transactions.h>
#include <src/fs/cache.h>

#define IS_NEGATIVE(RES)    ((RES) < 0)
#define init_fsbuf(BUF)     init_struct(BUF, FS_BLOCK_SIZE)

#define IS_SLASH(CHAR)      ((CHAR) == '/')
#define IS_PATH_END(CHAR)   ((CHAR) == '\0')
#define IS_NULL(PTR)        ((PTR) == NULL)

#define setFileNewInode(F_ID, F_TYPE, LOG_ADDR) setNewInode(F_ID, INO_NUM_EMPTY, F_TYPE, LOG_ADDR)

#define init_data_buf(BUF)	fsMemset(buf, FS_EMPTY_FILE_BYTE, FS_BLOCK_SIZE)
#define verifyNameLen(LEN) ((LEN) >0 && (LEN) <= (FS_MAX_NAME_LEN))

#define getFreeReadCache(INO_NUM)  getFreeCacheIndirect(INO_NUM)

#define ENTRY_DIRECT    0
#define ENTRY_INDIRECT  1
#define ENTRY_DOUBLE    2
#define ENTRY_TRIPLE    3

#define FS_ACTIVE_INDIRECT_READ_YES  1
#define FS_ACTIVE_INDIRECT_READ_NO   0
#define ILLEGAL_CHARS_NUM 1

#define fsReadBlockSimple(LOG_ADDR, BUF)  fsReadBlock(LOG_ADDR, BUF, TID_EMPTY, CACHE_ENTRY_OFFSET_EMPTY, FLAG_CACHEABLE_READ_NO)

/****************** function prototypes *********************/
void init_dirstream(NANDFS_DIR* ds);

/**
 * @brief
 * initialize open file entry at offset fd
 * @param fd offset in open file entries array
 */
void init_file_entry(uint32_t fd);

/**
 * @brief
 * init all file entries
 */
void init_file_entries(void);

/**
 * @brief
 * init a single vnode
 */
void init_vnode(uint32_t vnode_idx);

/**
 * @brief
 * init all vnodes
 */
void init_vnodes(void);

/**
 * @brief
 * init a specific indirect cache
 */
void init_indirect_cache(int32_t cid);
/**
 * @brief
 * init all indirect caches
 */
void init_indirect_caches();

void initializeRamStructs();
/**
 * @brief
 * auxiliary to read and write. verify fd is legal
 *
 * @param fd file descriptor
 * @return 0 if legal, -1 otherwise
 */
int32_t verifyFd(int32_t fd);

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
int32_t getFileBlocks(int32_t ino_num);

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
int32_t getFileSize(int32_t ino_num);

/**
 * @brief
 * get a an existing vnode index for a given inode
 * @param ino_num the inode number
 * @return vnode index if successful, VNODE_EMPTY otherwise
 */
int32_t getVnodeByInodeNum(uint32_t ino_num);

/**
 * @brief
 * get a free open file entry
 * @return file descriptor is found one, FD_EMPTY otherwise
 */
int32_t getFreeOpenFileEntry(void);

/**
 * @brief
 * check if we have a conflict between flags of different open file entries:
 * 1. trying to write to an already open file
 * 2. trying to read from a file that's being writen
 * @param vnode_idx index of a file vnode
 * @param flags flags to check conflict
 * @return 1 if a conflict exists, 0 otherwise
 */
error_t isFlagsConflict(uint32_t vnode_idx, uint32_t flags);

/**
 * @brief
 * a mock checkpoint writer function. writes a pseudo file system checkopint data
 * and the sequencing cp data using commit()
 * @param isPartOfHeader is the checkpoint written as part of a header
 * @return 0 if successful, 1 if an error occured in commit
 */
error_t fsCheckpointWriter(bool_t isPartOfHeader);

/**
 * @brief
 * move to next directory entry in a buffer
 * @param dirent_ptr
 * @param offset
 */
void moveToNextDirentry(dirent_flash **dirent_ptr, uint32_t offset);

/**
 * @brief
 * set new directory entry according to given details, in the buffer pointer
 *
 * Assupmtions:
 * 1. name length is legal
 * @param dirent_ptr pointer to a directory entry
 * @param f_id file id
 * @param f_type file type
 * @param name file name
 */
void setNewDirentry(dirent_flash *dirent_ptr, int32_t f_id, uint32_t f_type, uint8_t *name);

/**
 * @brief
 * auxiliary to verifyLegalPathname().
 * check that the character c is not in the illegal characters array
 * @return 1 if legal, 0 if illegal.
 */
error_t isLegalChar(uint8_t c);

/**
 * @brief
 * calc name len
 * @param name the name
 * @return name length
 */
uint32_t calcNameLen(uint8_t *name);

/**
 * @brief
 * verify that a given pathname is composed only from legal characters -
 * 1. 0x00 - 0x1f not accepted, except a configurable set of characters
 * 2. 0x80 - 0xff not accepted unless specified in exclude list for supporting unicode by treating a char as single UTF-8 octet
 * @param pathname pah name to check
 * @return 1 if legal, 0 otherwise
 */
error_t verifyLegalPathname(uint8_t *pathname);

/**
 * @brief
 * find next '/' in pathname, and return offset
 * @return offset in pathname of next '/'
 */
uint32_t findNextSeperator(uint8_t *pathname);

/**
 * @brief
 * initialize file system with inode0, and root directory.
 * use mock transaction
 * @return 0 if successful, -1 otherwise
 */
int32_t handleNoFs(void);

/**
 * @brief
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
int32_t fsReadBlock(logical_addr_t log_addr,
				    uint8_t *buf,
				    int32_t tid,
				    int32_t parent_offset,
				    bool_t is_cacheable);

/**
 * @brief
 * auxiliary to findIndirectEntriesBlock().
 * get file block offset, and calculate it's indirect block offset
 *
 * @return indirect block offset
 */
int32_t calcIndirectBlockOffset(int32_t fileBlockOffset);

/**
 * @brief
 * auxiliary to readFileBlock.
 * read to buf inode block of file with inode number  ino_num, and save inode address to ino_log_addr
 * if we read inode for a file involved in a transaction, make sure to read inode according to transaction data
 *
 * @param buf buffer to save inode to
 * @param ino_num file inode number
 * @param ino_log_addr logical address of inode. if 0, find it from inode0
 * @param tid transaction id
 * @return 0 if successful, -1 otherwise
 */
int32_t readInodeBlock(uint8_t *buf, uint32_t ino_num, logical_addr_t ino_log_addr, int32_t tid);

/**
 * @brief
 * auxiliary to readFileBlock(). get cached indirect block cache of a file
 * identified by it's inode
 *
 * @param ino_num inode number of file
 * @return cache id if found,
 */
int32_t getCachedIndirect(int32_t ino_num);

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
					 bool_t is_active_indirect_read);

/**
 * @brief
 * get a free cache to be related to a given transaction
 *
 * Assumptions:
 * 1. called only when there is such a buffer
 *
 * @param tid the transaction id to reate the buffer to
 * @return pointer to cache
 */
uint8_t *getFreeCacheTid(int32_t tid);

///**
// * @brief
// * initialize caches realted to a given file
// *
// * @param ino_num file id
// */
//void init_related_caches(ino_num);

/**
 * @brief
 * read a block from file ino_num in offset to buf.
 * as an optimiztion for sequential reading, after the first read of a file block, it's inode address is stored
 * in ino_log_addr. this way. sequential calls for this function will not require reading inode0 again
 * @param buf buffer to write data to
 * @param ino_num inode number of the file
 * @param offset offset to read from
 * @param ino_log_addr possible logical address of inode (if empty find it fron inode o)
 * @param tid transaction id during which we are reading this file
 * @return 0 if successful, 1 otherwise
 */
error_t
readFileBlock(uint8_t *buf,
			  int32_t ino_num,
			  uint32_t offset,
			  logical_addr_t ino_log_addr,
			  int32_t tid);
/**
 * @brief
 * compare all directory entries in directory identified by ino_num
 * to name starting in pathname until offset
 * @param pathname name string
 * @param offset offset of name end in pathname
 * @param ino_num inode number of directory file, and where the file inode (if found) will be stored
 * @param f_type the file type we will find (if found)
 * @param directoryOffset file directory entry block offset in parent directory
 * @return 0 if successful
 * -1 in case of io error
 * FS_ERROR_OFST_OVRFLW if directory entry was not found
 */
int32_t compareDirectoryEntries(uint8_t *pathname, uint32_t offset, int32_t *ino_num, uint32_t *f_type, int32_t *directoryOffset);

/**
 * @brief
 * set open file entry with given details
 *
 * @param fd file descriptor
 */
void setFentry(int32_t fd, int32_t flags, int32_t offset, int32_t vnode_idx, int32_t f_type, int32_t ino_num);

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
int32_t setOpenFentry(int32_t ino_num, int32_t flags, uint32_t mode, uint32_t f_type);

/**
 *@brief
 * translate a given pathname to the matching inode number
 * @param pathname path name to file
 * @param f_type pointer to store file type
 * @param de_offset file directory entry block offset in parent directory
 * @param ino_num current working directory inode number
 * @return inode number if the path is legal, -1 otherwise
 */
int32_t namei(uint8_t *pathname, uint32_t *f_type, int32_t *de_offset, int32_t ino_num);

/**
 * @brief
 * set a new inode in fs_buffer.
 * Write first direntries block if it is a directory inode
 *
 * @param f_id file id
 * @param parent_f_id file id of parent of this file (if it is a directory)
 * @param f_type file type
 * @param tid if of transaction associated with this write
 * @return 0 if successful. if an allocAndWriteBlock() error occurs, it's error code is returned
 */
error_t setNewInode(int32_t f_id, int32_t parent_f_id, uint32_t f_type, int32_t tid);

/**
 * @brief
 * verify prefix of a given filename
 * @param pathname the file full pathname
 * @param temp_pathname buffer for temporary pathname storage
 * @param dir_num pointer to store the prefix file inode
 * @return 0 if successful, -1 otherwise
 */
int32_t verifyPrefix(uint8_t *pathname, uint8_t *temp_pathname, int32_t *dir_num, uint32_t *f_offset);

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
int32_t findEmptyDirectEntry(logical_addr_t block_addr,
							 int32_t *offset,
							 int32_t entry_pointer_size,
							 int32_t entry_offset,
							 int32_t tid);

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
					 int32_t tid);

/**
 * @brief
 * verify file ino_num is not open
 *
 * @param ino_num file inode number
 * @return 0 if successful, -1 otherwise
 */
int32_t verifyFileNotOpen(int32_t ino_num);

/**
 * @bried
 * initialize all directory streams
 *
 */
void init_distreams(void);

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
							    int32_t tid);

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
									int32_t tid);

/**
 * @brief
 * find an empty transaction to use
 */
int32_t getFreeDirstreamId(void);

void setDirstreamDirEntry(NANDFS_DIR *ds, dirent_flash *de_ptr, int32_t offset);

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
int32_t readDirStreamNextEntry(NANDFS_DIR *ds);

/**
 * @brief
 * auxiliary to dirstream functions.
 * veridy a given dirstream ds is valid.
 *
 * @param ds dirstream
 * @return 1 if verified, 0 if not
 */
error_t verifyDirstream(NANDFS_DIR *ds);

/**
 * @brief
 * auxiliart to scandir.
 * free array in case of an error
 *
 * @param namelist array to free
 * @param de_idx direntries count
 * @param freer function to free with
 */
void freeNamesArrary(nandfs_dirent **namelist, int32_t de_idx, void(*freer)(void *ptr));

/**
 * @brief
 * auxiliary. this is the actual fsunc fuction. however, since sync()
 * may call fsync several times we need to control the mutex lock externally
 * through lock_mutex directive
 *
 * @param fd the file descriptor.
 * @param lock_mutex boolean indicator whther to lock mutex
 * @return On success, zero is returned. If the file descriptor is illegal -1 is returned.
 * If an error occurs when commiting the file to flash -1 is returned
 */
int32_t fsyncUtil(int32_t fd);

/**
 * create a copy of the file descriptor oldfd
 * makes newfd be the copy of oldfd, closing newfd first if necessary.
 * auxiliary to dup2 and dup (which is actually a private case of dup2)
 *
 * @param oldfd the old file descriptor.
 * @param is_dup2 is this a direct call to dup2, or dup (need to get a free fd)
 * @return the new descriptor, or -1 on failure
 */
int32_t dupUtil(int32_t old_fd, int32_t new_fd, bool_t is_dup2);

/**
 * auxiliary to close() and dup2(). closes a file descriptor, so that it no longer refers to any file
 * and may be reused.
 * if the file was open for writing, close commits it's transaction.
 * if we are called from dup2(), no need to re-lock dup2()
 *
 * @param fd the file descriptor.
 * @param lock_fs_mutex voolean indicator whther tio try to lock fs mutex
 * @return zero on success. On error, -1 is returned
 */
int32_t closeUtil(int32_t fd);

#endif /*FSUTILS_H_*/
