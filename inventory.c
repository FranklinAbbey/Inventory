/*
 * File: inventory.c
 *
 * Description: An inventory system that manages parts and assemblies, which
 *              are created using parts and sub-assemblies. Customer orders 
 *              can be made, and the inventory can be manually updated.
 * 
 * Table of Contents:
 *
 *      Section:           Line:
 *      ------------------ -----
 *      GLOBAL DEFINITIONS    39
 *      VALIDATION            48
 *      STOCK/RESTOCK        144
 *      LOOKUPS              266
 *      ADD FUNCTIONS        347
 *      TO ARRAY             527
 *      COMPARE              614
 *      MAKE/GET             673 
 *      PRINT                779
 *      PROCESS REQUESTS     890
 *      FREES               1204
 *      MAIN                1260
 *
 * @author: Frank Abbey (fra1489)
 * @version: 4/23/2020
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inventory.h"
#include "trimit.h"

#define MAX_LENGTH 351

/* - - - GLOBAL DEFINITIONS - - -*/

// inventory to be shared across the life of the program
inventory_t* inventory;
//required to free one-time-use items_needed_t* lists
static void free_items_needed(items_needed_t* items);
//used to 'clear' inventory 
void free_inventory(inventory_t* invp);

/* - - - VALIDATION - - -*/

/*
 * Determine if an id is a valid part id. This means the id
 * must begin with 'P' and be the proper length
 * 
 * @param char* id - the id to be examined
 * 
 * @return int - 1: valid, 0: invalid
 */
static int valid_part_id(char* id) {

    if(*(id) != 'P') {
        fprintf(stderr, "!!! %s: part ID must start with 'P'\n", id);
        return 0;
    }
    else if(strlen(id) > ID_MAX) {
        fprintf(stderr, "!!! %s: part ID too long\n", id);
        return 0;
    }
    else {
        return 1;
    }  
}

/*
 * Determine if an id is a valid assembly id. This means the id
 * must begin with 'A' and be the proper length
 * 
 * @param char* id - the id to be examined
 * 
 * @return int - 1: valid, 0: invalid
 */
static int valid_assembly_id(char* id) {

    if(*(id) != 'A') {
        fprintf(stderr, "!!! %s: assembly ID must start with 'A'\n", id);
        return 0;
    }   
    else if(strlen(id) > ID_MAX) {
        fprintf(stderr, "!!! %s: assembly ID too long\n", id);
        return 0;
    }
    else {
        return 1;
    }   
}

/*
 * Determine if an item is valid. This means the id must be valid,
 * and the part or assembly must exist in the inventory.
 *
 * @param inventory_t* invp - the inventory to be searched
 * @param char* id - the id to be examined
 *
 * @return int - 1: valid, 0: invalid
 */
static int valid_item(inventory_t* invp, char* id) {

    if(*(id) == 'P') {
        if(valid_part_id(id)) {
            if(lookup_part(invp -> part_list, id) != NULL) {
                return 1;
            }
            else {
                fprintf(stderr,
                "!!! %s: part/assembly ID is not in the inventory\n", id);
                return 0;
            }
        }
        else {
            return 0;
        }
    }
    else if(*(id) == 'A') {
        if(valid_assembly_id(id)) {
            if(lookup_assembly(invp -> assembly_list, id) != NULL) {
                return 1;
            }
            else {
                fprintf(stderr,
                "!!! %s: part/assembly ID is not in the inventory\n", id);
                return 0;
            }
        }
        else {
            return 0;
        }
    }
    else {
        fprintf(stderr,
        "!!! %s: part/assembly ID is not in the inventory\n", id);
        return 0;
    }
}

/* - - - STOCK/RESTOCK - - -*/

/*
 * Manufacture a given amount of an assembly
 *
 * @param inventory_t* invp - the inventory containing the assembly
 * @param char* id - the ID of the assembly
 * @param items_needed_t* parts - the list of items required to make this 
 *                                assembly
 */
