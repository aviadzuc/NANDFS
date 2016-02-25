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

/**
 *
 * nand simulator driver
 *
 * Aviad Tsuck
 *
 */

#include <system.h>
#include <peripherals/nand.h>
#include <system.h>
#include <stdlib.h>
#include <string.h>


#ifdef Debug
	#include <test/macroTest.h>
#endif

#ifdef Profiling
extern int nand_total_reads;
extern int nand_spare_reads;
extern int nand_total_writes;
extern int nand_spare_writes;
extern int nand_erases;
#ifdef PROFILING_ARRAYS
	extern int writes[NAND_PAGE_COUNT];
	extern int erases[NAND_PAGE_COUNT / NAND_PAGES_PER_ERASE_UNIT];
#endif
#endif

extern unsigned char main_area_writes[NAND_PAGE_COUNT];
extern unsigned char spare_writes[NAND_PAGE_COUNT];
#define POWER_PAGES_PER_GIGA   (30-POWER_NAND_PAGE_SIZE)
#define PAGES_PER_GIGA         (1 << POWER_PAGES_PER_GIGA)
#define GIGA_SIZE              (PAGES_PER_GIGA * NAND_TOTAL_SIZE)
#define NUM_SIM_FILES          ((POWER_NAND_PAGE_COUNT > POWER_PAGES_PER_GIGA)?(1 << (POWER_NAND_PAGE_COUNT-POWER_PAGES_PER_GIGA)):1)
#define SIM_FILE_PAGE_COUNT    ((POWER_NAND_PAGE_COUNT > POWER_PAGES_PER_GIGA)?PAGES_PER_GIGA:NAND_PAGE_COUNT)
FILE *fps[NUM_SIM_FILES];
uint8_t buf[NAND_TOTAL_SIZE];
uint32_t nActions;

#define BAD_BLOCKS_FILE_NAME "badeus.dat"

static void simulatorTest(void){
	nActions++;

//	if(nActions>MAX_DEVICE_ACTIONS){
//		nandTerminate();
//		exit(1);
//	}
}

void zeroize_buf(uint8_t *buf, uint32_t size){
	while(size>0){
		buf[size-1] = 0;
		size--;
	}
}

#ifdef DO_ECC
/**
 * @brief
 * verify that the expected ECC of the page is as expected
 * @param buf buffer with page data
 * @return 1 if ecc failed, 0 if succeeded
 */
error_t verifyECC(uint8_t *buf){
	uint8_t expected_ecc[ECC_BYTS_PER_256_BYTES], read_ecc[ECC_BYTS_PER_256_BYTES];
	INIT_FLAGS_POINTER_TO_BUFFER(flags, buf);
	uint32_t i,j;
	zeroize_buf(expected_ecc, ECC_BYTS_PER_256_BYTES);
	zeroize_buf(read_ecc, ECC_BYTS_PER_256_BYTES);

	for(i=0;i<NAND_PAGE_SIZE >> POWER_256; i++){
		yaffs_ECCCalculate(&(buf[i*256]), expected_ecc);

		for(j=0;j<ECC_BYTS_PER_256_BYTES;j++){
			read_ecc[j] = flags->bytes[ECC_LOCATION+i*ECC_BYTS_PER_256_BYTES+j];
		}

		for(j=0;j<ECC_BYTS_PER_256_BYTES;j++){
//			PRINT_MSG_AND_HEX("\nverifyECC() - j=",j);
//			PRINT_MSG_AND_HEX("\n expected_ecc[j]=",expected_ecc[j]);
//			PRINT_MSG_AND_HEX("\n read_ecc[j]=",read_ecc[j]);
			if(expected_ecc[j]!=read_ecc[j]){
				/* try to correct*/
				if (yaffs_ECCCorrect(&(buf[i*256]), read_ecc, expected_ecc) == -1){
					return 1;
				}
			}
		}
	}

	return 0;
}

/**
 * @brief
 * set ecc field of page flags
 * @param buf buffer with page data
 * @return 1 if error, return 0 if successful
 */
