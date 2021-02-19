/**
 * @memory.c
 * @brief memory management functions (dump, init from file, etc.)
 *
 * @author Jean-Cédric Chappelier
 * @date 2018-19
 */

#if defined _WIN32  || defined _WIN64
#define __USE_MINGW_ANSI_STDIO 1
#endif

#define STR(X) #X
#define INPUT_LIM(X) "%" STR(X) "s"
//macro in order to limit fscanf to MAX_STR_LENGTH 
//without using constants

#include "memory.h"
#include "page_walk.h"
#include "addr_mng.h"
#include "util.h" // for SIZE_T_FMT
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for memset()
#include <inttypes.h> // for SCNx macros
#include <assert.h>
#include "addr.h"
#include <stdbool.h>

//opening file and doing checks
//getting the capacity as suggested in instructions
//then allocating enough memory and reading from binary
//file into the memory 
int mem_init_from_dumpfile(const char* filename, void** memory, size_t* mem_capacity_in_bytes){

	M_REQUIRE_NON_NULL_CUSTOM_ERR(filename, ERR_IO);
	M_REQUIRE_NON_NULL_CUSTOM_ERR(memory, ERR_MEM);
	M_REQUIRE_NON_NULL(mem_capacity_in_bytes);


	FILE* file = NULL;
	file = fopen(filename, "rb"); 	
	M_REQUIRE_NON_NULL_CUSTOM_ERR(file, ERR_IO);

	// va tout au bout du fichier
	fseek(file, 0L, SEEK_END);

	// indique la position, et donc la taille (en octets)
	*mem_capacity_in_bytes = (size_t) ftell(file);

	M_REQUIRE((*mem_capacity_in_bytes) % PAGE_SIZE == 0, ERR_BAD_PARAMETER,
		"mem_capacity_in_bytes must be a multiple of PAGE_SIZE", mem_capacity_in_bytes);
	
	// revient au début du fichier (pour le lire par la suite)
	rewind(file);
	
	
	*memory = calloc(*mem_capacity_in_bytes, sizeof(char)); 	
	if(*memory == NULL){
		fclose(file);
		return ERR_MEM;
	}
	
	//checking if we read correct number of bytes
	if(fread(*memory, sizeof(char), *mem_capacity_in_bytes, file) != *mem_capacity_in_bytes){
		fclose(file);
		return ERR_IO;
	}

	fclose(file);

	return ERR_NONE;
	
}

//helper function in order to use 
//each time we need to transfer data from binary file into 
//memory as translation pages 
//casting *memory in order to be able to index to memory
int page_file_read(void* memory, uint32_t phyPageNumber, const char* filename){
		M_REQUIRE_NON_NULL_CUSTOM_ERR(filename, ERR_IO);
		M_REQUIRE_NON_NULL_CUSTOM_ERR(memory, ERR_MEM);

		FILE* ptrFile = NULL;	
		ptrFile = fopen(filename, "rb"); 	
		M_REQUIRE_NON_NULL(ptrFile);
	
		uint32_t* newMem = memory;
		
		//dividing PhyPageNumber to 4, same logic as read_page_entry in page_walk.c
		if(fread(newMem + phyPageNumber/sizeof(uint32_t) , sizeof(char) , PAGE_SIZE , ptrFile) != PAGE_SIZE){
			fclose(ptrFile);
			return ERR_IO;
		}
		
		fclose(ptrFile);
		
		return ERR_NONE;
	}


