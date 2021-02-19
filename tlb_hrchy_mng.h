#pragma once

/**
 * @file tlb_hrchy_mng.h
 * @brief TLB management functions for two-level hierarchy of TLBs
 *
 * @author Mirjana Stojilovic
 * @date 2018-19
 */

#include "tlb_hrchy.h"
#include "mem_access.h"
#include "addr.h"

//some macros in order to fasten the tlb processes
#define initialize_tlb_entries(tlb_type, nb_entry) \
	tlb_type* ptr = tlb; \
	for(int i = 0; i < nb_entry ; ++i){ \
		zero_init_var(ptr[i]); \
	} \

#define insert_tlb_entry(tlb_entry, line_index, tlb_type, tlb_lines) \
	tlb_type *  tlb_to_insert = tlb; \
	if(line_index < tlb_lines){	\
		const tlb_type * new_tlb_entry = tlb_entry; \
		tlb_to_insert[line_index] = *new_tlb_entry; \
	}else{ \
		return ERR_BAD_PARAMETER; \
	} \

//in case of a match(condition on if) sets the corresponding paddr fields
//and returns 1 meaning hit 0 otherwise
#define hit_miss_process(tlb_type, tlb_lines, tlb_lines_bits) \
	index = virt_page_num % tlb_lines; \
	const tlb_type*  new_tlb = tlb; \
	if(new_tlb[index].v == 1 && (new_tlb[index].tag == (virt_page_num >> tlb_lines_bits))){ \
		paddr->phy_page_num = new_tlb[index].phy_page_num; \
		paddr->page_offset = vaddr->page_offset; \
		return 1; \
	}else{ \
		return 0; \
	} \

#define set_init_values(tlb_type, tlb_line_bits) \
	tlb_type* tlb_init = tlb_entry; \
	tlb_init->tag = (virt_page_num >>  tlb_line_bits); \
	tlb_init->phy_page_num = phy_page_num; \
	tlb_init->v = 1; \


//=========================================================================
/**
 * @brief Clean a TLB (invalidate, reset...).
 *
 * This function erases all TLB data.
 * @param  tlb (generic) pointer to the TLB
 * @param tlb_type an enum to distinguish between different TLBs
 * @return  error code
 */

int tlb_flush(void *tlb, tlb_t tlb_type);

//=========================================================================
/**
 * @brief Check if a TLB entry exists in the TLB.
 *
 * On hit, return success (1) and update the physical page number passed as the pointer to the function.
 * On miss, return miss (0).
 *
 * @param vaddr pointer to virtual address
 * @param paddr (modified) pointer to physical address
 * @param tlb pointer to the beginning of the tlb
 * @param tlb_type to distinguish between different TLBs
 * @return hit (1) or miss (0)
 */

int tlb_hit( const virt_addr_t * vaddr,
             phy_addr_t * paddr,
             const void  * tlb,
             tlb_t tlb_type);

//=========================================================================
/**
 * @brief Insert an entry to a tlb. Eviction policy is simple since
 *        direct mapping is used.
 * @param line_index the number of the line to overwrite
 * @param tlb_entry pointer to the tlb entry to insert
 * @param tlb pointer to the TLB
 * @param tlb_type to distinguish between different TLBs
 * @return  error code
 */

int tlb_insert( uint32_t line_index,
                const void * tlb_entry,
                void * tlb,
                tlb_t tlb_type);

//=========================================================================
/**
 * @brief Initialize a TLB entry
 * @param vaddr pointer to virtual address, to extract tlb tag
 * @param paddr pointer to physical address, to extract physical page number
 * @param tlb_entry pointer to the entry to be initialized
 * @param tlb_type to distinguish between different TLBs
 * @return  error code
 */

int tlb_entry_init( const virt_addr_t * vaddr,
                    const phy_addr_t * paddr,
                    void * tlb_entry,
                    tlb_t tlb_type);

//=========================================================================
/**
 * @brief Ask TLB for the translation.
 *
 * @param mem_space pointer to the memory space
 * @param vaddr pointer to virtual address
 * @param paddr (modified) pointer to physical address (returned from TLB)
 * @param access to distinguish between fetching instructions and reading/writing data
 * @param l1_itlb pointer to the beginning of L1 ITLB
 * @param l1_dtlb pointer to the beginning of L1 DTLB
 * @param l2_tlb pointer to the beginning of L2 TLB
 * @param hit_or_miss (modified) hit (1) or miss (0)
 * @return error code
 */

int tlb_search( const void * mem_space,
                const virt_addr_t * vaddr,
                phy_addr_t * paddr,
                mem_access_t access,
                l1_itlb_entry_t * l1_itlb,
                l1_dtlb_entry_t * l1_dtlb,
                l2_tlb_entry_t * l2_tlb,
                int* hit_or_miss);
