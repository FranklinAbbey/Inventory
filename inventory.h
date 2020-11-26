/*
 * File: inventory.h
 *
 * Description: Function and struct definitions for an inventory system of
 *              parts and assemblies which are composed of parts and
 *              sub-assemblies
 *
 * @author: Frank Abbey (fra1489)
 * @version: 4/23/2020
 */
#ifndef INVENTORY_H
#define INVENTORY_H

//format a given string
extern char * trim(char *);
//extern int getline(char **, size_t *, FILE *);

//All IDs must not exceed 11 in length
#define ID_MAX 11

//struct to represent a part in the inventory
struct part {
    char id[ID_MAX+1];        // ID_MAX plus NUL
    struct part * next; // the next part in the list of parts
};

//struct to represent an assembly in the inventory
struct assembly {
    char id[ID_MAX+1];
    int capacity;
    int on_hand;
    struct items_needed * items; // parts/sub-assemblies needed for this ID
    struct assembly * next;      // the next assembly in the inventory list
};

//struct to represent an inventory item (a part or an assembly)
struct item {
    char id[ID_MAX+1];           // ID_MAX plus NUL
    int quantity;
    struct item * next; // next item in the part/assembly list
};

//the inventory struct (parts and assemblies)
struct inventory {
    struct part * part_list;         // list of parts by ID
    int part_count;                  // number of distinct parts
    struct assembly * assembly_list; // list of assemblies by ID
    int assembly_count;              // number of distinct assemblies
};

//parts/sub-assemblies needed to make required assemblies
struct items_needed {
    struct item * item_list;
    int item_count;
};

//struct to represent a request and the function needed to process (unused)
struct req {
    char * req_string;
    void (*req_fn)(struct inventory *);
};

//struct typedef declarations for ease of use
typedef struct inventory inventory_t;
typedef struct items_needed items_needed_t;
typedef struct item item_t;
typedef struct part part_t;
typedef struct assembly assembly_t;

//determine if a part is in a parts list
part_t * lookup_part(part_t * pp, char * id);
//determine if an assembly is in an assembly list
assembly_t * lookup_assembly(assembly_t * ap, char * id);
//determine if an item is in an item list
item_t * lookup_item(item_t * ip, char * id);

//add a part identifier to the inventory
void add_part(inventory_t * invp, char * id);
//add an assembly to the inventory
void add_assembly(inventory_t * invp,
                  char * id,
                  int capacity,
                  items_needed_t * items);
//add an item (part or assembly) to an items_needed list
void add_item(items_needed_t * items, char * id, int quantity);

// these are used for sorting purposes
//convert a linked list of parts to an array or parts
part_t ** to_part_array(int count, part_t * part_list);
//convert a linked list of assemblies to an array of assemblies
assembly_t ** to_assembly_array(int count, assembly_t * assembly_list);
//convert a linked list of items to an array of items
item_t ** to_item_array(int count, item_t * item_list);

//compare two parts based on their IDs
int part_compare(const void *, const void *);
//compare two assemblies based on their IDs
int assembly_compare(const void *, const void *);
//compare two items based on their IDs
int item_compare(const void *, const void *);

//Make a given amount of assemblies from an inventory
void make(inventory_t * invp, char * id, int n, items_needed_t * parts);
//Determine if there is enough of a given amount of assemblies
void get(inventory_t * invp, char * id, int n, items_needed_t * parts);

//display a sorted list of assemblies in the inventory
void print_inventory(inventory_t * invp);
//display a sorted list of parts
void print_parts(inventory_t * invp);
//display a sorted list of items from an items_needed list
void print_items_needed(items_needed_t * items);

//delete the entire inventory and free all allocated memory
void free_inventory(inventory_t * invp);

#endif // INVENTORY_H
