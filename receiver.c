#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdint.h>
#include "./cacheutils.h"

#define MIN_CACHE_MISS_CYCLES (285)
#define SYNCING_CYCLE 1000000000.
#define WORKING_CYCLE 100000000 

size_t kpause = 0;

int nb_hits = 0;
int nb_miss = 0;
int firstHit = 0; // bool
int isSynced = 0; // bool
size_t beginTime;
char message[9];
int cpt = 0;

void reset() {
  firstHit = 0;
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
      
      if(isSynced) {
       if(!firstHit) {
         reset();
         beginTime = rdtsc();
         firstHit = 1;          
       }
       nb_hits++;     
      }

      printf("%lu: Cache Hit (%lu cycles) after a pause of %lu cycles\n", time, delta, kpause);
    }
    kpause = 0;
  }
  else {
    kpause++;
    if(isSynced) nb_miss++;
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

  while(1) {    

    if(((int)(rdtsc() / SYNCING_CYCLE)) % 10 == 0)  {isSynced = 1; reset();}
    else                                            isSynced = 0;    

    flushandreload(addr + offset);

    // printf("%d\n", isSynced);

    if(isSynced && firstHit && rdtsc() - beginTime >= WORKING_CYCLE) {
      double mean = nb_hits / ((double) nb_miss);
      printf("NbHits: %d, NbMiss: %d, Mean: %f \n", nb_hits, nb_miss, mean);
      /*if(cpt < 8) {
        if(mean > 0.7) message[cpt] = 1;
        else message[cpt] = 0;
        cpt++;
        if(cpt==8) {
          message[cpt] = '\0';
          printf("%s", message);
        }
      }*/
      reset();  
    }    

    for (int i = 0; i < 3; ++i) sched_yield();
  }
  return 0;
}
