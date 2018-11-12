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

size_t kpause = 0;

int nb_hits = 0;
int nb_miss = 0;
int firstHit = 0; // bool
size_t beginTime;
char message[9];
int cpt = 0;

void flushandreload(void* addr)
{
  size_t time = rdtsc();
  maccess(addr);
  size_t delta = rdtsc() - time;
  flush(addr);
  if (delta < MIN_CACHE_MISS_CYCLES)
  {
    if (kpause > 0)
    {
      
       if(!firstHit) {
         firstHit = 1;
         beginTime = rdtsc();
         nb_hits = 0;   
         nb_miss = 0;   
       }

        nb_hits++;     

      printf("%lu: Cache Hit (%lu cycles) after a pause of %lu cycles\n", time, delta, kpause);
    }
    kpause = 0;
  }
  else {
    kpause++;
    nb_miss++;
  }
  if(firstHit && rdtsc() - beginTime >= 10000000) {
      double mean = nb_hits/(nb_miss*1.0);
      printf("NbHits: %d, NbMiss: %d, Mean: %f \n", nb_hits, nb_miss, mean);
      if(cpt < 8) {        
        if(mean > 0.7) message[cpt] = 1;
        else message[cpt] = 0;
        cpt++;
        if(cpt==8) {
          message[cpt] = '\0';
          printf("%s", message);
        }
      }      
      firstHit = 0;
      nb_hits = 0;   
      nb_miss = 0;   
  }
}

long file_size(const char *filename)
{
   struct stat s;
 
   if  (stat(filename,&s) != 0) {
      printf("error!\n" );
      return 0;
   }
 
   return s.st_size;
}

int main(int argc, char** argv)
{
  char* name = argv[1];
  char* offsetp = argv[2];
  if (argc != 3)
  {    
    printf("  usage: ./receiver file offset\n"
                 "example: ./receiver chaton.jpeg 0x00\n");
    return 1;
  }
  unsigned int offset = 0;
  !sscanf(offsetp,"%x",&offset);
  int fd = open(name,O_RDONLY);
  if (fd < 3)
  {
    printf("error: failed to open file\n");
    return 2;
  }

  unsigned char* addr = (unsigned char*)mmap(0, file_size(name), PROT_READ, MAP_SHARED, fd, 0);
  if (addr == (void*)-1 || addr == (void*)0)
  {
    printf("error: failed to mmap\n");
    return 2;
  }
  
  size_t time;
  int r;

  while(1)
  {
    time = rdtsc();    
    r = time / 10000000000;
    if(r % 10 == 0) {
      flushandreload(addr + offset);
    }    
    for (int i = 0; i < 3; ++i)
      sched_yield();
  }
  return 0;
}