static void stock(inventory_t* invp, char* id, int n, items_needed_t* parts) {
    //determine if the quantity to stock is valid   
    if(n <= 0) {
        fprintf(stderr, "!!! %d: illegal quantity for ID %s\n",
        n, id);
    }
    else {
        struct assembly* assembly = lookup_assembly(invp -> assembly_list, id);
        //the assembly was not found
        if(assembly == NULL) {
            fprintf(stderr,
            "!!! %s: assembly ID is not in the inventory\n",
            id);
        }
        else {
            int amount_needed = 0;
            //stocking the assembly by 'n' would exceed the capacity
            if(assembly -> on_hand + n > assembly -> capacity) {
                amount_needed = (assembly -> capacity) - (assembly -> on_hand);
                assembly -> on_hand = assembly -> capacity;
            }
            //stocking the assembly by 'n' would NOT exceed capacity
            else {
                assembly -> on_hand += n;
                amount_needed = n;
            }
            //there is at least one unit needing to be made
            if(amount_needed)
                printf(">>> make %d units of assembly %s\n", amount_needed, id);
            
            if(amount_needed > 0) {
                //for every item in this assembly's 'items_needed" list
                int i, quantity;
                int num_items = (assembly -> items) -> item_count;
                struct item* item = (assembly -> items) -> item_list;
                for(i = 0; i < num_items; i++) {

                    quantity = (item -> quantity) * amount_needed;
                    //if this item is a part
                    if(item -> id[0] == 'P') {
                        add_item(parts, item -> id, quantity);
                    }
                    //if this item is an assembly 
                    if(item -> id[0] == 'A') {
                        //get needed amount of 'sub' assemblies
                        get(invp, item -> id, quantity, parts);
                    }

                    item = item -> next;

                }
            }
                
        }

    }

}

/*
 * Manufacture assemblies if they are below the restock threshold 
 * (if the on hand amount is less than half of the capacity). 
 *
 * @param inventory_t* invp - the inventory containing the assembly/assemblies
 * @param char* id - the ID of the assembly. If this value is NULL, restock 
 *                   every item in the inventory 
 * @param items_needed_t* parts - the list of items required for the request
 */
static void restock(inventory_t* invp, char* id, items_needed_t* parts) {
    
    struct assembly* assembly;
    int amount;

    //request to restock entire inventory
    if(id == NULL) {
    
        assembly = invp -> assembly_list;
        
        while(assembly != NULL) {
            //check if 'on_hand' value meets the threshold 
            if((assembly -> on_hand) < ((double)(assembly -> capacity) / 2.0)) {
                amount = assembly -> capacity - assembly -> on_hand;
                printf(">>> restocking assembly %s with %d items\n",
                assembly -> id, amount);
                stock(invp, assembly -> id, amount, parts);
            }
            assembly = assembly -> next;
        }

    }
    //request to restock a specific assembly
    else {
        assembly = lookup_assembly(invp -> assembly_list, id);

        if(assembly == NULL) {
            fprintf(stderr,
            "!!! %s: assembly ID is not in the inventory\n",
            id);
        }
        else {
            if((assembly -> on_hand) < ((double)(assembly -> capacity) / 2.0)) {
                amount = (assembly -> capacity) - (assembly -> on_hand);
                printf(">>> restocking assembly %s with %d items\n", 
                id, amount);
                stock(invp, id, amount, parts);
            }
        }
    
    }

}

/* - - - LOOKUPS - - -*/

/*
 * Search for a part in a given part list
 * 
 * @param part_t* pp - the part list to be searched
 * @param char* id - the ID to be searched for 
 * 
 * @return part_t* part - the address of the part if it is found, 
 *                        'NULL' if not found
 */
part_t* lookup_part(part_t* pp, char* id) {
    
    struct part* part = pp;
    
    while(part != NULL) {
        //if the matching ID is found
        if(strncmp(part -> id, id, strlen(id)) == 0
            && strlen(id) == strlen(part -> id)) {
            
            return part;  
        }
        part = part -> next;
    }

    return NULL;

}

/*
 * Search for an assembly in a given assembly list
 *
 * @param assembly_t* ap - the assembly list to be searched
 * @param char* id - the ID to be searched for
 *
 * @return assembly_t* part - the address of the assembly if it is found,
 *                            'NULL' if not found
 */
assembly_t* lookup_assembly(assembly_t* ap, char* id) {
    
    struct assembly* assembly = ap;

    while(assembly != NULL) {
        //if the matching ID is found
        if(strncmp(assembly -> id, id, strlen(id)) == 0 
            && strlen(id) == strlen(assembly -> id)) {
            
            return assembly;  
        }
        assembly = assembly -> next;
    }

    return NULL;

}

/*
 * Search for an item in a given item list
 *
 * @param item_t* ip - the item list to be searched
 * @param char* id - the ID to be searched for
 *
 * @return item_t* item - the address of the item if it is found,
 *                        'NULL' if not found
 */
item_t* lookup_item(item_t* ip, char* id) {
    
    struct item* item = ip;

    while(item != NULL) {
        //if the matching ID is found
        if(strncmp(item -> id, id, strlen(id)) == 0 &&
        strlen(id) == strlen(item -> id)) {
            return item;
        }
        item = item -> next;
    }

    return NULL;
}

