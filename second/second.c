#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>


struct Line{
    int valid;
    int tag;
    int age;
    int memAdd;
};

int updateCache(struct Line** cache, int assoc, int tag, int index, int* ageCounter, int address){
    *ageCounter = *ageCounter + 1;
    int evictedAdd = -1;

    for (int j = 0; j < assoc; j++){ //if there's an empty line, put the data in that line
        if (cache[index][j].valid == 0){
            cache[index][j].valid = 1;
            cache[index][j].tag = tag;
            cache[index][j].age = *ageCounter;
            cache[index][j].memAdd = address;
            return evictedAdd;
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
            evictedAdd = cache[index][j].memAdd;
            cache[index][j].tag = tag;
            cache[index][j].age = *ageCounter;
            cache[index][j].memAdd = address;
        }
    }
    return evictedAdd;
}

int computeTag(unsigned int address, int blockSize, int numOfSets){
    double bitsCalc = log2(blockSize);
    int blockBits = bitsCalc;
    bitsCalc = log2(numOfSets);
    int indexBits = bitsCalc;
    
    int tag = address >> (blockBits + indexBits);
    return tag;
}

int computeIndex(unsigned int address, int blockSize, int numOfSets){
    double bitsCalc = log2(blockSize);
    int blockBits = bitsCalc;
    int index = address >> blockBits & (numOfSets - 1);
    return index;
}

