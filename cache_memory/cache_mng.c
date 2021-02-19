/**
 * @file cache_mng.c
 * @brief cache management functions
 *
 * @author Mirjana Stojilovic
 * @date 2018-19
 */

#include "cache_mng.h"
#include "cache.h"
#include "util.h"
#include "error.h"
#include "addr.h"
#include "lru.h"
#include "stdlib.h"
#include <stdbool.h>
#include <inttypes.h> // for PRIx macros
#include <limits.h> // for UCHAR_MAX



int cache_entry_init(const void * mem_space,
                     const phy_addr_t * paddr,
                     void * cache_entry,
                     cache_t cache_type){
	
	M_REQUIRE_NON_NULL_CUSTOM_ERR(mem_space,ERR_MEM);
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(cache_entry);					 
	
	uint32_t phy_addr = convert_paddr(paddr);
	uint32_t addr_beginning = 0;		

				 
	if(cache_type == L1_ICACHE){
		
		set_cache_init_val(l1_icache_entry_t, L1_ICACHE_TAG_REMAINING_BITS, 
			L1_ICACHE_WORDS_PER_LINE, L1_ICACHE_LINE);

	}else if(cache_type == L1_DCACHE){	
		
		set_cache_init_val(l1_dcache_entry_t, L1_DCACHE_TAG_REMAINING_BITS, 
			L1_DCACHE_WORDS_PER_LINE, L1_DCACHE_LINE);

	}else if(cache_type == L2_CACHE){
		
		set_cache_init_val(l2_cache_entry_t, L2_CACHE_TAG_REMAINING_BITS, 
			L2_CACHE_WORDS_PER_LINE, L2_CACHE_LINE);

	}else{
		return ERR_BAD_PARAMETER;
	}				 
		 
						 
	return ERR_NONE;
}
                

int cache_flush(void *cache, cache_t cache_type){
		
	M_REQUIRE_NON_NULL(cache);

	if(cache_type == L1_ICACHE){
		
		init_cache_for_flush(l1_icache_entry_t, L1_ICACHE_LINES, L1_ICACHE_WAYS);

	}else if(cache_type == L1_DCACHE){	
		
		init_cache_for_flush(l1_dcache_entry_t, L1_DCACHE_LINES, L1_DCACHE_WAYS);
		
	}else if(cache_type == L2_CACHE){
		
		init_cache_for_flush(l2_cache_entry_t, L2_CACHE_LINES, L2_CACHE_WAYS);
		
	}else{
		return ERR_BAD_PARAMETER;
	}				 
		 
						 
	return ERR_NONE;
}


int cache_hit (const void * mem_space,
               void * cache,
               phy_addr_t * paddr,
               const uint32_t ** p_line,
               uint8_t *hit_way,
               uint16_t *hit_index,
               cache_t cache_type){
		
	M_REQUIRE_NON_NULL_CUSTOM_ERR(mem_space,ERR_MEM);	   
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(p_line);
	M_REQUIRE_NON_NULL(hit_way);
	M_REQUIRE_NON_NULL(hit_index);
	M_REQUIRE_NON_NULL(cache);
	
	
	
	uint32_t phy_addr = convert_paddr(paddr);
	uint32_t index = 0;
	uint32_t tag = 0;
	
	if(cache_type == L1_ICACHE){

	cache_hit_miss_process(L1_ICACHE_WORDS_PER_LINE, L1_ICACHE_LINES, 
		L1_ICACHE_WAYS, L1_ICACHE_TAG_REMAINING_BITS, l1_icache_entry_t);

	}else if(cache_type == L1_DCACHE){	
		
	cache_hit_miss_process(L1_DCACHE_WORDS_PER_LINE, L1_DCACHE_LINES, 
		L1_DCACHE_WAYS, L1_DCACHE_TAG_REMAINING_BITS, l1_dcache_entry_t);

		
	}else if(cache_type == L2_CACHE){
	cache_hit_miss_process(L2_CACHE_WORDS_PER_LINE, L2_CACHE_LINES, 
		L2_CACHE_WAYS, L2_CACHE_TAG_REMAINING_BITS, l2_cache_entry_t);
		
	}else{
		return ERR_BAD_PARAMETER;
	}	
					   
		return ERR_NONE;		   
		   
}

