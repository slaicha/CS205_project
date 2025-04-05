#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 1MB is equal to 1024 * 1024 bytes
static const int max_block_size = 1024; // in bytes as for the following
static const int max_cache_size = 2097152; // 2MB max cache size  1024 * 1024 * 2
static const int max_memory_size = 16777216; // = 16 * 1024 * 1024 = 16MB memory size we can think about it as virtual memory space

// array of 16MB to represent main memory
unsigned char memory[max_memory_size];  // should be in char bc the machine is byte addresses and 1 byte is 1 char; char didn't work when I printed the char

int global_timestamp = 0;

int log2(int n) {
    int r=0;
    while (n>>=1) r++;
    return r;
}

int power_of_two(int exponent) {
    // exponent is positive
    int result = 1;
    for (int i = 0; i < exponent; i++) {
        result *= 2;
    }
    return result;
}

int ones(int n) {
    return (1<<n)-1;
} 

typedef struct {
    int dirty;
    int tag;
    int valid;
    int timestamp;
    unsigned char data[max_block_size]; // this holds the data in the corresponding block; max block size is in bytes
    // data[i] is one byte
} Block;



//////////////////////////////////////////////// STORE

void execute_store(Block **cache, int address, int block_offset, int set_index, int tag, unsigned char *data, int access_size, int associativity,
                  int block_size, int idx_bits, int offset_bits){

    // looping over the blocks in the set
    for (int i = 0; i < associativity; i++) { 
        // store hit
        if (cache[set_index][i].valid == 1 && cache[set_index][i].tag == tag) {
            printf("store 0x%x hit\n", address);
            // now that we have a hit, we need to store data in the corresponding place in block
            for (int j = 0; j < access_size; j++) {
                cache[set_index][i].data[block_offset + j] = data[j]; 
                // printf("%x\n", data[j]);
            }
            cache[set_index][i].dirty = 1; // set dirty to one because data in memory and cache are different and cache is write-back
            cache[set_index][i].timestamp = global_timestamp;
            return;
        }

        // else store miss; we can't find tag
        // is there a case when tag match and valid bit = 0; yes only when the tag of the block is 0 and the block has never been used before
        else if (cache[set_index][i].valid == 0 && cache[set_index][i].tag == tag) {
            printf("store 0x%x miss\n", address);

            // fetch whole block from memory
            int first_byte_address = tag * power_of_two(idx_bits+offset_bits) + set_index * power_of_two(offset_bits);

            for (int j = 0; j < block_size; j++) {
                cache[set_index][i].data[j] = memory[first_byte_address + j]; 
            }

            // update the data 
            for (int j = 0; j < access_size; j++) {
                cache[set_index][i].data[block_offset + j] = data[j]; 
            }

            cache[set_index][i].dirty = 1;
            cache[set_index][i].valid = 1;
            cache[set_index][i].timestamp = global_timestamp;

            return;
        } 
    }

    // so no tag match -> store miss


    // check if there is an invalid block in the set
    for (int i = 0; i < associativity; i++) {
        if(cache[set_index][i].valid == 0){

            // fetch whole block from memory and put it in the invalid block's place
            int first_byte_address = tag * power_of_two(idx_bits+offset_bits) + set_index * power_of_two(offset_bits);

            for (int j = 0; j < block_size; j++) {
                cache[set_index][i].data[j] = memory[first_byte_address + j]; 
            }

            // update block data 
            for (int j = 0; j < access_size; j++) {
                cache[set_index][i].data[block_offset + j] = data[j]; 
            }    

            cache[set_index][i].dirty = 1;
            cache[set_index][i].valid = 1;
            cache[set_index][i].tag = tag;
            cache[set_index][i].timestamp = global_timestamp;

            printf("store 0x%x miss\n", address);
            return;     
        }
    }

    // if there are only valid blocks in the set -> replacement  
    // fecth the block to evict by comparing the timestamps
    // what if two blocks have the same min_timestamp?
    int min_timestamp = global_timestamp;
    int idx_evict = 0;
    for (int i = 0; i < associativity; i++) {
        if (cache[set_index][i].timestamp <= min_timestamp){
            min_timestamp = cache[set_index][i].timestamp;
            idx_evict = i;
        }
    }

    // need to find the address in memory of the block to evict
    int block_evict_address = cache[set_index][idx_evict].tag * power_of_two(idx_bits+offset_bits) + set_index * power_of_two(offset_bits);

    // so we got the idx to evict; check if block is dirty
    // replacement of dirty block
    if (cache[set_index][idx_evict].dirty == 1){

        // update the evicted block's data in memory
        for (int i = 0; i<block_size; i++){
            memory[block_evict_address + i] =  cache[set_index][idx_evict].data[i];
        }

        // fetch whole new block from memory and put it in the evicted block's place
        int first_byte_address = tag * power_of_two(idx_bits+offset_bits) + set_index * power_of_two(offset_bits);

        for (int j = 0; j < block_size; j++) {
            cache[set_index][idx_evict].data[j] = memory[first_byte_address + j]; 
        }  

        // update block data 
        for (int j = 0; j < access_size; j++) {
            cache[set_index][idx_evict].data[block_offset + j] = data[j]; 
        }    

        cache[set_index][idx_evict].dirty = 1;
        cache[set_index][idx_evict].valid = 1;
        cache[set_index][idx_evict].tag = tag;
        cache[set_index][idx_evict].timestamp = global_timestamp;

        printf("replacement 0x%x dirty\n", block_evict_address);

        printf("store 0x%x miss\n", address);

        return;
    }

    // replcament of clean block

    // fetch whole new block from memory and put it in the evicted block's place
    int first_byte_address = tag * power_of_two(idx_bits+offset_bits) + set_index * power_of_two(offset_bits);

    for (int j = 0; j < block_size; j++) {
        cache[set_index][idx_evict].data[j] = memory[first_byte_address + j]; 
    }  

    // update block data 
    for (int j = 0; j < access_size; j++) {
        cache[set_index][idx_evict].data[block_offset + j] = data[j]; 
    }    

    cache[set_index][idx_evict].dirty = 1;
    cache[set_index][idx_evict].valid = 1;
    cache[set_index][idx_evict].tag = tag;
    cache[set_index][idx_evict].timestamp = global_timestamp;

    printf("replacement 0x%x clean\n", block_evict_address);

    printf("store 0x%x miss\n", address);

    return;
};






