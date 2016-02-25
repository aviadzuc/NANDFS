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

/** @file fs.c
 * File system high-level functions. Implementation of a POSIX-like file system*/

#include <system.h>
#include <utils/memlib.h>
#include <src/sequencing/sequencing.h>
#include <src/sequencing/sequencingUtils.h>
#include <src/fs/fs.h>
#include <src/fs/fsUtils.h>
#include <utils/string_lib.h>
#include <src/fs/insertionsort.h>

#ifdef Debug
	#include <test/fs/testsHeader.h>
#endif

extern transaction_t transactions[FS_MAX_N_TRANSACTIONS];
extern vnode_t vnodes[FS_MAX_VNODES];
extern open_file_t open_files[FS_MAX_OPEN_FILES];
extern fs_t fs_ptr;
extern uint8_t fs_buffer[FS_BLOCK_SIZE];
extern fs_dirstream dirstreams[FS_MAX_OPEN_DIRESTREAMS];

/**
 * Given a pathname for a file, open() returns a file descriptor
 * a small, non-negative integer.
 *
 * @param pathname pathname for a file.
 * @param flags file creation, access modes etc.
 * @param mode permissions to use in case a new file is created.
 * @return the file descriptor, or:
 * FD_EMPTY if an error occurred
 * FS_ERROR_FENTRY_MAX if no free file entries
 * FS_ERROR_VNODE_MAX if no free vnodes (for a file that is not alreday open)
 */
