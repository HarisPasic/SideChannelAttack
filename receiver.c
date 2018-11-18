#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdint.h>
#include "./cacheutils.h"
#include "./queue.h"

#define MIN_CACHE_MISS_CYCLES (210)
#define MASK 1<<23

// MASK 1<<23 : 148 bits/s

size_t kpause = 0;

int nb_hits = 0;
int nb_miss = 0;
int mut = 0; // boolean
size_t beginTime;
int size = 16;
size_t cpt = 0;

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

	char lastBits[size+1];

	for(int i = 0; i < size; i++) {
		lastBits[i] = '0';
	}

	lastBits[size] = '\0';
	size_t mask = MASK;
  
	
  /*
	// Display the last [size=16] bits...
	while(1) {    
		
    // if(((int)(rdtsc() / SYNCING_CYCLE)) % 10 == 0)  { // IN SYNC
    if((rdtsc() & mask) == 0)  { // IN SYNC
      if(!mut) {
				mut = 1;
        reset();
      }      
    } else { // NOT IN SYNC      
      if(mut) {
        mut = 0;
        double mean = nb_hits;      
        if(nb_miss != 0) mean = nb_hits / ((double) nb_miss);        
        if(mean > 0.15) lastBits[cpt % size] = '1';
				else 					 lastBits[cpt % size] = '0';
        cpt++;
				if (cpt == 8000) cpt = 0;
				printf("Message: [%s], NbHits: %d, NbMiss: %d, Mean: %f\n", lastBits, nb_hits, nb_miss, mean);
      }
    }

    flushandreload(addr + offset);  

    for (int i = 0; i < 3; ++i) sched_yield();
  }

	*/


	// 16 consecutives "1" => beginning
	int consecutive1 = 0;
  while(1) {    
		
    // if(((int)(rdtsc() / SYNCING_CYCLE)) % 10 == 0)  { // IN SYNC
    if((rdtsc() & mask) == 0)  { // IN SYNC
      if(!mut) {
				mut = 1;
        reset();
      }      
    } else { // NOT IN SYNC      
      if(mut) {
        mut = 0;				

        double mean = nb_hits;      
        if(nb_miss != 0) mean = nb_hits / ((double) nb_miss);        
        if(mean > 0.15) {
					lastBits[cpt % size] = '1';
					consecutive1++;
				}
				else {
					lastBits[cpt % size] = '0';
					consecutive1 = 0;
				}

        cpt++;
				if (cpt == 8000) cpt = 0;

				if(consecutive1 == 16) {
					cpt = 0;
				}

				printf("Message: [%s], NbHits: %d, NbMiss: %d, Mean: %f\n", lastBits, nb_hits, nb_miss, mean);
      }
    }

    flushandreload(addr + offset);  

    for (int i = 0; i < 3; ++i) sched_yield();
  }

  return 0;
}
