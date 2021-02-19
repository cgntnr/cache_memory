#pragma once

/**
 * @file cache_mng.h
 * @brief cache management functions
 *
 * @author Mirjana Stojilovic
 * @date 2018-19
 */

#include "mem_access.h"
#include "addr.h"
#include "cache.h"
#include <stdio.h> // for FILE

enum cache_replacement_policy { LRU };
typedef enum cache_replacement_policy cache_replace_t;

#define HIT_WAY_MISS   ((uint8_t)  -1)
#define HIT_INDEX_MISS ((uint16_t) -1)

#define BYTE_WIDTH 8
#define LSB_THREE_MASK 0x7
#define LINE_INDEX_BITS 6
#define TAG_DIFFERENCE_BITS 3 

//=========================================================================
/**
 * @brief Useful macro to loop over ways
 *
 */
#define foreach_way(var, ways) \
  for (uint8_t var = 0; var < (ways); var++)
  
//some macros for cache operations
 
#define convert_paddr(paddr) ((paddr->phy_page_num << PAGE_OFFSET) | paddr->page_offset) 

#define init_cache_for_flush(cache_type, cache_lines, cache_ways) \
	cache_type* chosen_cache = cache; \
	for(int i=0; i<(cache_lines * cache_ways); i++){ \
		zero_init_var(chosen_cache[i]); \
	} \

#define set_cache_init_val(cache_type, remaining_bits, words_per_line, cache_line)\
	cache_type* cache_init = cache_entry; \
	cache_init -> tag = (phy_addr >> remaining_bits); \
	cache_init -> age = 0; \
	cache_init -> v = 1; \
	addr_beginning = phy_addr - (phy_addr % cache_line); \
	addr_beginning /= words_per_line; \
	const uint32_t* newMem = mem_space;	\
	for(int i=0; i<words_per_line; i++){ \
		cache_init -> line[i] = *(newMem + addr_beginning + i);	\
	} \
	
//checking if there is a match condition for ways in index
//sets hit_way and hit_index accordingly	
#define cache_hit_miss_process(words_per_line, cache_lines, cache_ways, cache_remaining_bits, cache_type) \
	index = phy_addr / (words_per_line * sizeof(word_t)); \
	index %=  cache_lines; \
	tag = (phy_addr >>  cache_remaining_bits); \
	foreach_way(way,cache_ways){ \
		if(cache_valid(cache_type, cache_ways, index, way) == 1 \
			&& cache_tag(cache_type, cache_ways, index, way) == tag){ \
			*hit_way = way; \
			*hit_index = index; \
			*p_line = cache_line(cache_type, cache_ways, index, way); \
			return ERR_NONE; \
		}else{ \
			*hit_way = HIT_WAY_MISS; \
			*hit_index = HIT_INDEX_MISS; \
		} \
	} \
	
//controlling if line and way index are in bounds then
//inserting the cache entry in argument
#define insert_cache(cache_lines, cache_ways, cache_type) \
	M_REQUIRE(cache_line_index < cache_lines, ERR_BAD_PARAMETER, "Wrong index for insertion in cache", cache_line_in); \
	M_REQUIRE(cache_way < cache_ways, ERR_BAD_PARAMETER, "Wrong cache_way for insertion in cache", cache_way); \
	cache_type *  cache_to_insert = cache; \
	const cache_type *  cache_entry = cache_line_in; \
	cache_to_insert[cache_way +  ((cache_ways) * cache_line_index)] = *cache_entry; \
	

//founding the way index of the entry to evict
//memorising the entry to evict 
#define place_on_max_way(cache_type, cache_ways, insertion_entry , insertion_index, cache, cache_enum) \
	uint8_t eviction_way = 0; \
	uint8_t max_age = 0; \
		for(size_t i = 0; i<cache_ways; i++){ \
			if(max_age < cache_age(cache_type,cache_ways,insertion_index, i)){ \
				max_age = cache_age(cache_type,cache_ways,insertion_index, i); \
				eviction_way = i; \
			} \
		} \
		l1_entry_evicted = *(cache_entry(cache_type, cache_ways,insertion_index, eviction_way)); \
		M_EXIT_IF_ERR(cache_insert(insertion_index,eviction_way, &insertion_entry, cache, cache_enum) \
				,"insertion of entry on lru"); \
		LRU_age_update(cache_type, cache_ways,eviction_way, insertion_index); \
  
