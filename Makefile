# CS 241
# University of Illinois

CC = gcc
INC = -I.
FLAGS = -g -W -Wall
LIBS = -lpthread

all: client server 

client: client.o
	$(CC) $(FLAGS) $(INC) $^ -o $@ $(LIBS)
	
server: server.o
	$(CC) $(FLAGS) $(INC) $^ -o $@ $(LIBS)
	
client.o: client.c
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)

server.o: server.c
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)
	
clean:
	$(RM) *.o client server
