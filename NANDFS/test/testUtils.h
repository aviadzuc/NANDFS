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

/** @file testUtils.h
 *  Header file for various common utility functions
 */
#ifndef TESTUTILS_H_
#define TESTUTILS_H_

#include <system.h>

error_t markEuAsMockBad(uint32_t page_addr);

void init_flash(void);

void printBlock(uint8_t *data_buf);

void printSeqBlock(uint8_t *buf);

/**
 * return 1 if buffers are exact, 0 otherwise
 */
bool_t compare_bufs(void *buf1, void *buf2, int32_t len);

/* system calls macros*/
#define fsBooting()					nandfs_fsBooting()
#define open(PATHNAME, FLAGS, MODE)	nandfs_open(PATHNAME, FLAGS, MODE)
#define creat(PATHNAME, MODE)      	nandfs_creat(PATHNAME, MODE)
#define lseek(FD, OFFSET, WHENCE)   nandfs_lseek(FD, OFFSET, WHENCE) 
#define read(FD, BUF, COUNT)      	nandfs_read(FD, BUF, COUNT)
#define write(FD, BUF, COUNT)     	nandfs_write(FD, BUF, COUNT)
#define sync()					  	nandfs_sync()
#define fsync(FD)				  	nandfs_fsync(FD) 
#define fdatasync(FD)             	nandfs_fdatasync(FD) 
#define unlink(PATHNAME)          	nandfs_unlink(PATHNAME)
#define close(FD)                 	nandfs_close(FD)
#define mkdir(PATHNAME, MODE)     	nandfs_mkdir(PATHNAME, MODE)
#define rmdir(PATHNAME)           	nandfs_rmdir(PATHNAME)
#define opendir(NAME)			  	nandfs_opendir(NAME)
#define readdir(DIR_VAR)         	nandfs_readdir(DIR_VAR)
#define seekdir(DIR_VAR, OFFSET)  	nandfs_seekdir(DIR_VAR, OFFSET)
#define rewinddir(DIR_VAR)        	nandfs_rewinddir(DIR_VAR)
#define scandir(DIR_NAME, NAMELIST, FILTER, COMPARER, REALLOCATOR, FREER)  nandfs_scandir(DIR_NAME, NAMELIST, FILTER, COMPARER, REALLOCATOR, FREER)
#define telldir(DIR_VAR)          	nandfs_telldir(DIR_VAR)
#define dirfd(DIR_VAR)            	nandfs_dirfd(DIR_VAR)
#define closedir(DIR_VAR)         	nandfs_closedir(DIR_VAR)
#define stat(PATH, BUF)           	nandfs_stat(PATH, BUF) 
#define dup(OLD_FD)               	nandfs_dup(OLD_FD)
#define dup2(OLD_FD, NEW_FD)      	nandfs_dup2(OLD_FD, NEW_FD)

#endif /*TESTUTILS_H_*/