//cache = l2_cache is for LRU age changes to work on level 2 cache macro
//we compute the line to insert evicted entry back in level 2
//to do so we use last three bits of tag(0x7 masking) and index in level1 
//then we look for a free place in ways on l2_evicted_insertion_index
//in case we couldn't find a place on l2_evicted_insertion_index
//we put it on least recently used
//also finding the way index of the entry to evict
#define insert_evicted_into_l2(evicted_l1, index_for_l1) \
	cache = l2_cache; \
	uint16_t l2_evicted_insertion_index = ((evicted_l1.tag & LSB_THREE_MASK) << LINE_INDEX_BITS) | index_for_l1; \
	l2_cache_entry_t l2_evicted_insertion; \
	l2_evicted_insertion.v = 1; \
	l2_evicted_insertion.age = 0; \
	l2_evicted_insertion.tag = evicted_l1.tag >> TAG_DIFFERENCE_BITS; \
	for(size_t i=0; i<L2_CACHE_WORDS_PER_LINE; i++){ \
		l2_evicted_insertion.line[i] = evicted_l1.line[i]; \
	} \
	way_found = false; \
	way = 0; \
	while(way < L2_CACHE_WAYS && !way_found){ \
		if(cache_valid(l2_cache_entry_t,L2_CACHE_WAYS, l2_evicted_insertion_index, way) == 0){ \
			M_EXIT_IF_ERR(cache_insert(l2_evicted_insertion_index, \
				way, &l2_evicted_insertion, l2_cache, L2_CACHE),"insertion of the evicted entry "); \
			way_found = true; \
			LRU_age_increase(l2_cache_entry_t,L2_CACHE_WAYS, way, l2_evicted_insertion_index); \
		} \
	way++; \
	} \
	if(!way_found){ \
		uint8_t eviction_way = 0; \
		uint8_t max_age = 0; \
		for(size_t i = 0; i<L2_CACHE_WAYS; i++){ \
			if(max_age < cache_age(l2_cache_entry_t,L2_CACHE_WAYS,l2_evicted_insertion_index, i)){ \
				max_age = cache_age(l2_cache_entry_t,L2_CACHE_WAYS,l2_evicted_insertion_index, i); \
				eviction_way = i; \
			} \
		} \
		M_EXIT_IF_ERR(cache_insert(l2_evicted_insertion_index,eviction_way, \
			&l2_evicted_insertion,l2_cache,L2_CACHE),"insertion of the evicted entry "); \
		LRU_age_update(l2_cache_entry_t,L2_CACHE_WAYS,eviction_way, l2_evicted_insertion_index); \
	} \
	
//we look for a free place in ways on l1_insertion_index
#define search_place_and_insert(cache_type, cache_ways, insertion_index, insertion_entry, cache, cache_enum) \
	bool way_found = false; \
	uint8_t way = 0; \
	while(way < cache_ways && !way_found){ \
		if(cache_valid(cache_type,cache_ways, insertion_index, way) == 0){ \
			M_EXIT_IF_ERR(cache_insert(insertion_index, way, &insertion_entry, cache, cache_enum), \
				"insertion of entry on free way"); \
			way_found = true; \
			LRU_age_increase(cache_type,cache_ways,way,insertion_index); \
		} \
		way++; \
	} \



//=========================================================================
/**
 * @brief Clean a cache (invalidate, reset...).
 *
 * This function erases all cache data.
 * @param cache pointer to the cache
 * @param cache_type an enum to distinguish between different caches
 * @return error code
 */
int cache_flush(void *cache, cache_t cache_type);

//=========================================================================
/**
 * @brief Check if a instruction/data is present in one of the caches.
 *
 * On hit, update hit infos to corresponding index
 *         and update the cache-line-size chunk of data passed as the pointer to the function.
 * On miss, update hit infos to HIT_WAY_MISS or HIT_INDEX_MISS.
 *
 * @param mem_space starting address of the memory space
 * @param cache pointer to the beginning of the cache
 * @param paddr pointer to physical address
 * @param p_line pointer to a cache-line-size chunk of data to return
 * @param hit_way (modified) cache way where hit was detected, HIT_WAY_MISS on miss
 * @param hit_index (modified) cache line index where hit was detected, HIT_INDEX_MISS on miss
 * @param cache_type to distinguish between different caches
 * @return error code
 */

int cache_hit (const void * mem_space,
               void * cache,
               phy_addr_t * paddr,
               const uint32_t ** p_line,
               uint8_t *hit_way,
               uint16_t *hit_index,
               cache_t cache_type);