error_t setECC(uint8_t *buf){
	INIT_FLAGS_POINTER_TO_BUFFER(flags, buf);
	uint8_t i;
//	return 0;
//	PRINT_MSG_AND_HEX("\nsetECC() - ECC_LOCATION=",ECC_LOCATION);
	for(i=0;i<NAND_PAGE_SIZE >> POWER_256; i++){
//		PRINT_MSG_AND_NUM("\nsetECC() - i=",i);
//		PRINT("\n");
		yaffs_ECCCalculate(&(buf[i*256]), &(flags->bytes[i*3+ECC_LOCATION]));

//		PRINT_MSG_AND_HEX("\nsetECC() - ecc[i*3] = ",flags->bytes[i*3+ECC_LOCATION]);
//		PRINT_MSG_AND_HEX("\nsetECC() - ecc[i*3+1] = ",flags->bytes[i*3+1+ECC_LOCATION]);
//		PRINT_MSG_AND_HEX("\nsetECC() - ecc[i*3+2] = ",flags->bytes[i*3+2+ECC_LOCATION]);
	}

	return 0;
}
#endif

uint32_t nandReadID(void){
	return NAND_DEVICE_ID;
}

error_t nandInit(void){
	uint32_t i, fp_idx;
	char sim_temp_name[100];
	simulatorTest();

	// init erase buf
	for(i=0; i<NAND_TOTAL_SIZE;i++){
		buf[i] = 0xff;
	}

//	printf("\nnandInit() - starting. NUM_SIM_FILES %u (POWER_NAND_PAGE_COUNT %u POWER_PAGES_PER_GIGA %u)",
//			                         NUM_SIM_FILES,
//			                         POWER_NAND_PAGE_COUNT,
//			                         POWER_PAGES_PER_GIGA);

	strcpy(sim_temp_name, SIM_FILE_NAME);
	sim_temp_name[strlen(SIM_FILE_NAME)]   = '0';
	sim_temp_name[strlen(SIM_FILE_NAME)+1] = '\0';

	for(fp_idx=0; fp_idx< NUM_SIM_FILES; fp_idx++){
		/* prepare sim file name, and current fp*/
		fps[fp_idx] = NULL;
		sim_temp_name[strlen(SIM_FILE_NAME)] = '0' + fp_idx;

		// open for reading just to check that it exists
		fps[fp_idx] = fopen(sim_temp_name, "r");

		if(fps[fp_idx] == NULL){
			printf("\nfile did not exits, create %s with %u pages", sim_temp_name, SIM_FILE_PAGE_COUNT);
			fps[fp_idx] = fopen(sim_temp_name, "w+");
			//verify no error
			if(fps[fp_idx] == NULL){
				printf("\ncouldn't open file");
				return 1;
			}

			// set file contents to all 0xff
			for(i=0; i< SIM_FILE_PAGE_COUNT; i++){
				if(fwrite(buf, sizeof(uint8_t), NAND_TOTAL_SIZE, fps[fp_idx]) != NAND_TOTAL_SIZE){
					printf("\ncouldn't erase EU's");
					return 1;
				}

				fflush(fps[fp_idx]);
			}

			fclose(fps[fp_idx]);

			// file exists, open it
			fps[fp_idx] = fopen(sim_temp_name, "rb+");

			//verify no error
			if(fps[fp_idx] == NULL){
				printf("\ncouldn't open file");
				return 1;
			}
		}
		else{
			//printf("\nreopen file");
			// close the old file pointer
			fclose(fps[fp_idx]);

			// file exists, open it
//			printf("\nfile exists, opening %s", sim_temp_name);
			fps[fp_idx] = fopen(sim_temp_name, "rb+");

			//verify no error
			if(fps[fp_idx] == NULL){
				printf("\ncouldn't open file");
				return 1;
			}

			/* delete file contents on re-opening */
			for(i=0; i< SIM_FILE_PAGE_COUNT; i++){
				main_area_writes[i] = spare_writes[i] = 0;
				if(fwrite(buf, sizeof(uint8_t), NAND_TOTAL_SIZE, fps[fp_idx]) != NAND_TOTAL_SIZE){
					printf("couldn't erase EU's");
					return 1;
				}
			}
		}
	}
	return 0;


}

static FILE*
get_fp(uint32_t page){
	uint32_t fps_offset = page / PAGES_PER_GIGA;
	if(fps_offset > NUM_SIM_FILES-1){
		printf("\nget_fp() - fps_offset > NUM_SIM_FILES-1 (page=%u)", page);
		fflush(stdout);
		exit(-1);
		return NULL;
	}

	return fps[fps_offset];
}

