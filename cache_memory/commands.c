#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include "commands.h"
#include "error.h"
#include "addr_mng.h"
#include "util.h"

int program_init(program_t* program){
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(program->listing);
	
	//program->listing is allocated for initial size(=10)
	program->listing = calloc(PROGRAM_INITIAL_SIZE, sizeof(command_t));
	M_REQUIRE_NON_NULL(program->listing);
	
	//number of initial lines and allocated lines
	program -> nb_lines = 0;
	program -> allocated = PROGRAM_INITIAL_SIZE * sizeof(command_t); 

	return ERR_NONE;
}

//printing program in suggested format
int program_print(FILE* output, const program_t* program){
	
	M_REQUIRE_NON_NULL_CUSTOM_ERR(output, ERR_IO);
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(program->listing);
	
	for(int i = 0; i < program->nb_lines; ++i){
		
		if(program->listing[i].order == READ){
			fprintf(output, "R ");	
		}
		else{
			fprintf(output, "W ");
		}
		if(program->listing[i].type == INSTRUCTION){
			fprintf(output, "I ");
		}
		else{ //We are dealing with DATAs here
			
			if(program->listing[i].data_size == sizeof(word_t)){
				fprintf(output, "DW ");
			}else{
				fprintf(output, "DB ");
			}
		}	
		if(program->listing[i].order == WRITE){
			 if(program->listing[i].data_size == sizeof(word_t)){
				fprintf(output, "0x%08" PRIX32, program->listing[i].write_data);
			}
			else{
				fprintf(output, "0x%02" PRIX32, program->listing[i].write_data);
			}
		}
		fprintf(output, "@0x%016" PRIX64, virt_addr_t_to_uint64_t(&(program->listing[i].vaddr)));		
		printf("\n");
	}	
	return ERR_NONE;
}

int program_read(const char* filename, program_t* program){   
   
	M_REQUIRE_NON_NULL(filename);
	M_REQUIRE_NON_NULL(program);
	
	M_EXIT_IF_ERR(program_init(program), "initializing program");
	
    FILE *fp = fopen(filename, "r");
	M_REQUIRE(fp != NULL, ERR_IO, "File not found", filename);
	
	bool commands_not_finished = true;
	
    while(commands_not_finished && !feof(fp) && !ferror(fp)){
        char order_char = ' ';
        if(fscanf(fp, "%c", &order_char) <= 0){
			commands_not_finished = false;
        }else{
        
		//Local variables to form command
		command_word_t order;
		mem_access_t type;
		size_t data_size = 0;
		word_t write_data = 0;
		virt_addr_t vaddr;
		zero_init_var(vaddr);
		uint64_t vaddr64 = 0;
			
        char mem_access[2];
            
        if(order_char == 'W'){
			order = WRITE;
        }
        else if(order_char == 'R'){
			order = READ;
		}
		else{
			fclose(fp);
            fprintf(stderr, "Can't read order");
			return ERR_IO;
		}     
        if(fscanf(fp, "%s", mem_access) <= 0){
            fclose(fp);
            fprintf(stderr, "Can't read memory access type");
		    return ERR_IO;
         }

		if(mem_access[0] == 'I'){
			type = INSTRUCTION;
			data_size = sizeof(word_t);
		}
		else if(mem_access[0] == 'D'){
			type = DATA;
			
			if(mem_access[1] == 'W'){
				data_size = sizeof(word_t);
			}
			else if(mem_access[1] == 'B'){
				data_size = sizeof(byte_t);
			}
			else{
				fclose(fp);
				fprintf(stderr, "Invalid data size");
				return ERR_IO;
				}
			}
		else{
			fclose(fp);
			fprintf(stderr, "Can't read memory access type");
			return ERR_IO;
		}
								
        if(order_char == 'W' ){
            if(fscanf(fp, "%"SCNx32, &write_data) <= 0){
                 fclose(fp);
				 fprintf(stderr, "Can't read data to write");
				 return ERR_IO;
            }
                    
		}

         char address_char  = ' ';
         while(address_char == ' ' && !feof(fp) && !ferror(fp)){
			if(fscanf(fp, "%c", &address_char) <= 0 ){ 
				fclose(fp);
				fprintf(stderr, "Can't read virtual address");
				return ERR_IO;
							
			}
		}
                                        
		if(address_char != '@'){ 
			fclose(fp);
			fprintf(stderr, "Invalid address beginning, should have start with @");
			return ERR_IO;
		}
             
		if(fscanf(fp, "%"SCNx64, &vaddr64) <= 0){
			fclose(fp);
			fprintf(stderr, "Can't read virtual address");
			return ERR_IO;	
							
		}
		
		char new_line_char  = ' ';
		
		while(new_line_char != '\n' && !feof(fp) && !ferror(fp)){
			if(fscanf(fp, "%c", &new_line_char) <= 0 ){ 
				fclose(fp);
				fprintf(stderr, "Can't skip the line");
				return ERR_IO;
			}
		}

            
		if(init_virt_addr64(&vaddr, vaddr64) != ERR_NONE){
			fclose(fp);
			M_EXIT(ERR_BAD_PARAMETER, "unable to initialize virtual address", vaddr);
		}
	
	
		//creating new command
		command_t new_command = {order, type, data_size, write_data, vaddr};
		
		
		//adding the newly created command to program
		if(program_add_command(program, &new_command) != ERR_NONE){
			fclose(fp);
			M_EXIT(ERR_BAD_PARAMETER, "unable to add command", new_command);	
		}
		
		}	
	}
	
	if(program_shrink(program) != ERR_NONE){
		fclose(fp);
		M_EXIT(ERR_BAD_PARAMETER, "unable to shrink program", program);		
	}
	
	//Closing the file and return
	fclose(fp);
	return ERR_NONE;
}


