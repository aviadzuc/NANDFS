/** @file sequencing.h
 * Header File for sequencing layer functions, definitions and prototypes
 */
#ifndef SEQUENCING_H_
#define SEQUENCING_H_
#include <system.h>
#include <peripherals/nand.h>

/* specific defenitions */
#define POWER_SEQ_N_SLOTS_COUNT  9
#define SEQ_N_SLOTS_COUNT        (1 << POWER_SEQ_N_SLOTS_COUNT) /*512*/
#define SEQ_OBS_COUNTERS         SEQ_N_SLOTS_COUNT

#define POWER_SEQ_K_RESERVE_SLOTS   1
#define SEQ_K_RESERVE_SLOTS  		(1 << POWER_SEQ_K_RESERVE_SLOTS)
#define SEQ_SEG_HEADER_PAGES_COUNT  1
#define OBSOLETE_BYTE_OFFSET        OBS_LOCATION
#define SEQ_SEG_SIZE_IN_BITS		12
#define SEQ_OFFSET_SIZE_IN_BITS		12
#define SEQ_VALID_EU_Q_SIZE			32

#define SEQ_SEG_MASK			(0x000fff)
#define SEQ_OFFSET_MASK         (0xfff000)

#define SEQ_LOG_ADDR_SIZE           4
#define POWER_SEQ_LOG_ADDR_SIZE     2
#define CALC_IN_LOG_ADDRESSES(SIZE) ((SIZE) >> POWER_SEQ_LOG_ADDR_SIZE)
#define POWER_LOG_ADDR_PER_PAGE     (POWER_NAND_PAGE_SIZE - POWER_SEQ_LOG_ADDR_SIZE)
#define CALC_LOG_ADDR_IN_BYTES(NUM) ((NUM) << POWER_SEQ_LOG_ADDR_SIZE)

/* various sequencing structs */
/* logical page address struct */
/*
typedef struct _log_addr{
	bitfield_t segment_num:12;
	bitfield_t page_offset:12;
	bitfield_t padding:8;
} logical_addr_struct;
*/
typedef struct _log_addr{
	uint8_t bytes[SEQ_LOG_ADDR_SIZE];
} logical_addr_struct;

/*  valid eu's array*/
typedef struct _valid_eus{
	uint32_t eus[SEQ_VALID_EU_Q_SIZE];
} valid_eu_q;

/* segment map struct */
typedef struct _segmap{
	uint16_t seg_to_slot_map[SEQ_N_SLOTS_COUNT];
	logical_addr_struct reclaimed_logical_addr;
	uint32_t previously_written_segment;	 // located with sequence number, and maintained during runtime
	uint32_t reserve_eu_addr;
	uint32_t nSegments; /* count of currently allocated segments*/
	bool_t   is_eu_reserve;
	uint32_t new_slot_id;
	valid_eu_q valid_eus;
	uint64_t sequence_num;
} segment_map;

/* segment obsolete pages counters */
typedef struct _counters{
#ifndef EXACT_COUNTERS
	uint8_t counters[SEQ_OBS_COUNTERS >> 1]; // 4 bits per counter in a 8 bit unsigned integer
#else
	uint32_t counters[SEQ_OBS_COUNTERS]; // 32 bit number per counter
#endif
//	bool_t  isCountFinal;
	int32_t counter;
	int32_t free_counter;
} obs_pages_per_seg_counters;


/* general defenitions */
#define SEQ_SLOT_EU_MASK        (NAND_EU_MASK >> POWER_NAND_PAGES_PER_ERASE_UNIT)
#define SEQ_SLOT_MASK 	  	    (~SEQ_SLOT_EU_MASK) // to extract slot from the slot& offset spare area field
#define SEQ_SEGMENTS_COUNT      (SEQ_N_SLOTS_COUNT - SEQ_K_RESERVE_SLOTS - 1)
#define SEQ_DATA_SEGMENTS_COUNT (SEQ_N_SLOTS_COUNT - SEQ_K_RESERVE_SLOTS-1)

#define POWER_SEQ_PAGES_PER_SLOT    (POWER_NAND_PAGE_COUNT - POWER_SEQ_N_SLOTS_COUNT)
#define SEQ_PAGES_PER_SLOT          (1 << POWER_SEQ_PAGES_PER_SLOT)
#define SEQ_MAX_LOGICAL_OFFSET		(SEQ_PAGES_PER_SLOT-1)
#define SEQ_EUS_PER_SLOT            (SEQ_PAGES_PER_SLOT >> POWER_NAND_PAGES_PER_ERASE_UNIT)
#define SEQ_OBS_COUNTERS_ARRAY_SIZE ((SEQ_OBS_COUNTERS) >> 1)
#define SEQ_ALLOCATABLE_PAGE        (SEQ_SEG_HEADER_PAGES_COUNT + SEQ_CHECKPOINT_PAGES_COUNT)
#define SEQ_FIRST_RESERVE_SEGMENT   (SEQ_N_SLOTS_COUNT - SEQ_K_RESERVE_SLOTS)
#define SEQ_NEW_SEGMENT 			(SEQ_FIRST_RESERVE_SEGMENT-1)
#define SEQ_CHECKPOINT_SIZE			(sizeof(obs_counters_map) + sizeof(uint32_t)*2) /* 19.06.07 no (...+sizeof(segment_map)) since we no longer save seg map to cp
																					   08.01.08 save lfst states*/