error_t
nandReadPage(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len){
	uint32_t i;

	FILE *fp = get_fp(page);
	if(fp == NULL){
		printf("fp is null");
		return 1;
	}

	simulatorTest();
//
//	/* can't read page beyond its size*/
//	if (offset+len > NAND_TOTAL_SIZE){
//		printf("\nnandReadPage() - offset+len > NAND_TOTAL_SIZE (page %u, offset %u, len %u)",
//																 page,
//																 offset,
//																 len);
//		return 1;
//	}

	// cant read non-existing page
	if (page >= NAND_PAGE_COUNT){
		printf("\nnandReadPage() - page >= NAND_PAGE_COUNT (page=%u)", page);
		return 1;
	}

//	printf("\nnandReadPageSpare() - page * NAND_TOTAL_SIZE+ offset=%u", page * NAND_TOTAL_SIZE+ offset);
	assert((page % PAGES_PER_GIGA) * NAND_TOTAL_SIZE+offset < GIGA_SIZE);
	if (fseek(fp, (page % PAGES_PER_GIGA) * NAND_TOTAL_SIZE+offset, SEEK_SET) != 0){
		printf("\nfseek(fp, page (=%u)* NAND_TOTAL_SIZE+offset (=%u), SEEK_SET) != 0", page, offset);
		return 1;
	}

	//printf("\nfile position=%d",page * NAND_TOTAL_SIZE+offset);

	if(fread(buf, sizeof(uint8_t) , len,fp) != len){
		printf("\nnandReadPage() fread(buf, sizeof(uint8_t) , len,fp) != len (page %u, offset %u, len %u)", page, offset, len);
		return 1;
	}

#ifdef DO_ECC
	if(verifyECC(buf)){
		printf("\necc failed page %u",page);

		for(i=0;i<NAND_TOTAL_SIZE;i++){
			printf("\n%u. %x",i,buf[i]);
		}

		return 1;
	}
#endif

#ifdef Profiling
//	printf("\nnandReadPage() - read page %d", page);
	nand_total_reads++;
#endif

	return 0;
}

error_t
nandReadPageSpare(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len){
	FILE *fp = get_fp(page);
	if(fp == NULL){
		printf("fp is null");
		return 1;
	}

	simulatorTest();

	// cant read non-existing page
	if (page >= NAND_PAGE_COUNT){
		printf("\nnandReadPageSpare() - page >= NAND_PAGE_COUNT (page=%u)", page);
		assert(0);
		return 1;
	}

//	printf("\nnandReadPageSpare() - offset=%u", offset);
	/* can't overflow page*/
	if(offset > NAND_SPARE_SIZE){
		printf("\noffset >= NAND_SPARE_SIZE");
		printf("(page=%u)", page);
		return 1;
	}

	/* fix len if necessary*/
	if(offset+len>NAND_SPARE_SIZE){
		printf("\nfixed len. page=%u offset=%u len=%u", page,offset, len);
		len = NAND_SPARE_SIZE - offset;
	}

//	printf("\nnandReadPageSpare() - len=%u", len);
//	printf("\nnandReadPageSpare() - page * NAND_TOTAL_SIZE+NAND_PAGE_SIZE + offset=%u", page * NAND_TOTAL_SIZE+NAND_PAGE_SIZE + offset);
	assert((page % PAGES_PER_GIGA) * NAND_TOTAL_SIZE + NAND_PAGE_SIZE + offset < GIGA_SIZE);
	if (fseek(fp, (page % PAGES_PER_GIGA) * NAND_TOTAL_SIZE + NAND_PAGE_SIZE + offset, SEEK_SET) != 0){
		printf("\nfseek(fp, page (=%u)* NAND_TOTAL_SIZE+NAND_PAGE_SIZE+offset (=%u), SEEK_SET) != 0", page, offset);
		return 1;
	}

	uint32_t res;
	res = fread(buf, sizeof(uint8_t) , len,fp);
	if(res != len){
		printf("\nnandReadPageSpare() - fread(buf, sizeof(uint8_t) , len,fp) = %u != len (page %u, offset %u, len %u)", res, page, offset, len);
		return 1;
	}
//	if(fread(buf, sizeof(uint8_t) , len,fp) != len){
//		printf("\nnandReadPageSpare() - fread(buf, sizeof(uint8_t) , len,fp) != len (page %u, offset %u, len %u)", page, offset, len);
//		return 1;
//	}
#ifdef Profiling
	nand_spare_reads++;
#endif
	return 0;
}

