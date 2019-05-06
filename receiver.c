#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdint.h>
#include <time.h>
#include "./cacheutils.h"

size_t kpause = 0;
int nb_hits = 0;
int nb_miss = 0;

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

	int step = 0; // 1: init received, 2: display message
	int	message_bits[MAX_MESSAGE_BITS_SIZE];
	time_t time_of_begin_tag;

	int bit_1_count = 0;
  size_t bit_count = 0;
  int mut = 0; // mutex (boolean)
  while(1) {    

		if(step == 2) { // END(MESSAGE)
			time_t diff = time(NULL) - time_of_begin_tag;

			printf("Duration : %lu seconds\n", diff);
			printf("Speed    : %.2f bits per second\n", ((float)MAX_MESSAGE_BITS_SIZE) / diff);
			printf("Message  :\n<<<BEGIN>>>\n");
      
      int toInc = BYTE_SIZE * REDUNDANCY;
			for(int i = 0; i < MAX_MESSAGE_BITS_SIZE; i += toInc) {
        int ch = 0; // char
        for(int j = 0; j < BYTE_SIZE; ++j) {
            int nbOf1 = 0;
            int indJ = j * BYTE_SIZE;
            for(int k = 0; k < REDUNDANCY; ++k) {
              if(message_bits[i + indJ + k]) nbOf1++;
            }
            if(nbOf1 > 4) ch += (1<<(7-j));
        }
        printf("%c", (char) ch);        
			}

			printf("\n<<<END>>>\n\n");

			step = 0;
      bit_count = 0;
		} else {
		  if((rdtsc() & MASK) == 0)  { // IN SYNC
		    if(!mut) {
					mut = 1;
		      reset();
		    }      
		  } else { // NOT IN SYNC      
		    if(mut) {
		      mut = 0;				

		      double mean = nb_hits;      
		      if(nb_miss != 0) mean = nb_hits / ((double) nb_miss);  
      
		      if(mean > THRESHOLD) {
						if (step) message_bits[bit_count] = 1;
						else bit_1_count++;						
					} else {
						if (step) message_bits[bit_count] = 0;
						else if(bit_1_count > 0) bit_1_count--;					
					}

		      bit_count++;
					
					if (bit_count == MAX_MESSAGE_BITS_SIZE && step) step = 2;

					if(bit_1_count == 13) { // BEGIN(MESSAGE)
						printf("GOT TAG \\o/ ! \n");
						time_of_begin_tag = time(NULL);
						bit_1_count = 0;
						bit_count = 0;
						step = 1;
					}
		    }
		  }
		}

    flushandreload(addr + offset);  

    for (int i = 0; i < 3; ++i) sched_yield();
  }

  return 0;
}