/* - - - ADD FUNCTIONS - - -*/

/*
 * Add a new part identifier to the part inventory
 *
 * @param inventory_t* invp - the inventory holding parts
 * @param char* id - the ID of the part to be created and 
 *                   added
 */
void add_part(inventory_t* invp, char* id) {
    //create a new part struct 
    struct part* new_part = calloc(1, sizeof(struct part));
    
    //assign the values in the id array to the given pointer
    unsigned int i;
    for(i = 0; i < strlen(id); i++) {
        (new_part -> id)[i] = *(id + i);
    }
    
    //check if part id already exists
    if(lookup_part(invp -> part_list, id) != NULL) {
        fprintf(stderr, "!!! Part: duplicate part ID\n");
        free(new_part);
    }
    
    //otherwise, add the part
    else {
        //retreive the beginning of the list
        struct part* current_part = invp -> part_list;
        //add the part to the end of the parts list
        if(current_part != NULL) {
            while(current_part != NULL) {

                //if there is another part after this one in the list
                if(current_part -> next != NULL)
                    current_part = current_part -> next;
                //otherwise, insert the part into the list
                else { 
                    current_part -> next = new_part;
                    current_part = new_part -> next;
                    invp -> part_count++;
                }
        
            }
        }
        //current_part was NULL, meaning the list was empty
        else {
            invp -> part_list = new_part;
            new_part -> next = NULL;
            invp -> part_count++;
        }
    }

}

/*
 * Add a new assembly to the inventory
 *
 * @param inventory_t* invp - the inventory to be added to
 * @param char* id - the ID of the assembly to be created and
 *                   added
 * @param int capacity - the total capacity that is to be set 
 *                       for this assembly
 * @param items_needed_t* items - the items (parts/assemblies) 
 *                                required to create this assembly
 */
void add_assembly(inventory_t* invp, char* id, int capacity,
                  items_needed_t* items) {

        //validity of assembly ID
        if(!valid_assembly_id(id)) {
            return;
        }
        //negative capacity
        else if(capacity < 0) {
            fprintf(stderr, "!!! %d: illegal capacity for ID %s\n", 
            capacity, id);
        }
        //a return value of 'NULL" indicates the assembly is not already in
        //the inventory
        else if(lookup_assembly(invp -> assembly_list, id) != NULL) {
            fprintf(stderr, "!!! %s: duplicate assembly ID\n", id);
        }
        //after all error-checks pass, add the assembly
        else {
            //create a new assembly struct 
            struct assembly* new_assembly = calloc(1, sizeof(struct assembly));

            //assign the values in the id array to the given pointer
            unsigned int i;
            for(i = 0; i < strlen(id); i++) {
                (new_assembly -> id)[i] = *(id + i);
            }
           
            new_assembly -> capacity = capacity;
            new_assembly -> on_hand = 0;
            new_assembly -> items = items;
            new_assembly -> next = NULL;

            //add the new assembly to the beginning of the assembly list
            if(invp -> assembly_list != NULL) {
                new_assembly -> next = invp -> assembly_list;
                invp -> assembly_list = new_assembly;
                invp -> assembly_count++;
            }
            //current_assembly was NULL, meaning the list was empty
            else {
                invp -> assembly_list = new_assembly;
                new_assembly -> next = NULL;
                invp -> assembly_count++;
            }

        }
    
}

/*
 * Add an item (part or assembly) to an items_needed_t list
 *
 * @param items_needed_t* items - the list to add the item to
 * @param char* id - the ID of the item
 * @param int quantity - the amount of the item that is/was required
 */
void add_item(items_needed_t* items, char* id, int quantity) {

    int valid = 0;
    //do not add the item if the id is not valid as a part or assembly
    if(*(id) == 'P') {
        if(valid_part_id(id)) {
            if(lookup_part(inventory -> part_list, id) != NULL) {
                valid = 1;
            }
            else {
                fprintf(stderr, "!!! %s: part/assembly ID is not in the inventory\n",
                id);    
            }
        }
    }
    else if(*(id) == 'A') {
        if(valid_assembly_id(id)) {
            if(lookup_assembly(inventory -> assembly_list, id) != NULL) {
                valid = 1;
            }
            else {
                fprintf(stderr, "!!! %s: part/assembly ID is not in the inventory\n",
                id);
            }
        }
    }

    if(valid) {

        //initialize a new item
        struct item* item = calloc(1, sizeof(struct item));
    
        unsigned int i;
        for(i = 0; i < strlen(id); i++) {
            item -> id[i] = *(id + i);
        } 

        item -> quantity = quantity;
        item -> next = NULL;
        
        struct item* current_item = lookup_item(items -> item_list, id);
        //increment quantity if the item is alrady in the list 
        if(current_item != NULL) {
            current_item -> quantity += quantity;
            //delete the item that now does not need to be added
            free(item);
        }
        //otherwise, add it to the list
        else {
            item -> next = items -> item_list;
            items -> item_list = item;
            items -> item_count++;
        }
    }

}