int32_t nandfs_open(uint8_t *pathname, int32_t flags, uint32_t mode){
	int32_t res, ino_num, de_offset;
	uint32_t f_type = 0;

	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	/* verify read or write*/
	FS_VERIFY(IS_WRONLY(flags) | IS_RDONLY(flags));

	/* get inode number for file*/
//	PRINT("\nopen() - call namei");
//	for(cid=0 ; cid< FS_CACHE_BUFFERS; cid++){
//		PRINT_MSG_AND_NUM("\nopen() - cid=", cid);
//		PRINT_MSG_AND_HEX(", actual ino_num=", INDIRECT_CACHE_GET_INO_NUM(cid));
//		PRINT_MSG_AND_NUM(", actual offset=", INDIRECT_CACHE_GET_OFFSET(cid));
//		PRINT_MSG_AND_HEX(", actual tid=", INDIRECT_CACHE_GET_TID(cid));
//	}
	ino_num = namei(pathname, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
//	PRINT("\nopen() - namei over");
//	for(cid=0 ; cid< FS_CACHE_BUFFERS; cid++){
//		PRINT_MSG_AND_NUM("\nopen() - cid=", cid);
//		PRINT_MSG_AND_HEX(", actual ino_num=", INDIRECT_CACHE_GET_INO_NUM(cid));
//		PRINT_MSG_AND_NUM(", actual offset=", INDIRECT_CACHE_GET_OFFSET(cid));
//		PRINT_MSG_AND_HEX(", actual tid=", INDIRECT_CACHE_GET_TID(cid));
//	}
//	PRINT_MSG_AND_NUM("\nopen() - found ino_num=", ino_num);
	/********** handle various flag options *********/
//	PRINT_MSG_AND_NUM("\nopen() - ino_num empty?=", IS_INO_EMPTY(ino_num));
//	PRINT_MSG_AND_NUM("\nopen() - creat flags?=", IS_CREAT(flags));
	/* if we have no inode and we don't create the file*/
	if(IS_NEGATIVE(ino_num) && !IS_CREAT(flags)){
		SYS_RETURN(FD_EMPTY);
	}
//	PRINT("\ncheck if no inode but we do create file");
	/* if no inode but we do create file, call creat()*/
	if(IS_NEGATIVE(ino_num)){
		/* unlock to allow other file system access*/
		FS_MUTEX_UNLOCK();
//		PRINT("\ncreate file");
		return nandfs_creat(pathname,0);
	}
//	PRINT("\nopen() - do setOpenFentry()");

	res = setOpenFentry(ino_num, flags, mode, f_type);

	SYS_RETURN(res);
}

/**
 * repositions the offset of the open file associated with the file descriptor fd
 * to the argument offset  according to the directive whence.
 * if offset exceeds file size, error is returned.
 *
 * offset is allowed to be set beyond file size. regular sparse files are not allowed,
 * subsequent reads/writes from offset larger than file size will result in an error.
 *
 * @param fd a file descriptor.
 * @param offset required offset (must be positive value).
 * @param whence directive for the seek.
 * @return the resulting offset location in bytes from the beginning of the file, -1 on error
 */

int32_t nandfs_lseek(int32_t fd, int32_t offset, int32_t whence){
	int32_t new_offset, file_size;

	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	/* verify fd is valid */
	if(verifyFd(fd)){
		SYS_RETURN(-1);
	}

//	PRINT("\nlseek() - fd verified");
	/* verify offset is non-negative*/
	if(offset < 0){
		SYS_RETURN(-1);
	}
//	PRINT("\nlseek() - offset verified");
	init_fsbuf(fs_buffer);

	/* calc file size */
	file_size = getFileSize(GET_FILE_ID_BY_FD(fd));
//	PRINT_MSG_AND_NUM("\nlseek() - file size=",file_size);

	/* verify file size non-negative*/
	if(IS_NEGATIVE(file_size)){
		SYS_RETURN(-1);
	}
//	PRINT("\nlseek() - verified file size");

	switch(whence){
		case FS_SEEK_CUR:
			new_offset = OPEN_FILE_GET_OFFSET(fd)+offset;
			break;
		case FS_SEEK_SET:
			new_offset = offset;
//			PRINT_MSG_AND_NUM("\nlseek() - new_offset=", new_offset);
			break;
		case FS_SEEK_END:
			new_offset = file_size+offset;
			break;
		default:
			SYS_RETURN(-1);
	}

	/* check offset doesn't exceed maximum offset*/
	SYS_VERIFY(new_offset <= FS_MAX_FILESIZE);

	/* check offset doesn't exceed file size*/
	SYS_VERIFY(new_offset <= file_size);

	/* set offset*/
	OPEN_FILE_SET_OFFSET(fd, new_offset);

	SYS_RETURN(new_offset);
}



/**
 * Given a pathname for a file, creates a new file and returns a file descriptor
 * (in Linux equivelant to open with O_CREAT|O_WRONLY).
 *
 * @param pathname pathname for a file.
 * @param mode permissions to use in case a new file is created (0 for now)
 * @return the file descriptor, or -1 if an error occurred
 */
int32_t nandfs_creat(uint8_t *pathname, uint32_t mode){
	int32_t res;

	/* lock all other file system access*/
	FS_MUTEX_LOCK();
//	PRINT("\ncreat() - starting");
//	{
//		int32_t i;
//		for(i=0; i< FS_MAX_OPEN_FILES-1;i++){
//			PRINT_MSG_AND_NUM("\n", i);
//			PRINT(". ");
//			PRINT_MSG_AND_HEX(" fentry vnode=", OPEN_FILE_GET_VNODE(i));
//		}
//		PRINT_MSG_AND_HEX("\n15. fentry vnode=", OPEN_FILE_GET_VNODE(15));
//	}
	res = createNewFile(FTYPE_FILE, pathname);

	SYS_RETURN(res);
}

/**
 * writes up to count bytes to the file referenced by the file descriptor fd
 * from the buffer starting at buf.
 *
 * @param fd a file descriptor.
 * @param buf buffer containing data to write.
 * @param count how many bytes to read from buffer.
 * @return number of bytes written, On error, -1 is returned
 */
int32_t nandfs_write (int32_t fd, void *buf, int32_t count){
	int32_t tid, res = 0, fileOffset, ino_num , writeSize, f_size;
	int32_t tempCount = count;
	int32_t bytesWritten = 0, commitedBytesWritten = 0;
	uint8_t *data_buf;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
//	FENT();

	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	init_logical_address(log_addr);

//	PRINT("\nwrite() - starting");
//	PRINT_MSG_AND_NUM(", fd=",fd);
//	PRINT_MSG_AND_NUM(", count=",count);
//	PRINT_MSG_AND_NUM(", tempCount=",tempCount);

	/* verify file descriptor*/
	SYS_VERIFY(!verifyFd(fd));
//	PRINT(", fd verified");

	/* verify count is legal */
	SYS_VERIFY(count >= 0);
//	PRINT(", count verified");

	/* verify file open for writing*/
	SYS_VERIFY(IS_WRONLY(OPEN_FILE_GET_FLAGS(fd)));
//	PRINT(", flags verified");

	/* if count is 0, return*/
	if(count == 0){
		SYS_RETURN(0);
	}

	ino_num = GET_FILE_ID_BY_FD(fd);
//	L("get File Size of ino num %d", ino_num);
	f_size  = getFileSize(ino_num);
//	L("got f_size %d", f_size);
//	if(OPEN_FILE_GET_OFFSET(fd) > f_size){
//		PRINT_MSG_AND_NUM("\nfentry offset=", OPEN_FILE_GET_OFFSET(fd));
//		PRINT_MSG_AND_NUM(", f_size=", f_size);
//	}

	/* if we're trying to write beyond file size, abort*/
//	L("OPEN_FILE_GET_OFFSET(fd) %d, f_size %d", OPEN_FILE_GET_OFFSET(fd), f_size);
	SYS_VERIFY(OPEN_FILE_GET_OFFSET(fd) <= f_size);
//	PRINT(", file offset<f_size verified");
	/* check for existing transaction handling the write*/
	tid     = getTidByFileId(ino_num);
	if(IS_EMPTY_TID(tid)){
		SYS_RETURN(0);
	}

//	PRINT(", tid verified");
//	PRINT_MSG_AND_HEX("\nwrite() - ino addr=", *((uint32_t*)(TRANSACTION_GET_INO_ADDR_PTR(tid))));
	TRANSACTION_SET_TYPE(tid,T_TYPE_WRITE);
	TRANSACTION_SET_FILE_SIZE(tid, f_size);
	TRANSACTION_SET_FTYPE(tid, FTYPE_FILE);

	data_buf = TRANSACTION_GET_DATA_BUF_PTR(tid);
//	PRINT("\nwrite() - check if ino a/ddr not empty");
//	assert(!IS_ADDR_EMPTY(TRANSACTION_GET_INO_ADDR_PTR(tid)));

	fileOffset = OPEN_FILE_GET_OFFSET(fd);
//	PRINT_MSG_AND_NUM("\nwrite() - fileOffset=",fileOffset);

	/* handle first blcok, so we can move on to block-size writes.
	 * if we are in inode data offset, make sure to change only until INODE_FILE_DATA_SIZE. */
//	L("b4 read file block, is transaction ino addr empty? %d",IS_ADDR_EMPTY(TRANSACTION_GET_INO_ADDR_PTR(tid)));
	SYS_TR_VERIFY(res, readFileBlock(data_buf, ino_num, fileOffset, log_addr, tid));
	init_logical_address(log_addr);
//	PRINT_MSG_AND_NUM("\nwrite() - after read file block, is transaction ino addr empty?=",IS_ADDR_EMPTY(TRANSACTION_GET_INO_ADDR_PTR(tid)));
//	L("readFileBlock() success");

	if(fileOffset < INODE_FILE_DATA_SIZE){
//		PRINT("\nwrite() - fileOffset < INODE_FILE_DATA_SIZE");
		/* copy data to inode.
		 * if we have more to write than inode file data size: */
		if(tempCount > INODE_FILE_DATA_SIZE-fileOffset){
			writeSize = INODE_FILE_DATA_SIZE-fileOffset;
		}else{
			writeSize = tempCount;
		}
//		PRINT_MSG_AND_NUM("\nwrite() - writeSize=",writeSize);
		fsMemcpy(data_buf+fileOffset, CAST_TO_UINT8(buf), writeSize);
//		PRINT_MSG_AND_HEX("\nwrite() - write page. first byte=", data_buf[0]);
		/* write block. this will also read indirect block to transaction (if it is empty)*/
//		L("call first writeFileBlock() in offset 0");
		SYS_TR_VERIFY(res, writeFileBlock(0, fileOffset+writeSize, DATA_TYPE_REGULAR, tid, log_addr, 1));
	}
	else{
		if(tempCount> FS_BLOCK_SIZE - CALC_OFFSET_IN_FILE_BLOCK(fileOffset)){
			writeSize = FS_BLOCK_SIZE - CALC_OFFSET_IN_FILE_BLOCK(fileOffset);
		}else{
			writeSize = tempCount;
		}
//		L("writeSize %d", writeSize);
		fsMemcpy(data_buf+CALC_OFFSET_IN_FILE_BLOCK(fileOffset), CAST_TO_UINT8(buf), writeSize);

		/* write block. this will also read indirect block to transaction (if it is empty)*/
//		L("first writeFileBlock() in offset %d", fileOffset-CALC_OFFSET_IN_FILE_BLOCK(fileOffset));
		SYS_TR_VERIFY(res, writeFileBlock(fileOffset-CALC_OFFSET_IN_FILE_BLOCK(fileOffset), CALC_OFFSET_IN_FILE_BLOCK(fileOffset)+writeSize, DATA_TYPE_REGULAR, tid, log_addr, 1));
//		L("first writeFileBlock() done");
	}

//	PRINT_MSG_AND_NUM("\nwrite() - after 1st write, writeSize=",writeSize);
	OPEN_FILE_INC_OFFSET(fd, writeSize);
	bytesWritten += writeSize;
	tempCount    -= writeSize;

	/* the inode was commited before the write was completed and marked
	 * in indirect block. therefore commitedBytesWritten is not changed */
	if(TRANSACTION_GET_WAS_COMMITED(tid)){
		TRANSACTION_SET_WAS_COMMITED(tid, 0);
	}

	init_logical_address(log_addr);
//	PRINT_MSG_AND_NUM(", tempCount=",tempCount);
//	PRINT("\nwrite() - start loop");
//	PRINT_MSG_AND_NUM(", commitedBytesWritten=",commitedBytesWritten);
	/* write blocks sequentially, until we have less than FS_BLOCK_SIZE left to write*/
	while(tempCount > FS_BLOCK_SIZE){
		fileOffset = OPEN_FILE_GET_OFFSET(fd);
//		L("in loop, write to file offset %d", fileOffset);
		fsMemcpy(data_buf, &(CAST_TO_UINT8(buf)[bytesWritten]), FS_BLOCK_SIZE);
//		PRINT("\nwrite() - memcpy success");
		WRITE_VERIFY(commitedBytesWritten, writeFileBlock(fileOffset, FS_BLOCK_SIZE, DATA_TYPE_REGULAR, tid, log_addr, 1));
//		L("\nwrite() - write success");

		OPEN_FILE_INC_OFFSET(fd, FS_BLOCK_SIZE);
		bytesWritten += FS_BLOCK_SIZE;
		tempCount    -= FS_BLOCK_SIZE;
		if(TRANSACTION_GET_WAS_COMMITED(tid)){
			commitedBytesWritten = bytesWritten-FS_BLOCK_SIZE;
//			PRINT_MSG_AND_NUM("\ntransaction commited flag=",TRANSACTION_GET_WAS_COMMITED(tid));
//			PRINT_MSG_AND_NUM(" commitedBytesWritten=",commitedBytesWritten);
			TRANSACTION_SET_WAS_COMMITED(tid, 0);
		}
		init_logical_address(log_addr);
	}
//	PRINT("\nwrite() - finished loop");

	/* if no more bytes to write, return s*/
	if(tempCount == 0){
		SYS_RETURN(bytesWritten);
	}

	fileOffset = OPEN_FILE_GET_OFFSET(fd);
	/* we have last tempCount bytes (tempCount < FS_BLOCK_SIZE) to write.
	 * - read last block to be written
	 * - copy last bytes to data buffer
	 * - write block
	 * - change various counters */
//	L("call last readFileBlock() in offset %d", fileOffset);
	WRITE_VERIFY(commitedBytesWritten, readFileBlock(data_buf, ino_num, fileOffset, TRANSACTION_GET_INO_ADDR_PTR(tid), tid));
	init_logical_address(log_addr);
	fsMemcpy(data_buf, &(CAST_TO_UINT8(buf)[bytesWritten]), tempCount);
//	L("last write to file offset ", fileOffset);
	WRITE_VERIFY(commitedBytesWritten, writeFileBlock(fileOffset, tempCount, DATA_TYPE_REGULAR, tid, log_addr, 1));

	bytesWritten += tempCount;
	OPEN_FILE_INC_OFFSET(fd, tempCount);

//	PRINT("\nwrite() - finished.");
//	PRINT_MSG_AND_NUM(" return bytesWritten=",bytesWritten);
	SYS_RETURN(bytesWritten);
}

/**
 * closes a file descriptor, so that it no longer refers to any file
 * and may be reused.
 *
 * @param fd the file descriptor.
 * @return zero on success. On error, -1 is returned
 */
int32_t nandfs_close(int32_t fd){
	int32_t res;
	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	res = closeUtil(fd);

	/* lock all other file system access*/
	FS_MUTEX_UNLOCK();

	return res;
}

/**
 * read up to count bytes from file descriptor fd into the buffer starting at buf.
 *
 * @param fs the file system.
 * @param usr user details.
 * @param fd a file descriptor.
 * @param buf buffer to contain read data.
 * @param count how many bytes to read.
 * @return number of bytes read. On error, -1 is returned
 */
int32_t nandfs_read(int32_t fd, void *buf, int32_t count){
	int32_t fileOffset, ino_num , readSize, f_size, block_offset, tid = TID_EMPTY;
	int32_t tempCount = count;
	int32_t bytesRead = 0;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
//	FENT();

	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	init_logical_address(ino_addr);

//	PRINT("\nread() - starting. ");
//	PRINT_MSG_AND_NUM(" count=", count);
	/* verify file descriptor*/
	SYS_VERIFY(!verifyFd(fd));
//	PRINT("fd legal ");

	/* if count isn't legal */
	SYS_VERIFY(count >= 0);
//	PRINT("\nfd legal ");

	/* if file is not open for reading*/
	SYS_VERIFY(IS_RDONLY(OPEN_FILE_GET_FLAGS(fd)));
//	PRINT("flags are of read ");

	/* can't read directory */
	SYS_VERIFY(!IS_DIRECTORY(OPEN_FILE_GET_FTYPE(fd)));

//	PRINT("\nread() - flags legal");
	/* read first block (read size <= FS_BLOCK_SIZE)*/
	ino_num = GET_FILE_ID_BY_FD(fd);
	f_size  = getFileSize(ino_num);
	fileOffset = OPEN_FILE_GET_OFFSET(fd);
//	PRINT_MSG_AND_NUM("\nread() - f_size=",f_size);

	/* if we're trying to read beyond file size, abort*/
	SYS_VERIFY(fileOffset <= f_size);

	/* if the file is also open for writing, get it's transaction data*/
	tid = findTidByFileId(ino_num);

//	PRINT_MSG_AND_NUM("\nread() - tid=",tid);
//	PRINT_MSG_AND_NUM("\nread() - fileOffset=",fileOffset);
//	PRINT_MSG_AND_NUM(", tempCount=",tempCount);
//	PRINT_MSG_AND_NUM(", fileOffset=",fileOffset);
	/* handle first block, so we can continue to block-size reads.
	 * if we are in inode data offset, make sure to change fd offset only as far as INODE_FILE_DATA_SIZE. */
	if(fileOffset < INODE_FILE_DATA_SIZE){
		/* copy data to inode.
		 * if we have more to write than inode file data size: */
		if(tempCount > INODE_FILE_DATA_SIZE-fileOffset){
			readSize = INODE_FILE_DATA_SIZE-fileOffset;
		}else{
			readSize = tempCount;
		}

		block_offset = fileOffset;
	}
	else{
//		PRINT("\nfileOffset < INODE_FILE_DATA_SIZE. ");
//		PRINT_MSG_AND_NUM(", tempCount=",tempCount);
//		PRINT_MSG_AND_NUM(", CALC_OFFSET_IN_FILE_BLOCK(fileOffset)=",CALC_OFFSET_IN_FILE_BLOCK(fileOffset));
//		PRINT_MSG_AND_NUM(", tempCount> FS_BLOCK_SIZE - CALC_OFFSET_IN_FILE_BLOCK(fileOffset)=",tempCount> FS_BLOCK_SIZE - CALC_OFFSET_IN_FILE_BLOCK(fileOffset));
		if(tempCount> FS_BLOCK_SIZE - CALC_OFFSET_IN_FILE_BLOCK(fileOffset)){
			readSize = FS_BLOCK_SIZE - CALC_OFFSET_IN_FILE_BLOCK(fileOffset);
		}else{
			readSize = tempCount;
		}

		block_offset = CALC_OFFSET_IN_FILE_BLOCK(fileOffset);
	}

//	PRINT_MSG_AND_NUM("\nread() - b4 f_size fix readSize=",readSize);
	/* complete first read */
	if(fileOffset+readSize > f_size){
		readSize = f_size-fileOffset;
	}
//	PRINT_MSG_AND_NUM("\nread() - first readSize=",readSize);
//	PRINT_MSG_AND_NUM(", fileOffset=",fileOffset);
//	L("@@@@@@@@@@@@@@@@@@@@ - FIRST READ");
	SYS_VERIFY(!readFileBlock(fs_buffer, ino_num, fileOffset, ino_addr, tid));
	fsMemcpy(CAST_TO_UINT8(buf), fs_buffer+block_offset, readSize);
//	PRINT("\nread() - first read successful");

	OPEN_FILE_INC_OFFSET(fd, readSize);
	bytesRead += readSize;
	tempCount -= readSize;
	fileOffset = OPEN_FILE_GET_OFFSET(fd);
//	PRINT_MSG_AND_NUM(", tempCount=",tempCount);
//	PRINT("\nread() - start loop");
//	PRINT_MSG_AND_NUM(" tempCount=",tempCount);
//	PRINT_MSG_AND_NUM(" bytesRead=",bytesRead);
//	PRINT_MSG_AND_NUM(" fileOffset=",fileOffset);
//	PRINT_MSG_AND_NUM(" f_size=",f_size);

	/* check if we have any block size reads to perform*/
	if(tempCount>FS_BLOCK_SIZE){
		readSize = FS_BLOCK_SIZE;
	}
	/* if we have less than a block to read, fix readSize eaccordingly*/
	else{
		readSize = tempCount;
	}

	/* verify the next read doesn't overflow file size */
	if(fileOffset+readSize > f_size){
		readSize = f_size-fileOffset;
	}

//	PRINT_MSG_AND_NUM(" readSize=",readSize);
	/* read middle blocks (read size = FS_BLOCK_SIZE).
	 * read blocks sequentially, until we have less than FS_BLOCK_SIZE left to read
	 * (NOTICE - also possible if we've reached EOF)*/
//	L("@@@@@@@@@@@@@@@@@@@@ - LOOP READS");
	while(tempCount > FS_BLOCK_SIZE && readSize == FS_BLOCK_SIZE){
//		PRINT_MSG_AND_NUM("\nread() - read from file offset ", fileOffset);
		SYS_VERIFY(!readFileBlock(fs_buffer, ino_num, fileOffset, ino_addr, tid));
		fsMemcpy(&(CAST_TO_UINT8(buf)[bytesRead]), fs_buffer, readSize);

		OPEN_FILE_INC_OFFSET(fd, FS_BLOCK_SIZE);
		bytesRead += readSize;
		tempCount -= readSize;
		fileOffset = OPEN_FILE_GET_OFFSET(fd);

		if(fileOffset+readSize > f_size){
			readSize = f_size-fileOffset;
		}
//		PRINT_MSG_AND_NUM("\nread() - next readSize=", readSize);
	}

	fileOffset = OPEN_FILE_GET_OFFSET(fd);
//	PRINT("\nread() - finished loop");
//	PRINT_MSG_AND_NUM(" readSize=",readSize);
//	PRINT_MSG_AND_NUM(" tempCount=",tempCount);
//	PRINT_MSG_AND_NUM(" bytesRead=",bytesRead);
//	PRINT_MSG_AND_NUM(" fileOffset=",fileOffset);
	/* if no more bytes to read, or reached EOF return*/
	if(readSize == 0 || tempCount == 0){
//		L("@@@@@@@@@@@@@@@@@@@@ - DONE");
		SYS_RETURN(bytesRead);
	}

	/* if we need to read last than next read mamimum size, fix read size */
	if(readSize > tempCount){
		readSize = tempCount;
	}

//	PRINT("\nread() - do last read.");
//	PRINT_MSG_AND_NUM(" fileOffset=",fileOffset);
//	PRINT_MSG_AND_NUM(" readSize=",readSize);
	/* read last block (read size <= FS_BLOCK_SIZE).
	 *  we have last tempCount bytes (tempCount < FS_BLOCK_SIZE) to read. */
//	L("@@@@@@@@@@@@@@@@@@@@ - LAST READ");
	SYS_VERIFY(!readFileBlock(fs_buffer, ino_num, fileOffset, ino_addr, tid));

//	PRINT_MSG_AND_NUM("\n readSize=",readSize);
	fsMemcpy(&(CAST_TO_UINT8(buf)[bytesRead]), fs_buffer, readSize);

	bytesRead += readSize;
	OPEN_FILE_INC_OFFSET(fd, readSize);
//	L("@@@@@@@@@@@@@@@@@@@@ - DONE");
	SYS_RETURN(bytesRead);
}


/**
 * unlink a file and it's direntry. deletes the file and direntry from it's directory
 *
 * @param path path to a file
 * @return 0 if successful, -1 otherwise
 */
int32_t nandfs_unlink(uint8_t *pathname){
	int32_t res;
//	PRINT("\nnandfs_unlink() - starting");
	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	res = removeFile(FTYPE_FILE, pathname);
//	PRINT_MSG_AND_NUM("\nnandfs_unlink() - res=", res);
	SYS_RETURN(res);
}

/**
 * @brief
 * boot file system -
 * - if file system doesn't exist create it
 * - load file system data from checkpoint, terminate open transactions
 * - finish deleting VOTs of closed transaction
 */
int32_t nandfs_fsBooting(void){
	int32_t res, i;
	bool_t pendingVOTs = 0;

//	PRINT("\nfsBooting() - starting");
//	PRINT_MSG_AND_NUM(". mutex=", *FS_GET_MUTEX_PTR());
	initializeRamStructs();
	/* lock mutex*/
	FS_MUTEX_LOCK();

	/* boot sequencing layer -
	 * locate most recent checkpoint, erase following data etc. */
//	PRINT("\nfsBooting() - about to perform initial sequencing booting");
//	PRINT_MSG_AND_NUM(". mutex=", *FS_GET_MUTEX_PTR());
	SYS_RES_VERIFY(res, sequencingBooting(fs_ptr, sizeof(filesystem_t)-sizeof(nandfs_mutex_t), &pendingVOTs, fsCheckpointWriter));
//	L("initial sequencing booting success. rec offset %d", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(". mutex=", *FS_GET_MUTEX_PTR());
//	PRINT_MSG_AND_NUM(", obs count=", GET_OBS_COUNT());
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nfsBooting() - after initial sequencing booting -  flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}

	/* if file system is empty:
	 * - create inode0, root directory
	 * - write checkpoint */
	if(IS_FS_EMPTY()){
		res = handleNoFs();
//		L("handleNoFs() done. rec offset %d", GET_RECLAIMED_OFFSET());
//			PRINT_MSG_AND_NUM(". mutex=", *FS_GET_MUTEX_PTR());
		SYS_RETURN(res);
	}

//	PRINT("\nfs located success");
//	PRINT_MSG_AND_NUM(". mutex=", *FS_GET_MUTEX_PTR());
//	PRINT_MSG_AND_HEX("\nhandle last closed tid, from addr", *CAST_TO_UINT32(FS_GET_LAST_CLOSED_TID_ADDR()));
	/* finish handling vots of last commited transaction (if there is one)*/
	if(!IS_ADDR_EMPTY(FS_GET_LAST_CLOSED_TID_ADDR())){
		SYS_RES_VERIFY(res, handleTransactionVOTs(FS_GET_LAST_CLOSED_TID_ADDR(), FLAG_FROM_REBOOT_YES));
	}
//	PRINT_MSG_AND_NUM("\n after handling closed tid, obs count=", GET_OBS_COUNT());
//	MARK_OBS_COUNT_FINAL();
//	PRINT_MSG_AND_NUM("\nfsBooting() - b4 loop mutex=", *FS_GET_MUTEX_PTR());
	/* abort all open transactions */
	for(i=0; i< FS_MAX_N_TRANSACTIONS; i++){
		/* don't obsolete pages of last commited transaction. just initialize it's last
		 * written addresses markers*/
//		PRINT_MSG_AND_HEX("\nlast closed tid addr", *CAST_TO_UINT32(FS_GET_LAST_CLOSED_TID_ADDR()));
//		PRINT_MSG_AND_HEX("\ntid last addr", *CAST_TO_UINT32(FS_GET_TID_LAST_WRITTEN_ADDR(i)));
		if(COMPARE_ADDR(FS_GET_TID_LAST_WRITTEN_ADDR(i), FS_GET_LAST_CLOSED_TID_ADDR())){
			init_logical_address(FS_GET_TID_LAST_WRITTEN_ADDR(i));
			continue;
		}

		SYS_VERIFY(!deleteTransactionPages(FS_GET_TID_LAST_WRITTEN_ADDR(i), i, FLAG_FROM_REBOOT_YES));
//		PRINT_MSG_AND_NUM("\nfinished handling delete pages of transaction ", i);
		init_logical_address(FS_GET_TID_LAST_WRITTEN_ADDR(i));
	}
//	PRINT_MSG_AND_NUM("\n after handling other tids, obs count=", GET_OBS_COUNT());
	init_logical_address(FS_GET_LAST_CLOSED_TID_ADDR());

//	PRINT_MSG_AND_NUM("\nfsBooting() - b4 returning successfuly, mutex=", *FS_GET_MUTEX_PTR());
	SYS_RETURN(0);
}