int main(int argc, char** argv){
    //l1 attributes
    int cacheSize = atoi(argv[1]);
    char* token = strtok(argv[2], ":");
    token = strtok(NULL, ":");
    int linesInSet = atoi(token);
    char* replacePol = argv[3];
    int blockSize = atoi(argv[4]);

    int setSize = blockSize * linesInSet;
    int numSet = cacheSize/setSize;

    //l2 attributes
    int l2cacheSize = atoi(argv[5]);
    token = strtok(argv[6], ":");
    token = strtok(NULL, ":");
    int l2linesInSet = atoi(token);

    int l2setSize = blockSize * l2linesInSet;
    int l2numSet = l2cacheSize/l2setSize;

    int cacheHits = 0;
    int cacheMisses = 0;
    int l2cacheHits = 0;
    int l2cacheMisses = 0;
    int memReads = 0;
    int memWrites = 0;

    //allocating space for the cache1
    struct Line** cache1 = malloc(numSet * sizeof(struct Line*));

    for (int i = 0; i < numSet; i++){
        cache1[i] = malloc(linesInSet * sizeof(struct Line));
    }

    //initializing all valid bits to 0
    for (int i = 0; i < numSet; i++){
        for (int j = 0; j < linesInSet; j++){
            cache1[i][j].valid = 0;
        }
    }

    //allocating space for cache2 and initializing all valid bits to 0
    struct Line** cache2 = malloc(l2numSet * sizeof(struct Line*));

    for (int i = 0; i < l2numSet; i++){
        cache2[i] = malloc(l2linesInSet * sizeof(struct Line));
    }

    for (int i = 0; i < l2numSet; i++){
        for (int j = 0; j < l2linesInSet; j++){
            cache2[i][j].valid = 0;
        }
    }

    FILE* fp = fopen(argv[8], "r");

    char instruct;
    char memBuff[20];
    int ageCounter = 0;


    //loop through the file
    while (fscanf(fp, "%c", &instruct) != EOF){
        fscanf(fp, "%s\n", memBuff);
        unsigned int memAddress = strtol(memBuff, NULL, 16);
        //tag and index bits
        int tag1 = computeTag(memAddress, blockSize, numSet);
        int index1 = computeIndex(memAddress, blockSize, numSet);

        int tag2 = computeTag(memAddress, blockSize, l2numSet);
        int index2 = computeIndex(memAddress, blockSize, l2numSet);

        int hitormiss = 0; //keeps track of whether it's a cache hit(1) or miss(0)

        if (strcmp(replacePol, "fifo") == 0){ //for fifo policy;
            if (instruct == 'R'){ //for read instruction
                for (int j = 0; j < linesInSet; j++){ //loop through lines at the specified index in cache1
                    if (cache1[index1][j].valid == 1 && cache1[index1][j].tag == tag1){ //if it's a hit, increment cachehits and change hitormiss to 1
                        hitormiss = 1;
                        cacheHits++;
                        break;
                    }
                }
                if (hitormiss == 0){ //if data is not found in cache1, look through cache2
                    cacheMisses++;
                    for (int j = 0; j < l2linesInSet; j++){
                        if (cache2[index2][j].valid == 1 && cache2[index2][j].tag == tag2){ //if data is found in cache2, increment cachehits, change hitormiss to 1, bring data to cache1, and if something is evicted, bring it to cache2
                            hitormiss = 1;
                            l2cacheHits++;
                            cache2[index2][j].valid = 0;
                            int evicted = updateCache(cache1, linesInSet, tag1, index1, &ageCounter, memAddress); 
                            if (evicted != -1){ //if a line was evicted 
                                updateCache(cache2, l2linesInSet, computeTag(evicted, blockSize, l2numSet), computeIndex(evicted, blockSize, l2numSet), &ageCounter, evicted);
                            }
                            break;
                        }
                    }
                }
                if (hitormiss == 0){ //if data is not found in either cache, bring from memory to l1, and if l1 evicts, put it in l2
                    l2cacheMisses++;
                    memReads++;
                    int evicted = updateCache(cache1, linesInSet, tag1, index1, &ageCounter, memAddress);
                    if (evicted != -1){ //if a line was evicted 
                        updateCache(cache2, l2linesInSet, computeTag(evicted, blockSize, l2numSet), computeIndex(evicted, blockSize, l2numSet), &ageCounter, evicted);
                        }
                    }
            }
            else if (instruct == 'W'){
                for (int j = 0; j < linesInSet; j++){ //loop through lines at the specified index in cache1
                    if (cache1[index1][j].valid == 1 && cache1[index1][j].tag == tag1){ //if write-hit, increment cachehits, memWrites and change hitormiss to 1
                        hitormiss = 1;
                        cacheHits++;
                        memWrites++;
                        break;
                    }
                }

                if (hitormiss == 0){
                    cacheMisses++;
                    for (int j = 0; j < l2linesInSet; j++){
                        if (cache2[index2][j].valid == 1 && cache2[index2][j].tag == tag2){ //if data is found in cache2, increment cachehits + memWrites, chance hitormiss to 1, bring data to cache1, and if something is evicted, bring it to cache2
                            hitormiss = 1;
                            memWrites++;
                            l2cacheHits++;
                            cache2[index2][j].valid = 0;
                            int evicted = updateCache(cache1, linesInSet, tag1, index1, &ageCounter, memAddress); 
                            if (evicted != -1){ //if a line was evicted 
                                updateCache(cache2, l2linesInSet, computeTag(evicted, blockSize, l2numSet), computeIndex(evicted, blockSize, l2numSet), &ageCounter, evicted);
                            }
                            break;
                        }
                    }
                }
                
                if (hitormiss == 0){ //if write-miss, updatecache, increment cacheMisses, memReads, memWrites, and updateCache
                    l2cacheMisses++;
                    memReads++;
                    memWrites++;
                    int evicted = updateCache(cache1, linesInSet, tag1, index1, &ageCounter, memAddress);
                    if (evicted != -1){ //if a line was evicted 
                                updateCache(cache2, l2linesInSet, computeTag(evicted, blockSize, l2numSet), computeIndex(evicted, blockSize, l2numSet), &ageCounter, evicted);
                            }
                }
            }
        }
        else if (strcmp(replacePol, "lru") == 0){
            if (instruct == 'R'){ //for read instruction
                for (int j = 0; j < linesInSet; j++){ //loop through lines at the specified index in cache1
                    if (cache1[index1][j].valid == 1 && cache1[index1][j].tag == tag1){ //if it's a hit, increment cachehits and change hitormiss to 1, update age
                        ageCounter++;
                        cache1[index1][j].age = ageCounter;
                        hitormiss = 1;
                        cacheHits++;
                        break;
                    }
                }
                if (hitormiss == 0){ //if data is not found in cache1, look through cache2
                    cacheMisses++;
                    for (int j = 0; j < l2linesInSet; j++){
                        if (cache2[index2][j].valid == 1 && cache2[index2][j].tag == tag2){ //if data is found in cache2, increment cachehits, chance hitormiss to 1, bring data to cache1, and if something is evicted, bring it to cache2
                            hitormiss = 1;
                            l2cacheHits++;
                            cache2[index2][j].valid = 0;
                            int evicted = updateCache(cache1, linesInSet, tag1, index1, &ageCounter, memAddress); 
                            if (evicted != -1){ //if a line was evicted 
                                updateCache(cache2, l2linesInSet, computeTag(evicted, blockSize, l2numSet), computeIndex(evicted, blockSize, l2numSet), &ageCounter, evicted);
                            }
                            break;
                        }
                    }
                }
                if (hitormiss == 0){ //if data is not found in either cache, bring from memory to l1, and if l1 evicts, put it in l2
                    l2cacheMisses++;
                    memReads++;
                    int evicted = updateCache(cache1, linesInSet, tag1, index1, &ageCounter, memAddress);
                    if (evicted != -1){ //if a line was evicted 
                        updateCache(cache2, l2linesInSet, computeTag(evicted, blockSize, l2numSet), computeIndex(evicted, blockSize, l2numSet), &ageCounter, evicted);
                        }
                    }
            }
            else if (instruct == 'W'){
                for (int j = 0; j < linesInSet; j++){ //loop through lines at the specified index in cache1
                    if (cache1[index1][j].valid == 1 && cache1[index1][j].tag == tag1){ //if write-hit, increment cachehits, memWrites and change hitormiss to 1, update age counter
                        ageCounter++;
                        cache1[index1][j].age = ageCounter;
                        hitormiss = 1;
                        cacheHits++;
                        memWrites++;
                        break;
                    }
                }

                if (hitormiss == 0){
                    cacheMisses++;
                    for (int j = 0; j < l2linesInSet; j++){
                        if (cache2[index2][j].valid == 1 && cache2[index2][j].tag == tag2){ //if data is found in cache2, increment cachehits + memWrites, chance hitormiss to 1, bring data to cache1, and if something is evicted, bring it to cache2
                            hitormiss = 1;
                            memWrites++;
                            l2cacheHits++;
                            cache2[index2][j].valid = 0;
                            int evicted = updateCache(cache1, linesInSet, tag1, index1, &ageCounter, memAddress); 
                            if (evicted != -1){ //if a line was evicted 
                                updateCache(cache2, l2linesInSet, computeTag(evicted, blockSize, l2numSet), computeIndex(evicted, blockSize, l2numSet), &ageCounter, evicted);
                            }
                            break;
                        }
                    }
                }
                
                if (hitormiss == 0){ //if write-miss, updatecache, increment cacheMisses, memReads, memWrites, and updateCache
                    l2cacheMisses++;
                    memReads++;
                    memWrites++;
                    int evicted = updateCache(cache1, linesInSet, tag1, index1, &ageCounter, memAddress);
                    if (evicted != -1){ //if a line was evicted 
                                updateCache(cache2, l2linesInSet, computeTag(evicted, blockSize, l2numSet), computeIndex(evicted, blockSize, l2numSet), &ageCounter, evicted);
                            }
                }
            }
        }
    }

    printf("memread:%d\n", memReads);
    printf("memwrite:%d\n", memWrites);
    printf("l1cachehit:%d\n", cacheHits);
    printf("l1cachemiss:%d\n", cacheMisses);
    printf("l2cachehit:%d\n", l2cacheHits);
    printf("l2cachemiss:%d\n", l2cacheMisses);
    //printf("%d\n", ageCounter);

//freeing the cache
   for (int i = 0; i < numSet; i++){
    free(cache1[i]);
   }
   free(cache1);

   for (int i = 0; i < l2numSet; i++){
    free(cache2[i]);
   }

   free(cache2);

   return 0;
} 