//=========================================================================
/**
 * @brief Insert an entry to a cache.
 *
 * @param cache_line_index the number of the line to overwrite
 * @param cache_way the number of the way where to insert
 * @param cache_line_in pointer to the cache line to insert
 * @param cache pointer to the cache
 * @param cache_type to distinguish between different caches
 * @return error code
 */
int cache_insert(uint16_t cache_line_index,
                 uint8_t cache_way,
                 const void * cache_line_in,
                 void * cache,
                 cache_t cache_type);

//=========================================================================
/**
 * @brief Initialize a cache entry (write to the cache entry for the first time)
 *
 * @param mem_space starting address of the memory space
 * @param paddr pointer to physical address, to extract the tag
 * @param cache_entry pointer to the entry to be initialized
 * @param cache_type to distinguish between different caches
 * @return error code
 */
int cache_entry_init(const void * mem_space,
                     const phy_addr_t * paddr,
                     void * cache_entry,
                     cache_t cache_type);

//=========================================================================
/**
 * @brief Ask cache for a word of data.
 *  Exclusive policy (https://en.wikipedia.org/wiki/Cache_inclusion_policy)
 *      Consider the case when L2 is exclusive of L1. Suppose there is a
 *      processor read request for block X. If the block is found in L1 cache,
 *      then the data is read from L1 cache and returned to the processor. If
 *      the block is not found in the L1 cache, but present in the L2 cache,
 *      then the cache block is moved from the L2 cache to the L1 cache. If
 *      this causes a block to be evicted from L1, the evicted block is then
 *      placed into L2. This is the only way L2 gets populated. Here, L2
 *      behaves like a victim cache. If the block is not found neither in L1 nor
 *      in L2, then it is fetched from main memory and placed just in L1 and not
 *      in L2.
 *
 * @param mem_space pointer to the memory space
 * @param paddr pointer to a physical address
 * @param access to distinguish between fetching instructions and reading/writing data
 * @param l1_cache pointer to the beginning of L1 CACHE
 * @param l2_cache pointer to the beginning of L2 CACHE
 * @param word pointer to the word of data that is returned by cache
 * @param replace replacement policy
 * @return error code
 */
int cache_read(const void * mem_space,
               phy_addr_t * paddr,
               mem_access_t access,
               void * l1_cache,
               void * l2_cache,
               uint32_t * word,
               cache_replace_t replace);

//=========================================================================
/**
 * @brief Ask cache for a byte of data. Endianess: LITTLE.
 *
 * @param mem_space pointer to the memory space
 * @param p_addr pointer to a physical address
 * @param access to distinguish between fetching instructions and reading/writing data
 * @param l1_cache pointer to the beginning of L1 CACHE
 * @param l2_cache pointer to the beginning of L2 CACHE
 * @param byte pointer to the byte to be returned
 * @param replace replacement policy
 * @return error code
 */
int cache_read_byte(const void * mem_space,
                    phy_addr_t * p_paddr,
                    mem_access_t access,
                    void * l1_cache,
                    void * l2_cache,
                    uint8_t * p_byte,
                    cache_replace_t replace);

//=========================================================================
/**
 * @brief Change a word of data in the cache.
 *  Exclusive policy (see cache_read)
 *
 * @param mem_space pointer to the memory space
 * @param paddr pointer to a physical address
 * @param l1_cache pointer to the beginning of L1 CACHE
 * @param l2_cache pointer to the beginning of L2 CACHE
 * @param word const pointer to the word of data that is to be written to the cache
 * @param replace replacement policy
 * @return error code
 */
int cache_write(void * mem_space,
                phy_addr_t * paddr,
                void * l1_cache,
                void * l2_cache,
                const uint32_t * word,
                cache_replace_t replace);

//=========================================================================
/**
 * @brief Write to cache a byte of data. Endianess: LITTLE.
 *
 * @param mem_space pointer to the memory space
 * @param paddr pointer to a physical address
 * @param l1_cache pointer to the beginning of L1 ICACHE
 * @param l2_cache pointer to the beginning of L2 CACHE
 * @param p_byte pointer to the byte to be returned
 * @param replace replacement policy
 * @return error code
 */
int cache_write_byte(void * mem_space,
                     phy_addr_t * paddr,
                     void * l1_cache,
                     void * l2_cache,
                     uint8_t p_byte,
                     cache_replace_t replace);

//=========================================================================
/**
 * @brief Print the contents of a cache to a stream.
 * @param output the stream to print to.
 * @param cache pointer to the cache
 * @param cache_type to distinguish between different caches
 * @return error code
 */
int cache_dump(FILE* output, const void* cache, cache_t cache_type);
