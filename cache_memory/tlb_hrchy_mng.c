#include <stdlib.h>
#include "tlb_hrchy_mng.h"
#include "tlb_hrchy.h"
#include "error.h"
#include "addr_mng.h"
#include "page_walk.h"
#include "util.h"

//erases all data in tlb corresponding to tlb_type
int tlb_flush(void *tlb, tlb_t tlb_type){
	M_REQUIRE_NON_NULL(tlb);
	
	if(tlb_type == L1_ITLB){
	
		initialize_tlb_entries(l1_itlb_entry_t,L1_ITLB_LINES);

	}else if(tlb_type == L1_DTLB){
		
		initialize_tlb_entries(l1_dtlb_entry_t,L1_DTLB_LINES);
	
	}else if(tlb_type == L2_TLB){

		initialize_tlb_entries(l2_tlb_entry_t,L2_TLB_LINES);
	
	}else{
		return ERR_BAD_PARAMETER;
	}
	return ERR_NONE;
}


int tlb_hit( const virt_addr_t * vaddr,
             phy_addr_t * paddr,
             const void  * tlb,
             tlb_t tlb_type){

	//since this method should only return 1 or 0 
	//we simply say "miss" in case of a parameter problem			 
	if(vaddr == NULL || paddr == NULL || tlb == NULL){
		return 0;
	}			 
				 
	size_t index = 0;			 
	uint64_t virt_page_num = virt_addr_t_to_virtual_page_number(vaddr);

	 if(tlb_type == L1_ITLB){
		
		hit_miss_process(l1_itlb_entry_t, L1_ITLB_LINES, L1_ITLB_LINES_BITS);
		
	}else if(tlb_type == L1_DTLB){
			
		hit_miss_process(l1_dtlb_entry_t, L1_DTLB_LINES, L1_DTLB_LINES_BITS);
		
	}else if(tlb_type == L2_TLB){
			
		hit_miss_process(l2_tlb_entry_t, L2_TLB_LINES, L2_TLB_LINES_BITS);
		
	}
	return 0;
}


int tlb_insert( uint32_t line_index,
                const void * tlb_entry,
                void * tlb,
                tlb_t tlb_type){
	
	M_REQUIRE_NON_NULL(tlb_entry);
	M_REQUIRE_NON_NULL(tlb);

	if(tlb_type == L1_ITLB){
	
		insert_tlb_entry(tlb_entry, line_index, l1_itlb_entry_t, L1_ITLB_LINES);

	}else if(tlb_type == L1_DTLB){

		insert_tlb_entry(tlb_entry, line_index, l1_dtlb_entry_t, L1_DTLB_LINES);

	}else if(tlb_type == L2_TLB){
		
		insert_tlb_entry(tlb_entry, line_index, l2_tlb_entry_t, L2_TLB_LINES);

	}else{
		return ERR_BAD_PARAMETER;
	}	

	return ERR_NONE;			
}

int tlb_entry_init( const virt_addr_t * vaddr,
                    const phy_addr_t * paddr,
                    void * tlb_entry,
                    tlb_t tlb_type){
						
			
	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE_NON_NULL(paddr);		
	M_REQUIRE_NON_NULL(tlb_entry);		
	
	uint32_t phy_page_num = paddr->phy_page_num;
	uint64_t virt_page_num = virt_addr_t_to_virtual_page_number(vaddr);
	
	
	if(tlb_type == L1_ITLB){
		set_init_values(l1_itlb_entry_t, L1_ITLB_LINES_BITS);

	}else if(tlb_type == L1_DTLB){
			
		set_init_values(l1_dtlb_entry_t, L1_DTLB_LINES_BITS);

	}else if(tlb_type == L2_TLB){
			
		set_init_values(l2_tlb_entry_t, L2_TLB_LINES_BITS);
	}else{
		return ERR_BAD_PARAMETER;
	}
	
	return ERR_NONE;							
}