/* - - - TO ARRAY - - -*/

/*
 * Convert a part_t* part list from a linked list to an array
 * 
 * @param int count - the amount of parts in this list
 * @param part_t* part_list - the parts list to be converted
 *
 * @return part_t** part_array - the array of parts 
 */
part_t** to_part_array(int count, part_t* part_list) {
    
    //dynamically allocate an array of void pointers
    part_t** part_array = calloc(count, sizeof(part_t*));
    struct part* part = part_list;

    //fill the array with values from the linked list
    int i;
    for(i = 0; i < count; i++) {
        if(part != NULL) {
            part_array[i] = (void*)part;
            part = part -> next;
        }

    }
    
    return part_array;

}

/*
 * Convert an assembly_t* assembly list from a linked list 
 * to an array
 *
 * @param int count - the amount of assemblies in this list
 * @param assembly_t* assembly_list - the assembly list to be converted
 *
 * @return assembly_t** assembly_array - the array of assemblies
 */
assembly_t** to_assembly_array(int count, assembly_t* assembly_list) {

    //dynamically allocate an array of void pointers
    assembly_t** assembly_array = calloc(count, sizeof(assembly_t*));
    struct assembly* assembly = assembly_list;

    //fill the array with values from the linked list
    int i;
    for(i = 0; i < count; i++) {
        if(assembly != NULL) {
            assembly_array[i] = (void*)assembly;
            assembly = assembly -> next;
        }

    }

    return assembly_array;

}

/*
 * Convert an item_t* item list from a linked list to an array
 *
 * @param int count - the amount of items in this list
 * @param item_t* item_list - the item list to be converted
 *
 * @return item_t** item_array - the array of items
 */
item_t ** to_item_array(int count, item_t * item_list) {
    
    //dynamically allocate an array of void pointers
    item_t** item_array = calloc(count, sizeof(item_t*));
    struct item* item = item_list;

    //fill the array with values from the linked list
    int i;
    for(i = 0; i < count; i++) {
        if(item != NULL) {
            item_array[i] = (void*)item;
            item = item -> next;
        }

    }

    return item_array;

}

/* - - - COMPARE - - -*/

/*
 * Compare two parts based on their IDs
 * 
 * @param const void* p1 - a void pointer representing a part
 * @param const void* p2 - a void pointer representing a part
 * 
 * @return int - 0: the IDs are equal
 *              <0: p1's ID is less than p2's ID
 *              >0: p2's ID is less than p1's ID   
 */
int part_compare(const void* p1, const void* p2) {
    
    const struct part* part1 = *(part_t**)p1;
    const struct part* part2 = *(part_t**)p2;

    return strncmp(part1 -> id, part2 -> id, strlen(part1 -> id));
    
}

/*
 * Compare two assemblies based on their IDs
 *
 * @param const void* a1 - a void pointer representing an assembly
 * @param const void* a2 - a void pointer representing an assembly
 *
 * @return int - 0: the IDs are equal
 *              <0: a1's ID is less than a2's ID
 *              >0: a2's ID is less than a1's ID
 */
int assembly_compare(const void* a1, const void* a2) {

    const struct assembly* assembly1 = *(assembly_t**)a1;
    const struct assembly* assembly2 = *(assembly_t**)a2;

    return strncmp(assembly1 -> id, assembly2 -> id, strlen(assembly1 -> id));

}

/*
 * Compare two items based on their IDs
 *
 * @param const void* i1 - a void pointer representing an item
 * @param const void* i2 - a void pointer representing an item
 *
 * @return int - 0: the IDs are equal
 *              <0: i1's ID is less than i2's ID
 *              >0: i2's ID is less than i1's ID
 */
int item_compare(const void* i1, const void* i2) {

    const struct item* item1 = *(item_t**)i1;
    const struct item* item2 = *(item_t**)i2;

    return strncmp(item1 -> id, item2 -> id, strlen(item1 -> id));

}

/* - - - MAKE/GET - - -*/

