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

/** @file fs.h
 * File System header, function prototypes, structs etc.
 */

#ifndef FS_H_
#define FS_H_

#include <system.h>
#include <src/sequencing/lfsr.h>
#include <src/sequencing/sequencing.h>

/* some minimum requirements*/
#define FS_MIN_LRU_Q_SIZE          4 /* lru ar least > 4*/
#define FS_BUFFERS_PER_TRANSACTION 3 /* at least 3 buffers for 1 transaction */
#define FS_MIN_N_TRANSACTIONS      1 /* at least one transaction*/

/* general definitions*/
#define FS_MAX_VNODES            6
#define FS_MAX_OPEN_FILES        8
#define FS_MAX_OPEN_DIRESTREAMS  2
#define LRU_CACHE_SIZE           0 /* we need at least FS_MIN_LRU_Q_SIZE*/
#define FS_CACHE_BUFFERS         ((LRU_CACHE_SIZE>=FS_MIN_LRU_Q_SIZE)?LRU_CACHE_SIZE:0)

#define FS_MAX_N_TRANSACTIONS      2
//#define FS_EXTRA_INDIRECT_CACHES   0
//#define FS_CACHE_BUFFERS           ((FS_MIN_N_TRANSACTIONS*FS_BUFFERS_PER_TRANSACTION)+FS_EXTRA_INDIRECT_CACHES)
//#define FS_MAX_N_TRANSACTIONS      (FS_CACHE_BUFFERS/FS_BUFFERS_PER_TRANSACTION)


#define FS_EMPTY_FILE_BYTE    0xff
#define FS_BLOCK_SIZE         (CAST_VAL_TO_UINT32(NAND_PAGE_SIZE))
#define POWER_FS_BLOCK_SIZE   POWER_NAND_PAGE_SIZE
#define FS_CHECKPOINT_PAGE_COUNT (CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+sizeof(filesystem_t)))

#include <src/fs/transactions.h>

#define CALC_IN_BLOCKS(OFFSET)         ((OFFSET) >> POWER_NAND_PAGE_SIZE)
#define CALC_IN_BYTES(BLOCKS)          ((BLOCKS) << (POWER_NAND_PAGE_SIZE))

#define CALC_BLOCK_START(FILE_OFFSET)  CALC_IN_BYTES(CALC_IN_BLOCKS(FILE_OFFSET))

#define IS_ADDR_EMPTY(LOG_ADDR_PTR)    ((0xffffff & (*((uint32_t*)LOG_ADDR_PTR))) == 0xffffff)
#define IS_ADDR_NEGATIVE(LOG_ADDR_PTR) (IS_NEGATIVE((*((int32_t*)(LOG_ADDR_PTR)))))
#define MARK_ADDR_NEGATIVE(LOG_ADDR)   *(CAST_TO_UINT32(LOG_ADDR)) |= 0x80000000

/* data types */
#define DATA_TYPE_EMPTY    -1
#define DATA_TYPE_REGULAR  0
#define DATA_TYPE_VOTS     1
#define DATA_TYPE_ENTRIES  2
#define DATA_TYPE_DIRENTS  3
#define DATA_TYPE_INODE    4

/************* inode defintions **********************/
#define LOG_ADDRESSES_PER_BLOCK        CALC_IN_LOG_ADDRESSES(FS_BLOCK_SIZE)
#define POWER_LOG_ADDRESSES_PER_BLOCK  (POWER_FS_BLOCK_SIZE -POWER_SEQ_LOG_ADDR_SIZE)

#define INODE_SIZE                 NAND_PAGE_SIZE
#define TRIPLE_INDEX_ENTRIES	   1
#define TRIPLE_INDEX_LOCATION      (INODE_SIZE-(TRIPLE_INDEX_ENTRIES*sizeof(logical_addr_struct)))
#define DOUBLE_INDEX_ENTRIES	   1
#define DOUBLE_INDEX_LOCATION	   (TRIPLE_INDEX_LOCATION-(DOUBLE_INDEX_ENTRIES*sizeof(logical_addr_struct)))
#define INDIRECT_INDEX_ENTRIES	   1
#define INDIRECT_INDEX_LOCATION    (DOUBLE_INDEX_LOCATION-(INDIRECT_INDEX_ENTRIES*sizeof(logical_addr_struct)))
#define DIRECT_INDEX_ENTRIES	   10
#define DIRECT_INDEX_LOCATION	   (INDIRECT_INDEX_LOCATION-(DIRECT_INDEX_ENTRIES*sizeof(logical_addr_struct)))
#define FILE_ID_LOCATION		   (DIRECT_INDEX_LOCATION-sizeof(int32_t))
#define NBLOCKS_LOCATION 		   (FILE_ID_LOCATION-sizeof(uint32_t))
#define NBYTES_LOCATION 		   (NBLOCKS_LOCATION-sizeof(uint32_t))
#define INODE_FILE_TYPE_LOCATION   (NBYTES_LOCATION-sizeof(uint8_t))
#define INODE_FILE_DATA_LOCATION   0

#define CALC_LOG_ADDR_OFFSET_IN_BLOCK(BYTE_OFFSET) (BYTE_OFFSET >> POWER_SEQ_LOG_ADDR_SIZE)

#define TRIPLE_INDEX_OFFSET_IN_LOG_ADDRESSES   CALC_LOG_ADDR_OFFSET_IN_BLOCK(TRIPLE_INDEX_LOCATION)
#define DOUBLE_INDEX_OFFSET_IN_LOG_ADDRESSES   CALC_LOG_ADDR_OFFSET_IN_BLOCK(DOUBLE_INDEX_LOCATION)
#define INDIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES CALC_LOG_ADDR_OFFSET_IN_BLOCK(INDIRECT_INDEX_LOCATION)
#define DIRECT_INDEX_OFFSET_IN_LOG_ADDRESSES   CALC_LOG_ADDR_OFFSET_IN_BLOCK(DIRECT_INDEX_LOCATION)

