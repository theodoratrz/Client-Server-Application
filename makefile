#paths
BIN = ./bin
MODULES = ./src
INCLUDE = ./include

#flags
CC = gcc
CFLAGS = -g3 -Wall -I$(INCLUDE)

#Programs
PROG = Client
PROG1 = Server
SRCS = Client.c checkArguments.c queueList.c
SRCS1 = Server.c checkArguments.c queueList.c

all: $(BIN)/$(PROG) $(BIN)/$(PROG1)

#Compilation rules
$(BIN)/Client.o: $(MODULES)/Client.c $(INCLUDE)/checkArguments.h $(INCLUDE)/queueList.h
	$(CC) $(CFLAGS) -c $(MODULES)/Client.c -o $(BIN)/Client.o

$(BIN)/Server.o: $(MODULES)/Server.c $(INCLUDE)/checkArguments.h $(INCLUDE)/queueList.h 
	$(CC) $(CFLAGS) -c $(MODULES)/Server.c -o $(BIN)/Server.o

$(BIN)/checkArguments.o: $(MODULES)/checkArguments.c $(INCLUDE)/checkArguments.h
	$(CC) $(CFLAGS) -c $(MODULES)/checkArguments.c -o $(BIN)/checkArguments.o

$(BIN)/queueList.o: $(MODULES)/queueList.c $(INCLUDE)/queueList.h 
	$(CC) $(CFLAGS) -c $(MODULES)/queueList.c -o $(BIN)/queueList.o

#Linking rules
$(BIN)/$(PROG): $(BIN)/Client.o $(BIN)/checkArguments.o $(BIN)/queueList.o 
	$(CC) $(BIN)/Client.o $(BIN)/checkArguments.o $(BIN)/queueList.o  -o $(BIN)/$(PROG) $(CFLAGS) -lpthread

$(BIN)/$(PROG1): $(BIN)/Server.o $(BIN)/checkArguments.o $(BIN)/queueList.o 
	$(CC) $(BIN)/Server.o $(BIN)/checkArguments.o $(BIN)/queueList.o  -o $(BIN)/$(PROG1) $(CFLAGS) -lpthread

#Clean rule
clean:
	rm -rf $(BIN)/*.o $(BIN)/$(PROG) $(BIN)/$(PROG1)