#define SEQ_CHECKPOINT_PAGES_COUNT  (SEQ_CHECKPOINT_SIZE >> POWER_NAND_PAGE_SIZE)

/* probabilities for random bit generator*/
#define OBS_NEEDED_PROB      (POWER_SEQ_PAGES_PER_SLOT-3) /* log2(SEQ_PAGES_PER_SLOT) - log(obs counter levels) */
#define SEG_NEEDED_PROB      POWER_SEQ_N_SLOTS_COUNT  /*  log2(SEQ_N_SLOTS_COUNT) */
//#ifdef Debug
//#define WEAR_NEEDED_PROB     30
//#else
#define WEAR_NEEDED_PROB     POWER_SEQ_N_SLOTS_COUNT /* every time we complete a segment reclamation cycle*/
//#endif

#ifdef Profiling
	#define MOCK_FS_DATA_SIZE 1000
#endif
#ifdef Debug
	#define MOCK_FS_DATA_SIZE       100// (NAND_PAGE_SIZE+200-SEQ_CHECKPOINT_SIZE)
#endif

#define SEQ_TOTAL_DATA_BLOCKS 	     ((SEQ_PAGES_PER_SLOT-3) * SEQ_DATA_SEGMENTS_COUNT)

/* varioous macros*/
#define SEQ_VERIFY(TEST) if (!(TEST)) return 1
#define INCREMENT(VAL)   VAL += 1

#define GET_SEG_MAP_NSEGMENTS(SEG_MAP_PTR)			          (SEG_MAP_PTR->nSegments)
#define GET_SEG_MAP_SEQUENCE_NUM(SEG_MAP_PTR)		          (SEG_MAP_PTR->sequence_num)
#define GET_SEG_MAP_IS_EU_RESERVE(SEG_MAP_PTR)		          (SEG_MAP_PTR->is_eu_reserve)
#define GET_SEG_MAP_RESERVE_EU_ADDR(SEG_MAP_PTR)		   	  (SEG_MAP_PTR->reserve_eu_addr)
#define GET_SEG_MAP_NEW_SLOT_ID(SEG_MAP_PTR)		          (SEG_MAP_PTR->new_slot_id)
#define GET_SEG_MAP_SLOT_FOR_SEG(SEG_MAP_PTR,SEG_ID)          (SEG_MAP_PTR->seg_to_slot_map[SEG_ID])
#define GET_SEG_MAP_PREV_SEG(SEG_MAP_PTR)	 	              (SEG_MAP_PTR->previously_written_segment)
#define GET_SEG_VALID_EUS(SEG_MAP_PTR)					      (SEG_MAP_PTR->valid_eus.eus)

#define SET_SEG_MAP_NSEGMENTS(SEG_MAP_PTR, NSEGMENTS)         SEG_MAP_PTR->nSegments = (NSEGMENTS)
#define SET_SEG_MAP_SEQUENCE_NUM(SEG_MAP_PTR, SEQ_NUM)        SEG_MAP_PTR->sequence_num = (SEQ_NUM)
#define SET_SEG_MAP_IS_EU_RESERVE(SEG_MAP_PTR, FLAG)   	      SEG_MAP_PTR->is_eu_reserve = FLAG
#define SET_SEG_MAP_RESERVE_EU_ADDR(SEG_MAP_PTR, PHY_ADDR) 	  SEG_MAP_PTR->reserve_eu_addr = (PHY_ADDR)
#define SET_SEG_MAP_NEW_SLOT_ID(SEG_MAP_PTR, SLOT_ID)         SEG_MAP_PTR->new_slot_id = SLOT_ID
#define SET_SEG_MAP_SLOT_FOR_SEG(SEG_MAP_PTR,SLOT_ID, SEG_ID) SEG_MAP_PTR->seg_to_slot_map[SEG_ID] = (SLOT_ID)
#define SET_SEG_MAP_PREV_SEG(SEG_MAP_PTR,SEG_ID)     		  SEG_MAP_PTR->previously_written_segment = (SEG_ID)