/*
 * Make a given amount of assemblies from an inventory
 *
 * @param inventory_t* invp - the inventory holding the assemblies
 * @param char* id - the ID of the assembly to be made
 * @param int n - the number of assemblies needed
 * @param items_needed* parts - the parts/sub-assemblies required to 
 *                              create this assembly
 */
void make(inventory_t* invp, char* id, int n, items_needed_t* parts) {
    //check for valid amount 
    if(n <= 0) {
        fprintf(stderr, 
        "!!! %d: illegal order quantity for ID %s -- order canceled\n", n, id);
    }
    else {
        struct assembly* assembly = lookup_assembly(invp -> assembly_list, id);
        //assembly was not found
        if(assembly == NULL) {
            fprintf(stderr,
            "!!! %s: assembly ID is not in the inventory -- order canceled\n",
            id);
        }
        else {
            
            int amount_to_make = 0;
            if(n >= assembly -> on_hand) {
                amount_to_make = n - (assembly -> on_hand);
                printf(">>> make %d units of assembly %s\n", amount_to_make, id);
                assembly -> on_hand = 0;
            }
            else {
                assembly -> on_hand = assembly -> on_hand - n;   
            }
            if(amount_to_make > 0) { 
                //for every item in this assembly's 'items_needed" list
                int i, quantity;
                int num_items = (assembly -> items) -> item_count;
                struct item* item = (assembly -> items) -> item_list;
                for(i = 0; i < num_items; i++) {

                    quantity = (item -> quantity) * amount_to_make;
                    //if this item is a part
                    if(item -> id[0] == 'P') {
                        add_item(parts, item -> id, quantity);
                    }
                    //if this item is an assembly 
                    if(item -> id[0] == 'A') {
                        //get needed amount of 'sub' assemblies
                        get(invp, item -> id, quantity, parts); 
                    }
                
                    item = item -> next;

                }
            }
            
        }

    }

}

/*
 * Determine if there is enough of a given amount of assemblies 
 * from an inventory
 *
 * @param inventory_t* invp - the inventory holding the assembly
 * @param char* id - the ID of the assembly to be made
 * @param int n - the number of assemblies needed
 * @param items_needed* parts - the parts/sub-assemblies required to 
 *                              create this assembly
 */
void get(inventory_t * invp, char * id, int n, items_needed_t * parts) {
    //check for valid amount
    if(n <= 0) {
        fprintf(stderr, "!!! %d: illegal order quantity for ID %s\n",
        n, id);
    }
    else {
        struct assembly* assembly = lookup_assembly(invp -> assembly_list, id);
        
        //assembly was not found
        if(assembly == NULL) {
            fprintf(stderr,
            "!!! %s: assembly ID is not in the inventory -- order canceled\n",
            id);
        }
        else {
            //check if there are enough of this assembly in stock
            if(assembly -> on_hand >= n) {
                (assembly -> on_hand) -= n;
            }
            //more of this assembly will need to be made
            else {
                int amount_to_make = n - (assembly -> on_hand);
                assembly -> on_hand = 0;
                make(invp, id, amount_to_make, parts); 
            }
        }
    }

}

/* - - - PRINT - - -*/

/*
 * Print all assemblies in the inventory along with their capacity and
 * the amount on hand of each
 *
 * @param inventory_t* invp - the inventory to be printed
 */
void print_inventory(inventory_t* invp) {

    //sort all assemblies in the inventory 
    assembly_t** assembly_array = to_assembly_array(invp -> assembly_count,
    invp -> assembly_list);
    qsort(assembly_array, invp -> assembly_count, sizeof(void*), assembly_compare);

    printf("Assembly inventory:\n");
    printf("-------------------\n");

    //if there is at least one assembly in the inventory
    if(invp -> assembly_count > 0) {
    
        printf("Assembly ID Capacity On Hand\n");
        printf("=========== ======== =======\n");
       
        int i;
        for(i = 0; i < invp -> assembly_count; i++) {
            printf("%-11s%9d%8d", assembly_array[i] -> id,
            assembly_array[i] -> capacity, assembly_array[i] -> on_hand);
            
            if(assembly_array[i] -> on_hand < 
            (double)(assembly_array[i] -> capacity) / 2.0) {
                printf("*");
            }
            printf("\n");
        
        }
    }
    else {
        printf("EMPTY INVENTORY\n");
    }
    
    free(assembly_array);

}

/*
 * Print all parts currently registered in the inventory
 *
 * @param inventory_t* invp - the inventory containing the parts
 */
