
CC := gcc
CFLAGS := -Wall -g

all: configurator filler reader 

configurator: configurator.c
	$(CC) $(CFLAGS) -o configurator configurator.c

filler: filler.c
	$(CC) $(CFLAGS) -o filler filler.c

reader: reader.c
	$(CC) $(CFLAGS) -o reader reader.c


clean:
	rm -f configurator filler reader inter