int mem_init_from_description(const char* master_filename, void** memory, size_t* mem_capacity_in_bytes){
	
	M_REQUIRE_NON_NULL_CUSTOM_ERR(master_filename, ERR_IO);
	M_REQUIRE_NON_NULL_CUSTOM_ERR(memory, ERR_MEM);
	M_REQUIRE_NON_NULL_CUSTOM_ERR(mem_capacity_in_bytes, ERR_MEM);

	error_code error = ERR_NONE;
	
	
	//master file from which we extract description
	FILE* ptrMasterFile = NULL;
	ptrMasterFile = fopen(master_filename, "r"); 
	M_REQUIRE_NON_NULL_CUSTOM_ERR(ptrMasterFile, ERR_IO);
	

	char pgd_page_filename[MAX_STR_LENGTH];
	size_t nbr_pages = 0;
	
	//getting memory capacity
	//file format check and closing if necessary
	if(fscanf(ptrMasterFile, "%zu", mem_capacity_in_bytes) <= 0){
	 fclose(ptrMasterFile);
	 fprintf(stderr, "Wrong file format");
	 return ERR_IO; 
	}
	
	M_REQUIRE((*mem_capacity_in_bytes) % PAGE_SIZE == 0, ERR_BAD_PARAMETER,
	 "mem_capacity_in_bytes must be a multiple of PAGE_SIZE", mem_capacity_in_bytes);
	
	//allocating enough memory and checking validity
	*memory = calloc((size_t)*mem_capacity_in_bytes, sizeof(char));
	if(*memory == NULL){
		fclose(ptrMasterFile);
		return ERR_MEM;
	}
	
	//acquiring pgd filename and checking validity
	//INPUT_LIM macro uses %ns limiting
	int bytes_read = fscanf(ptrMasterFile, INPUT_LIM(MAX_STR_LENGTH), pgd_page_filename);
		
	if(bytes_read <= 0 || bytes_read > MAX_STR_LENGTH){
		fclose(ptrMasterFile);
		free(*memory);
		*memory = NULL;
		fprintf(stderr, "Wrong file format");
		return ERR_IO;
	}
	
	//using helper function to stock its content at the beginning of memory
	//error check and memory finalisation in case of error
	error = page_file_read(*memory, 0 , pgd_page_filename);	
	
	if(error != ERR_NONE){
		free(*memory);
		*memory = NULL;
		fclose(ptrMasterFile);
		return error;
	}
	
	//acquiring number of translation pages as we will loop through
	if(fscanf(ptrMasterFile, "%zu", &nbr_pages) < 0){
		fclose(ptrMasterFile);
		free(*memory);
		*memory = NULL;
		fprintf(stderr, "Wrong file format");
		return ERR_IO;
	}	

	for(int i = 0; i < nbr_pages ; ++i){
		
		uint32_t addr = 0;
		char page_file_name[MAX_STR_LENGTH];
	
		//acquiring address and page file name in order to use with helper function
		if(fscanf(ptrMasterFile, "%"SCNx32, &addr) <= 0){
			fclose(ptrMasterFile);
			free(*memory);
			*memory = NULL;
			fprintf(stderr, "Wrong file format");
			return ERR_IO;
		}
			
		if(fscanf(ptrMasterFile, INPUT_LIM(MAX_STR_LENGTH), page_file_name) <= 0 ){ 
			fclose(ptrMasterFile);
			free(*memory);
			*memory = NULL;			
			fprintf(stderr, "Wrong file format");
			return ERR_IO;
		}
		
		error = page_file_read(*memory, addr , page_file_name);
		if(error != ERR_NONE){
			free(*memory);
			*memory = NULL;	
			fclose(ptrMasterFile);
			return error;
		}

	}
	
	bool fileNotFinished = true;
	
	while(fileNotFinished){
		
		uint64_t addr = 0;
		char file_name[MAX_STR_LENGTH];
		
		//this if-else bloc wasn't intended as an error check
		//but signals the end of file thus bloc is used 
		//as a part of the error-free process
		//that's why we didn't free memory 
		//closing of file will occur after going out of while loop 
		//as fileNotFinished is set to false
		if(fscanf(ptrMasterFile, "%"SCNx64, &addr) <= 0){ 
			fileNotFinished = false;
			
		}else if(fscanf(ptrMasterFile, INPUT_LIM(MAX_STR_LENGTH), file_name) <= 0){ 
			fileNotFinished = false;
		}
		
		if(fileNotFinished){

			//this part is to first get the proper physical addresses
			//from virtual addresses which are at the end of description file
			//we initialize these physical and virtual addresses in order to use 
			//in page_walk
			
			phy_addr_t phyAddr;
			
			error = init_phy_addr(&phyAddr, 0, 0);
			if(error != ERR_NONE){
				free(*memory);
				*memory = NULL;	
				fclose(ptrMasterFile);
				return error;
			}
			
			virt_addr_t vaddr;
			error = init_virt_addr64(&vaddr,addr);
			if(error != ERR_NONE){
				free(*memory);
				*memory = NULL;	
				fclose(ptrMasterFile);
				return error;
			}
			
			error = page_walk(*memory, &vaddr, &phyAddr);
			if(error != ERR_NONE){
				free(*memory);
				*memory = NULL;	
				fclose(ptrMasterFile);
				return error;
			}
			
			//then we transform phy_addr_t into unint32_t 
			//in order to use it as index with helper
			//function page_file_read
			uint32_t physicalAddress = (phyAddr.phy_page_num) << PAGE_OFFSET;			
			
			error = page_file_read(*memory, physicalAddress, file_name);
			if(error != ERR_NONE){
				free(*memory);
				*memory = NULL;	
				fclose(ptrMasterFile);
				return error;					
			}
		}		
	}
	
	
	fclose(ptrMasterFile);
	
	return ERR_NONE;
}