#define SEQ_FIRST_RESERVE_SLOT								  (GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, SEQ_FIRST_RESERVE_SEGMENT))
#define INCREMENT_SEG_MAP_SEQUENCE_NUM(SEG_MAP_PTR)    SET_SEG_MAP_SEQUENCE_NUM(SEG_MAP_PTR, GET_SEG_MAP_SEQUENCE_NUM(SEG_MAP_PTR)+1)
#define INCREMENT_SEG_MAP_NSEGMENTS(SEG_MAP_PTR)       SET_SEG_MAP_NSEGMENTS(SEG_MAP_PTR, GET_SEG_MAP_NSEGMENTS(SEG_MAP_PTR)+1)
#define IS_REC_OBSOLETE()							   (GET_SEG_MAP_IS_REC_OBS(seg_map_ptr) == IS_REC_OBS_TRUE)

#define SEQ_NO_SLOT		     (0xfff)
#define SEQ_NO_PAGE_OFFSET   0xfff
#define SEQ_NO_SEGMENT_NUM   0xfff
#define SEQ_NO_SEQUENCE_NUM  (0xffffffffffffffffll)
#define SEQ_NO_SLOT_AND_EU_OFFSET 0xffffff
#define SEQ_NO_LOG_ADDRESS   0xffffff

#define IS_SEG_EMPTY(SEG_NUM)  (SEG_NUM==SEQ_NO_SEGMENT_NUM)
#define IS_SLOT_EMPTY(SLOT_ID) (SLOT_ID==SEQ_NO_SLOT)

#define SEQ_SLOT_EU_ID_EMPTY		0xffffff
#define SEQ_RESERVE_EU_NOT_OCCUPIED SEQ_SLOT_EU_ID_EMPTY
#define SEQ_RESERVE_EU_IS_HEADER    0xf0ffff

#define SEQ_PHY_ADDRESS_EMPTY		0xffffffff
#define SEQ_FIRST_PHY_ADDRESS_EMPTY	0xf00fffff

#define IS_PHY_ADDR_EMPTY(PHY_ADDR) (PHY_ADDR == SEQ_PHY_ADDRESS_EMPTY)

/* various levels for segment obsolete pages counters */
#define OBS_COUNT_NO_OBSOLETES 0x0
#define OBS_COUNT_LEVEL_0 	   0x1
#define OBS_COUNT_LEVEL_1 	   0x2
#define OBS_COUNT_LEVEL_2 	   0x3
#define OBS_COUNT_LEVEL_3 	   0x4
#define OBS_COUNT_LEVEL_4 	   0x5
#define OBS_COUNT_LEVEL_5 	   0x6
#define OBS_COUNT_LEVEL_6 	   0x7
#define OBS_COUNT_LEVEL_7 	   0x8
#define OBS_COUNT_FULL 	       0x9

/* macros for decreasing exact counter, and refering to counters*/
#ifdef EXACT_COUNTERS
#define SET_EXACT_COUNTER_LEVEL(COUNTER, LEVEL)    obs_counters_map_ptr->counters[COUNTER] = LEVEL
#define GET_EXACT_COUNTER_LEVEL(COUNTER)    obs_counters_map_ptr->counters[COUNTER]
#define INC_EXACT_COUNTER(COUNTER)   		obs_counters_map_ptr->counters[COUNTER] += 1
#define DEC_EXACT_COUNTER(COUNTER)  		obs_counters_map_ptr->counters[COUNTER] -= 1
#define INIT_EXACT_COUNTER(COUNTER)  		obs_counters_map_ptr->counters[COUNTER]  = 0
#define IS_SEGMENT_FULL(SEG_NUM)            (obs_counters_map_ptr->counters[SEG_NUM] == SEQ_PAGES_PER_SLOT)

#else
#define IS_SEGMENT_FULL(SEG_NUM) (obs_counters_map_ptr->counters[SEG_NUM] == OBS_COUNT_FULL)
#endif
/* location of block in checkpoint */
#define CP_LOCATION_FIRST  0
#define CP_LOCATION_MIDDLE 1
#define CP_LOCATION_LAST   2
#define CP_NOT_PART_OF_CP  3

/* copy flag */
typedef enum{
	COPY_FLAG_TRUE,
	COPY_FLAG_FALSE
} copy_marker;

/* segment types */
#define	SEG_TYPE_RESERVE  0 // 000
#define SEG_TYPE_USED     3 // 011
#define SEG_TYPE_NOT_USED 7 // 111

#define VOTS_FLAG_FALSE 1 // default
#define VOTS_FLAG_TRUE  0

/* obsolete flags options */
#define OBS_FLAG_TRUE  0
#define OBS_FLAG_FALSE 1

/* is eu reserve flag values */
#define IS_EU_RESERVE_TRUE  0
#define IS_EU_RESERVE_FALSE 1