/**
 * @brief
 * unmount stub. actually does nothing except initializing RAM structs.
 *
 * @return 0;
 */
int32_t nandfs_unmount(){
	initializeRamStructs();

	return 0;
}

/**
 * mkdir() attempts to create a directory named pathname.
 *
 * @param pathname the path of the directory to be created
 * @param mode various creation modes
 * @return 0 on success, or -1 if an error occurred
 */
int32_t nandfs_mkdir(uint8_t *pathname, uint32_t mode){
	int32_t res;

	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	res = createNewFile(FTYPE_DIR, pathname);

	SYS_RETURN(res);
}

/**
 * @brief
 * rmdir() attempts to unlink a directory named pathname
 *
 * @param pathname the path of the directory to be created
 * @return zero on success, or -1 if an error occurred
 */
int32_t nandfs_rmdir(uint8_t *pathname){
	int32_t res;

	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	res = removeFile(FTYPE_DIR, pathname);

	SYS_RETURN(res);
}

/**
 * transfers ("flushes") all modified data of the file referred to
 * by the file descriptor fd to flash.
 *
 * @param fd the file descriptor.
 * @return On success, zero is returned. If the file descriptor is illegal -1 is returned.
 * If an error occurs when commiting the file to flash -1 is returned
 */
int32_t nandfs_fsync(int32_t fd){
	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	SYS_VERIFY(!fsyncUtil(fd));

	/* unlock to allow other file system access*/
	FS_MUTEX_UNLOCK();

	return 0;
}

 /**
  * transfers ("flushes") all modified data of the file referred to
  * by the file descriptor fd to flash. identical to fsync
  *
  * @param fd the file descriptor.
  */