#define POWER_INODE_INDIRECT_DATA_SIZE (POWER_LOG_ADDR_PER_PAGE + POWER_FS_BLOCK_SIZE)
#define POWER_INODE_DOUBLE_DATA_SIZE   (POWER_INODE_INDIRECT_DATA_SIZE + POWER_LOG_ADDR_PER_PAGE)
#define POWER_INODE_TRIPLE_DATA_SIZE   (POWER_INODE_DOUBLE_DATA_SIZE + POWER_LOG_ADDR_PER_PAGE)

#define INODE_FILE_DATA_SIZE      INODE_FILE_TYPE_LOCATION /* file beginning, whose data is saved in the inode*/
#define INODE_DIRECT_DATA_SIZE    CALC_IN_BYTES(DIRECT_INDEX_ENTRIES)
#define INODE_INDIRECT_DATA_SIZE  CALC_IN_BYTES(CALC_IN_LOG_ADDRESSES(FS_BLOCK_SIZE))
#define INODE_DOUBLE_DATA_SIZE    CALC_IN_BYTES(CALC_IN_LOG_ADDRESSES(INODE_INDIRECT_DATA_SIZE))
#define INODE_TRIPLE_DATA_SIZE    CALC_IN_BYTES(CALC_IN_LOG_ADDRESSES(INODE_DOUBLE_DATA_SIZE))

#define CALC_OFFSET_IN_INDIRECT_SIZE(FILE_OFFSET)    ((FILE_OFFSET) & ((1 << POWER_INODE_INDIRECT_DATA_SIZE)-1))
#define CALC_OFFSET_IN_INDIRECT_BLOCK(FILE_OFFSET)   ((FILE_OFFSET) >> POWER_FS_BLOCK_SIZE)
#define CALC_OFFSET_IN_DOUBLE_SIZE(FILE_OFFSET)      ((FILE_OFFSET) & ((1 << POWER_INODE_DOUBLE_DATA_SIZE)-1))
#define CALC_OFFSET_IN_DOUBLE_BLOCK(FILE_OFFSET)     ((FILE_OFFSET) >> POWER_INODE_INDIRECT_DATA_SIZE)
#define CALC_OFFSET_IN_TRIPLE_BLOCK(FILE_OFFSET)     ((FILE_OFFSET) >> POWER_INODE_DOUBLE_DATA_SIZE)

#define INDIRECT_DATA_OFFSET              (INODE_FILE_DATA_SIZE+INODE_DIRECT_DATA_SIZE)
#define DOUBLE_DATA_OFFSET                (INDIRECT_DATA_OFFSET+INODE_INDIRECT_DATA_SIZE)
#define TRIPLE_DATA_OFFSET                (DOUBLE_DATA_OFFSET+INODE_DOUBLE_DATA_SIZE)

#define CALC_OFFSET_IN_FILE_BLOCK(OFFSET) ((OFFSET >= INODE_FILE_DATA_SIZE)?CALC_OFFSET_IN_BLOCK(OFFSET-INODE_FILE_DATA_SIZE):0)
#define CALC_FILE_OFFSET(INDIRECTS, DOUBLES) (INDIRECT_DATA_OFFSET + ((INDIRECTS)*INODE_INDIRECT_DATA_SIZE) + ((DOUBLES)*INODE_DOUBLE_DATA_SIZE))
#define CALC_OFFSET_FROM_INDIRECT(OFFSET) (OFFSET-INDIRECT_DATA_OFFSET)
#define CALC_OFFSET_FROM_DOUBLE(OFFSET)   (CALC_OFFSET_FROM_INDIRECT(OFFSET)-INODE_INDIRECT_DATA_SIZE)
#define CALC_OFFSET_FROM_TRIPLE(OFFSET)   (CALC_OFFSET_FROM_DOUBLE(OFFSET)-INODE_DOUBLE_DATA_SIZE)

#define INODE_GET_TRIPLE(INODE_PTR, LOG_ADDR)      copyLogicalAddress(LOG_ADDR, CAST_TO_LOG_ADDR(GET_UINT32_PTR(INODE_PTR, TRIPLE_INDEX_LOCATION)))
#define INODE_GET_DOUBLE(INODE_PTR, LOG_ADDR)      copyLogicalAddress(LOG_ADDR, CAST_TO_LOG_ADDR(GET_UINT32_PTR(INODE_PTR, DOUBLE_INDEX_LOCATION)))
#define INODE_GET_INDIRECT(INODE_PTR, LOG_ADDR)    copyLogicalAddress(LOG_ADDR, CAST_TO_LOG_ADDR(GET_UINT32_PTR(INODE_PTR, INDIRECT_INDEX_LOCATION)))
#define INODE_GET_DIRECT(INODE_PTR, NUM, LOG_ADDR) copyLogicalAddress(LOG_ADDR, CAST_TO_LOG_ADDR(GET_UINT32_PTR(INODE_PTR, (DIRECT_INDEX_LOCATION+CALC_LOG_ADDR_IN_BYTES(NUM)))))
#define INODE_GET_FILE_ID(INODE_PTR)     GET_INT32(INODE_PTR, FILE_ID_LOCATION)
#define INODE_GET_NBLOCKS(INODE_PTR)     GET_UINT32(INODE_PTR, NBLOCKS_LOCATION)
#define INODE_GET_NBYTES(INODE_PTR)      GET_UINT32(INODE_PTR, NBYTES_LOCATION)
#define INODE_GET_FILE_TYPE(INODE_PTR)   GET_BYTE(INODE_PTR, INODE_FILE_TYPE_LOCATION)