int tlb_search( const void * mem_space,
                const virt_addr_t * vaddr,
                phy_addr_t * paddr,
                mem_access_t access,
                l1_itlb_entry_t * l1_itlb,
                l1_dtlb_entry_t * l1_dtlb,
                l2_tlb_entry_t * l2_tlb,
                int* hit_or_miss){
					

			M_REQUIRE_NON_NULL(vaddr);		
			M_REQUIRE_NON_NULL(paddr);		
			M_REQUIRE_NON_NULL(l1_itlb);
			M_REQUIRE_NON_NULL(l1_dtlb);	
			M_REQUIRE_NON_NULL(l2_tlb);	
			M_REQUIRE_NON_NULL(hit_or_miss);
			M_REQUIRE_NON_NULL_CUSTOM_ERR(mem_space, ERR_MEM);
			M_REQUIRE(access == INSTRUCTION || access == DATA, ERR_BAD_PARAMETER,
				"access asked is neither instruction nor data", access);
			
			
			tlb_t tlb_type = (access == INSTRUCTION) ? L1_ITLB : L1_DTLB;
			
			void * tlb;

			if(tlb_type ==  L1_ITLB){ 
				tlb = l1_itlb;
			 }else if(tlb_type ==  L1_DTLB){
				tlb = l1_dtlb;
			}
			
			//found in level 1 tlb, tlb_hit handles rest
			if(tlb_hit(vaddr,paddr,tlb,tlb_type) == 1){
				
				*hit_or_miss = 1;	

			}else{
				
				//checking if it's in l2_tlb, if so initializing
				//and inserting into proper l1_tlb
				if(tlb_hit(vaddr,paddr,l2_tlb, L2_TLB)){
					*hit_or_miss = 1;	
	
					size_t line_index = 0;			 
					uint64_t virt_page_num = virt_addr_t_to_virtual_page_number(vaddr);

					
					if(tlb_type ==  L1_ITLB){ 		
						line_index = virt_page_num % L1_ITLB_LINES;		
						l1_itlb_entry_t new_l1_itlb_entry;
						M_EXIT_IF_ERR(tlb_entry_init(vaddr,paddr,&new_l1_itlb_entry,L1_ITLB), "tlb entry initializing");
						M_EXIT_IF_ERR(tlb_insert(line_index,&new_l1_itlb_entry,l1_itlb,L1_ITLB), "tlb entry inserting");

					}else{
						line_index = virt_page_num % L1_DTLB_LINES;	
						l1_dtlb_entry_t new_l1_dtlb_entry;
						M_EXIT_IF_ERR(tlb_entry_init(vaddr,paddr,&new_l1_dtlb_entry,L1_DTLB), "tlb entry initializing");
						M_EXIT_IF_ERR(tlb_insert(line_index,&new_l1_dtlb_entry,l1_dtlb,L1_DTLB), "tlb entry inserting");
					}

					
				// if not found in level 2, 
				//acquring physical address from virtual address
				//creating and inserting tlb entries for both l2 and l1
				//invalidating the entry in "other" l1_tlb
				}else{
					
					*hit_or_miss = 0;
							
					M_EXIT_IF_ERR(page_walk(mem_space,vaddr,paddr), "page_walk to acquire physical address");			
					l2_tlb_entry_t new_l2_tlb_entry;							
					M_EXIT_IF_ERR(tlb_entry_init(vaddr,paddr,&new_l2_tlb_entry,L2_TLB), "l2 tlb entry initializing");
		
					uint64_t virt_page_num = virt_addr_t_to_virtual_page_number(vaddr);		
					size_t   l1_line_index = 0;
					size_t   l2_line_index = virt_page_num % L2_TLB_LINES; 
	
					//eviction policy
					uint8_t valid_replacement = 0;
					uint32_t adjusted_tag = 0;
					
					//case the "to-be-replaced entry" is valid
					if(l2_tlb[l2_line_index].v == 1){
						
						//adding two bits from l2_line_index in order to 
						//compare with an 32 bit tag from l1
						adjusted_tag = l2_tlb[l2_line_index].tag << 2;
						adjusted_tag |= (l2_line_index >> 4);
						valid_replacement = 1;
					}                   

					M_EXIT_IF_ERR(tlb_insert(l2_line_index,&new_l2_tlb_entry,l2_tlb, L2_TLB),"l2 tlb entry inserting");

					//after inserting a non existing tlb to level 2, we should initialize
					//and insert into level 1 tlb as well, according to access -tlb_type-
					if(tlb_type == L1_ITLB){
						
						l1_line_index = virt_page_num % L1_ITLB_LINES;				
						l1_itlb_entry_t l1_insertion_entry;
						M_EXIT_IF_ERR(tlb_entry_init(vaddr,paddr, &l1_insertion_entry, L1_ITLB),"l1 tlb entry initializing");
						M_EXIT_IF_ERR(tlb_insert(l1_line_index, &l1_insertion_entry,l1_itlb, tlb_type), "l1 tlb entry inserting");	
						
						//if there were a valid tag and it corresponds to a entry in other l1_tlb
						if((valid_replacement == 1) && (l1_dtlb[l1_line_index].tag == adjusted_tag)){
							l1_dtlb[l1_line_index].v =0;                                                                       
						}
					

					}else if(tlb_type == L1_DTLB){	
						l1_line_index = virt_page_num % L1_DTLB_LINES;
						l1_dtlb_entry_t l1_insertion_entry;
						M_EXIT_IF_ERR(tlb_entry_init(vaddr,paddr,&l1_insertion_entry, L1_DTLB),"l1 tlb entry initializing");
						M_EXIT_IF_ERR(tlb_insert(l1_line_index, &l1_insertion_entry,l1_dtlb, tlb_type), "l1 tlb entry inserting");
						
						//if there were a valid tag and it corresponds to a entry in other l1_tlb
						if((valid_replacement == 1) && (l1_itlb[l1_line_index].tag == adjusted_tag)){
							l1_itlb[l1_line_index].v = 0; 
						}
					}		
	
				}
	
			}
			
	return ERR_NONE;				
}