//=========================================================================
/**
 * @brief Insert an entry to a cache.
 *
 * @param cache_line_index the number of the line to overwrite
 * @param cache_way the number of the way where to insert
 * @param cache_line_in pointer to the cache line to insert
 * @param cache pointer to the cache
 * @param cache_type to distinguish between different caches
 * @return  error code
 */
int cache_insert(uint16_t cache_line_index,
                 uint8_t cache_way,
                 const void * cache_line_in,
                 void * cache,
                 cache_t cache_type){
	
	M_REQUIRE_NON_NULL(cache_line_in);	
	M_REQUIRE_NON_NULL(cache);
	
			 
	if(cache_type == L1_ICACHE){	
		
		insert_cache(L1_ICACHE_LINES, L1_ICACHE_WAYS, l1_icache_entry_t);
		 
	}else if(cache_type == L1_DCACHE){
		
		insert_cache(L1_DCACHE_LINES, L1_DCACHE_WAYS, l1_dcache_entry_t);

	}else if(cache_type == L2_CACHE){
	
		insert_cache(L2_CACHE_LINES, L2_CACHE_WAYS, l2_cache_entry_t);		

	}else{
		return ERR_BAD_PARAMETER;
	}					 
					 		 
	return ERR_NONE;
}


int cache_read(const void * mem_space,
               phy_addr_t * paddr,
               mem_access_t access,
               void * l1_cache,
               void * l2_cache,
               uint32_t * word,
               cache_replace_t replace){
			  	
	M_REQUIRE_NON_NULL_CUSTOM_ERR(mem_space, ERR_MEM);	
	M_REQUIRE_NON_NULL(paddr);		
	M_REQUIRE_NON_NULL(l1_cache);
	M_REQUIRE_NON_NULL(l2_cache);	
	M_REQUIRE_NON_NULL(word);	
	M_REQUIRE(replace == LRU, ERR_BAD_PARAMETER, "Wrong replacement policy", replace);
	M_REQUIRE(paddr->page_offset % sizeof(word_t) == 0, ERR_BAD_PARAMETER, "Physical address not aligned with words", paddr);
	M_REQUIRE(access == INSTRUCTION || access == DATA, ERR_BAD_PARAMETER, "Wrong access demand", access);
	void* cache;			
	
	word_t l1_line[L1_DCACHE_WORDS_PER_LINE];
	word_t l2_line[L2_CACHE_WORDS_PER_LINE];
	
	const uint32_t** p_l1_line = &l1_line;
	const uint32_t**  p_l2_line = &l2_line;
	 

	uint8_t hit_way = HIT_WAY_MISS;
	uint16_t hit_index = HIT_INDEX_MISS;
	uint32_t phy_addr = convert_paddr(paddr);	
	
	//getting the word select bytes
	uint8_t word_select = phy_addr & 0xC;
	uint8_t way = 0;	
	bool way_found = false;
	
	cache_t cache_type = (access == INSTRUCTION) ? L1_ICACHE : L1_DCACHE;
		
	cache = (l1_icache_entry_t*)l1_cache;
		
	if(cache_type == L1_DCACHE){
		cache = (l1_dcache_entry_t*)l1_cache;
	}
	
	
	M_EXIT_IF_ERR(cache_hit(mem_space, l1_cache, paddr, p_l1_line, &hit_way, &hit_index, L1_DCACHE),
		"looking for hit in level 1");		
			
	
	//data is on level 1 		
	if(hit_way != HIT_WAY_MISS){
		if(cache_type == L1_ICACHE){
		*word = cache_line(l1_icache_entry_t, L1_ICACHE_WAYS, hit_index, hit_way)[word_select];
		}else{
		*word = cache_line(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_index, hit_way)[word_select];	
		}
		return ERR_NONE;
		
		
	}else{
		
		//not found on level 1, looking for it in level 2	
		
		M_EXIT_IF_ERR(cache_hit(mem_space, l2_cache, paddr, p_l2_line, &hit_way, &hit_index, L2_CACHE),
			"looking for hit in level 2");	
		
		if(hit_way != HIT_WAY_MISS){
			//found in level2
			
			//devalidating entry in level 2
			cache_valid(l2_cache_entry_t,L2_CACHE_WAYS, hit_index,hit_way) = 0;

			*word = cache_line(l2_cache_entry_t, L2_CACHE_WAYS, hit_index,hit_way)[word_select];
			
			uint32_t l1_insertion_index = phy_addr / (L1_DCACHE_WORDS_PER_LINE * sizeof(word_t));
			l1_insertion_index %=  L1_DCACHE_LINES;
			
			word_t read_line[L2_CACHE_WORDS_PER_LINE];
			
			for(size_t i=0; i<L2_CACHE_WORDS_PER_LINE; i++)
				read_line[i] = cache_line(l2_cache_entry_t, L2_CACHE_WAYS, hit_index, hit_way)[i];
			
			void* entry_to_insert;
			
			if(cache_type == L1_ICACHE){
			l1_icache_entry_t i_entry_to_insert;
			i_entry_to_insert.age = 0;
			i_entry_to_insert.v = 1 ;
			i_entry_to_insert.tag = phy_addr >> L1_ICACHE_TAG_REMAINING_BITS;
			search_place_and_insert(l1_icache_entry_t,L1_ICACHE_WAYS, hit_index, i_entry_to_insert, l1_cache, L1_ICACHE);
			for(size_t i=0; i<L2_CACHE_WORDS_PER_LINE; i++)
				i_entry_to_insert.line[i] = read_line[i];
			
			entry_to_insert = &i_entry_to_insert;
			
			}else{
			l1_dcache_entry_t d_entry_to_insert;
			d_entry_to_insert.v = 1 ;
			d_entry_to_insert.age = 0;
			d_entry_to_insert.tag = phy_addr >> L1_DCACHE_TAG_REMAINING_BITS;
			for(size_t i=0; i<L2_CACHE_WORDS_PER_LINE; i++)
				d_entry_to_insert.line[i] = read_line[i];
			search_place_and_insert(l1_dcache_entry_t,L1_DCACHE_WAYS, hit_index, d_entry_to_insert, l1_cache, L1_DCACHE);
			
			entry_to_insert = &d_entry_to_insert;
			}

			if(!way_found){
	
				if(cache_type == L1_ICACHE){
				l1_icache_entry_t l1_entry_evicted;
				place_on_max_way(l1_icache_entry_t, L1_ICACHE_WAYS, entry_to_insert , 
						l1_insertion_index, l1_cache, L1_ICACHE);
				insert_evicted_into_l2(l1_entry_evicted, l1_insertion_index);
				
				}else{
				l1_dcache_entry_t l1_entry_evicted;
				place_on_max_way(l1_dcache_entry_t, L1_DCACHE_WAYS, entry_to_insert , 
						l1_insertion_index, l1_cache, L1_DCACHE);
				insert_evicted_into_l2(l1_entry_evicted, l1_insertion_index);
						
				}			
			}
			
		}else{
		//not found in either caches
	
			
			void* l1_final_insertion;
			cache = l1_cache;	
			uint16_t l1_final_index;
			
			if(cache_type == L1_ICACHE){
				l1_icache_entry_t l1_i_insertion;
				M_EXIT_IF_ERR(cache_entry_init(mem_space,paddr, &l1_i_insertion, L1_ICACHE), 
					"initializing from central memory");
				*word = l1_i_insertion.line[word_select];
				l1_final_insertion = &l1_i_insertion;
				l1_final_index = phy_addr / (L1_ICACHE_WORDS_PER_LINE * sizeof(word_t));
				l1_final_index %= L1_ICACHE_LINES; 
				
				search_place_and_insert(l1_icache_entry_t, L1_ICACHE_WAYS, l1_final_index, 
					l1_final_insertion, l1_cache, L1_ICACHE);
			}else{
				l1_dcache_entry_t l1_d_insertion;
				M_EXIT_IF_ERR(cache_entry_init(mem_space,paddr, &l1_d_insertion, L1_ICACHE),
					"initializing from central memory");
				*word = l1_d_insertion.line[word_select];
				l1_final_insertion = &l1_d_insertion;
				l1_final_index = phy_addr / (L1_DCACHE_WORDS_PER_LINE * sizeof(word_t));
				l1_final_index %= L1_DCACHE_LINES; 
				
				search_place_and_insert(l1_dcache_entry_t, L1_DCACHE_WAYS, l1_final_index, 
						l1_final_insertion, l1_cache, L1_DCACHE);	
			}


			if(!way_found){
	
				if(cache_type == L1_ICACHE){
				l1_icache_entry_t l1_entry_evicted;
				place_on_max_way(l1_icache_entry_t, L1_ICACHE_WAYS, l1_final_insertion , 
						l1_final_index, l1_cache, L1_ICACHE);
				insert_evicted_into_l2(l1_entry_evicted, l1_final_index);
				
				}else{
				l1_dcache_entry_t l1_entry_evicted;
				place_on_max_way(l1_dcache_entry_t, L1_DCACHE_WAYS, l1_final_insertion , 
						l1_final_index, l1_cache, L1_DCACHE);
				insert_evicted_into_l2(l1_entry_evicted, l1_final_index);
						
				}			
			}

		}	

	}
	
     return ERR_NONE;
}