#define INODE_SET_TRIPLE(INODE_PTR, LOG_ADDR)      copyLogicalAddress(CAST_TO_LOG_ADDR(GET_UINT32_PTR(INODE_PTR, TRIPLE_INDEX_LOCATION)),LOG_ADDR)
#define INODE_SET_DOUBLE(INODE_PTR, LOG_ADDR)      copyLogicalAddress(CAST_TO_LOG_ADDR(GET_UINT32_PTR(INODE_PTR, DOUBLE_INDEX_LOCATION)),LOG_ADDR)
#define INODE_SET_INDIRECT(INODE_PTR, LOG_ADDR)    copyLogicalAddress(CAST_TO_LOG_ADDR(GET_UINT32_PTR(INODE_PTR, INDIRECT_INDEX_LOCATION)),LOG_ADDR)
#define INODE_SET_DIRECT(INODE_PTR, NUM, LOG_ADDR) copyLogicalAddress(CAST_TO_LOG_ADDR(GET_UINT32_PTR(INODE_PTR, DIRECT_INDEX_LOCATION+CALC_LOG_ADDR_IN_BYTES(NUM))),LOG_ADDR)

#define INODE_SET_FILE_ID(INODE_PTR, VAL)     SET_INT32(INODE_PTR, FILE_ID_LOCATION, VAL)
#define INODE_SET_NBLOCKS(INODE_PTR, VAL)     SET_UINT32(INODE_PTR, NBLOCKS_LOCATION, VAL)
#define INODE_SET_NBYTES(INODE_PTR, VAL)      SET_UINT32(INODE_PTR, NBYTES_LOCATION, VAL)
#define INODE_SET_FILE_TYPE(INODE_PTR, VAL)   SET_BYTE(INODE_PTR, INODE_FILE_TYPE_LOCATION, VAL)

#define CAST_TO_INODE(BUF) ((inode_t*)(BUF))
#define INODE_GET_DIRECT(INODE_PTR, NUM, LOG_ADDR) copyLogicalAddress(LOG_ADDR, CAST_TO_LOG_ADDR(GET_UINT32_PTR(INODE_PTR, (DIRECT_INDEX_LOCATION+CALC_LOG_ADDR_IN_BYTES(NUM)))))
#define BLOCK_GET_ADDR_PTR_BY_INDEX(BUF, NUM)      (&(CAST_TO_LOG_ADDR(BUF)[NUM]))
#define BLOCK_GET_INDEX(BUF, NUM, LOG_ADDR)        copyLogicalAddress(LOG_ADDR, BLOCK_GET_ADDR_PTR_BY_INDEX(BUF, NUM))
#define BLOCK_SET_INDEX(BUF, NUM, LOG_ADDR)        copyLogicalAddress(BLOCK_GET_ADDR_PTR_BY_INDEX(BUF, NUM), LOG_ADDR)

/* block sparseness. a block has a hole if:
 *    a. it doesn't contain data beyond half=block size in directory block
 * or b. not used in inode0*/
#define ADDR_FLAG_FREE                   1
#define ADDR_FLAG_TAKEN                  0
#define SET_ADDR_FLAG(LOG_ADDR, FLAG)	 *CAST_TO_UINT32(LOG_ADDR) = (((*CAST_TO_UINT32(LOG_ADDR)) & 0x7fffffff) | (FLAG << 31))
#define GET_ADDR_FLAG(LOG_ADDR)			 ((*CAST_TO_UINT32(LOG_ADDR)) >> 31)
#define IS_ADDR_FREE(LOG_ADDR)           (GET_ADDR_FLAG(LOG_ADDR) == ADDR_FLAG_FREE)
#define MARK_BLOCK_WITH_HOLE(LOG_ADDR)   SET_ADDR_FLAG(LOG_ADDR, ADDR_FLAG_FREE)
#define MARK_BLOCK_NO_HOLE(LOG_ADDR)     SET_ADDR_FLAG(LOG_ADDR, ADDR_FLAG_TAKEN)

#define IS_NOT_REWRITE(OLD_LOG_ADDR)     (IS_ADDR_EMPTY(OLD_LOG_ADDR) && IS_BLOCK_FRESH(OLD_LOG_ADDR))

#define INO_NUM_EMPTY 0xffffffff
#define FTYPE_EMPTY   0x00
#define FTYPE_DIR     0x01
#define FTYPE_FILE    0x02
#define IS_DIRECTORY(TYPE)               ((TYPE) == FTYPE_DIR)
#define IS_FILE(TYPE)                    ((TYPE) == FTYPE_FILE)
#define IS_INO_EMPTY(INO_NUM)            ((INO_NUM) == INO_NUM_EMPTY)
#define IS_INODE0(INO_NUM)				 ((INO_NUM) == 0)
#define CALC_IN_INODES(INO0_OFFSET)      (CALC_IN_BLOCKS(INO0_OFFSET-INODE_FILE_DATA_SIZE)+1)
#define CALC_OFFSET_FROM_INODE(INO_NUM)  (CALC_IN_BYTES(INO_NUM-1)+INODE_FILE_DATA_SIZE)

typedef struct{
	uint8_t bytes[INODE_SIZE];
} inode_t;

//typedef struct {
//	uint8_t  data[FS_BLOCK_SIZE-16*4-1];
//	uint8_t  file_type;	// directory/file
//	uint32_t nBytes;      // file size in bytes
//	uint32_t nBlocks;   // physical blocks occupied by the file
//	uint32_t file_id;
//	uint32_t block_index[INDEX_ARRAY_SIZE]; // allocate 0-(triple indirect location) for indexers
//} inode_t;

/* directory entries definitions */
#define DIRENT_MAX_SIZE		       256
#define DIRENT_FLASH_SIZE          10
#define DIRENT_FLASH_FIELDS_SIZE   (DIRENT_FLASH_SIZE-1)
#define DIRENT_FILE_TYPE_LOCATION  0
#define DIRENT_INO_NUM_LOCATION    (DIRENT_FILE_TYPE_LOCATION+sizeof(uint8_t))
#define DIRENT_LEN_LOCATION        (DIRENT_INO_NUM_LOCATION+sizeof(uint32_t))
#define DIRENT_NAME_LOCATION       (DIRENT_LEN_LOCATION+sizeof(uint32_t))
#define DIRENT_MAX_NAME_LENGTH     (DIRENT_MAX_SIZE-DIRENT_NAME_LOCATION-1)

