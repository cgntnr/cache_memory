#include "util.h"
#include "error.h"
#include "tlb.h"
#include "addr_mng.h"
#include "list.h"
#include "tlb_mng.h"
#include "page_walk.h"

#define SIMPLE_TLB_SIZE 128

//simply set all fields of entries to 0
int tlb_flush(tlb_entry_t * tlb){
	
	M_REQUIRE_NON_NULL(tlb);
	
	for(int i=0; i<TLB_LINES; i++){
		
		zero_init_var(tlb[i]);
		
	} 
	
	return ERR_NONE;
}


int tlb_hit(const virt_addr_t * vaddr,
            phy_addr_t * paddr,
            const tlb_entry_t * tlb,
            replacement_policy_t * replacement_policy){
	
	if(vaddr == NULL || paddr == NULL || tlb == NULL || replacement_policy == NULL){
		return 0;	
	}
	
	uint64_t virt_page_number = virt_addr_t_to_virtual_page_number(vaddr);
		
		
		//going backwards the link list and if there is a hit
		//we modify paddr and use replacement policy accordingly
		for_all_nodes_reverse(current,replacement_policy->ll){
				
			list_content_t index = current->value;			
			
			if(tlb[index].tag == virt_page_number && tlb[index].v == 1){
				
				paddr->phy_page_num = tlb[index].phy_page_num;
				paddr->page_offset = vaddr->page_offset;

				replacement_policy->move_back(replacement_policy->ll, current);
				
				return 1;				
			}			
		}
		
	return 0;
}
            
         
            
//inserting an element in tlb after parameter checks
int tlb_insert(uint32_t line_index,
                const tlb_entry_t * tlb_entry,
                tlb_entry_t * tlb){
					
		M_REQUIRE_NON_NULL(tlb_entry);
		M_REQUIRE_NON_NULL(tlb);
		M_REQUIRE(line_index < SIMPLE_TLB_SIZE, ERR_BAD_PARAMETER, 
			"line_index must be smaller than SIMPLE_TLB_SIZE", line_index);
		
		tlb[line_index] = *tlb_entry;
		
		return ERR_NONE;			
					
}


int tlb_entry_init( const virt_addr_t * vaddr,
                    const phy_addr_t * paddr,
                    tlb_entry_t * tlb_entry){
	
	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE_NON_NULL(paddr);		
	M_REQUIRE_NON_NULL(tlb_entry);	
		
						
	tlb_entry->tag = virt_addr_t_to_virtual_page_number(vaddr);
	tlb_entry->phy_page_num = paddr->phy_page_num;					
	tlb_entry->v = 1 ;
	
	return ERR_NONE;				
						
}
                    
              

int tlb_search( const void * mem_space,
                const virt_addr_t * vaddr,
                phy_addr_t * paddr,
                tlb_entry_t * tlb,
                replacement_policy_t * replacement_policy,
                int* hit_or_miss){
					
		M_REQUIRE_NON_NULL_CUSTOM_ERR(mem_space, ERR_MEM);
		M_REQUIRE_NON_NULL(vaddr);
		M_REQUIRE_NON_NULL(paddr);
		M_REQUIRE_NON_NULL(tlb);
		M_REQUIRE_NON_NULL(replacement_policy);
		M_REQUIRE_NON_NULL(hit_or_miss);
		

					
		*hit_or_miss = tlb_hit(vaddr, paddr, tlb, replacement_policy);	
		
		//in case of miss initializing a new tlb entry,inserting it and 
		//moving accordingly with replacement policy
		if(!(*hit_or_miss)){
			
			
			M_EXIT_IF_ERR(page_walk(mem_space, vaddr, paddr), "page walk process");
			
			tlb_entry_t new_tlb_entry = {0, 0, 0};
			
			M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &new_tlb_entry), "initializing tlb entry");
			
			M_EXIT_IF_ERR(tlb_insert(replacement_policy->ll->front->value, &new_tlb_entry, tlb), "inserting tlb entry");

			replacement_policy->move_back(replacement_policy->ll, replacement_policy->ll->front);
				
		}
			              
	return ERR_NONE;
}