int cache_read_byte(const void * mem_space,
                    phy_addr_t * p_paddr,
                    mem_access_t access,
                    void * l1_cache,
                    void * l2_cache,
                    uint8_t * p_byte,
                    cache_replace_t replace){

	M_REQUIRE_NON_NULL(p_paddr);
	//other controls are made in cache_read
	word_t word;
	int err_code = cache_read(mem_space,p_paddr, access, l1_cache, l2_cache, &word, replace);
	if(err_code != ERR_NONE){
		return err_code;
	}
	uint32_t phy_addr = convert_paddr(p_paddr);
	uint8_t byte_select = phy_addr % sizeof(word_t);
	
	*p_byte = (word >> (BYTE_WIDTH * byte_select)) & UCHAR_MAX;
	return ERR_NONE;
}

int cache_write(void * mem_space,
                phy_addr_t * paddr,
                void * l1_cache,
                void * l2_cache,
                const uint32_t * word,
                cache_replace_t replace){
				
	M_REQUIRE_NON_NULL_CUSTOM_ERR(mem_space, ERR_MEM);	
	M_REQUIRE_NON_NULL(paddr);		
	M_REQUIRE_NON_NULL(l1_cache);
	M_REQUIRE_NON_NULL(l2_cache);	
	M_REQUIRE_NON_NULL(word);	
	M_REQUIRE(replace == LRU, ERR_BAD_PARAMETER, "Wrong replacement policy", replace);
	M_REQUIRE(paddr->page_offset % sizeof(word_t) == 0, ERR_BAD_PARAMETER, "Physical address not aligned with words", paddr);
	void* cache;
	
	word_t l1_line[L1_DCACHE_WORDS_PER_LINE];
	word_t l2_line[L2_CACHE_WORDS_PER_LINE];
	
	const uint32_t** p_l1_line = &l1_line;
	const uint32_t**  p_l2_line = &l2_line;
	
	
	l1_dcache_entry_t l1_entry_evicted; //in case we have to evict something

	uint8_t hit_way = HIT_WAY_MISS;
	uint16_t hit_index = HIT_INDEX_MISS;
	uint32_t phy_addr = convert_paddr(paddr);
	
	//getting the word select bytes
	uint8_t word_select = phy_addr & 0xC;
	uint32_t* central_mem = mem_space;	
	
	
	M_EXIT_IF_ERR(cache_hit(mem_space, l1_cache, paddr, p_l1_line, &hit_way, &hit_index, L1_DCACHE),
			"looking for hit in level 1");
	
	//data is on level 1 		
	if(hit_way != HIT_WAY_MISS){
		
		cache = l1_cache;
		
		l1_dcache_entry_t modified_entry; 
		modified_entry.v = 1;
		modified_entry.age = 0;
		modified_entry.tag = cache_tag(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_index, hit_way);
		
		for(size_t i=0; i<L1_DCACHE_WORDS_PER_LINE; i++)
			modified_entry.line[i] = cache_line(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_index, hit_way)[i];
	
		
		modified_entry.line[word_select] = *word;	
		M_EXIT_IF_ERR(cache_insert(hit_index, hit_way, &modified_entry, l1_cache, L1_DCACHE),
			"reinserting data in level 1");
		LRU_age_update(l1_dcache_entry_t,L1_DCACHE_WAYS,hit_way,hit_index);
		
		uint32_t addr_beginning = 0;		
		addr_beginning = phy_addr - (phy_addr % L1_DCACHE_LINE);
		addr_beginning /= L1_DCACHE_WORDS_PER_LINE;
		
		for(size_t i=0; i<L1_DCACHE_WORDS_PER_LINE; i++)
			*(central_mem + addr_beginning + i) = modified_entry.line[i];
	

	}else{
	//not found on level 1, looking for it in level 2	
		
		M_EXIT_IF_ERR(cache_hit(mem_space, l2_cache, paddr, p_l2_line, &hit_way, &hit_index, L2_CACHE),
			"looking for hit in level 2");	
		
		
		if(hit_way != HIT_WAY_MISS){
		//found on level 2, now we will modify and re insert in level2 
		//then work on transfering it to level1 
			cache = l2_cache;
			
			l2_cache_entry_t l2_modified_entry;
			l2_modified_entry.v = 1;
			l2_modified_entry.tag = cache_tag(l2_cache_entry_t,L2_CACHE_WAYS,hit_index ,hit_way);
			
			for(size_t i=0; i<L2_CACHE_WORDS_PER_LINE; i++)
				l2_modified_entry.line[i] = cache_line(l2_cache_entry_t, L2_CACHE_WAYS, hit_index, hit_way)[i];
			
			l2_modified_entry.line[word_select] = *word;
			M_EXIT_IF_ERR(cache_insert(hit_index, hit_way, &l2_modified_entry, l2_cache, L2_CACHE),
				"reinserting data in level 2");	
			LRU_age_update(l2_cache_entry_t, L2_CACHE_WAYS,hit_way,hit_index);

			//devalidating entry in level 2
			cache_valid(l2_cache_entry_t,L2_CACHE_WAYS, hit_index,hit_way) = 0;
		
			//now insertion to level1
			l1_dcache_entry_t l1_insertion_entry;
			l1_insertion_entry.v = 1;
			l1_insertion_entry.tag = phy_addr >> L1_DCACHE_TAG_REMAINING_BITS;
			
			for(size_t i=0; i<L2_CACHE_WORDS_PER_LINE; i++)
				l1_insertion_entry.line[i] = l2_modified_entry.line[i];
				
			uint32_t l1_insertion_index = phy_addr / (L1_DCACHE_WORDS_PER_LINE * sizeof(word_t));
			l1_insertion_index %=  L1_DCACHE_LINES;
			
			//this is for macros to work - LRU_age_increase
			cache = l1_cache;
			
			search_place_and_insert(l1_dcache_entry_t, L1_DCACHE_WAYS, l1_insertion_index, 
						l1_insertion_entry, l1_cache, L1_DCACHE);

			//case there is no place in level 1
			if(!way_found){
				
				//this macro 
				place_on_max_way(l1_dcache_entry_t, L1_DCACHE_WAYS, l1_insertion_entry , 
						l1_insertion_index, l1_cache, L1_DCACHE);

				insert_evicted_into_l2(l1_entry_evicted, l1_insertion_index);

			}
			
			
		}else{
			//case data is not found on either caches	

			uint32_t addr_beginning = 0;		
			addr_beginning = phy_addr - (phy_addr % L1_DCACHE_LINE);
			addr_beginning /= L1_DCACHE_WORDS_PER_LINE;
			word_t line_read[L1_DCACHE_WORDS_PER_LINE];
		
			//uint32_t* central_mem = mem_space;	
			for(size_t i=0; i<L1_DCACHE_WORDS_PER_LINE; i++)
				line_read[i] = *(central_mem + addr_beginning + i);
		
			//writing the word
			line_read[word_select] = *word;
			
			l1_dcache_entry_t l1_final_insertion;
			l1_final_insertion.v = 1;
			l1_final_insertion.age = 0;
			l1_final_insertion.tag = phy_addr >> L1_DCACHE_TAG_REMAINING_BITS;
		
			for(size_t i=0; i<L1_DCACHE_WORDS_PER_LINE; i++){
			*(central_mem + addr_beginning + i) = line_read[i];
			l1_final_insertion.line[i] = line_read[i];
			}

			cache = l1_cache;		
			uint16_t l1_final_index = phy_addr / (L1_DCACHE_WORDS_PER_LINE * sizeof(word_t)); 
			l1_final_index %= L1_DCACHE_LINES; 

			
			
			search_place_and_insert(l1_dcache_entry_t, L1_DCACHE_WAYS, l1_final_index, 
						l1_final_insertion, l1_cache, L1_DCACHE);

			if(!way_found){
				
				
			place_on_max_way(l1_dcache_entry_t, L1_DCACHE_WAYS, l1_final_insertion, 
					l1_final_index, l1_cache, L1_DCACHE);
			insert_evicted_into_l2(l1_entry_evicted, l1_final_index);

			}

		}
	
	}

		return ERR_NONE;
}