#define DIRENT_GET_TYPE(DE_PTR)	   GET_BYTE(DE_PTR, DIRENT_FILE_TYPE_LOCATION)
#define DIRENT_GET_INO_NUM(DE_PTR) GET_INT32(DE_PTR, DIRENT_INO_NUM_LOCATION)
#define DIRENT_GET_LEN(DE_PTR)     GET_UINT32(DE_PTR, DIRENT_LEN_LOCATION)
#define DIRENT_GET_NAME(DE_PTR)    GET_BYTE_PTR(DE_PTR, DIRENT_NAME_LOCATION)

#define DIRENT_SET_TYPE(DE_PTR, VAL)     SET_BYTE(DE_PTR, DIRENT_FILE_TYPE_LOCATION, VAL)
#define DIRENT_SET_INO_NUM(DE_PTR, VAL)  SET_INT32(DE_PTR, DIRENT_INO_NUM_LOCATION, VAL)
#define DIRENT_SET_LEN(DE_PTR, NAME_LEN) SET_UINT32(DE_PTR, DIRENT_LEN_LOCATION, NAME_LEN+DIRENT_FLASH_FIELDS_SIZE+1) /* len is total record len - name len +ino num + type+'\0\*/
#define DIRENT_SET_NAME(DE_PTR, NAME)    fsStrcpy(CAST_TO_UINT8(&(DE_PTR->bytes[DIRENT_NAME_LOCATION])),NAME)

#define DIRENT_GET_NAME_LEN(DE_PTR)      ((DIRENT_GET_LEN(DE_PTR)) - (uint32_t)(DIRENT_FLASH_FIELDS_SIZE+1))

#define FS_MAX_NAME_LEN               DIRENT_MAX_NAME_LENGTH
#define CAST_TO_DIRENT(BUF)           ((dirent_flash*)(BUF))
#define CALC_OFFSET_IN_BLOCK(OFFSET)  ((OFFSET) & ((1 << POWER_FS_BLOCK_SIZE)-1))
#define IS_DIRENT_EMPTY(DE_PTR)       IS_INO_EMPTY(DIRENT_GET_INO_NUM(DE_PTR))
#define IS_DIRENT_BLOCK_FULL(OFFSET)  (FS_BLOCK_SIZE-CALC_OFFSET_IN_FILE_BLOCK(OFFSET) < sizeof(nandfs_dirent))
typedef struct {
	uint8_t bytes[DIRENT_FLASH_SIZE];
} dirent_flash;

typedef struct {
	uint8_t bytes[DIRENT_MAX_SIZE];
} nandfs_dirent;
/**
 * @struct
 * directory entry.
 */
//typedef struct __dirent{
//	uint8_t	    d_type;      /** type of file */
//	uint32_t	d_ino;       /** inode number */
//  uint32_t	d_len;    /** length of this record */
//  uint8_t		*d_name; /** filename */
//} dirent_t;

#define GET_VNODE_BY_IDX(IDX)         (&(vnodes[IDX]))

#define VNODE_GET_INO_NUM(VNODE_IDX)      (GET_VNODE_BY_IDX(VNODE_IDX)->ino_num)
#define VNODE_GET_NREFS(VNODE_IDX)        (GET_VNODE_BY_IDX(VNODE_IDX)->nReferences)
#define VNODE_SET_INO_NUM(VNODE_IDX, VAL) GET_VNODE_BY_IDX(VNODE_IDX)->ino_num     = VAL
#define VNODE_SET_NREFS(VNODE_IDX, VAL)   GET_VNODE_BY_IDX(VNODE_IDX)->nReferences = VAL

#define VNODE_DECREMENT_NREFS(VNODE_IDX)   GET_VNODE_BY_IDX(VNODE_IDX)->nReferences--
#define VNODE_INCREMENT_NREFS(VNODE_IDX)   GET_VNODE_BY_IDX(VNODE_IDX)->nReferences++

#define getFreeVnode() 				  getVnodeByInodeNum(INO_NUM_EMPTY)
#define IS_OPEN_FILE_ENTRY_EMPTY(FD)  IS_VNODE_EMPTY(OPEN_FILE_GET_VNODE(FD))
#define IS_VNODE_EMPTY(VNODE_IDX)     ((VNODE_IDX) == (VNODE_EMPTY))
typedef struct{
	int32_t ino_num;   // pointer to the inode info in-memory
	uint32_t nReferences; // file entry references
} vnode_t;

#define UID_EMPTY   0

typedef struct {
   processid_t process;
   user_id user;
} process_t;

/* open file entry macros*/
#define GET_ENTRY_BY_FD(FD)         (&(open_files[FD]))

#define OPEN_FILE_GET_FLAGS(FD)    (GET_ENTRY_BY_FD(FD)->flags)
#define OPEN_FILE_GET_UID(FD)      (GET_ENTRY_BY_FD(FD)->uid)
#define OPEN_FILE_GET_OFFSET(FD)   (GET_ENTRY_BY_FD(FD)->offset)
#define OPEN_FILE_GET_VNODE(FD)    (GET_ENTRY_BY_FD(FD)->vnode)
#define OPEN_FILE_GET_FTYPE(FD)    (GET_ENTRY_BY_FD(FD)->f_type)

#define OPEN_FILE_SET_FLAGS(FD, VAL)    GET_ENTRY_BY_FD(FD)->flags  = VAL
#define OPEN_FILE_SET_UID(FD, VAL)      GET_ENTRY_BY_FD(FD)->uid    = VAL
#define OPEN_FILE_SET_OFFSET(FD, VAL)   GET_ENTRY_BY_FD(FD)->offset = VAL
#define OPEN_FILE_SET_VNODE(FD, VNODE)  GET_ENTRY_BY_FD(FD)->vnode  = VNODE
#define OPEN_FILE_SET_FTYPE(FD, FTYPE)  GET_ENTRY_BY_FD(FD)->f_type = FTYPE
#define OPEN_FILE_INC_OFFSET(FD, VAL)   OPEN_FILE_SET_OFFSET(FD, OPEN_FILE_GET_OFFSET(FD)+VAL)
#define VNODE_EMPTY  -1
#define FD_EMPTY     -1