error_t
nandProgramPageA(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len){
	FILE *fp = get_fp(page);
	if(fp == NULL){
		printf("fp is null");
		return 1;
	}

	simulatorTest();

	/* cant read non-existing page */
	if (page >= NAND_PAGE_COUNT){
		return 1;
	}

//	L("write to main area of page %d", page);
	if(!main_area_writes[page] == 0){
		L("ERROR trying to re-write main area of page %d (assert)", page);
		assert(main_area_writes[page] == 0);
	}
#ifdef DO_ECC
	/* set ecc flags, if we program whole page*/
	if(len >= NAND_TOTAL_SIZE-1){
		setECC(buf);
	}
#endif

	// verify not overflowing page with len
	if(offset % NAND_TOTAL_SIZE + len > NAND_TOTAL_SIZE){
		len = NAND_TOTAL_SIZE - offset % NAND_TOTAL_SIZE;
	}
	assert((page % PAGES_PER_GIGA) * NAND_TOTAL_SIZE+offset < GIGA_SIZE);
	if (fseek(fp, (page % PAGES_PER_GIGA) * NAND_TOTAL_SIZE+offset, SEEK_SET) != 0)
		return 1;

	if(fwrite(buf, sizeof(uint8_t), len, fp) == 0)
		return 1;

//	printf("\nprogramA() - page=%u file position=%u",page, page * NAND_TOTAL_SIZE+offset);
	fflush(fp);
#ifdef Profiling
	nand_total_writes++;
#ifdef PROFILING_ARRAYS
	writes[page]++;
#endif
#endif

	main_area_writes[page] = 1;
	if(len == NAND_TOTAL_SIZE){
		spare_writes[page] = 1;
	}
	return 0;
}

error_t nandProgramPageB(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len){
	FILE *fp = get_fp(page);
	if(fp == NULL){
		printf("fp is null");
		return 1;
	}

	simulatorTest();
	// cant read non-existing page
	if (page >= NAND_PAGE_COUNT)
		return 1;

#ifdef DO_ECC
	/* set flags*/
	if(len>NAND_PAGE_SIZE/2)
		setECC(buf);
#endif
	// verify not overflowing page with len
	if((NAND_PAGE_SIZE /2)+offset % NAND_TOTAL_SIZE + len > NAND_TOTAL_SIZE){
		len = NAND_TOTAL_SIZE - ((NAND_PAGE_SIZE /2) + offset % NAND_TOTAL_SIZE);
	}
	assert((page % PAGES_PER_GIGA) * NAND_TOTAL_SIZE+ (NAND_PAGE_SIZE /2) + offset < GIGA_SIZE);
	if (fseek(fp, (page % PAGES_PER_GIGA) * NAND_TOTAL_SIZE+ (NAND_PAGE_SIZE /2) + offset, SEEK_SET) != 0){
		return 1;
	}

	if(fwrite(buf, sizeof(uint8_t), len, fp) == 0){
		return 1;
	}

	fflush(fp);

	return 0;
}

error_t
nandProgramPageC(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len){
	FILE *fp = get_fp(page);
	if(fp == NULL){
		printf("fp is null");
		return 1;
	}

	simulatorTest();

//	printf("\ntrying to program spare area of page %u", page);
	assert(spare_writes[page] == 0);
	// cant read non-existing page
	if (page >= NAND_PAGE_COUNT){
		printf("\ncant program non-existing page %u",page);
		return 1;
	}

	// verify not overflowing page with len
	if(NAND_PAGE_SIZE + offset % NAND_TOTAL_SIZE + len > NAND_TOTAL_SIZE){
		len = NAND_TOTAL_SIZE - (offset % NAND_TOTAL_SIZE + NAND_PAGE_SIZE);
	}
	assert((page % PAGES_PER_GIGA) * NAND_TOTAL_SIZE+ NAND_PAGE_SIZE + offset < GIGA_SIZE);
	if (fseek(fp, (page % PAGES_PER_GIGA) * NAND_TOTAL_SIZE+ NAND_PAGE_SIZE + offset, SEEK_SET) != 0){
		printf("\nseeking file failed ");
		return 1;
	}

//	printf("\nprogramC() - page=%u file position=%u",page, page * NAND_TOTAL_SIZE+ NAND_PAGE_SIZE + offset);

	if(fwrite(buf, sizeof(uint8_t), len, fp) == 0){
		printf("\nfwrite failed");
		return 1;
	}

	fflush(fp);
#ifdef Profiling
	nand_spare_writes++;
#endif

	spare_writes[page] = 1;

	return 0;
}

