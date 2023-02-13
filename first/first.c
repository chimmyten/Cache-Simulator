#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>


struct Line{
    int valid;
    int tag;
    int age;
};

void updateCache(struct Line** cache, int assoc, int tag, int index, int* ageCounter){
    *ageCounter = *ageCounter + 1;

    for (int j = 0; j < assoc; j++){ //if there's an empty line, put the data in that line
        if (cache[index][j].valid == 0){
            cache[index][j].valid = 1;
            cache[index][j].tag = tag;
            cache[index][j].age = *ageCounter;
            return;
        }
    } 

    int lowestAge = INT_MAX; //if there's no empty slot, find the lowest age line and replace it with the data
    for (int j = 0; j < assoc; j++){
        if (cache[index][j].age < lowestAge){
            lowestAge = cache[index][j].age;
        }
    }
    for (int j = 0; j < assoc; j++){
        if (cache[index][j].age == lowestAge){
            cache[index][j].tag = tag;
            cache[index][j].age = *ageCounter;
        }
    }
    return;
}


int main(int argc, char** argv){
    int cacheSize = atoi(argv[1]);
    char* token = strtok(argv[2], ":");
    token = strtok(NULL, ":");
    int linesInSet = atoi(token);
    char* replacePol = argv[3];
    int blockSize = atoi(argv[4]);

    int setSize = blockSize * linesInSet;
    int numSet = cacheSize/setSize;

    int cacheHits = 0;
    int cacheMisses = 0;
    int memReads = 0;
    int memWrites = 0;

    //allocating space for the cache
    struct Line** cache = malloc(numSet * sizeof(struct Line*));

    for (int i = 0; i < numSet; i++){
        cache[i] = malloc(linesInSet * sizeof(struct Line));
    }

    //initializing all valid bits to 0
    for (int i = 0; i < numSet; i++){
        for (int j = 0; j < linesInSet; j++){
            cache[i][j].valid = 0;
        }
    }

    FILE* fp = fopen(argv[5], "r");

    char instruct;
    char memBuff[20];
    int ageCounter = 0;

    //calculate bits for block and index
    double bitsCalc = log2(blockSize);
    int blockBits = bitsCalc;
    bitsCalc = log2(numSet);
    int indexBits = bitsCalc;

    //loop through the file
    while (fscanf(fp, "%c", &instruct) != EOF){
        fscanf(fp, "%s\n", memBuff);
        unsigned int memAddress = strtol(memBuff, NULL, 16);
        int tag = memAddress >> (blockBits + indexBits);
        int index = memAddress >> blockBits & (numSet - 1);
        //printf("%d %d\n", tag, index);
        int hitormiss = 0; //keeps track of whether it's a cache hit(1) or miss(0)

        if (strcmp(replacePol, "fifo") == 0){ //for fifo policy;
            if (instruct == 'R'){ //for read instruction
                for (int j = 0; j < linesInSet; j++){ //loop through lines at the specified index
                    if (cache[index][j].valid == 1 && cache[index][j].tag == tag){ //if it's a hit, increment cachehits and change hitormiss to 1
                        hitormiss = 1;
                        cacheHits++;
                        break;
                    }
                }
                if (hitormiss == 0){ //if data is not found in the cache
                cacheMisses++;
                memReads++;
                updateCache(cache, linesInSet, tag, index, &ageCounter);
                }
            }
            else if (instruct == 'W'){
                for (int j = 0; j < linesInSet; j++){ //loop through lines at the specified index
                    if (cache[index][j].valid == 1 && cache[index][j].tag == tag){ //if write-hit, increment cachehits, memWrites and change hitormiss to 1
                        hitormiss = 1;
                        cacheHits++;
                        memWrites++;
                        break;
                    }
                }
                if (hitormiss == 0){ //if write-miss, increment cacheMisses, memReads, memWrites, and updateCache
                    cacheMisses++;
                    memReads++;
                    memWrites++;
                    updateCache(cache, linesInSet, tag, index, &ageCounter);
                }
            }
        }
        else if (strcmp(replacePol, "lru") == 0){
            if (instruct == 'R'){ //for read instruction
                for (int j = 0; j < linesInSet; j++){ //loop through lines at the specified index
                    if (cache[index][j].valid == 1 && cache[index][j].tag == tag){ //if it's a hit, increment cachehits and change hitormiss to 1 and update age
                        ageCounter++;
                        cache[index][j].age = ageCounter;
                        hitormiss = 1;
                        cacheHits++;
                        break;
                    }
                }
                if (hitormiss == 0){ //if data is not found in the cache
                cacheMisses++;
                memReads++;
                updateCache(cache, linesInSet, tag, index, &ageCounter);
                }
            }
            else if (instruct == 'W'){
                for (int j = 0; j < linesInSet; j++){ //loop through lines at the specified index
                    if (cache[index][j].valid == 1 && cache[index][j].tag == tag){ //if write-hit, increment cachehits, memWrites and change hitormiss to 1
                        ageCounter++;
                        cache[index][j].age = ageCounter;
                        hitormiss = 1;
                        cacheHits++;
                        memWrites++;
                        break;
                    }
                }
                if (hitormiss == 0){ //if write-miss, updatecache, increment cacheMisses, memReads, memWrites, and updateCache
                    cacheMisses++;
                    memReads++;
                    memWrites++;
                    updateCache(cache, linesInSet, tag, index, &ageCounter);
                }
            }
        }
    }

    printf("memread:%d\n", memReads);
    printf("memwrite:%d\n", memWrites);
    printf("cachehit:%d\n", cacheHits);
    printf("cachemiss:%d\n", cacheMisses);
    //printf("%d\n", ageCounter);

//freeing the cache
   for (int i = 0; i < numSet; i++){
    free(cache[i]);
   }
   free(cache);

   return 0;
} 