// ======================================================================
/**
 * @brief Tool function to print an address.
 *
 * @param show_addr the format how to display addresses; see addr_fmt_t type in memory.h
 * @param reference the reference address; i.e. the top of the main memory
 * @param addr the address to be displayed
 * @param sep a separator to print after the address (and its colon, printed anyway)
 *
 */
static void address_print(addr_fmt_t show_addr, const void* reference,
                          const void* addr, const char* sep)
{
    switch (show_addr) {
    case POINTER:
        (void)printf("%p", addr);
        break;
    case OFFSET:
        (void)printf("%zX", (const char*)addr - (const char*)reference);
        break;
    case OFFSET_U:
        (void)printf(SIZE_T_FMT, (const char*)addr - (const char*)reference);
        break;
    default:
        // do nothing
        return;
    }
    (void)printf(":%s", sep);
}

// ======================================================================
/**
 * @brief Tool function to print the content of a memory area
 *
 * @param reference the reference address; i.e. the top of the main memory
 * @param from first address to print
 * @param to first address NOT to print; if less that `from`, nothing is printed;
 * @param show_addr the format how to display addresses; see addr_fmt_t type in memory.h
 * @param line_size how many memory byted to print per stdout line
 * @param sep a separator to print after the address and between bytes
 *
 */
static void mem_dump_with_options(const void* reference, const void* from, const void* to,
                                  addr_fmt_t show_addr, size_t line_size, const char* sep)
{
    assert(line_size != 0);
    size_t nb_to_print = line_size;
    for (const uint8_t* addr = from; addr < (const uint8_t*) to; ++addr) {
        if (nb_to_print == line_size) {
            address_print(show_addr, reference, addr, sep);
        }
        (void)printf("%02"PRIX8"%s", *addr, sep);
        if (--nb_to_print == 0) {
            nb_to_print = line_size;
            putchar('\n');
        }
    }
    if (nb_to_print != line_size) putchar('\n');
}

// ======================================================================
// See memory.h for description
int vmem_page_dump_with_options(const void *mem_space, const virt_addr_t* from,
                                addr_fmt_t show_addr, size_t line_size, const char* sep)
{
#ifdef DEBUG
    debug_print("mem_space=%p\n", mem_space);
    (void)fprintf(stderr, __FILE__ ":%d:%s(): virt. addr.=", __LINE__, __func__);
    print_virtual_address(stderr, from);
    (void)fputc('\n', stderr);
#endif
    phy_addr_t paddr;
    zero_init_var(paddr);

    M_EXIT_IF_ERR(page_walk(mem_space, from, &paddr),
                  "calling page_walk() from vmem_page_dump_with_options()");
#ifdef DEBUG
    (void)fprintf(stderr, __FILE__ ":%d:%s(): phys. addr.=", __LINE__, __func__);
    print_physical_address(stderr, &paddr);
    (void)fputc('\n', stderr);
#endif

    const uint32_t paddr_offset = ((uint32_t) paddr.phy_page_num << PAGE_OFFSET);
    const char * const page_start = (const char *)mem_space + paddr_offset;
    const char * const start = page_start + paddr.page_offset;
    const char * const end_line = start + (line_size - paddr.page_offset % line_size);
    const char * const end   = page_start + PAGE_SIZE;
    debug_print("start=%p (offset=%zX)\n", (const void*) start, start - (const char *)mem_space);
    debug_print("end  =%p (offset=%zX)\n", (const void*) end, end   - (const char *)mem_space) ;
    mem_dump_with_options(mem_space, page_start, start, show_addr, line_size, sep);
    const size_t indent = paddr.page_offset % line_size;
    if (indent == 0) putchar('\n');
    address_print(show_addr, mem_space, start, sep);
    for (size_t i = 1; i <= indent; ++i) printf("  %s", sep);
    mem_dump_with_options(mem_space, start, end_line, NONE, line_size, sep);
    mem_dump_with_options(mem_space, end_line, end, show_addr, line_size, sep);
    return ERR_NONE;
}
