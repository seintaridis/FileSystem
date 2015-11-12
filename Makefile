
OBJS 	= terminal.o MFS.o 
SOURCE	= terminal.o MFS.o
HEADER  = terminal.h MFS.h MDS.h
OUT  	= myfilesystem
CC	= gcc
FLAGS   = -g -c 
# -g option enables debugging mode 
# -c flag generates object code for separate files

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT)

# create/compile the individual files >>separately<< 
terminal.o: terminal.c
	$(CC) $(FLAGS) terminal.c

MFS.o: MFS.c
	$(CC) $(FLAGS) MFS.c



# clean house
clean:
	rm -f $(OBJS) $(OUT)

# do a bit of accounting
count:
	wc $(SOURCE) $(HEADER)