/* reserve eu is a copy back flag*/
#define COPY_BACK_FLAG_TRUE  0
#define COPY_BACK_FLAG_FALSE 1

/* is the last page being reclaimed obsolete*/
#define IS_REC_OBS_TRUE  1
#define IS_REC_OBS_FALSE 0

/* macros and typedefs*/
/* define pointer type */
typedef logical_addr_struct* logical_addr_t;

/* macros*/
#define COMPARE(A,B)       ((A) == (B))
#define IS_EVEN(NUM)       (!(NUM & 1))
#define DIV_BY_2(NUM)	   ((NUM) >> 1)
#define CAST_TO_LOG_ADDR(PTR) ((logical_addr_t)(PTR))

#define CALC_OFFSET_IN_EU(PHY_ADDR) ((PHY_ADDR) & ((1 << POWER_NAND_PAGES_PER_ERASE_UNIT)-1))
#define CALC_EU_START(PHY_ADDR)     ((PHY_ADDR) & NAND_EU_MASK)

#define INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(ADDRESS_NAME) logical_addr_struct  strct ## ADDRESS_NAME ## log_struct ;\
														  logical_addr_t ADDRESS_NAME = &(strct ## ADDRESS_NAME ## log_struct)
													      /*init_logical_address(ADDRESS_NAME)*/

#define IS_PAGE_EMPTY(flags) 						   (GET_SEG_TYPE_FLAG(flags) == SEG_TYPE_NOT_USED)
#define IS_PAGES_IN_IDENTICAL_EU(offset1, offset2)	   (COMPARE(offset1 & NAND_EU_MASK, offset2 & NAND_EU_MASK))
#define IS_RESERVE_EU_NOT_TAKEN(RESERVE_EU_FLAG)	   ((RESERVE_EU_FLAG & SEQ_SLOT_MASK) == 0xffffff) // eu is erased
#define IS_BAD_EU_FLAG_MARKED(FLAG)					   ((FLAG) != 0xff)
#define IS_PAGE_OBSOLETE(FLAGS)                        (GET_OBS_FLAG(FLAGS) == OBS_FLAG_TRUE)
#define SET_LOGICAL_ADDRESS(ADDRESS, SEG, OFFSET)      {SET_LOGICAL_OFFSET(ADDRESS, OFFSET);\
													    SET_LOGICAL_SEGMENT(ADDRESS, SEG);}
#define IS_LAST_RECLAIMED_PAGE_START_OF_EU()           (CALC_OFFSET_IN_EU(GET_RECLAIMED_OFFSET()) == 0)

/* calculate a physical address*/
#define CALC_ADDRESS(SLOT_ID, EU_OFFSET, EU_PAGE_OFFSET) (((SLOT_ID) << POWER_SEQ_PAGES_PER_SLOT) + ((EU_OFFSET) << POWER_NAND_PAGES_PER_ERASE_UNIT ) + (EU_PAGE_OFFSET))
#define CALC_SLOT_OFFSET(EU_OFFSET, EU_PAGE_OFFSET)      (((EU_OFFSET) << POWER_NAND_PAGES_PER_ERASE_UNIT) + EU_PAGE_OFFSET)
#define CALC_RECLAIMED_PHYSICAL_ADDRESS()                (CALC_ADDRESS(GET_RECLAIMED_SEGMENT_SLOT(),0,GET_RECLAIMED_OFFSET()))

#define CALC_PHYSICAL_ADDRESS_FOR_LOGICAL(LOG_ADDR)      (CALC_ADDRESS(GET_SEG_MAP_SLOT_FOR_SEG(GET_LOGICAL_SEGMENT(LOG_ADDR)),0,GET_LOGICAL_OFFSET(LOG_ADDR)))
#define CALC_OFFSET_IN_SLOT(ADDRESS)					 ((ADDRESS) & ((1 << POWER_SEQ_PAGES_PER_SLOT)-1))
#define CALC_EU_OFFSET(ADDRESS)                          (CALC_OFFSET_IN_SLOT(ADDRESS) >> POWER_NAND_PAGES_PER_ERASE_UNIT)

/* calculate eu offset in the slot, move it to first NAND_ADDRESS_EU_BITS_COUNT bits
 * and then calculate slot id and place it after those bits*/
#define SLOT_OFFSET_MASK							   ((1 << POWER_SEQ_PAGES_PER_SLOT)-1)
#define EU_OFFSET_MASK								   ((1 << NAND_ADDRESS_EU_BITS_COUNT)-1)
#define SLOT_MASK									   (((1 << POWER_SEQ_N_SLOTS_COUNT)-1) << NAND_ADDRESS_EU_BITS_COUNT)
#define EXTRACT_SLOT(SLOT_EU_FLAG)					   ((SLOT_EU_FLAG & SLOT_MASK) >> NAND_ADDRESS_EU_BITS_COUNT)
#define EXTRACT_EU(SLOT_EU_FLAG)					   (SLOT_EU_FLAG & EU_OFFSET_MASK)
#define IS_EU_START(OFFSET)							   (CALC_OFFSET_IN_EU(OFFSET) == CALC_OFFSET_IN_EU(CALC_EU_START(OFFSET)))