int program_add_command(program_t* program, const command_t* command){
	
	M_REQUIRE_NON_NULL(command);
	M_REQUIRE_NON_NULL(program);
	
	//doubles times allocation in case program's allocated
	//part is full 
	if((program -> nb_lines * sizeof(command_t)) == program->allocated){	
		//buffer check
		size_t new_allocated = program -> allocated * 2 ; 	
		if(new_allocated > (SIZE_MAX / sizeof(command_t))){
			new_allocated = program -> allocated;
		}
		program -> allocated = new_allocated;
		
		program->listing = realloc(program->listing, program->allocated);
		M_REQUIRE_NON_NULL(program->listing);
	} 
	
	M_REQUIRE(command->order == READ || command->order == WRITE, 
		ERR_BAD_PARAMETER,"Order is not correct", command -> order);
	
	M_REQUIRE(command->type == INSTRUCTION || command->type == DATA, 
		ERR_BAD_PARAMETER,"Type is not correct", command -> type);
		
	M_REQUIRE(command->data_size == sizeof(byte_t) ||command->data_size == sizeof(word_t), 
		ERR_BAD_PARAMETER,"Data size is not correct", command -> data_size);	
	

	if(command -> type == INSTRUCTION ){
		M_REQUIRE(command->data_size == sizeof(word_t),
		ERR_BAD_PARAMETER, "Instruction type must have data size 4 not %d ", command->data_size);
		M_REQUIRE(command -> order == READ, ERR_BAD_PARAMETER, "Instruction type can only be read",command -> order);	
	}
	
	if(command -> data_size == sizeof(byte_t) && command->order == WRITE){
		M_REQUIRE(command -> write_data <= UCHAR_MAX, ERR_BAD_PARAMETER, "write data too large for write size", command -> write_data);
	}

	int rest = command -> vaddr.page_offset  % command -> data_size;
	M_REQUIRE(rest == 0, ERR_BAD_PARAMETER,
	 "Incorrect virtual address with offset= %d, data_size = %d", 
		command -> vaddr.page_offset, command -> data_size );
	
	//CASE ALL PARAMETERS ARE CORRECT
	program -> listing[program -> nb_lines] = *command; 
	++program -> nb_lines;	 
	return ERR_NONE;
}


int program_shrink(program_t* program){
	M_REQUIRE_NON_NULL(program);
	if(program->nb_lines == 0){//no lines in program
		program->listing = calloc(PROGRAM_INITIAL_SIZE, sizeof(command_t));	
	}
	else{	
		//reallocating to a smaller memory for unnecessary allocations 
		program->allocated = program->nb_lines * sizeof(command_t);
		program->listing = realloc(program->listing, program->allocated);	
	}
	
	M_REQUIRE_NON_NULL_CUSTOM_ERR(program->listing, ERR_MEM);
	return ERR_NONE;
}

int program_free(program_t* program){
	if(program != NULL){
		program->nb_lines = 0;
		program->allocated = 0;
				
		free(program-> listing);
		program-> listing = NULL;
		
		return ERR_NONE;
	}
	//in case it tries to free for a second time 
	return ERR_BAD_PARAMETER;
}
