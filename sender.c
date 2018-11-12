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

int main(int argc, char** argv)
{
  char* name = argv[1];
  char* offsetp = argv[2];
  char* valuesetp = argv[3]; // 1 ou 0

  if (argc != 4)
  {    
    printf("  usage: ./sender file offset value\n"
                 "example: ./sender chaton.jpeg 0x00 1\n");
    return 1;
  }

  unsigned int offset = 0;
  unsigned int value = 0;
  !sscanf(offsetp,"%x",&offset);
  !sscanf(valuesetp,"%d",&value);

  int fd = open(name,O_RDONLY);
  if (fd < 3)
  {
    printf("error: failed to open file\n");
    return 2;
  }

  unsigned char* addr = (unsigned char*)mmap(0, 64*1024*1024, PROT_READ, MAP_SHARED, fd, 0);

  if (addr == (void*)-1 || addr == (void*)0)
  {
    printf("error: failed to mmap\n");
    return 2;
  }

  if(value) {
    size_t time = rdtsc(); 
    printf("Sending... \n");
    while(rdtsc() - time < 10000000000) maccess(addr + offset);
  }
  
  return 0;
}