#define CALCULATE_SLOT_ID_AND_EU_OFFSET(PHY_ADDR) 	   (((((PHY_ADDR) & SLOT_OFFSET_MASK) & NAND_EU_MASK) >> POWER_NAND_PAGES_PER_ERASE_UNIT) | \
											           (((PHY_ADDR) >> POWER_SEQ_PAGES_PER_SLOT) << NAND_ADDRESS_EU_BITS_COUNT))
#define EXTRACT_ADDR_FROM_SLOT_ID_AND_EU_OFFSET(SLOT_EU_FLAG) CALC_ADDRESS(EXTRACT_SLOT(SLOT_EU_FLAG) ,EXTRACT_EU(SLOT_EU_FLAG) ,0)


#define GET_LOGICAL_OFFSET(LOG_ADDR_PTR)				 (GET_BYTE(LOG_ADDR_PTR,0) | ((GET_BYTE(LOG_ADDR_PTR,1) & 0x0f) << 8))
#define GET_LOGICAL_SEGMENT(LOG_ADDR_PTR)				 ((GET_BYTE(LOG_ADDR_PTR,2) << 4) | ((GET_BYTE(LOG_ADDR_PTR,1) & 0xf0) >> 4))
#define GET_LOGICAL_SPARE(LOG_ADDR_PTR)				     (GET_BYTE(LOG_ADDR_PTR,3))

//#define SET_LOGICAL_OFFSET(LOG_ADDR_PTR, OFFSET)		 SET_BYTE(LOG_ADDR_PTR,0, (OFFSET) & 0xff); SET_BYTE(LOG_ADDR_PTR,1, ((((OFFSET) & 0xf00) >> 8) | (GET_BYTE(LOG_ADDR_PTR,1) & 0xf0)))
//#define SET_LOGICAL_SEGMENT(LOG_ADDR_PTR, SEG_NUM)		 SET_BYTE(LOG_ADDR_PTR,1, ((GET_BYTE(LOG_ADDR_PTR,1) & 0x0f) | (((SEG_NUM) & 0x0f) << 4))) ; SET_BYTE(LOG_ADDR_PTR,2, ((SEG_NUM) & 0xff0) >> 4)
#define SET_LOGICAL_OFFSET(LOG_ADDR_PTR, OFFSET)		 setLogicalOffset(LOG_ADDR_PTR, OFFSET)
#define SET_LOGICAL_SEGMENT(LOG_ADDR_PTR, SEG_NUM)		 setLogicalSegment(LOG_ADDR_PTR, SEG_NUM)
#define SET_LOGICAL_SPARE(LOG_ADDR_PTR, SPARE)		     SET_BYTE(LOG_ADDR_PTR,3, (SPARE) & 0xff)

//#define GET_LOGICAL_OFFSET(LOG_ADDR_PTR)				 ((LOG_ADDR_PTR)->page_offset)
//#define GET_LOGICAL_SEGMENT(LOG_ADDR_PTR)				 ((LOG_ADDR_PTR)->segment_num)
//#define GET_LOGICAL_SPARE(LOG_ADDR_PTR)					 ((LOG_ADDR_PTR)->padding)
//#define SET_LOGICAL_OFFSET(LOG_ADDR_PTR, OFFSET)		 (LOG_ADDR_PTR)->page_offset = OFFSET
//#define SET_LOGICAL_SEGMENT(LOG_ADDR_PTR, SEG_NUM)		 (LOG_ADDR_PTR)->segment_num = SEG_NUM
//#define SET_LOGICAL_SPARE(LOG_ADDR_PTR, VAL)			 (LOG_ADDR_PTR)->padding = VAL


#define GET_RECLAIMED_ADDRESS_PTR()						 (&(seg_map_ptr->reclaimed_logical_addr))
#define GET_RECLAIMED_SEGMENT()							 GET_LOGICAL_SEGMENT(GET_RECLAIMED_ADDRESS_PTR())
#define GET_RECLAIMED_OFFSET()							 GET_LOGICAL_OFFSET(GET_RECLAIMED_ADDRESS_PTR())
#define GET_RECLAIMED_SEGMENT_SLOT()					 (seg_map_ptr->seg_to_slot_map[GET_RECLAIMED_SEGMENT()])
#define SET_RECLAIMED_SEGMENT(SEG_NUM)					 SET_LOGICAL_SEGMENT(GET_RECLAIMED_ADDRESS_PTR(),SEG_NUM)
#define SET_RECLAIMED_OFFSET(OFFSET)					 SET_LOGICAL_OFFSET(GET_RECLAIMED_ADDRESS_PTR(),OFFSET)
#define INCREMENT_RECLAIMED_OFFSET(SEG_MAP_PTR)          SET_RECLAIMED_OFFSET(GET_RECLAIMED_OFFSET()+1)
#define DECREMENT_RECLAIMED_OFFSET(SEG_MAP_PTR)          SET_RECLAIMED_OFFSET(GET_RECLAIMED_OFFSET()-1)