void print_parts(inventory_t* invp) {
   
    //sort all parts in the part_list
    part_t** part_array = to_part_array(invp -> part_count, 
    invp -> part_list);
    qsort(part_array, invp -> part_count, sizeof(void*), part_compare);

    printf("Part inventory:\n");
    printf("---------------\n");
    
    //there is at least one part
    if(invp -> part_count > 0) {
        printf("Part ID\n");
        printf("===========\n");
        
        int i;
        for(i = 0; i < invp -> part_count; i++) {
            printf("%s\n", part_array[i] -> id);
        }
    
    }    
    //there are no parts
    else {
        printf("NO PARTS\n");
    }

    free(part_array);

}

/*
 * Print all items from an items_needed_t* list
 *
 * @param items_needed_t* items - the list containing the parts
 */
void print_items_needed(items_needed_t* items) {
    
    //sort all items in the items_list
    item_t** item_array = to_item_array(items -> item_count,
    items -> item_list);
    qsort(item_array, items -> item_count, sizeof(void*), item_compare);


    printf("%-11s %s\n", "Part ID", "quantity");
    printf("=========== ========\n");

    if(items -> item_count > 0) {
        int i;
        for(i = 0; i < items -> item_count; i++) {
            printf("%-11s %8d\n", item_array[i] -> id, 
            item_array[i] -> quantity);
        }

    }
    else {
        printf("NO PARTS\n");
    }

    free(item_array);
}

/* - - - PROCESS REQUESTS - - -*/

/*
 * Direct requests to the proper functions based on the string given
 *
 * @param char* array[] - the array containing the command and its arguments
 * @param int size - the size of the array
 *
 * @return int - 0: if command was 'quit', halts processing
 *               1: all other cases
 */
