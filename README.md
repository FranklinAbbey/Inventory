INVENTORY.C
by Franklin Abbey

Included in the _Extras_ folder:
 - error_test.txt: an examble of errors and constraints that will be caught when ran
 - fishing.txt: expected output from running the 'fishingRun.txt' file (output is at end of file) 
 - fishingRun.txt: text formatted to be runnable by inventory.c
 - memcheck.txt: readout to show the final memory condition of project
 - revisions.txt: revisions of project recorded in Git source control throughout project
 - samples.txt: sample input and output

* INTRODUCTION:

Inventory system manages an inventory of parts and assemblies. Assemblies are made up of parts and/or sub-assemblies (ex: a fishing pole "assembly" could consist of two "parts", a rod and a reel). Customer orders can be made and substracted from the inventory based on how many parts/assemblies are available. Totals can be manually updated. 

* PARTS:

Parts can be added using the 'addPart' command, followed by the name of the name in this format: 'P.partName'. 

* ASSEMBLIES:

Assemblies can be created in a similar fashion, by calling 'addAssembly' followed by the name of the assembly formatted like: 'A.assemblyName'. Following this command is a value that represents the number of "bins" or, the max number of assemblies the system can hold at once. These containers start off empty, so this is only a definition. Finally, after that definition, add to this command the parts required for this assembly, along with how many of each part are required. 

ex: addAssembly A.pole 12 P.rod 1 P.reel

This command creates a new assembly named 'pole', and specifies that a maximum of 12 can be on hand at once. The 'pole' assembly is made up of one rod and one reel, and both of these parts will be needed in it's creation (parts must be defined prior to the creation of an assembly). 

* STOCK:

Assemblies can be stocked with the command 'stock' followed by the assembly name (without the 'A' in front), followed by the amount. Parts are not stocked, but get created when assemblies are.  

ex: stock pole 13

If a value larger than the capacity of the assembly is entered, only the capacity will be created. The needed parts for each assemlby will also be created and read out. 

* RESTOCK: 

The restock command stocks all asseblies at less than half of their capacity to max capacity. It can be used with the 'restock' command and no arguments.


* INVENTORY: 

The current inventory of assemblies can be viewed with 'inventory' and the current number of parts can be seen with the 'parts' command