#define IS_FD_EMPTY(FD)			      ((FD) == FD_EMPTY)
#define IS_FD_ERROR(FD)               ((FD) <0)
#define IS_FD_LEGAL(FD)				  ((FD) >=0 && ((FD) < (FS_MAX_OPEN_FILES)))
#define IS_USER_LEGAL(FD)			  (OPEN_FILE_GET_UID(FD) == GET_CURRENT_USER())
#define IS_OPEN_FILE_ENTRY_EMPTY(FD)  IS_VNODE_EMPTY(OPEN_FILE_GET_VNODE(FD))
#define GET_FILE_ID_BY_FD(FD)         ((IS_OPEN_FILE_ENTRY_EMPTY(FD))?INO_NUM_EMPTY:VNODE_GET_INO_NUM(OPEN_FILE_GET_VNODE(FD)))

/* various macros */
#define ADVANCE_TO_NEXT_BLOCK(FILE_OFFSET)    FILE_OFFSET = FILE_OFFSET-CALC_OFFSET_IN_FILE_BLOCK(FILE_OFFSET) + FS_BLOCK_SIZE
#define IS_PAST_BLOCK_HALF(OFFSET)            (CALC_OFFSET_IN_BLOCK(OFFSET) > (DIV_BY_2(FS_BLOCK_SIZE)))
#define getInode(BUF, INO_NUM, INO_LOG_ADDR)  readFileBlock(BUF, 0, INODE_FILE_DATA_SIZE+CALC_IN_BYTES(INO_NUM-1),INO_LOG_ADDR, TID_EMPTY)

/* regular function call in fs layer verification. should return 1 */
#define SYS_VERIFY(TEST)              if (!(TEST)){ \
									     FS_MUTEX_UNLOCK(); \
									     L("ERROR"); \
									     return -1; \
										}
#define SYS_RES_VERIFY(RES_VAR, ACTION)  RES_VAR = ACTION; \
									 if(RES_VAR){ \
									 	FS_MUTEX_UNLOCK(); \
									 	L("ERROR res %d", RES_VAR); \
										return RES_VAR; \
									 }

/* regular function call in fs layer verification. should return 1 */
#define FS_VERIFY(TEST)              if (!(TEST)) {L("ERROR"); return -1;}
/* function verification which returns a special error code*/
#define RES_VERIFY(RES_VAR, ACTION)  RES_VAR = ACTION; \
									 if(RES_VAR){ \
										L("ERROR res %d", RES_VAR); \
										return RES_VAR; \
									 }

#define SYS_RETURN(RES)              FS_MUTEX_UNLOCK(); \
									 return RES
typedef struct {
	uint32_t flags;
	uint32_t uid;
	int32_t  offset; /* current file offset */
	int32_t  vnode;
	uint32_t f_type;
} open_file_t;

/* stat struct. fields are standard.
 * example: http://linux.die.net/include/bits/stat.h*/
typedef struct __stat {
    uint32_t   st_ino;     /** inode number */
    uint32_t   st_size;    /** total size, in bytes */
    uint32_t   st_blksize; /** block size for filesystem I/O */
    int32_t    st_blocks;  /** number of blocks allocated */
    uint64_t   st_dev;     /** ID of device containing file */
} file_stat_t;

#define STAT_GET_INO_NUM(STAT)  (STAT->st_ino)
#define STAT_GET_SIZE(STAT)     (STAT->st_size)
#define STAT_GET_BLK_SIZE(STAT) (STAT->st_blksize)
#define STAT_GET_BLOCKS(STAT)   (STAT->st_blocks)
#define STAT_GET_DEV_ID(STAT)   (STAT->st_dev)

#define STAT_SET_INO_NUM(STAT, VAL)  STAT->st_ino = VAL
#define STAT_SET_SIZE(STAT, VAL)     STAT->st_size = VAL
#define STAT_SET_BLK_SIZE(STAT, VAL) STAT->st_blksize = VAL
#define STAT_SET_BLOCKS(STAT, VAL)   STAT->st_blocks = VAL
#define STAT_SET_DEV_ID(STAT, VAL)   STAT->st_dev = VAL

/* dirstream definitions*/
typedef struct
{
    int32_t             fd; /* -1 for failed rewind */
    logical_addr_struct ino_addr;
    nandfs_dirent              de; /* d_name null iff first time */
} fs_dirstream;

typedef fs_dirstream NANDFS_DIR;
#define DS_ID_EMPTY    -1
#define IS_EMPTY_DS_ID(DS_ID)         ((DS_ID) == DS_ID_EMPTY)
#define IS_DIRSTREAM_EMPTY(DS_ID)     (IS_NEGATIVE(DS_GET_FD(DS_ID)))
#define IS_DIRSTREAM_EMPTY_BY_PTR(DS) (IS_NEGATIVE(DS_GET_FD_BY_PTR(DS)))

#define GET_DS_BY_ID(DS_ID)         (&(dirstreams[DS_ID]))
#define DS_GET_FD(DS_ID)			(GET_DS_BY_ID(DS_ID)->fd)
#define DS_GET_DIR_OFFSET(DS_ID)	(GET_DS_BY_ID(DS_ID)->dir_offset)
#define DS_GET_INO_ADDR_PTR(DS_ID)  (&(GET_DS_BY_ID(DS_ID)->ino_addr))
#define DS_GET_INO_ADDR(DS_ID, LOG_ADDR) copyLogicalAddress(LOG_ADDR, DS_GET_INO_ADDR_PTR(DS_ID))
#define DS_GET_DIRENTRY_PTR(DS)  	(&(DS->de))
#define DS_GET_DIRENTRY_LEN(DS)     DIRENT_GET_LEN(DS_GET_DIRENTRY_PTR(DS))