error_t nandReadStatus(void){
	// always 0, no error in simulator
	return 0;
}

error_t markEuAsBad(uint32_t page_addr){
	uint8_t bad_byte = 0x00;
//	PRINT_MSG_AND_NUM("\nmarkEuAsBad() - marking eu as bad in address ",page_addr);
	// mark eu as bad  (if not already marked)
	if(!nandCheckEuStatus(page_addr)){
//		PRINT("\nmarkEuAsBad() - EU is already bad");
		return 1;
	}

//	PRINT_MSG_AND_NUM("\nmarkEuAsBad() - mark page as bad ",page_addr & NAND_EU_MASK);
	if(nandProgramPageC(&bad_byte, page_addr & NAND_EU_MASK, NAND_BAD_EU_FLAG_BYTE_NUM-1, 1)){
		return 0;
	}
//	PRINT("\nmarkEuAsBad() - EU is marked as bad");
	return 1;
}

#ifdef Debug
/**
 * @brief
 * erase an EU regardless of it's status (bad or not)
 */
error_t nandBruteErase(uint32_t page_in_erase_unit){
	uint32_t j;
	FILE *fp = get_fp(page_in_erase_unit);
	if(fp == NULL){
		printf("fp is null");
		return 1;
	}

	simulatorTest();

	/* cant read non-existing page */
	if (page_in_erase_unit >= NAND_PAGE_COUNT){
		printf("\nnandErase() - page_in_erase_unit(%u) >= NAND_PAGE_COUNT", page_in_erase_unit);
		return 1;
	}

	/* get address of first page in EU */
	page_in_erase_unit &= NAND_EU_MASK;
	assert((page_in_erase_unit % PAGES_PER_GIGA) * NAND_TOTAL_SIZE < GIGA_SIZE);
	if (fseek(fp, (page_in_erase_unit % PAGES_PER_GIGA) * NAND_TOTAL_SIZE, SEEK_SET) != 0){
		printf("\nnandErase() - failed fseek");
		return 1;
	}

	/* erase page by page in erase unit */
	for(j=0; j< NAND_PAGES_PER_ERASE_UNIT;j++){
		main_area_writes[page_in_erase_unit+j] = 0;
		spare_writes[page_in_erase_unit+j]     = 0;

		if(fwrite(buf, 1, NAND_TOTAL_SIZE, fp) == 0){
			printf("\nnandErase() - failed fwrite");
			return 1;
		}
	}
	fflush(fp);
#ifdef Profiling
	nand_erases++;
#ifdef PROFILING_ARRAYS
	erases[page_in_erase_unit/NAND_PAGES_PER_ERASE_UNIT]++;
#endif
#endif

	return 0;
}
#endif

error_t nandErase(uint32_t page_in_erase_unit){
	uint32_t res, j;
	FILE *fp = get_fp(page_in_erase_unit);
	if(fp == NULL){
		printf("fp is null");
		return 1;
	}

	simulatorTest();

	/* cant read non-existing page */
	if (page_in_erase_unit >= NAND_PAGE_COUNT){
		printf("\nnandErase() - page_in_erase_unit(%u) >= NAND_PAGE_COUNT", page_in_erase_unit);
		return 1;
	}

	/* get address of first page in EU */
	page_in_erase_unit &= NAND_EU_MASK;

	res = nandCheckEuStatus(page_in_erase_unit);
	if(!res){
		return 0;
	}
	assert((page_in_erase_unit % PAGES_PER_GIGA) * NAND_TOTAL_SIZE < GIGA_SIZE);
	if (fseek(fp, (page_in_erase_unit % PAGES_PER_GIGA) * NAND_TOTAL_SIZE, SEEK_SET) != 0){
		printf("\nnandErase() - failed fseek");
		return 1;
	}

	/* erase page by page in erase unit */
	for(j=0; j< NAND_PAGES_PER_ERASE_UNIT;j++){
		main_area_writes[page_in_erase_unit+j] = 0;
		spare_writes[page_in_erase_unit+j]     = 0;

		if(fwrite(buf, 1, NAND_TOTAL_SIZE, fp) == 0){
			printf("\nnandErase() - failed fwrite");
			return 1;
		}
	}
	fflush(fp);
#ifdef Profiling
	nand_erases++;
#ifdef PROFILING_ARRAYS
	erases[page_in_erase_unit/NAND_PAGES_PER_ERASE_UNIT]++;
#endif
#endif

	return 0;
}

