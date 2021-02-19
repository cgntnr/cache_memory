#pragma once

/**
 * @file tlb.h
 * @brief definitions associated to a fully-associative TLB
 *
 * @author Mirjana Stojilovic
 * @date 2018-19
 */

#include "addr.h"
#include <stdint.h>

#define TLB_LINES 128 // the number of entries

#define VALIDATION_BIT 1


typedef struct {
	
	uint64_t tag : VIRT_PAGE_NUM ;
	uint32_t phy_page_num : PHY_PAGE_NUM ;
	uint8_t v : VALIDATION_BIT ;
	
	
}tlb_entry_t;