#define DS_GET_FD_BY_PTR(DS)			     (DS->fd)
#define DS_GET_DIR_OFFSET_BY_PTR(DS)	     OPEN_FILE_GET_OFFSET(DS_GET_FD_BY_PTR(DS))
#define DS_GET_INO_ADDR_PTR_BY_PTR(DS)       (&(DS->ino_addr))
#define DS_GET_INO_ADDR_BY_PTR(DS, LOG_ADDR) copyLogicalAddress(LOG_ADDR, DS_GET_INO_ADDR_PTR_BY_PTR(DS))
#define DS_GET_DIRENTRY_PTR_BY_PTR(DS)       (&(DS->de))

#define DS_SET_FD(DS_ID, VAL)		     GET_DS_BY_ID(DS_ID)->fd = VAL
#define DS_SET_DIR_OFFSET(DS_ID, VAL)    OPEN_FILE_SET_OFFSET(DS_GET_FD(DS), VAL)
#define DS_SET_INO_ADDR(DS_ID, LOG_ADDR) copyLogicalAddress(DS_GET_INO_ADDR_PTR(DS_ID), LOG_ADDR)
#define DS_SET_DIRENTRY(DS_ID, DE_PTR)   fsMemcpy(DS_GET_DIRENTRY_PTR(DS_ID), DE_PTR, sizeof(nandfs_dirent))

#define DS_SET_FD_BY_PTR(DS, VAL)		     DS->fd = VAL
#define DS_SET_DIR_OFFSET_BY_PTR(DS, VAL)    OPEN_FILE_SET_OFFSET(DS_GET_FD_BY_PTR(DS), VAL)
#define DS_SET_INO_ADDR_BY_PTR(DS, LOG_ADDR) copyLogicalAddress(DS_GET_INO_ADDR_PTR_BY_PTR(DS), LOG_ADDR)
#define DS_SET_DIRENTRY_BY_PTR(DS, DE_PTR)   fsMemcpy(DS_GET_DIRENTRY_PTR_BY_PTR(DS), DE_PTR, sizeof(nandfs_dirent))

#define DIRECTORY_FIRST_ENTRY_OFFSET   INODE_FILE_DATA_SIZE

/* process macros*/
extern process_t *currentProcess;
#define GET_CURRENT_USER()     (currentProcess->user)
#define SET_CURRENT_USER(USER) currentProcess->user = USER

/* file system fields*/
#define FS_INO0_LOCATION                  0
#define FS_LAST_CLOSED_TID_ADDR_LOCATION  (FS_INO0_LOCATION+sizeof(logical_addr_struct))
#define FS_OPEN_TIDS_ARRAY_LOCATION       (FS_LAST_CLOSED_TID_ADDR_LOCATION+sizeof(logical_addr_struct))
#define FS_MUTEX_LOCATION				  (FS_OPEN_TIDS_ARRAY_LOCATION+FS_MAX_N_TRANSACTIONS*(sizeof(logical_addr_struct)))

#define FS_GET_INO0_ADDR_PTR()                 CAST_TO_LOG_ADDR(CAST_TO_UINT32(&(fs_ptr->bytes[FS_INO0_LOCATION])))
#define FS_GET_LAST_CLOSED_TID_ADDR()          CAST_TO_LOG_ADDR(CAST_TO_UINT32(&(fs_ptr->bytes[FS_LAST_CLOSED_TID_ADDR_LOCATION])))
#define FS_GET_TID_LAST_WRITTEN_ADDR(TID)      CAST_TO_LOG_ADDR(CAST_TO_UINT32(&(fs_ptr->bytes[FS_OPEN_TIDS_ARRAY_LOCATION+TID*sizeof(logical_addr_struct)])))
#define FS_GET_MUTEX_PTR()					   CAST_TO_MUTEX(&(fs_ptr->bytes[FS_MUTEX_LOCATION]))

#define FS_SET_INO0_ADDR(ADDR)                  copyLogicalAddress(FS_GET_INO0_ADDR_PTR(), ADDR)
#define FS_SET_LAST_CLOSED_TID_ADDR(ADDR)       copyLogicalAddress(FS_GET_LAST_CLOSED_TID_ADDR(), ADDR)
#define FS_SET_TID_LAST_WRITTEN_ADDR(TID, ADDR) copyLogicalAddress(FS_GET_TID_LAST_WRITTEN_ADDR(TID), ADDR)

/* mutex related macros*/
#ifndef ECOS_OS
#define FS_MUTEX_INIT()      *FS_GET_MUTEX_PTR() = 0
#define FS_MUTEX_LOCK()      if(*FS_GET_MUTEX_PTR()) return -1; else *FS_GET_MUTEX_PTR()  = 1
#define FS_MUTEX_TRYLOCK()
#define FS_MUTEX_UNLOCK()    if(!(*FS_GET_MUTEX_PTR())) return -1; else *FS_GET_MUTEX_PTR()  = 0
#define FS_MUTEX_RELEASE()
#else
#define FS_MUTEX_INIT()      cyg_mutex_init(FS_GET_MUTEX_PTR())
#define FS_MUTEX_LOCK()      cyg_mutex_lock(FS_GET_MUTEX_PTR())
#define FS_MUTEX_TRYLOCK()   cyg_mutex_trylock(FS_GET_MUTEX_PTR())
#define FS_MUTEX_UNLOCK()    cyg_mutex_unlock(FS_GET_MUTEX_PTR())
#define FS_MUTEX_RELEASE()   cyg_mutex_release(FS_GET_MUTEX_PTR())
#endif

#define LOCK_MUTEX  1

#define FILESYSTEM_T_SIZE      ((1+FS_MAX_N_TRANSACTIONS)*sizeof(logical_addr_struct)+sizeof(logical_addr_struct)+sizeof(nandfs_mutex_t))
#define init_fs() 			   init_struct(fs_ptr, sizeof(filesystem_t))
//typedef struct{
//	logical_addr_struct root_addr;
//	logical_addr_struct last_close_tid_addr;
//	logical_addr_struct tids_array[FS_MAX_N_TRANSACTIONS];
//} filesystem_t;

typedef struct{
	uint8_t bytes[FILESYSTEM_T_SIZE];
} filesystem_t;

