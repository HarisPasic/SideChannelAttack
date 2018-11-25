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
#include "./queue.h"

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

	int step = 0; // 1: init received, 2: creating file
	int	message_bits[MAX_MESSAGE_BITS_SIZE];
	time_t time_of_begin_tag;

	int consecutive1 = 0;
  while(1) {    

		//printf("STEP %d, consecutive1 : %d, ctp %ld\n", step, consecutive1, cpt);

		if(step == 2) {
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

			printf("\n<<<END>>>\n");
			sleep(2);
			step = 0;
		} else {
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
      
		      if(mean > THRESHOLD) {
						if(step == 1) message_bits[cpt] = 1;
						else consecutive1++;
						lastBits[cpt % size] = '1';
						
					} else {
						if(step == 1) message_bits[cpt] = 0;
						else if(consecutive1 > 0) consecutive1--;
						lastBits[cpt % size] = '0';						
					}

		      cpt++;
					
					if (cpt == MAX_MESSAGE_BITS_SIZE) {
						cpt = 0;
						if(step == 1) step = 2;
					}

					if(consecutive1 == 13) { // BEGINNING OF THE COMMUNICATION
						printf("GOT TAG \\o/ ! \n");
						time_of_begin_tag = time(NULL);
						consecutive1 = 0;
						cpt = 0;
						step = 1;
					}

					//printf("Message: [%s], NbHits: %d, NbMiss: %d, Mean: %f\n", lastBits, nb_hits, nb_miss, mean);
		    }
		  }
		}

    flushandreload(addr + offset);  

    for (int i = 0; i < 3; ++i) sched_yield();
  }

  return 0;
}