void nandTerminate(void){
	uint32_t fp_idx;

	for(fp_idx=0; fp_idx< NUM_SIM_FILES; fp_idx++){
		if(fps[fp_idx] != NULL){
			fclose(fps[fp_idx]);
		}
	}

}

void flushFps(void){
	uint32_t fp_idx;

	for(fp_idx=0; fp_idx< NUM_SIM_FILES; fp_idx++){
		if(fps[fp_idx] != NULL){
			fflush(fps[fp_idx]);
		}
	}
}

/**
 * verify eu status. read phy_addr of an eu, and perform eu validiy check
 * return 1 if eu is ok, 0 otherwise
 */
error_t nandCheckEuStatus(uint32_t phy_addr){
	uint8_t spare_buf[NAND_SPARE_SIZE];

	FILE *fp = get_fp(phy_addr);
	if(fp == NULL){
		printf("fp is null");
		return 1;
	}

//	L("starting. phy_addr=%x", phy_addr);
//	printf("\nread first apge spare (%u)", phy_addr & NAND_EU_MASK);
	nandReadPageSpare(spare_buf, phy_addr & NAND_EU_MASK, 0, NAND_SPARE_SIZE);
//	printf("\nverify it");
	/* verify the eu isn't bad
	 * check bad eu flag in first two pages of the EU (as required by the datasheet)*/
	if (spare_buf[NAND_BAD_EU_FLAG_BYTE_NUM-1] == 0xff){
//		printf("\nread second apge spare (%u)", (phy_addr & NAND_EU_MASK) + 1);
		nandReadPageSpare(spare_buf, (phy_addr & NAND_EU_MASK) + 1,0,NAND_SPARE_SIZE);
//		printf("\nverified first page in the eu of %u. verifying second", phy_addr);

//		L("is EU ok = %d", spare_buf[NAND_BAD_EU_FLAG_BYTE_NUM-1] == 0xff);
//		printf("\nfailed verifying first page in the eu of %u", phy_addr);
		return (spare_buf[NAND_BAD_EU_FLAG_BYTE_NUM-1] == 0xff);
	}

//	L("failed verifying first page in the eu of %u", phy_addr);
	return 0;
}

/**
 * mark bad eus, as marked on a file
 *
 */
error_t markBadEuFile(void){
	FILE *badeu_fp;
	uint32_t device_id;
	uint32_t eu_first_page;
	int32_t res;
	uint8_t bad_byte = 0x00;

	badeu_fp = fopen(BAD_BLOCKS_FILE_NAME, "rb+");

	if(badeu_fp == NULL){
		// we should have the file
		printf("\nerror - bad eus file not found");
		return 1;
	}

	res = fscanf(badeu_fp, ",%x,", &device_id);
	// verify id
	if(!res){
		printf("\nerror - no id found\n");
		return 1;
	}

	if (device_id != NAND_DEVICE_ID){
		printf("\nerror - bad eus file isn't for this device, id=%x", device_id);
		return 1;
	}

	while(1){
		res = fscanf(badeu_fp, "%u,", &eu_first_page);

		// stop if we've reached EOF or an error
		if(res == EOF || !res){
			printf("\nfailed to read from file, or reached EOF\n");
			break;
		}

		// mark eu as bad  (if not already marked)
		if(!nandCheckEuStatus(eu_first_page)){
			printf("\neu with page %u already marked as bad", eu_first_page);
			continue;
		}

		if(nandProgramPageC(&bad_byte, eu_first_page, NAND_BAD_EU_FLAG_BYTE_NUM-1, 1)){
			printf("\nerror marking page %u as bad", eu_first_page);
			return 1;
		}

		nandReadPageSpare(buf, eu_first_page, 0, NAND_SPARE_SIZE);

		printf("\nmarked page %u as bad", eu_first_page);
	}

	return 1;
}