#ifdef Debug
	#define VALID_MOCK_FILE_NAME_1   "/valid_file1.dat"
	#define VALID_MOCK_INO_NUM_1     1000
	#define VALID_MOCK_FILE_NAME_2   "/valid_file2.dat"
	#define VALID_MOCK_INO_NUM_2     2000
	#define INVALID_MOCK_FILE_NAME_1 "/invalid_file1.dat"
#endif

typedef filesystem_t* fs_t;

/* open file flags */
#define NANDFS_O_RDONLY 0x00000001
#define NANDFS_O_WRONLY 0x00000010
#define NANDFS_O_RDWR   (NANDFS_O_WRONLY | NANDFS_O_RDONLY)
#define NANDFS_O_APPEND 0x00000100
#define NANDFS_O_CREAT  0x00001000
#define NANDFS_O_EXCL   0x00010000

#define CREAT_FLAGS (NANDFS_O_CREAT | NANDFS_O_WRONLY)

#define IS_RDONLY(FLAGS)  ((FLAGS) & NANDFS_O_RDONLY)
#define IS_WRONLY(FLAGS)  ((FLAGS) & NANDFS_O_WRONLY)
#define IS_APPEND(FLAGS)  ((FLAGS) & NANDFS_O_APPEND)
#define IS_CREAT(FLAGS)   ((FLAGS) & NANDFS_O_CREAT)
#define IS_EXCL(FLAGS)    ((FLAGS) & NANDFS_O_EXCL)

#define IS_CREAT_EXCL(FLAGS)         (IS_EXCL(FLAGS) && IS_CREAT(FLAGS))
#define IS_TRANSACTIONAL_OPEN(FLAGS) (IS_WRONLY(FLAGS) || IS_APPEND(FLAGS) || IS_CREAT(FLAGS))

/* modes */
#define S_IRWXU 0x00700
#define S_IRUSR 0x00400
#define S_IWUSR 0x00200
#define S_IXUSR 0x00100
#define S_IRWXG 0x00070
#define S_IRGRP 0x00040
#define S_IWGRP 0x00020
#define S_IXGRP 0x00010
#define S_IRWXO 0x00007
#define S_IROTH 0x00004
#define S_IWOTH 0x00002
#define S_IXOTH 0x00001

/* lseek flags (whence) */
#define FS_SEEK_SET 0
#define FS_SEEK_CUR 1
#define FS_SEEK_END 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* file system errors */
#define FS_ERROR_SUCCESS      0
#define FS_ERROR_IO          -1
#define FS_ERROR_FENTRY_MAX  -2
#define FS_ERROR_VNODE_MAX   -3
#define FS_ERROR_OFST_OVRFLW -4
#define FS_ERROR_WRT_THRSHLD -5
#define FS_ERROR_ILLEGAL_CID -6
#define FS_ERROR_NONZERO_REF_COUNT      -7
#define FS_ERROR_FREE_CACHE_ACQUIREMENT -8
#define FS_ERROR_CRITICAL    	 		-9
#define FS_ERROR_CACHE_NOT_FOUND 		-10
#define FS_ERROR_ALREADY_VOTED   		-11
#define FS_ERROR_NON_EXISTING_HOLE      -12
#define FS_ERROR_FREEING_DIRTY_CACHE    -13
#define FS_ERROR_READING_UNCACHEABLE    -14
#define FS_ERROR_DEC_ZERO_REF_COUNT     -15

#define IS_IO_ERROR(ERROR) 	   ((ERROR) == FS_ERROR_IO)
#define IS_OVRFLW_ERROR(ERROR) ((ERROR) == FS_ERROR_OFST_OVRFLW)
#define IS_FS_SUCCESS(ERR_CODE) ((ERR_CODE)==FS_ERROR_SUCCESS)

#define IS_FS_EMPTY()       (IS_ADDR_EMPTY(FS_GET_INO0_ADDR_PTR()))

#define FLAG_CACHEABLE_READ_YES  1
#define FLAG_CACHEABLE_READ_NO   0

#define FLAG_FROM_REBOOT_YES  1
#define FLAG_FROM_REBOOT_NO   0
/* cwd defintions  (currently empty)*/
#define DEFAULT_CWD_INO_NUM   INO_NUM_EMPTY

/**
 * @brief
 * boot file system -
 * - if file system doesn't exist create it
 * - load file system data from checkpoint, terminate open transactions
 * - finish deleting VOTs of closed transaction
 */
int32_t nandfs_fsBooting(void);

/**
 * @brief
 * unmount stub. actually does nothing except initializing RAM structs.
 *
 * @return 0;
 */
int32_t nandfs_unmount();

/* FILE API*/
/**
 * Given a pathname for a file, open() returns a file descriptor
 * a small, non-negative integer.
 *
 * @param fs the file system.
 * @param usr user details.
 * @param pathname pathname for a file.
 * @param flags file creation, access modes etc.
 * @param mode permissions to use in case a new file is created.
 * @return the file descriptor, or -1 if an error occurred
 */
int32_t nandfs_open(uint8_t *pathname, int32_t flags, uint32_t mode);

/**
 * Given a pathname for a file, open() returns a file descriptor.
 *
 * @param fs the file system.
 * @param usr user details.
 * @param pathname pathname for a file.
 * @param flags file creation, access modes etc.
 * @return the file descriptor, or -1 if an error occurred
 */
//int32_t open(file_system_t *fs, user_t *usr, uint8_t *pathname, int32_t flags);

/**
 * Given a pathname for a file, creates a new file and returns a file descriptor
 * (in Linux equivelant to open with O_CREAT|O_WRONLY|O_TRUNC).
 *
 * @param fs the file system.
 * @param usr user details.
 * @param pathname pathname for a file.
 * @param flags file creation, access modes etc.
 * @param mode permissions to use in case a new file is created.
 * @return the file descriptor, or -1 if an error occurred
 */
int32_t nandfs_creat(uint8_t *pathname, uint32_t mode);