int cache_write_byte(void * mem_space,
                     phy_addr_t * paddr,
                     void * l1_cache,
                     void * l2_cache,
                     uint8_t p_byte,
                     cache_replace_t replace){
						 
	M_REQUIRE_NON_NULL(mem_space);
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(l1_cache);
	M_REQUIRE_NON_NULL(l2_cache);	
	M_REQUIRE(replace == LRU, ERR_BAD_PARAMETER, "Wrong replacement policy", replace);		 
	
	uint32_t phy_addr = convert_paddr(paddr);
	uint8_t byte_select = phy_addr % sizeof(word_t);
	word_t word;
	int err_code = cache_read(mem_space, paddr, L1_DCACHE, l1_cache, l2_cache, &word, replace);
	if(err_code != ERR_NONE){
		fprintf(stderr, "Some error encountered in cache_read\n");
		return err_code;
	}
	
	uint32_t mask =  UCHAR_MAX << (BYTE_WIDTH * byte_select);
	mask  = ~mask;
	word &= mask;
	uint32_t temp = p_byte << (BYTE_WIDTH * byte_select);
	word |= temp;
	return cache_write(mem_space, paddr, l1_cache, l2_cache, &word, replace);
}

//=========================================================================
#define PRINT_CACHE_LINE(OUTFILE, TYPE, WAYS, LINE_INDEX, WAY, WORDS_PER_LINE) \
    do { \
            fprintf(OUTFILE, "V: %1" PRIx8 ", AGE: %1" PRIx8 ", TAG: 0x%03" PRIx16 ", values: ( ", \
                        cache_valid(TYPE, WAYS, LINE_INDEX, WAY), \
                        cache_age(TYPE, WAYS, LINE_INDEX, WAY), \
                        cache_tag(TYPE, WAYS, LINE_INDEX, WAY)); \
            for(int i_ = 0; i_ < WORDS_PER_LINE; i_++) \
                fprintf(OUTFILE, "0x%08" PRIx32 " ", \
                        cache_line(TYPE, WAYS, LINE_INDEX, WAY)[i_]); \
            fputs(")\n", OUTFILE); \
    } while(0)

