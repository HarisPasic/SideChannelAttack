all: receiver sender 
receiver: receiver.c ./cacheutils.h
	gcc -std=gnu11 -O2 -o $@ $@.c
sender: sender.c ./cacheutils.h
	gcc -std=gnu11 -O2 -o $@ $@.c