/**
 * repositions the offset of the open file associated with the file descriptor fd
 * to the argument offset  according to the directive whence.
 *
 * @param fs the file system.
 * @param usr user details.
 * @param fd a file descriptor.
 * @param offset required offset.
 * @param whence directive for the seek.
 * @return the resulting offset location in bytes from the beginning of the file, -1 on error
 */

int32_t nandfs_lseek(int32_t fd, int32_t offset, int32_t whence);

/**
 * read up to count bytes from file descriptor fd into the buffer starting at buf.
 *
 * @param fs the file system.
 * @param usr user details.
 * @param fd a file descriptor.
 * @param buf buffer to contain read data.
 * @param count how many bytes to read.
 * @return number of bytes read
 */
int32_t nandfs_read(int32_t fd, void *buf, int32_t count);

/**
 * writes up to count bytes to the file referenced by the file descriptor fd
 * from the buffer starting at buf.
 *
 * @param fs the file system.
 * @param usr user details.
 * @param fd a file descriptor.
 * @param buf buffer containing data to write.
 * @param count how many bytes to read from buffer.
 * @return number of bytes written
 */
int32_t nandfs_write (int32_t fd, void *buf, int32_t count);

/**
 * first commits inodes to buffers, and then buffers to disk.
 */
void nandfs_sync(void);

/**
 * transfers ("flushes") all modified data of the file referred to
 * by the file descriptor fd to flash.
 *
 * @param fd the file descriptor.
 * @return On success, zero is returned. On error, -1 is returned
 */
int32_t nandfs_fsync(int32_t fd);

 /**
  * transfers ("flushes") all modified data of the file referred to
  * by the file descriptor fd to flash. identical to fsync
  *
  * @param fd the file descriptor.
  */
int32_t nandfs_fdatasync(int32_t fd);

/**
 * unlink a file and it's direntry. deletes the file and direntry from it's directory
 *
 * @param path path to a file
 * @return 0 ifsuccessful, -1 otherwise
 */
int32_t nandfs_unlink(uint8_t *path);

/**
 * closes a file descriptor, so that it no longer refers to any file
 * and may be reused.
 *
 * @param fd the file descriptor.
 * @return zero on success. On error, -1 is returned
 */
int32_t nandfs_close(int32_t fd);

/* DIRECTORY API*/
/**
 * mkdir() attempts to create a directory named pathname.
 *
 * @param pathname the path of the directory to be created
 * @param mode various creation modes
 * @return zero on success, or -1 if an error occurred
 */
int32_t nandfs_mkdir(uint8_t *pathname, uint32_t mode);

/**
 * @brief
 * rmdir() attempts to unlink a directory named pathname
 *
 * @param pathname the path of the directory to be created
 * @return zero on success, or -1 if an error occurred
 */
int32_t nandfs_rmdir(uint8_t *pathname);

/**
 * open a directory (by name) and return its' directory stream.
 *
 * @param name path of directory.
 * @return the directory stream
 */
NANDFS_DIR *nandfs_opendir(uint8_t *name);

/**
 * returns a pointer to a dirent structure representing the next directory entry
 * in the directory stream pointed to by dir.
 *
 * @param __dirp the directory stream.
 * @return a pointer to a dirent structure
 */
nandfs_dirent *nandfs_readdir (NANDFS_DIR *dir);

/**
 * set the position of the next readdir() call in the directory stream.
 *
 * @param dirs the directory stream.
 * @param offset offset for the position.
 */
void nandfs_seekdir(NANDFS_DIR *dirs, int32_t offset);

/**
 * reset directory stream.
 *
 * @param dirs the directory stream.
 */
void nandfs_rewinddir(NANDFS_DIR *dirs);

/**
 * function scans the directory dir, calling filter() on each directory entry,
 * Entries for which filter() returns non-zero are stored in strings allocated via allocator(),
 * sorted using insertionsort() with the comparison function compar(), and collected in array namelist.
 *
 * @param dir a directory path name.
 * @param namelist array of directory entries.
 * @param filter filter function.
 * @param compar the comparison function for the qsort.
 * @param reallocator function to reallocate namelist array
 * @return the number of directory entries or -1 if an error occurs.
 */
int32_t nandfs_scandir(uint8_t *dir, nandfs_dirent ***namelist,
			    int32_t(*filter)(nandfs_dirent*),int32_t(*compar)(nandfs_dirent **,  nandfs_dirent**),
			    void*(*reallocator)(void *ptr, int32_t size), void(*freer)(void *ptr));

/**
 * return current location in a directory stream.
 *
 * @param dir the directory stream.
 * @return current location in directory stream
 */
int32_t nandfs_telldir(NANDFS_DIR *dirs);

/**
 * returns the file descriptor associated with the directory stream dir
 * it is only useful for functions which do not depend on or alter the file position.
 *
 * @param dir the directory stream.
 * @return the file descriptor associated with the directory stream dir
 */
int32_t nandfs_dirfd(NANDFS_DIR *dir);

/**
 * closes the directory stream associated with dir.
 *
 * @param dir the directory stream.
 * @return 0 on success. On error, -1 is returned
 */
int32_t nandfs_closedir(NANDFS_DIR *dir);

/**
 * return information about a file
 *
 * @param path the file path.
 * @param buf buffer to store the file statistics.
 * @return On success, zero is returned. On error, -1 is returned
 */
int32_t nandfs_stat(uint8_t *path, file_stat_t *buf);

/* FILE DESCRIPTOR API */
/**
 * create a copy of the file descriptor oldfd
 * uses the lowest-numbered unused descriptor for the new descriptor.
 *
 * @param oldfd the old file descriptor.
 * @return the new descriptor
 */
int32_t nandfs_dup(int32_t old_fd);

/**
 * create a copy of the file descriptor oldfd
 * makes newfd be the copy of oldfd, closing newfd first if necessary.
 *
 * @param oldfd the old file descriptor.
 * @return the new descriptor
 */
int32_t nandfs_dup2(int32_t old_fd, int32_t new_fd);
#endif /*FS_H_*/
