#include "page_walk.h"
#include "addr_mng.h"
#include "error.h"


//in order to get to correct place in memory
//divided by 4.0 because of the byte index and 
//word index difference
static inline pte_t read_page_entry(const pte_t * start,pte_t page_start,
uint16_t index){
	 M_REQUIRE_NON_NULL(start);
	
	 uint16_t i = (page_start/sizeof(pte_t)) +   index ;

	 return start[i]; 
	
}

//as indicated in instructions we are walking 
//through translation pages in order to get physical address
int page_walk(const void* mem_space, const virt_addr_t* vaddr, phy_addr_t* paddr){
	
	M_REQUIRE_NON_NULL_CUSTOM_ERR(mem_space, ERR_MEM);
	M_REQUIRE_NON_NULL_CUSTOM_ERR(vaddr, ERR_BAD_PARAMETER);
	M_REQUIRE_NON_NULL_CUSTOM_ERR(paddr, ERR_BAD_PARAMETER);
	
	pte_t addressPUD = 0;
	pte_t addressPMD = 0;
	pte_t addressPT = 0;
	
	uint32_t tempPhyAdd = 0 ; //temporary physical address

	//first page_start is 0 because pgd is at the beginning of memory
	addressPUD = read_page_entry(mem_space, 0, vaddr->pgd_entry);
	
	addressPMD = read_page_entry(mem_space, addressPUD , vaddr->pud_entry);
	
	addressPT = read_page_entry(mem_space, addressPMD , vaddr->pmd_entry);
	
	tempPhyAdd = read_page_entry(mem_space, addressPT , vaddr->pte_entry );
	
	return init_phy_addr(paddr, tempPhyAdd, vaddr->page_offset);
		
}
