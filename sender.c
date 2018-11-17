#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdint.h>
#include "./cacheutils.h"

#define MIN_CACHE_MISS_CYCLES (210)
#define SYNCING_CYCLE 1000000000.

long file_size(const char *filename) {
   struct stat s; 
   if (stat(filename,&s) != 0) {
      printf("Error reading file stats !\n");
      return 0;
   } 
   return s.st_size; 
}

int main(int argc, char** argv) {
  char* name = argv[1];
  char* offsetp = argv[2];
  char* value = argv[3];
  char* length = argv[4];

  if (argc != 5) {    
    printf("usage: ./sender file offset value valueLength\n");
    printf("example: ./sender chaton.jpeg 0x00 100110 6\n");
    return 1;
  }

  unsigned int offset = 0;
  !sscanf(offsetp,"%x",&offset);

  unsigned int len = 0;
  !sscanf(length,"%d",&len);

  int fd = open(name,O_RDONLY);
  if (fd < 3) {
    printf("error: failed to open file\n");
    return 2;
  }

  unsigned char* addr = (unsigned char*) mmap(0, file_size(name), PROT_READ, MAP_SHARED, fd, 0);

  if (addr == (void*) -1 || addr == (void*) 0) {
    printf("error: failed to mmap\n");
    return 2;
  }

	int mut = 0;
	int mod2 = 1;
  int i = 0;
	// Sending 10101010101...
  while(1) {
    if(((int)(rdtsc() / SYNCING_CYCLE)) % 10 == 0)  { // IN SYNC
			if(!mut) mut = 1;			
      if(mod2) maccess(addr + offset);
			// printf("%d\n",i);			
    } else {
			if(mut) {
				i++;
				if(i == 2000000000) i = 0;
				if(i % 2 == 0) mod2 = 1;
				else mod2 = 0;
				mut = 0;		
			}
		}		
  }
  return 0;
}