static int process_request(char* array[], int size) {

    char* command = array[0];
    
    //***************************************************************ADD PART 
    if(strncmp(command, "addPart", strlen(command)) == 0) {
        
        printf("+ addPart %s\n", array[1]);

        //check validity of part id
        if(valid_part_id(array[1])) {
            add_part(inventory, array[1]);
            return 1;
        }

        return 1;
    }
    //***********************************************************ADD ASSEMBLY 
    else if(strncmp(command, "addAssembly", strlen(command)) == 0) {
        
        printf("+ addAssembly ");
        int i;
        for(i = 1; i < size; i++) {
            printf("%s ", array[i]);
        }
        printf("\n");

        struct items_needed* items_needed = calloc(
        1, sizeof( struct items_needed));

        items_needed -> item_count = 0;
        
        if(valid_assembly_id(array[1])) {

            //check if there are items needed for the assembly
            //array[0] = addAssembly
            //array[1] = assembly ID
            //array[2] = capacity
            //array[3...] = items needed
            int valid = 1;
            //iterate through the given array of tokens and create items 
            int i;
            for(i = 3; i < size; i+=2) {
                //if there is a value after the item ID
                if((i + 1) < size) {
                    
                    valid = valid_item(inventory, array[i]);
                    if(valid) {
                        if(strtol(array[i+1], NULL, 10) > 0) {
                            add_item(items_needed, array[i],
                            strtol(array[i+1], NULL, 10));
                        }
                        else {
                            fprintf(stderr, 
                            "!!! %s: illegal quantity for ID %s\n",
                            array[i+1], array[i]);
                            //deem the request invalid and eject from the loop
                            valid = 0;
                            i = size;
                        }
                    }
                    //eject from the loop
                    else {
                        i = size;
                    }

                }
            }
            //only add the assembly if all of it's needed items were valid
            if(valid) {
                add_assembly(inventory, array[1], strtol(array[2], NULL, 10)
                , items_needed);
            }
            else {
                free_items_needed(items_needed);
            }

        }

        return 1;
    }
    //**********************************************************FULFILL ORDER 
    else if(strncmp(command, "fulfillOrder", strlen(command)) == 0) {
        
        printf("+ fulfillOrder ");
        int i;
        for(i = 1; i < size; i++) {
            printf("%s ", array[i]);
        }
        printf("\n");    

        struct items_needed* parts = calloc(1, sizeof(struct items_needed));

        //array[0] = fulfillOrder
        //array[1] = assembly1
        //array[2] = amount1
        //array[n] = assemblyn
        //array[n+1] = amountn
        if(size >= 3) {
            int i;
            int valid = 1;
            for(i = 1; i < size; i+=2) {
    
                //make will throw proper errors if needed
                make(inventory, array[i], strtol(array[i+1], NULL, 10)
                , parts);
                
                //determine if the arguments are valid 
                if(strtol(array[i+1], NULL, 10) <= 0 
                    || lookup_assembly(inventory -> assembly_list,
                        array[i]) == NULL) {
                    
                    //eject from the loop and do not proceed
                    valid = 0;
                    i = size;
                }
            
            }
            //if the process was valid and 
            //if any parts were neeed for this request 
            if(valid && parts -> item_count > 0) {   
                printf("Parts needed:\n");
                printf("-------------\n");
                print_items_needed(parts);
            }
        
        } 
        
        free_items_needed(parts);
        return 1;

    }
    //******************************************************************STOCK
    else if(strncmp(command, "stock", strlen(command)) == 0) {

        printf("+ stock %s %s\n", array[1], array[2]);

        struct items_needed* parts = calloc(1, sizeof(struct items_needed));

        if(size >= 3) {
            
            stock(inventory, array[1], strtol(array[2], NULL, 10), parts);
            
            //check if any parts were neeed for this request
            if(parts -> item_count > 0) {
                printf("Parts needed:\n");
                printf("-----------\n");
                print_items_needed(parts);
            }
            //free the parts needed list no longer in use
            free_items_needed(parts);

            return 1;
        }

        return 1;

    }        
    //****************************************************************RESTOCK
    else if(strncmp(command, "restock", strlen(command)) == 0) {

        struct items_needed* parts = calloc(1, sizeof(struct items_needed));
        
        //if an assembly ID was given
        if(size == 2) {
            printf("+ restock %s\n", array[1]);
            restock(inventory, array[1], parts);
        }
        //no assembly ID was given
        else if(size == 1) {
            printf("+ restock\n");
            restock(inventory, NULL, parts);
        }
        //incorrect number of arguments
        else {
            free_items_needed(parts);
            return 1;
        }
            
        //check if any parts were neeed for this request
        if(parts -> item_count > 0) {
            printf("Parts needed:\n");
            printf("-------------\n");
            print_items_needed(parts);
        }
        //free the parts needed list no longer in use
        free_items_needed(parts);

        return 1;
    
    }    
    //******************************************************************EMPTY
    else if(strncmp(command, "empty", strlen(command)) == 0) {
        
        printf("+ empty %s\n", array[1]);
        
        if(array[1][0] != 'A') {
            fprintf(stderr, "!!! %s: ID not an assembly\n",
            array[1]); 
            return 1;
        }

        struct assembly* assembly = lookup_assembly(
        inventory -> assembly_list, array[1]);

        if(assembly != NULL) {
            assembly -> on_hand = 0;
            return 1;
        }
        else {
            fprintf(stderr, "!!! %s: assembly ID is not in the inventory\n",
            array[1]);
        }
        
        return 1;
    }
    //**************************************************************INVENTORY
    else if(strncmp(command, "inventory", strlen(command)) == 0) {
        //if no id argument was given
        if(size == 1) {
            printf("+ inventory\n");
            print_inventory(inventory);
        }
        //id argument was given
        else {
            printf("+ inventory %s\n", array[1]);
            if(valid_assembly_id(array[1])) {
                
                struct assembly* assembly = lookup_assembly(
                inventory -> assembly_list, array[1]);
                
                if(assembly != NULL) {
                    printf("Assembly ID:\t%s\n", assembly -> id);
                    printf("bin capacity:\t%d\n", assembly -> capacity);
                    printf("on hand:\t%d\n", assembly -> on_hand);
                    printf("Parts list:\n");
                    printf("-----------\n");
                    print_items_needed(assembly -> items);
                }
                else {
                    fprintf(stderr, "!!! %s: part/assembly ID is not in the inventory\n",
                    array[1]);
                    return 1;
                }
            }
            else {
                return 1;
            }
        }
        return 1;
    }
    //******************************************************************PARTS 
    else if(strncmp(command, "parts", strlen(command)) == 0) {
        printf("+ parts\n");
        print_parts(inventory);
        return 1;
    }
    //*******************************************************************HELP
    else if(strncmp(command, "help", strlen(command)) == 0) {
        printf("+ help\n");

        printf("Requests:\n");
        printf("\taddPart\n");
        printf("\taddAssembly ID capacity [x1 n1 [x2 n2 ...]]\n");
        printf("\tfulfillOrder [x1 n1 [x2 n2 ...]]\n");
        printf("\tstock ID n\n");
        printf("\trestock [ID]\n");
        printf("\tempty ID\n");
        printf("\tinventory [ID]\n");
        printf("\tparts\n");
        printf("\thelp\n");
        printf("\tclear\n");
        printf("\tquit\n");
        
        return 1;
    }
    //******************************************************************CLEAR
    else if(strncmp(command, "clear", strlen(command)) == 0) {
        printf("+ clear\n");
        
        free_inventory(inventory);
        inventory = malloc(sizeof(inventory_t));
        inventory -> part_list = NULL;
        inventory -> part_count = 0;
        inventory -> assembly_list = NULL;
        inventory -> assembly_count = 0;
        
        return 1;
    }
    //*******************************************************************QUIT
    else if(strncmp(command, "quit", strlen(command)) == 0) {
        printf("+ quit\n");
        return 0;
    }
    //****************************************************************UNKNOWN
    else {
        printf("+ %s\n", command);
        fprintf(stderr, "!!! %s: unknown command\n", command);
        return 1;
    }

}

