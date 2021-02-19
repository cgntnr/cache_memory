#include <stdio.h> // for fprintf()
#include <stdint.h> // for uint32_t
#include <inttypes.h> // for PRIx macros
#include <stdlib.h>
#include "list.h"
#include "error.h"

int is_empty_list(const list_t* this){		
	M_REQUIRE_NON_NULL(this);	
	return this -> front == NULL && this -> back == NULL ? 1 : 0;
}

void init_list(list_t* this){
	if(this == NULL)
		return;

	this -> front = NULL;
	this -> back = NULL;	
}

void clear_list(list_t* this){

	if(this != NULL && !is_empty_list(this)){
		
		node_t* next_node = NULL;
		node_t* temp = NULL;
		
		for(temp = this->front; temp != NULL; temp = next_node){
			next_node = temp->next;
			free(temp);
			temp = NULL;	
		}
			init_list(this);	
	}
}

node_t* push_back(list_t* this, const list_content_t* value){
	
	node_t* new_node_ptr = malloc(sizeof(node_t));
	if(this == NULL || new_node_ptr == NULL || value == NULL){
		return NULL;
	}
	new_node_ptr -> value = *value;
	new_node_ptr -> previous = NULL;
	new_node_ptr -> next = NULL;
	
	if(is_empty_list(this)){
		this -> front = new_node_ptr;	
	}
	else{
		new_node_ptr -> previous = this -> back;
		this -> back -> next = new_node_ptr;
	}
	this -> back = new_node_ptr;
	return new_node_ptr;
}


node_t* push_front(list_t* this, const list_content_t* value){
	node_t* new_node_ptr = malloc(sizeof(node_t));
	if(this == NULL || new_node_ptr == NULL || value == NULL){
		return NULL;
	}
	new_node_ptr -> value = *value;
	new_node_ptr -> previous = NULL;
	new_node_ptr -> next = NULL;
	if(is_empty_list(this)){
		this -> back = new_node_ptr;
	}
	else{
		new_node_ptr -> next = this -> front;
		this -> front -> previous = new_node_ptr;

	}
	this -> front = new_node_ptr;
	return new_node_ptr;
}

void pop_back(list_t* this){
	//CASE NO ERROR AND LIST IS NOT EMPTY
	if(is_empty_list(this) == 0 ){
		//CASE LIST HAS ONLY ONE ELEMENT
		if(this -> front == this -> back){
			free(this -> front);
			init_list(this);
		}
		else{
			this -> back = this -> back -> previous;
			free(this -> back -> next);
			this -> back -> next = NULL;
		}
	}	
}

void pop_front(list_t* this){
	//CASE NO ERROR AND LIST IS NOT EMPTY
	if(is_empty_list(this) == 0 ){
		//CASE LIST HAS ONLY ONE ELEMENT
		if(this -> front == this -> back){
			free(this -> front);
			init_list(this);
		}
		else{
			this -> front = this -> front -> next;
			free(this -> front -> previous);
			this -> front -> previous = NULL;
		}
	}	
}
/*
void move_back(list_t* this, node_t* n){

	//case n points to back already, there is nothing to be done
	if(this != NULL && n != NULL && n != this -> back){
		list_content_t const value = n -> value;
		//REMOVING THE ELEMENT FROM THE LIST
		if(n == this -> front){
			pop_front(this);
		}
		else{
			n -> previous -> next = n -> next;
			n -> next -> previous = n -> previous;
			free(n);
		}
		//PUSHING BACK TO LIST		
		push_back(this, &value);		
		
	}
}
*/
void move_back(list_t* this, node_t* n){
	//case n points to back already, there is nothing to be done
	if(this != NULL && n != NULL && n != this -> back){
		if(n == this -> front){
			this -> front = n -> next;
			this -> front -> previous = NULL;
		}
		else{
			n -> previous -> next = n -> next;
			n -> next -> previous = n -> previous;
		}
		n -> next = NULL;
		n -> previous = this -> back;
		this -> back -> next = n;
		this -> back = n;	
	}
}



//#define print_node(F, N) fprintf(F, "%"PRIu32, N)
int print_list(FILE* stream, const list_t* this){
	M_REQUIRE_NON_NULL(stream);
	M_REQUIRE_NON_NULL(this);

	int total = 0;
	total += fprintf(stream,"(");
	for_all_nodes(X, this){
		if(X != this -> front){
			total += fprintf(stream, " ");
		}
		total += print_node(stream, X -> value);
		if(X != this -> back){
			total += fprintf(stream, ",");
		}
	}
	total += fprintf(stream, ")");
	return total;
}

int print_reverse_list(FILE* stream, const list_t* this){
	M_REQUIRE_NON_NULL(stream);
	M_REQUIRE_NON_NULL(this);
	
	int total = 0;
	total += fprintf(stream,"(");
	for_all_nodes_reverse(X, this){
		if(X != this -> back){
			total += fprintf(stream, " ");
		}
		total += print_node(stream, X -> value);
		if(X != this -> front){
			total += fprintf(stream, ",");
		}
	}
	total += fprintf(stream, ")");
	return total;
}