//////////////////////////////////////////// LOAD


void execute_load(Block **cache, int address, int block_offset, int set_index, int tag, int access_size, int associativity, int block_size, 
            int idx_bits, int offset_bits){

    for (int i = 0; i < associativity; i++) { 
        // load hit
        if (cache[set_index][i].valid == 1 && cache[set_index][i].tag == tag) {
            printf("load 0x%x hit ", address);
            //printf("Z\n");

            // print the loaded data
            for (int j = 0; j < access_size; j++) {
                printf("%02hhx", cache[set_index][i].data[block_offset + j]);
            }
            printf("\n");
            cache[set_index][i].timestamp = global_timestamp;
            return;
        }

        // else load miss; tag match and valid bit = 0
        else if (cache[set_index][i].valid == 0 && cache[set_index][i].tag == tag) {
            printf("load 0x%x miss ", address);

            // fetch whole block from memory
            int first_byte_address = tag * power_of_two(idx_bits+offset_bits) + set_index * power_of_two(offset_bits);

            for (int j = 0; j < block_size; j++) {
                cache[set_index][i].data[j] = memory[first_byte_address + j]; 
            }

            // print the data
            for (int j = 0; j < access_size; j++) {
                printf("%02hhx", cache[set_index][i].data[block_offset + j]);
            }
            printf("\n");
            //printf("A\n");

            cache[set_index][i].valid = 1;
            cache[set_index][i].dirty = 0;
            cache[set_index][i].timestamp = global_timestamp;

            return;
        } 
    }

    // so no tag match -> load miss

    // check if there is an invalid block in the set
    for (int i = 0; i < associativity; i++) {
        if(cache[set_index][i].valid == 0){

            
            // fetch whole block from memory and put it in the invalid block's place
            int first_byte_address = tag * power_of_two(idx_bits+offset_bits) + set_index * power_of_two(offset_bits);

            for (int j = 0; j < block_size; j++) {
                cache[set_index][i].data[j] = memory[first_byte_address + j]; 
            }

            printf("load 0x%x miss ", address);
            // print the data
            for (int j = 0; j < access_size; j++) {
                printf("%02hhx", cache[set_index][i].data[block_offset + j]);
            }
            printf("\n");
            //printf("B\n");

            cache[set_index][i].valid = 1;
            cache[set_index][i].tag = tag;
            cache[set_index][i].dirty = 0;
            cache[set_index][i].timestamp = global_timestamp;

            return;     
        }
    }

    // if there are only valid blocks in the set -> replacement  
    // fecth the block to evict by comparing the timestamps
    // what if two blocks have the same min_timestamp?
    int min_timestamp = global_timestamp;
    int idx_evict = -1;
    for (int i = 0; i < associativity; i++) {
        if (cache[set_index][i].timestamp <= min_timestamp){
            min_timestamp = cache[set_index][i].timestamp;
            idx_evict = i;
        }
    }

    // need to find the address in memory of the block to evict
    int block_evict_address = cache[set_index][idx_evict].tag * power_of_two(idx_bits+offset_bits) + set_index * power_of_two(offset_bits);
    //printf("tag %d, set_index %d, ", cache[set_index][idx_evict].tag, );

    // so we got the idx to evict; check if block is dirty
    // replacement of dirty block
    if (cache[set_index][idx_evict].dirty == 1){

        // update the evicted block's data in memory
        for (int i = 0; i<block_size; i++){
            memory[block_evict_address + i] =  cache[set_index][idx_evict].data[i];
        }

        // fetch whole new block from memory and put it in the evicted block's place
        int first_byte_address = tag * power_of_two(idx_bits+offset_bits) + set_index * power_of_two(offset_bits);

        for (int j = 0; j < block_size; j++) {
            cache[set_index][idx_evict].data[j] = memory[first_byte_address + j]; 
        }  

        cache[set_index][idx_evict].valid = 1;
        cache[set_index][idx_evict].tag = tag;
        cache[set_index][idx_evict].dirty = 0;
        cache[set_index][idx_evict].timestamp = global_timestamp;

        printf("replacement 0x%x dirty\n", block_evict_address);

        printf("load 0x%x miss ", address);
        // print the data
        for (int j = 0; j < access_size; j++) {
            printf("%02hhx", cache[set_index][idx_evict].data[block_offset + j]);
        }
        printf("\n");
        //printf("C\n");  

        return;
    }

    // replacement of clean block

    // the first of the block we are loading
    int first_byte_address = tag * power_of_two(idx_bits+offset_bits) + set_index * power_of_two(offset_bits);

    // fetch whole new block from memory and put it in the evicted block's place
    for (int j = 0; j < block_size; j++) {
        cache[set_index][idx_evict].data[j] = memory[first_byte_address + j]; 
    }  


    cache[set_index][idx_evict].valid = 1;
    cache[set_index][idx_evict].tag = tag;
    cache[set_index][idx_evict].dirty = 0;
    cache[set_index][idx_evict].timestamp = global_timestamp;

    printf("replacement 0x%x clean\n", block_evict_address);

    printf("load 0x%x miss ", address);
    // print the data
    for (int j = 0; j < access_size; j++) {
        printf("%02hhx", cache[set_index][idx_evict].data[block_offset + j]);
    }
    printf("\n");  
    //printf("D\n");


    return;

};



