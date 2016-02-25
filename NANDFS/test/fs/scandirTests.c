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

/** @file scandirTests.c  */
#include <test/fs/scandirTests.h>
#include <test/fs/testsHeader.h>

void runAllScandirTests(){
	
	RUN_TEST(scandir,1);	
	RUN_TEST(scandir,3);
	RUN_TEST(scandir,4);
	RUN_TEST(scandir,5);
	
}

/**
 * @brief 
 * init scandir test
 * 
 */
void init_scandirTest(){
	if(nandInit())
		return;	
			
	init_flash();
	initializeRamStructs();
	handleNoFs();
	init_fsbuf(fs_buffer);		
}


int32_t mock_alphasort(nandfs_dirent ** a, nandfs_dirent **b){
	return fsStrcmp(DIRENT_GET_NAME(CAST_TO_DIRENT(*a)),DIRENT_GET_NAME(CAST_TO_DIRENT(*b))); 
}

#include <stdlib.h>	

void mock_free(void *ptr){
	free(ptr);
}

void* mock_realloc(void *ptr, int32_t size){
	return realloc(ptr, size);
}

void* limited_realloc(void *ptr, int32_t size){
	/* don't allow allocating more than 5 pointers*/
	if(size==(sizeof(nandfs_dirent*)*5)) return NULL;
	
	return realloc(ptr, size);
}	

/**
 * @brief 
 * tear down scandir test
 * 
 */
void tearDown_scandirTest(){
	init_flash();
	nandTerminate();
	initializeRamStructs();	
}

/**
 * @brief
 * mock filter for direntries.
 * filter all entries whose name is in length > 10
 * 
 * @param a directory entry
 * @return 0 if filtered. 1 if not filtered
 */
int32_t mock_filter(nandfs_dirent *a){
	return (DIRENT_GET_NAME_LEN(CAST_TO_DIRENT(a)) <= 10);		
}

int32_t mock_filterall(nandfs_dirent *a){
	return 0;
}

/**
 * @brief
 * scan a directory that contains directory entries, filter according to length, and sort by name.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t scandirTest1(){	
	nandfs_dirent **namelist;
    int32_t fd, i, res;
    int32_t n = 2; /* ".", ".."*/
    uint8_t file1[12] = "/1234567890", file2[14] = "/123456789011";
    
    /* create files that will be filtered beacuse their name length > 10*/
    for(i=0; i< 10; i++){
    	fd = creat(file2, 0);
    	VERIFY(!IS_NEGATIVE(fd));
    	VERIFY(!close(fd));
    	file2[5]++;
    }
    
    for(i=0; i< 10; i++){
    	fd = creat(file1, 0);
    	VERIFY(!IS_NEGATIVE(fd));
    	VERIFY(!close(fd));
    	file1[5]++;
    	
    	n++;
    }
//    PRINT_MSG_AND_NUM("\ncreats succes. n=", n);
    
    res = scandir("/", &namelist, mock_filter, mock_alphasort, mock_realloc, mock_free);
//    PRINT_MSG_AND_NUM("\nscandir res=", res);
    VERIFY(COMPARE(n, res));    
    
    /* verify directory entries*/
    for(i=0; i<n; i++){    	
//    	PRINT_MSG_AND_STR("\n", DIRENT_GET_NAME(namelist[i]));    	
//    	PRINT_MSG_AND_NUM("\ni=", i);
    	VERIFY(mock_filter(namelist[i]));
//    	PRINT("\nfilter ok");
    	if(i==n-1){
    		continue;
    	}
    	
    	VERIFY(mock_alphasort(&(namelist[i]), &(namelist[i+1])));
//    	PRINT("\ncomparison to next ok");
    }
	
	freeNamesArrary(namelist, res, mock_free);
	
	return 1;
}

/**
 * @brief
 * scan illegal directory name.
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t scandirTest3(){
	nandfs_dirent **namelist;
    int32_t fd, i;
    uint8_t file1[12] = "/1234567890", file2[14] = "/123456789011";
    
    /* create files that will be filtered beacuse their name length > 10*/
    for(i=0; i< 10; i++){
    	fd = creat(file2, 0);
    	VERIFY(!IS_NEGATIVE(fd));
    	VERIFY(!close(fd));
    	file2[5]++;
    }
    
    for(i=0; i< 10; i++){
    	fd = creat(file1, 0);
    	VERIFY(!IS_NEGATIVE(fd));
    	VERIFY(!close(fd));
    	file1[5]++;    	
    }
    
    VERIFY(IS_NEGATIVE(scandir("/trash", &namelist, mock_filterall, mock_alphasort, mock_realloc, mock_free)));   
       
	return 1;
}

/**
 * @brief
 * scan directory and filter all entries.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t scandirTest4(){
	nandfs_dirent **namelist;
    int32_t fd, i, res;
    uint8_t file1[12] = "/1234567890", file2[14] = "/123456789011";
    
    /* create files that will be filtered beacuse their name length > 10*/
    for(i=0; i< 10; i++){
    	fd = creat(file2, 0);
    	VERIFY(!IS_NEGATIVE(fd));
    	VERIFY(!close(fd));
    	file2[5]++;
    }
    
    for(i=0; i< 10; i++){
    	fd = creat(file1, 0);
    	VERIFY(!IS_NEGATIVE(fd));
    	VERIFY(!close(fd));
    	file1[5]++;    	
    }
//    PRINT_MSG_AND_NUM("\ncreats succes. n=", n);
    
    res = scandir("/", &namelist, mock_filterall, mock_alphasort, mock_realloc, mock_free);
    
    VERIFY(IS_NULL(namelist));
    VERIFY(COMPARE(0, res));
    
	return 1;
}

/**
 * @brief
 * scan a directory that contains directory entries, where allocating all entries will result in an error. 
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t scandirTest5(){
	nandfs_dirent **namelist;
    int32_t fd, i, res;
    int32_t n = 2; /* ".", ".."*/
    uint8_t file1[12] = "/1234567890", file2[14] = "/123456789011";
    
    /* create files that will be filtered beacuse their name length > 10*/
    for(i=0; i< 10; i++){
    	fd = creat(file2, 0);
    	VERIFY(!IS_NEGATIVE(fd));
    	VERIFY(!close(fd));
    	file2[5]++;
    }
    
    for(i=0; i< 10; i++){
    	fd = creat(file1, 0);
    	VERIFY(!IS_NEGATIVE(fd));
    	VERIFY(!close(fd));
    	file1[5]++;
    	
    	n++;
    }
//    PRINT_MSG_AND_NUM("\ncreats succes. n=", n);
    
    res = scandir("/", &namelist, mock_filter, mock_alphasort, limited_realloc, mock_free);
//    PRINT_MSG_AND_NUM("\nscandir res=", res);
    VERIFY(IS_NEGATIVE(res));    
    
	return 1;
}
