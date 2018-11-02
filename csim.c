//Jason King, jpking
//Kamil Gumienny, kmgumienny

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include "cachelab.h"
#include <math.h>

//Global variables that hold statistics printed at the end
static int hits = 0;
static int misses = 0;
static int evicts = 0;
//Global variables that hold cache parameters
static int sets = 0;
static int lines = 0;
static int blocks = 0;

typedef struct{ //Struct for a line
	int tag;
	int valid;
	int time;
} cacheL;

typedef struct{ //Struct for a set
	cacheL *lines;
} cacheS;

typedef struct{ //Struct for a cache
	cacheS *sets;
	int setNum;
	int linesPer;
} cache;

//if the verbose tag is set in the cmd line parameters, this function is called
void printHelp() {
	printf("Command Line : ./csim [-h] [-v] -s # -E # -b # -t file \n");
	printf(" h       = prints out help message");
	printf(" v       = verbose flags\n");
	printf(" s (req) = sets # to number of set index bits \n");
	printf(" E (req) = sets # to number of lines/set \n");
	printf(" b (req) = sets # to number of offset bits \n");
	printf(" t (req) = trace file to simulate \n");
}

//Executed once with parameters provided by user for cache simulation
void makeCache(cache *cache, int lines, int sets) { // creates a "cache"
	//makes the cache struct
	//cache->setNum = (int) pow(2, sets); //2^s
	cache->setNum = (2 << sets);
	cache->linesPer = lines;
	cache->sets = (cacheS*) calloc(cache->setNum, sizeof(cacheS));

	//initializes the cache
	for(int i = 0; i < cache->setNum; i++){
		cache->sets[i].lines = (cacheL*) calloc(cache->linesPer, sizeof(cacheL));
		for(int j = 0; j < cache->linesPer; j++){
			cache->sets[i].lines[j].valid = 0;
		}
	}
}

/* Ensures that the most recently accessed data will be accessed first,
 * checks if data is already in cache & if a line already accessed has time > time of line being accessed,
 * lowers time of any valid line already accessed, and set the access time to the number of lines per set
 */
int recentCache(cache *cache, int line, int set){
	for(int i = 0; i < cache->linesPer; i++)
		if((cache->sets[set].lines[i].valid == 1)
				&& (cache->sets[set].lines[i].time > cache->sets[set].lines[line].time))
			(cache->sets[set].lines[i].time)--;
	cache->sets[set].lines[line].time = cache->linesPer;
	return 0;
}


// Analyzes the traces to a cache and returns either hit, miss, or evict
int analyzeCache(cache *cache, char *output, int block, int line, int set) {
	//variables used to calculate where to check in the cache system
	int mask = 0x7fffffff;
	int address;
	//operation holds the operation indicator M, L, or S
	char operation;
	//acts as a boolean to determine if the M operation hits on both load & store
	int doubleHit = 0;
	sscanf(output, " %c %x", &operation, &address);
	// calculates set and tag bits using a mask to get cache address
	int cleanSet = (mask >> (31 - set)) & (address >> block);
	int cleanTag = (mask >> (31 - set - block)) & (address >> (set + block));

	for(int i = 0; i < cache->linesPer; i++){
		if((cache->sets[cleanSet].lines[i].valid == 1) // if valid & tags are equal
				&& (cache->sets[cleanSet].lines[i].tag == cleanTag)){
			if(operation == 'M'){
				hits+=2;
				doubleHit = 1;// sets doubleHit to indicate hit load & store
			}else
				hits++;
			recentCache(cache, i, cleanSet);
			if(doubleHit)
				return 3; // hit hit
			else
				return 2; // hit
		}
	}
	misses++; // if skipped loop -> missed

	for(int i = 0; i < cache->linesPer; i++) {
		if(cache->sets[cleanSet].lines[i].valid == 0) { // if true -> confirmed miss
			cache->sets[cleanSet].lines[i].valid = 1; // set line to 1
			cache->sets[cleanSet].lines[i].tag = cleanTag; // reinstate the tag
			recentCache(cache, i, cleanSet);
			if(operation == 'M'){ // data load + data store
				hits++;
				return 5; //miss hit
			}else
				return 1; //miss
		}
	}

	evicts++; // if operation didn't hit or miss then evict

	for(int i = 0; i < cache->linesPer; i++) {
		if(cache->sets[cleanSet].lines[i].time == 1){ // if matches time of the cache, evicts old data and stores new
			cache->sets[cleanSet].lines[i].valid = 1;
			cache->sets[cleanSet].lines[i].tag = cleanTag;
			recentCache(cache, i, cleanSet);
			if(operation == 'M'){ // data load + data store
				hits++;
				return 6; // miss evict hit
			}else
				return 4; // miss evict
		}
	}
	printf("Unexpected Error. Exiting.");
	exit(0);
	return 0;
}

//frees the cache by freeing memory calloc'd for the lines and set in cache
void freeCache(cache *cache) {
	for (int i = 0; i < cache->setNum; i++)
		free(cache->sets[i].lines);
	free(cache->sets);
}

int main(int argc, char *argv[]) {
	int op = 0; //stores command line argument
	int numCom = 0; //stores number of critical arguments
	int verbose = 0; //boolean for printHelp function
	int result = 0; //stores hit, miss, or evict result
	char fileName[255]; //holds file name
	char traces[255]; //hold string with trace
	FILE *file;


	while ((op = getopt(argc, argv, "hvs:E:b:t:")) != -1) { // reads cmd
		switch (op){
			case 'h':
				printHelp();
				break;
			case 'v':
				verbose = 1;
				break;
			case 's':
				sets = atoi(optarg);
				numCom++;
				break;
			case 'E':
				lines = atoi(optarg);
				numCom++;
				break;
			case 'b':
				blocks = atoi(optarg);
				numCom++;
				break;
			case 't':
				strcpy(fileName, optarg);
				numCom++;
				break;
			default:
				printf("ERROR: Invalid Argument Entered. Exiting.");
				return 0;
		}
	}
	//makes sure all critical arguments are entered and are natural numbers
	if(numCom < 4 || sets < 1 || lines < 1 || blocks < 1) {
		printf("Not enough critical arguments to run the simulation or invalid numbers input. Exiting.");
		return 0;
	}

	//Opens file and checks it exists
	file = fopen(fileName, "r");
	if (file == NULL) {
		printf("ERROR: Invalid Input File. Exiting.");
		return 0;
	}

	cache cache;
	makeCache(&cache, lines, sets);

	while (fgets(traces, 255, file) != NULL) {
		if (traces[0] == ' ') { // ignore instructions
			traces[strlen(traces) - 1] = '\0'; //add null terminator
			result = analyzeCache(&cache, traces, blocks, lines, sets);
			if (verbose == 1) { // -v output
				switch (result){
				case 1: // miss
					printf("%s miss\n", (traces + 1));
					break;
				case 2: // hit
					printf("%s hit\n", (traces + 1));
					break;
				case 3: // hit hit
					printf("%s hit hit\n", (traces + 1));
					break;
				case 4: // miss miss
					printf("%s miss eviction\n", (traces + 1));
					break;
				case 5: // miss hit
					printf("%s miss hit\n", (traces + 1));
					break;
				case 6: // miss evict hit
					printf("%s miss eviction hit\n", (traces + 1));
					break;
				default: //function would exit before ever reaching this point
					return 0;
					break;
				}
			}
		}
	}
	//closes file, prints summary of simulation, and frees the memory allocated for cache
	fclose(file);
    printSummary(hits, misses, evicts);
    freeCache(&cache);
    return 0;
}