#define NO_SEGMENTS_ALLOCATED() 				         (seg_map_ptr->nSegments == 0)
#define IS_OLD_PAGE_VALID(flags)					     (!SEQ_FLAGS_MARKED_OBSOLETE(flags) && GET_CHECKPOINT_FLAG(flags) == NOT_PART_OF_CP) // copy page only if valid && not part of checkpoint
#define IS_STATE_RECLAMATION()						     (seg_map_ptr->nSegments == SEQ_SEGMENTS_COUNT && seg_map_ptr->new_slot_id != GET_RECLAIMED_SEGMENT_SLOT()) // if the slot we wrote to is the same as new slot id, we are not in reclamation
#define IS_SEG_MAP_PAGE_OFFSET_OVERFLOWING()             (GET_RECLAIMED_OFFSET() >= SEQ_PAGES_PER_SLOT || GET_RECLAIMED_OFFSET()==0)
#define IS_SEG_MAP_ADDR_RESERVE()						 (seg_map_ptr->reserve_eu_addr != SEQ_PHY_ADDRESS_EMPTY)
#define ALL_SEGMENTS_ALLOCATED() 						 (seg_map_ptr->nSegments == SEQ_SEGMENTS_COUNT)

#define IS_SLOT_ALLOCATED_FOR_SEGMENT(SEG_MAP, SEG_ID)                ((SEG_MAP)->seg_to_slot_map[SEG_ID] != SEQ_NO_SLOT)
#define IS_ADDRESS_IN_RECLAIMED_SEGMENT(LOGICAL_ADDR, SEGMENT_MAP)    (GET_LOGICAL_SEGMENT(LOGICAL_ADDR) == GET_RECLAIMED_SEGMENT())
#define IS_ADDRESS_IN_RECLAIMED_PAGE(LOGICAL_ADDR, SEGMENT_MAP)       (GET_LOGICAL_OFFSET(LOGICAL_ADDR)  < GET_RECLAIMED_OFFSET())
#define SET_SEGMENG_MAP_RECLAIMED_ADDRESS(SEG_MAP,SEG_ID,PAGE_OFFSET) {SET_RECLAIMED_SEGMENT(SEG_ID); \
																      SET_RECLAIMED_OFFSET(PAGE_OFFSET);}
#define CALCULATE_IN_PAGES(SIZE)						 (((SIZE) >> POWER_NAND_PAGE_SIZE) + ((((SIZE) & ((1 << POWER_NAND_PAGE_SIZE)-1)) ==0)?0:1))
#define IDX_OVERFLOWS_PAGE_SIZE(IDX)                     (IDX >= NAND_PAGE_SIZE)
#define GET_PREVIOUSLY_RECLAIMED_SEGMENT_SLOT()          (GET_SLOT_BY_SEGMENT(seg_map_ptr->previously_written_segment))
#define IS_RECLAIMED_ADDRESS_IDENTICAL(log_addr)		 (GET_LOGICAL_SEGMENT(log_addr) == GET_RECLAIMED_SEGMENT() && \
														  GET_LOGICAL_OFFSET(log_addr) == GET_RECLAIMED_OFFSET())
