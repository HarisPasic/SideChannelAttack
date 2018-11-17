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

size_t kpause = 0;

int nb_hits = 0;
int nb_miss = 0;
int mut = 0; // boolean
size_t beginTime;
char lastEightBits[9] = {'0','0','0','0','0','0','0','0','\0'};
int cpt = 0.0;

void reset() {
  nb_hits = 0;   
  nb_miss = 0;  
}

void flushandreload(void* addr)
{
  size_t time = rdtsc();
  maccess(addr);
  size_t delta = rdtsc() - time;
  flush(addr);
  if (delta < MIN_CACHE_MISS_CYCLES) {
    if (kpause > 0) {      
      nb_hits++;
      // printf("%lu: Cache Hit (%lu cycles) after a pause of %lu cycles\n", time, delta, kpause);
    }
    kpause = 0;
  }
  else {
    kpause++;
    nb_miss++;
  }  
}

long file_size(const char *filename) {
   struct stat s; 
   if (stat(filename,&s) != 0) {
      printf("Error reading file stats !\n" );
      return 0;
   } 
   return s.st_size; 
}

int main(int argc, char** argv) {
  char* name = argv[1];
  char* offsetp = argv[2];

  if (argc != 3) {    
    printf("usage: ./receiver file offset\n");
    printf("example: ./receiver chaton.jpeg 0x00\n");
    return 1;
  }

  unsigned int offset = 0;
  !sscanf(offsetp,"%x",&offset);
  int fd = open(name,O_RDONLY);

  if (fd < 3) {
    printf("error: failed to open file\n");
    return 2;
  }

  unsigned char* addr = (unsigned char*) mmap(0, file_size(name), PROT_READ, MAP_SHARED, fd, 0);
  if (addr == (void*)-1 || addr == (void*)0) {
    printf("error: failed to mmap\n");
    return 2;
  }
  
	// Display the last eight bits...
  while(1) {    

    if(((int)(rdtsc() / SYNCING_CYCLE)) % 10 == 0)  { // IN SYNC
      if(!mut) {
				mut = 1;
        reset();
      }      
    } else { // NOT IN SYNC      
      if(mut) {
        mut = 0;
        double mean = 0;      
        if(nb_miss > 0) mean = nb_hits / ((double) nb_miss);
        printf("NbHits: %d, NbMiss: %d, Mean: %f \n", nb_hits, nb_miss, mean);
        if(mean > 0.67) lastEightBits[cpt%8] = '1';
				else 						lastEightBits[cpt%8] = '0';
        cpt++;
				if (cpt == 800000) cpt = 0;
				printf("[%s]\n", lastEightBits);
      }
    }

    flushandreload(addr + offset);  

    for (int i = 0; i < 3; ++i) sched_yield();
  }
  return 0;
}
