#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdlib.h>
#include <stdint.h>
#include "./cacheutils.h"

#define MIN_CACHE_MISS_CYCLES (210)
#define MASK 1<<23
#define MAX_MESSAGE_SIZE 128
#define MAX_MESSAGE_BITS_SIZE 1024

// 8 * 512 = 4096

long file_size(const char *filename) {
   struct stat s; 
   if (stat(filename,&s) != 0) {
      printf("Error reading file stats !\n");
      return 2;
   } 
   return s.st_size; 
}

int readFileContent(char *message, char *fileName) {
	int c;
	int ctr_characters = 0;
	FILE *fp = fopen(fileName, "r");

	if(fp == NULL) {
		printf("error: failed to open fileWithData\n");
    return -1;
	} 

	while ((c = fgetc(fp)) != EOF) {
		if(ctr_characters == MAX_MESSAGE_SIZE) break;
		message[ctr_characters] = (char) c;
		++ctr_characters;
	}
 	fclose(fp);
 
	return ctr_characters;
}

int main(int argc, char** argv) {

	if (argc != 4) {    
    printf("usage: ./sender fileForMmap offset fileWithData\n");
    printf("example: ./sender chaton.jpeg 0x00 data.txt\n");
    return 1;
  }

  char* fileForMmap = argv[1];
  char* offsetString = argv[2];
	char* fileWithData = argv[3];

  unsigned int offset = 0;
  !sscanf(offsetString, "%x", &offset);

  int fd = open(fileForMmap, O_RDONLY);
  if (fd < 3) {
    printf("error: failed to open fileForMmap\n");
    return 2;
  }

  unsigned char* addr = (unsigned char*) mmap(0, file_size(fileForMmap), PROT_READ, MAP_SHARED, fd, 0);
  if (addr == (void*) -1 || addr == (void*) 0) {
    printf("error: failed to mmap\n");
    return 2;
  }
	
	// READING DATA FROM FILE
	char message[MAX_MESSAGE_SIZE];
	int message_size = readFileContent(message, fileWithData);
	if(message_size == -1) return 2; // ERROR WHILE READING

	// BINARY REPRESENTATION OF THE DATA
	int message_bits[MAX_MESSAGE_BITS_SIZE];
	int message_bits_size = message_size * 8;
	for(int i = 0; i < message_size; ++i) {
		int ind = i * 8;
		for( int j = 7; j >= 0; --j ) message_bits[ind + (7-j)] = (( message[i] >> j ) & 1 ? 1 : 0);
	}	
	for(int i = message_bits_size; i < MAX_MESSAGE_BITS_SIZE; ++i) message_bits[i] = 0;

	// for(int i = 0; i < MAX_MESSAGE_BITS_SIZE; ++i) printf("%d", message_bits[i]);
	

	int mut = 0; // MUTEX	
  int i = 0; // COUNTER
	size_t mask = MASK;	
	
	int init_message[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
	int last_message[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	// int message[16] = {1,1,0,1,0,0,0,1,1,0,1,0,0,1,1,1}; // 1101000110100111		 
	
  /*
	int mod = 1;
	// Sending 10101010101...
	while(1) {
		if((rdtsc() & mask) == 0)  { // IN SYNC
			if(!mut) mut = 1;			
      if(mod) maccess(addr + offset);
			// printf("%d\n",i);			
    } else {
			if(mut) {
				i++;
				if(i == 2000000000) i = 0;
				if(i % 2 == 0) mod = 1;
				else mod = 0;
				mut = 0;		
			}
		}		
  }

	*/
	
	int step = 0;

	// sleep(2);

  while(1) {
		// &printf("step : %d , i: %d \n", step, i);
		if((rdtsc() & mask) == 0)  { // IN SYNC
			if(!mut) mut = 1;
			if(step == 0 && init_message[i]) maccess(addr + offset);
			if(step == 1 && message_bits[i]) maccess(addr + offset);      
			// printf("%d\n",i);			
    } else {
			if(mut) {
				i++;
				if(step == 0 && i == 16) {
					step = 1;
					i = 0;
				}
				if(i == MAX_MESSAGE_BITS_SIZE) break;
				mut = 0;		
			}
		}		
  }

  return 0;
}
