#include "addr_mng.h"
#include "addr.h"
#include "error.h"
#include <inttypes.h>


//printing fields of virtual address in suggested format
int print_virtual_address(FILE* where, const virt_addr_t* vaddr){
	
	
	M_REQUIRE_NON_NULL_CUSTOM_ERR(where, ERR_IO);
	M_REQUIRE_NON_NULL(vaddr);
	
	return fprintf(where, "PGD=0x%" PRIX16 "; PUD=0x%" PRIX16
	 "; PMD=0x%" PRIX16 "; PTE=0x%" PRIX16  "; offset=0x%" PRIX16, 
	 vaddr->pgd_entry, vaddr->pud_entry, vaddr->pmd_entry, vaddr->pte_entry, vaddr->page_offset);	
	
}

//initializing fields of virtual address from values in arguments
int init_virt_addr(virt_addr_t * vaddr,
                   uint16_t pgd_entry,
                   uint16_t pud_entry, uint16_t pmd_entry,
                   uint16_t pte_entry, uint16_t page_offset){
	
	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE(PAGE_SIZE > page_offset, ERR_BAD_PARAMETER, "Offset is bigger than page size", page_offset);
	
	vaddr -> pgd_entry = pgd_entry;
	vaddr -> pud_entry = pud_entry;
	vaddr -> pmd_entry = pmd_entry;
	vaddr -> pte_entry = pte_entry;
	vaddr -> page_offset = page_offset;
	vaddr -> reserved = 0;
	
	return ERR_NONE; 
}

//to initialize virtual address from uint64_t
//we use defined mask values in order to set
//unnecessary bits to 0 and then shift it
int init_virt_addr64(virt_addr_t * vaddr, uint64_t vaddr64){
	
	M_REQUIRE_NON_NULL(vaddr);
	
	const uint64_t mask_page_offset = 0xFFF;
	const uint64_t mask_pte_entry = 0x1FF000;
	const uint64_t mask_pmd_entry = 0x3FE00000;
	const uint64_t mask_pud_entry = 0x7FC0000000;
	const uint64_t mask_pgd_entry = 0xFF8000000000;
	
	int shift_amount = 0;
	
	uint16_t page_offset = vaddr64 & mask_page_offset;
	shift_amount += PAGE_OFFSET;
	
	uint16_t pte_entry = (vaddr64 & mask_pte_entry) >> shift_amount;
	shift_amount += PTE_ENTRY;
	
	uint16_t pmd_entry = (vaddr64 & mask_pmd_entry) >> shift_amount;
	shift_amount += PMD_ENTRY;
	
	uint16_t pud_entry = (vaddr64 & mask_pud_entry) >> shift_amount;
	shift_amount += PUD_ENTRY;
	
	uint16_t pgd_entry = (vaddr64 & mask_pgd_entry) >> shift_amount;
	
	init_virt_addr(vaddr, pgd_entry, pud_entry, pmd_entry, pte_entry, page_offset);
                   
	return ERR_NONE; 
}

//this time from virtual address we put fields one by one
//and then shift for correct values to get virtual page number
uint64_t virt_addr_t_to_virtual_page_number(const virt_addr_t * vaddr){
	
	M_REQUIRE_NON_NULL(vaddr);
	
	 uint64_t virtual_page_number = 0;
	 
	 virtual_page_number |= vaddr -> pgd_entry;
	 virtual_page_number <<= PUD_ENTRY;
	 
	 virtual_page_number |= vaddr -> pud_entry;
	 virtual_page_number <<= PMD_ENTRY;
	 
	 virtual_page_number |= vaddr -> pmd_entry;
	 virtual_page_number <<= PTE_ENTRY;
	 
	 virtual_page_number |= vaddr -> pte_entry;
	 
	 return virtual_page_number;
	 
	
}

//since this time we want offset as well
//we simply call virt_addr_t_to_virtual_page_number to handle
//previous fields, at the end we shift and disjunct to add offset
uint64_t virt_addr_t_to_uint64_t(const virt_addr_t * vaddr){
	
	M_REQUIRE_NON_NULL(vaddr);
	
	uint64_t result = virt_addr_t_to_virtual_page_number(vaddr);
	result = result << PAGE_OFFSET;
	result |= vaddr -> page_offset;
	
	return result;
}

//same process as printing virtual address
//printing in suggested format 
int print_physical_address(FILE* where, const phy_addr_t* paddr){

	M_REQUIRE_NON_NULL_CUSTOM_ERR(where, ERR_IO);
	M_REQUIRE_NON_NULL(paddr);
	
	return fprintf(where, "page num=0x%" PRIX32  "; offset=0x%" PRIX16, 
	 paddr -> phy_page_num, paddr -> page_offset);

}

//initializing a physical address from its physical page number and offset 
//with proper shifting 
int init_phy_addr(phy_addr_t* paddr, uint32_t page_begin, uint32_t page_offset){

	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE(PAGE_SIZE > page_offset, ERR_BAD_PARAMETER, "Offset is bigger than page size", page_offset);
	M_REQUIRE(page_begin % PAGE_SIZE == 0 , ERR_BAD_PARAMETER, "page_begin must be a multiple of PAGE_SIZE", page_begin);
	
	paddr -> phy_page_num = page_begin  >> PAGE_OFFSET;
	paddr -> page_offset = page_offset;
	
	return ERR_NONE; 
}