#define IS_CHECKPOINT_MORE_THAN_ONE_PAGE(fs_layer_cp_size) (1+SEQ_CHECKPOINT_SIZE+fs_layer_cp_size > NAND_PAGE_SIZE)
#define SEQ_SHIFT_PAGE_OFFSET_TO_EU_END(PAGE)			 ((PAGE) | ~NAND_EU_MASK)
#define IS_ADDRESS_EMPTY(LOG_ADDR)						 (GET_LOGICAL_SEGMENT(LOG_ADDR) == SEQ_NO_SEGMENT_NUM)
#define VALIDATE_RESERVE_ADDRESS(RESERVE_PHY_ADDR)		 if(IS_PHY_ADDR_EMPTY(RESERVE_PHY_ADDR)) return ERROR_NO_RESERVE_EU
#define IS_NO_RESERVE_EUS()								 (seg_map_ptr->is_eu_reserve == IS_EU_RESERVE_TRUE && seg_map_ptr->reserve_eu_addr == SEQ_PHY_ADDRESS_EMPTY)
#define SET_ADDRESS_EMPTY(LOG_ADDR)						 SET_LOGICAL_SEGMENT(LOG_ADDR, SEQ_NO_SEGMENT_NUM)
#define GET_PAGE_SLOT_HEADER_ADDRESS(PHY_ADDR)			 (PHY_ADDR - CALC_OFFSET_IN_SLOT(PHY_ADDR))
#define GET_SLOT_BY_SEGMENT(SEG_NUM)					 (seg_map_ptr->seg_to_slot_map[SEG_NUM])
#define SET_SEGMENT_TO_SLOT(SEG_NUM, SLOT_ID)			 (seg_map_ptr->seg_to_slot_map[SEG_NUM] = (SLOT_ID))
#define GET_SLOT(PHY_ADDR)								 ((PHY_ADDR) >> POWER_SEQ_PAGES_PER_SLOT)
#define IS_FIRST_IN_SLOT(PHY_ADDR)						 ((PHY_ADDR) == ((PHY_ADDR >> POWER_SEQ_N_SLOTS_COUNT) << POWER_SEQ_N_SLOTS_COUNT))
#define DATA_SEGMENTS_COUNT()						     (countSegments(0))
#define RESERVE_SEGMENTS_COUNT()						 (countSegments(1))
#define IS_PAGE_USED(PAGE_FLAGS)						 (GET_SEG_TYPE_FLAG(PAGE_FLAGS) != SEG_TYPE_NOT_USED)

#define GET_COPYBACK_EU()								 get_valid_eu_addr_in_location(GET_SLOT_BY_SEGMENT(SEQ_FIRST_RESERVE_SEGMENT),2)

/* obsolete counter macros */
#define SET_OBS_COUNT(VAL)								 obs_counters_map_ptr->counter = VAL
#define GET_OBS_COUNT()									 (obs_counters_map_ptr->counter)
#define INCREASE_OBS_COUNT()							 SET_OBS_COUNT(GET_OBS_COUNT()+1)
#define DECREMENT_OBS_COUNT()							 SET_OBS_COUNT(GET_OBS_COUNT()-1)

//#define IS_OBS_COUNT_TEMPORARY()						 (obs_counters_map_ptr->isCountFinal == 0)
//#define MARK_OBS_COUNT_FINAL()							 obs_counters_map_ptr->isCountFinal = 1
//#define MARK_OBS_COUNT_TEMPORARY()						 obs_counters_map_ptr->isCountFinal = 0


#define INCREMENT_FREE_COUNT()							 obs_counters_map_ptr->free_counter++
#define DECREMENT_FREE_COUNT()							 obs_counters_map_ptr->free_counter--
#define SET_FREE_COUNTER(NUM)							 obs_counters_map_ptr->free_counter = NUM
#define GET_FREE_COUNTER()							     (obs_counters_map_ptr->free_counter)

#define calcTotalFreePages() 							 (GET_OBS_COUNT() + GET_FREE_COUNTER())
#define COMPARE_ADDR(ADDR1, ADDR2)						 (GET_LOGICAL_SEGMENT(ADDR1) == GET_LOGICAL_SEGMENT(ADDR2) && GET_LOGICAL_OFFSET(ADDR1) == GET_LOGICAL_OFFSET(ADDR2))

#define MARK_OBSOLETE_AFTER_REBOOT      1
#define MARK_OBSOLETE_NOT_AFTER_REBOOT  0

/* errors */
#define ERROR_IO				   1
#define ERROR_NO_CP      		   2
#define ERROR_FLASH_FULL           3
#define ERROR_NO_RESERVE_EU        4
#define ERROR_BAD_WEAR_LEVELING    5

#define IS_ADDRESS_ERROR(PHY_ADDR) ((PHY_ADDR) >= NAND_PAGE_COUNT)
#define TEMP_RANDOM_SLOT           ((SEQ_SEGMENTS_COUNT>>1)-2)

#define IS_COMMIT_NOT       0
#define IS_COMMIT_REGULAR   1
#define IS_COMMIT_LAST      2

extern segment_map seg_map;
extern obs_pages_per_seg_counters obs_counters_map;
extern uint8_t sequencing_buffer[NAND_TOTAL_SIZE];
extern uint8_t obsolete_byte;

#define init_obs_byte()     obsolete_byte = 0x7f /* 0111 1111*/
#define init_valid_eus_q()  init_struct(GET_SEG_VALID_EUS(seg_map_ptr), sizeof(valid_eu_q))

#define initializeSequencingStructs(){ \
	init_obs_counters(); \
	init_seg_map(); \
	init_buf(sequencing_buffer); \
	init_obs_byte(); \
	lfsr_state = LFSR_STATE_INITIAL; \
}