/* - - - FREES - - -*/

/*
 * Properly delete an items_needed list
 *
 * @param items_needed_t* items - the list to be deleted from memory
 */
static void free_items_needed(items_needed_t* items) {
    //free the parts needed list no longer in use
    struct item* temp_item = items -> item_list;
    while(temp_item != NULL) {
        //"save" the next item for deletion before deleting the previous
        struct item* temp_item_2 = temp_item;
        temp_item = temp_item -> next;
        free(temp_item_2);
    }
    free(items);
}

/*
 * Properly delete the entire inventory
 *
 * @param inventory_t* invp - the inventory to be deleted from memory
 */
void free_inventory(inventory_t* invp) {
    
    //free the parts list
    struct part* temp_part = invp -> part_list;

    //loop through the part_list if it is not empty
    while(temp_part != NULL) {
        //"save" the next item for deletion before deleting the previous
        struct part* temp_part_2 = temp_part;
        temp_part = temp_part -> next;
        free(temp_part_2);
    }

    //free the assemblylist
    struct assembly* temp_assembly = invp -> assembly_list;

    //loop through the assembly_list if it is not empty
    while(temp_assembly != NULL) {
        
        //free this assembly's items needed list
        free_items_needed(temp_assembly -> items);

        //continue to free the current assembly
        //"save" the next item for deletion before deleting the previous
        struct assembly* temp_assembly_2 = temp_assembly;
        temp_assembly = temp_assembly -> next;
        free(temp_assembly_2);
    }

    free(invp);
}

/* - - - MAIN - - -*/

/*
 * Main function primarily handles the allocation of the inventory 
 * struct and interpreting request lines from a file or stdin.
 *
 * @param int argc - the amount of arguments given
 * @param char* argv[] - the arguments given
 *
 * @return int - EXIT_FAILURE: the input file could not be read, 
 *                             or there was an incorrect number of
 *                             command line arguments
 *               EXIT_SUCCESS: the program executed successfully
 */
int main(int argc, char* argv[]) {
    
    //initialize inventory
    inventory = malloc(sizeof(inventory_t));
    inventory -> part_list = NULL;
    inventory -> part_count = 0; 
    inventory -> assembly_list = NULL;
    inventory -> assembly_count = 0;

    FILE* fp;

    if(argc == 1) {
        fp = stdin;
    }
    else if(argc == 2) {
        fp = fopen(argv[1], "r");

        if(!fp) {
            perror(argv[1]);
            return EXIT_FAILURE;
        }
    }
    else {
        fprintf(stderr, "Useage: ./inventory [filename]");
        printf("\n");
        return EXIT_FAILURE;
    }
    
    //getline() variables 
    char* buffer = (char*) malloc(sizeof(char) * MAX_LENGTH);
    size_t n = MAX_LENGTH;
    ssize_t num  = 0;

    //strtok() variables
    char* token;
    int i = 0;
    int request_return = 1;
    char* request_array[MAX_LENGTH];
    
    //extracing commands from file/stdin
    while(request_return && (num = getline(&buffer, &n, fp) != -1)) {

        //make sure the line is not blank
        if(strlen(trim(buffer)) != 0) {
            //catch entire line comments
            if(buffer[0] != '#') {
                //tokenize the line into a char* array
                token = strtok(buffer, " ");
                while(token != NULL) {
                    //if a comment is reached, eject from loop
                    if(token[0] == '#') {
                        token = NULL;
                    }
                    else {
                        request_array[i] = token;
                        token = strtok(NULL, " ");
                        //printf("%s\n", request_array[i]);
                        i++;
                    }
            
                }
                
                request_return = process_request(request_array, i);
                
                //reset/re-fill request_array
                i = 0;
                request_array[0] = '\0';

            }   
        }
    }
   
    fclose(fp);
    free_inventory(inventory);
    free(buffer);

    return EXIT_SUCCESS;

}