#define PRINT_INVALID_CACHE_LINE(OUTFILE, TYPE, WAYS, LINE_INDEX, WAY, WORDS_PER_LINE) \
    do { \
            fprintf(OUTFILE, "V: %1" PRIx8 ", AGE: -, TAG: -----, values: ( ---------- ---------- ---------- ---------- )\n", \
                        cache_valid(TYPE, WAYS, LINE_INDEX, WAY)); \
    } while(0)

#define DUMP_CACHE_TYPE(OUTFILE, TYPE, WAYS, LINES, WORDS_PER_LINE)  \
    do { \
        for(uint16_t index = 0; index < LINES; index++) { \
            foreach_way(way, WAYS) { \
                fprintf(output, "%02" PRIx8 "/%04" PRIx16 ": ", way, index); \
                if(cache_valid(TYPE, WAYS, index, way)) \
                    PRINT_CACHE_LINE(OUTFILE, const TYPE, WAYS, index, way, WORDS_PER_LINE); \
                else \
                    PRINT_INVALID_CACHE_LINE(OUTFILE, const TYPE, WAYS, index, way, WORDS_PER_LINE);\
            } \
        } \
    } while(0)

//=========================================================================
// see cache_mng.h
int cache_dump(FILE* output, const void* cache, cache_t cache_type)
{
    M_REQUIRE_NON_NULL(output);
    M_REQUIRE_NON_NULL(cache);

    fputs("WAY/LINE: V: AGE: TAG: WORDS\n", output);
    switch (cache_type) {
    case L1_ICACHE:
        DUMP_CACHE_TYPE(output, l1_icache_entry_t, L1_ICACHE_WAYS,
                        L1_ICACHE_LINES, L1_ICACHE_WORDS_PER_LINE);
        break;
    case L1_DCACHE:
        DUMP_CACHE_TYPE(output, l1_dcache_entry_t, L1_DCACHE_WAYS,
                        L1_DCACHE_LINES, L1_DCACHE_WORDS_PER_LINE);
        break;
    case L2_CACHE:
        DUMP_CACHE_TYPE(output, l2_cache_entry_t, L2_CACHE_WAYS,
                        L2_CACHE_LINES, L2_CACHE_WORDS_PER_LINE);
        break;
    default:
        debug_print("%d: unknown cache type", cache_type);
        return ERR_BAD_PARAMETER;
    }
    putc('\n', output);

    return ERR_NONE;
}