int32_t nandfs_fdatasync(int32_t fd){
	return nandfs_fsync(fd);
}

/**
 * @brief
 * first commits inodes to buffers, and then buffers to disk.
 *
 */
void nandfs_sync(void){
	int32_t i;

	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	for(i=0; i< FS_MAX_OPEN_FILES; i++){
		if(IS_OPEN_FILE_ENTRY_EMPTY(i)){
			continue;
		}

		if(fsyncUtil(i)){
			break;
		}
	}

	/* unlock to allow other file system access*/
	FS_MUTEX_UNLOCK();
}

/**
 * open a directory (by name) and return its' directory stream.
 *
 * @param name path of directory.
 * @return the directory stream
 */
NANDFS_DIR* nandfs_opendir(uint8_t *name){
	int32_t de_offset, ds_idx, fd, ino_num;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	NANDFS_DIR *ds;
	uint32_t f_type;

	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	init_logical_address(ino_addr);
//	PRINT_MSG_AND_STR("\nopendir() - starting. name=",name);

	/* get inode number for file*/
	ino_num = namei(name, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
//	PRINT_MSG_AND_NUM("\nino_num=", ino_num);

	/* if no such file, or it is not a directory */
	if(IS_NEGATIVE(ino_num) || !IS_DIRECTORY(f_type)){
		SYS_RETURN(NULL);
	}

	/* get empty dirstream */
	ds_idx = getFreeDirstreamId();
	if(IS_EMPTY_DS_ID(ds_idx)){
		SYS_RETURN(NULL);
	}

	ds = GET_DS_BY_ID(ds_idx);
//	PRINT_MSG_AND_NUM("\n ds_idx=", ds_idx);
	/* set directory stream to first entry:
	 * - file descriptor
	 * - directory offset
	 * - directory inode address
	 * - first entry */
	if(getInode(fs_buffer, ino_num, ino_addr)){
		init_dirstream(ds);

		SYS_RETURN(NULL);
	}
	DS_SET_INO_ADDR_BY_PTR(ds, ino_addr);

	/* set file descriptor */
//	PRINT("\nopendir() - execute setOpenFentry()");
	fd = setOpenFentry(ino_num, NANDFS_O_RDONLY, 0, FTYPE_DIR);
//	PRINT_MSG_AND_NUM("\nopendir() - fd=", fd);
	if(IS_FD_EMPTY(fd)){
		init_dirstream(ds);

		SYS_RETURN(NULL);
	}
	DS_SET_FD_BY_PTR(ds, fd);
	OPEN_FILE_SET_OFFSET(DS_GET_FD_BY_PTR(ds), DIRECTORY_FIRST_ENTRY_OFFSET);

	SYS_RETURN(ds);
}

/**
 * returns a pointer to a dirent structure representing the next directory entry
 * in the directory stream pointed to by dir.
 *
 * @param ds the directory stream.
 * @return a pointer to a dirent structure
 */
nandfs_dirent *nandfs_readdir (NANDFS_DIR *ds){
	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	/* verify dirstream is legal*/
	if(!verifyDirstream(ds)){
		SYS_RETURN(NULL);
	}

	/* read next entry*/
	if(readDirStreamNextEntry(ds)){
		SYS_RETURN(NULL);
	}

	SYS_RETURN(DS_GET_DIRENTRY_PTR_BY_PTR(ds));
}

/**
 * return current location in a directory stream.
 *
 * @param dir the directory stream.
 * @return current location in directory stream. -1 on error
 */
int32_t nandfs_telldir(NANDFS_DIR *dirs){
	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	/* verify dirstream is legal*/
	if(!verifyDirstream(dirs)){
		SYS_RETURN(-1);
	}

	SYS_RETURN(DS_GET_DIR_OFFSET_BY_PTR(dirs));
}

/**
 * returns the file descriptor associated with the directory stream dir
 * it is only useful for functions which do not depend on or alter the file position.
 *
 * @param dirs the directory stream.
 * @return the file descriptor associated with the directory stream dir. -1 on error
 */
int32_t nandfs_dirfd(NANDFS_DIR *dirs){
	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	/* verify dirstream is legal*/
	if(!verifyDirstream(dirs)){
//		PRINT("\ndirfd() - ds not verified");
		SYS_RETURN(-1);
	}

	SYS_RETURN(DS_GET_FD_BY_PTR(dirs));
}

/**
 * set the position of the next readdir() call in the directory stream.
 *
 * @param dirs the directory stream.
 * @param offset offset for the position.
 */
void nandfs_seekdir(NANDFS_DIR *dirs, int32_t offset){
	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	if(verifyDirstream(dirs)){
		DS_SET_DIR_OFFSET_BY_PTR(dirs, offset);
	}

	/* unlock mutex*/
	FS_MUTEX_UNLOCK();
}

/**
 * reset directory stream.
 *
 * @param dirs the directory stream.
 */
void nandfs_rewinddir(NANDFS_DIR *dirs){
	nandfs_seekdir(dirs, DIRECTORY_FIRST_ENTRY_OFFSET);
}

/**
 * closes the directory stream associated with dir.
 *
 * @param dir the directory stream.
 * @return 0 on success. On error, -1 is returned
 */
int32_t nandfs_closedir(NANDFS_DIR *dir){
	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	/* verify directory stream*/
	if(!verifyDirstream(dir)){
		SYS_RETURN(-1);
	}

//	init_related_caches(GET_FILE_ID_BY_FD(DS_GET_FD_BY_PTR(DIR)));
	init_dirstream(dir);

	SYS_RETURN(0);
}

/**
 * return information about a file
 *
 * @param path the file path.
 * @param buf buffer to store the file statistics.
 * @return On success, zero is returned. On error, -1 is returned
 */
int32_t nandfs_stat(uint8_t *path, file_stat_t *buf){
	int32_t ino_num, de_offset, size;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	uint32_t f_type;

	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	init_logical_address(ino_addr);

	/* get file id*/
	ino_num = namei(path, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
	SYS_VERIFY(!IS_NEGATIVE(ino_num));

	STAT_SET_BLK_SIZE(buf, NAND_PAGE_SIZE);
	STAT_SET_INO_NUM(buf, ino_num);
	STAT_SET_DEV_ID(buf, nandReadID());

	/* set sizes */
	size = getFileBlocks(ino_num);
	SYS_VERIFY(!IS_NEGATIVE(size));
	STAT_SET_BLOCKS(buf, size);

	size = getFileSize(ino_num);
	SYS_VERIFY(!IS_NEGATIVE(size));
	STAT_SET_SIZE(buf, size);

	SYS_RETURN(0);
}

/**
 * create a copy of the file descriptor oldfd
 * makes newfd be the copy of oldfd, closing newfd first if necessary.
 *
 * @param oldfd the old file descriptor.
 * @return the new descriptor, or -1 on failure
 */
int32_t nandfs_dup2(int32_t old_fd, int32_t new_fd){
	int32_t res;
	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	res = dupUtil(old_fd, new_fd, 1);

	/* lock all other file system access*/
	FS_MUTEX_UNLOCK();

	return res;
}

/**
 * create a copy of the file descriptor oldfd
 * uses the lowest-numbered unused descriptor for the new descriptor.
 *
 * @param oldfd the old file descriptor.
 * @return the new descriptor, or -1 on error
 */
int32_t nandfs_dup(int32_t old_fd){
	int32_t res;
	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	res = dupUtil(old_fd, FD_EMPTY, 0);

	/* lock all other file system access*/
	FS_MUTEX_UNLOCK();

	return res;
}

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
			    void*(*reallocator)(void *ptr, int32_t size), void(*freer)(void *ptr)){
	int32_t ino_num, de_offset, f_size, offset, res;
	int32_t de_idx = 0;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ino_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(block_addr);
	nandfs_dirent *de_ptr;
	nandfs_dirent **arr = NULL, **new_arr = NULL;
	uint32_t f_type;

	/* lock all other file system access*/
	FS_MUTEX_LOCK();

	init_logical_address(ino_addr);
	init_logical_address(block_addr);

	/* verify directory */
	ino_num = namei(dir, &f_type, &de_offset, DEFAULT_CWD_INO_NUM);
	SYS_VERIFY(!IS_NEGATIVE(ino_num));
	SYS_VERIFY(IS_DIRECTORY(f_type));

	f_size = getFileSize(ino_num);
//	PRINT_MSG_AND_NUM("\nscandir() - ino_num=", ino_num);
//	PRINT_MSG_AND_NUM(", f_size=", f_size);

	/* iterate all directory blocks*/
	for(offset=DIRECTORY_FIRST_ENTRY_OFFSET; offset< f_size;){
		/* read file block*/
		SYS_VERIFY(!readFileBlock(fs_buffer, ino_num, offset, ino_addr, TID_EMPTY));

		/* read entries from block */
		de_ptr = (nandfs_dirent*)(fs_buffer);

		while(!IS_DIRENT_EMPTY(de_ptr)){
//			PRINT_MSG_AND_STR("\nentry name=", DIRENT_GET_NAME(CAST_TO_DIRENT(de_ptr)));
			/* check if this entry passes filter*/
			if(filter(de_ptr)){
				/* allocate direntry pointer in namelist*/
				de_idx++;
//				PRINT_MSG_AND_NUM("\nentry not filtered. de_idx=", de_idx);

				new_arr = reallocator(arr, sizeof(nandfs_dirent*)*de_idx);
				if(IS_NULL(new_arr)){
//					PRINT("\nfirst realloc failue. free");
					freeNamesArrary(arr, de_idx-1, freer);
//					PRINT("\nfreeNamesArrary success");
					SYS_RETURN(-1);
				}
				arr = new_arr;
//				PRINT("\nfirst realloc success");

				arr[de_idx-1] = NULL;
				arr[de_idx-1] = reallocator(arr[de_idx-1], sizeof(nandfs_dirent));
//				PRINT("\nabout to do 2nd realloc")
				if(IS_NULL(arr)){
					freeNamesArrary(arr, de_idx-1, freer);
					SYS_RETURN(-1);
				}
//				PRINT("\n2nd realloc success");

				/* copy direntry to nameList*/
				fsMemcpy(arr[de_idx-1], de_ptr, DIRENT_GET_LEN(de_ptr));
			}

			moveToNextDirentry((dirent_flash**)(&de_ptr), DIRENT_GET_LEN(de_ptr));
		}
//		PRINT("\nfind next entries block");
		/* directory entry is empty, keep reading directory until we find next direntry*/
		res =findNextNonEmptySparseBlock(ino_addr,
										 block_addr,
										 &offset,
										 TID_EMPTY);

		/* if there are no more entries in the directory, we're done*/
		if(res == FS_ERROR_OFST_OVRFLW){
			break;
		}
		else if(res){
			freeNamesArrary(*namelist, de_idx, freer);
			SYS_RETURN(-1);
		}

		/* read next non-sparse block*/
		SYS_VERIFY(!fsReadBlock(block_addr, fs_buffer, TID_EMPTY, CACHE_ENTRY_OFFSET_EMPTY, FLAG_CACHEABLE_READ_YES));
		de_ptr = (nandfs_dirent*)(fs_buffer);
	}

//	PRINT_MSG_AND_NUM("\nscandir() - about to do insertion_sort. de_idx=", de_idx);
	if(!de_idx){
		/* sort array*/
		insertion_sort((void**)arr, de_idx, sizeof(nandfs_dirent**), compar);
	}

	*namelist = arr;

	SYS_RETURN(de_idx);
}