#ifdef Debug
	/* for tests*/
	error_t checkpointWriter(bool_t isPartOfHeader);
#endif

/**
 * @brief *
 * write the block in data, indicating whether it is VOTs by onlyVOTs, and writing it's previuous
 * block address in the transaction in prev.
 * Currently - write page to next logical address in segment (without checking if we should copy/allocate
 * new segment/reserve EU etc. ignore  prev and onlyVOTs.
 * then advance the logical address.
 *
 * Assumptions -
 * 1. logical address is initialized properly
 * 2. reclaimed logical address in segment map is set to an empty address
 *
 * @param log_addr logical address to write the address to which the data is written
 * @param data the data buffer (NAND_PAGE_SIZE)
 * @param onlyVOTs indicate whther block data contains VOT records
 * @param prev previous logical address in list
 * @param cpWritten did we write a checkpoint during this allocAndWriteBlock (i.e. we moved to a new segment)
 * @param isCommit is the allocation part of a commit
 * allocAndWriteBlock(), we do not continue writing the data.
 * @return 0 if allocation was successful.
 * 		   1 if the write was bad, or an error occured during writing in wear leveling, or when truncating
 *         ERROR_FLASH_FULL if there are no avialable pages.
 * 		   ERROR_NO_RESERVE_EU if we need to replace a bad EU but have no spare one left
 *  	   ERROR_BAD_WEAR_LEVELING if an error occured during wear leveling
 */
error_t allocAndWriteBlock(logical_addr_t log_addr, void* data, bool_t onlyVOTs, logical_addr_t prev, bool_t *cpWritten, checkpoint_writer cp_write_p, bool_t isCommit);

/**
 * read the block in logical address b to data
 */
error_t readBlock(logical_addr_t b, void *data);

/**
 * write a checkpoint, of nBlocks
 * Erase releveant vots of the commited transaction
 */
error_t commit(void* data, uint32_t nBytes, bool_t isPartOfHeader);

/**
 * Call on system boot
 * find latest checkpoint, mark all following blocks as obsolete (truncation).
 * return whether the transaction that commited in the checkpoint mught still have pending VOTs.
 * If we wrote data after commit then the sequencing layer assumes the commited transaction has none.
 */
 error_t findCheckpointAndTruncate(void* data, uint32_t nBlocks, bool_t* pending_VOTs);

/**
 * @brief
 * traverse list of VOTs of transaction.
 * read block from flash, and if it contains VOTs, copy it's contents to RAM at VOTs.
 * prev is set to the address of the block written before b by it's transaction.
 *
 * @param b logical address of a block to read
 * @param VOTs buffer in RAM to read VOT's block to
 * @param prev logical address of block before b in the linked list of VOT's
 * @param isVOTs indicator whether the block contains vots
 * @return 0 if successful. if failed to read the block 1 is returned.
 */
error_t readVOTsAndPrev(logical_addr_t b, void* VOTs, logical_addr_t prev, bool_t *isVOTs);

/**
 * @brief
 * mark block b as obsolete (second write to spare area)
 *
 * @param b logical addr to mark as obsolete
 * @param is_after_reboot indicator whether we are handling vots after reboot.
 *                        1) YES - a page that is already marked as obsolete should update obs count
 *                        2) NO  - obs count was already uncremeneted. don't do it again
 * @return 0 if successful, 1 otherwise
 */
error_t markAsObsolete(logical_addr_t b, bool_t is_after_reboot);

/**
 * @brief
 * booting.
 * find reserve segments - find the first valid EU in every slot. REASON - if its reserve slot, we have to do this.
 *
 * for every segment with a header, verify validity of header:
 * if (not valid)/empty  - erase it (to recover from possible failed program).
 * we now have the reserve segments allocated
 *
 * check that number of reserve segments is as expected:
 * if not - continue allocating them, and continue to allocating first segment
 *
 * if yes - for every slot whose first EU is bad, find the EU we would have allocated as reserve for it, and check if
 * it contains it's header. if not/invalid header - erase it. otherwise mark in seg map.
 *
 * we should have marked by now in seg map the new slot. check state
 * a. regular allocation
 * b. reclamation - maybe we failed during weare leveling/changing segment
 * and continue accordingly
 *
 * Assumptions:
 * 1. all slots have at least 2 valid EU. otherwise, the flash is really not usable
 * @param data file system data
 * @param nBytes size of file system data
 * @param pendingVOTs indicator whther after booting we have pending VOTs to delete
 * @param cp_write_p checkpoint writer
 * @return 1 if n error occured. 0 if successful
 */
error_t sequencingBooting(void* data, uint32_t nBytes, bool_t *pendingVOTs, checkpoint_writer cp_write_p);

#endif /*SEQUENCING_H_*/