///////////////////////////////////////////////////////////// MAIN


int main(int argc, char *argv[]) {

    char *tracefile = argv[1];  
    int cache_size = atoi(argv[2]);
    int associativity = atoi(argv[3]);
    int block_size = atoi(argv[4]);

    int num_sets = 1024 * cache_size / (block_size * associativity);
    //printf("num sets: %d\n", num_sets);

    int offset_bits = log2(block_size);
    int index_bits = log2((cache_size * 1024) / (associativity*block_size)); // *1024 bc it's kB
    int tag_bits = 24 - index_bits - offset_bits;

    // printf("offset bits: %d\n", offset_bits);
    // printf("index bits: %d\n", index_bits);
    // printf("tag bits: %d\n", tag_bits);

    // cache as 2D array
    Block** cache = (Block**) malloc(num_sets * sizeof(Block*));
    for (int i = 0; i < num_sets; i++) {
        cache[i] = (Block*) malloc(associativity * sizeof(Block));
    }

    // initialize cache; everything to zero for every element in each block in the cache
    for (int i = 0; i < num_sets; i++) {
        for (int j = 0; j < associativity; j++) {
            // initialize all block elements to zero
            cache[i][j].dirty = 0;
            cache[i][j].valid = 0;
            cache[i][j].tag = -1;
            cache[i][j].timestamp = 0;
            //printf("test %d", cache[i][j].timestamp);
        }
    }

    // read tracefile
    FILE *file = fopen(tracefile, "r");

    unsigned int address;
    char access_type[10]; // either load or store
    int access_size; // how many bytes to load or store

    int tag;
    int set_index;
    int block_offset;

    char line[100]; // assuming maximum line length is 100 characters
    while (fgets(line, sizeof(line), file) != NULL) {
        // printf("%s", line);

        // increment global timestamp after reading each line/instruction
        global_timestamp += 1;

        sscanf(line, "%s", access_type);
        //printf("Access Type: %s\n", access_type);

        // if the instruction is a store
        if (strcmp(access_type, "store") == 0) {
            sscanf(line, "%*s %x %d", &address, &access_size);

            block_offset = address & ones(offset_bits);
            set_index = (address >> offset_bits) & ones(index_bits);
            tag = address >> (offset_bits + index_bits);

            // read the data
            unsigned char data[access_size]; 
            unsigned int hex_data;  

            // move the pointer to the hex value 
            char* hex_value = strchr(line, ' ') + 1;
            hex_value = strchr(hex_value, ' ') + 1; // Move to the next space (skipping the address)
            hex_value = strchr(hex_value, ' ') + 1; // Move to the next space (skipping the access size)

            //printf("Hex value: %s\n", hex_value); // it's read correctly

            for (int i = 0; i < access_size; i++) {
                sscanf(hex_value + 2 * i, "%2hhx", &data[i]);
            }

            // execute the store instruction
            execute_store(cache, address, block_offset, set_index, tag, data, access_size, associativity, block_size, index_bits, offset_bits);
        } 


        // if the instruction is a load
        else if (strcmp(access_type, "load") == 0) {
            sscanf(line, "%*s %x %d", &address, &access_size);
            // printf("address: 0x%x\n", address);
            // printf("access size: %d\n", access_size);

            block_offset = address & ones(offset_bits);
            set_index = (address >> offset_bits) & ones(index_bits);
            tag = address >> (offset_bits + index_bits);

            //printf("set index %d\n", set_index);

            // exectute the load instruction
            execute_load(cache, address, block_offset, set_index, tag, access_size, associativity, block_size, index_bits, offset_bits);
        }
    }

    fclose(file);

    // free memory for the 2D cache array
    for (int i = 0; i < num_sets; i++) {
        free(cache[i]);
    }
    free(cache);

    return 0; 
}
