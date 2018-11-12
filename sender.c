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
  char* value = argv[3];
  char* length = argv[4];

  if (argc != 5)
  {    
    printf("  usage: ./sender file offset value valueLength\n"
                 "example: ./sender chaton.jpeg 0x00 100110 6\n");
    return 1;
  }

  unsigned int offset = 0;
  !sscanf(offsetp,"%x",&offset);

  unsigned int len = 0;
  !sscanf(length,"%d",&len);

  int fd = open(name,O_RDONLY);
  if (fd < 3)
  {
    printf("error: failed to open file\n");
    return 2;
  }

  unsigned char* addr = (unsigned char*) mmap(0, file_size(name), PROT_READ, MAP_SHARED, fd, 0);

  if (addr == (void*)-1 || addr == (void*)0)
  {
    printf("error: failed to mmap\n");
    return 2;
  }

  int r;
  size_t time;

  // infinite loop

  for(int i = 0; i < len; ++i) {
    
    do {
      time = rdtsc();    
      r = time / 10000000000;
    }
    while(r%10!=0);

    if(value[i] == '1') while(rdtsc() - time < 10000000) maccess(addr + offset);
    else                while(rdtsc() - time < 10000000);
  }
  
  return 0;
}
