MF=	Makefile
IDIR =./include
CC=mpicc 
CFLAGS=-I$(IDIR) -lm

ODIR=build/obj
OFILES=\
	$(ODIR)/ran2.o\
	$(ODIR)/squirrel-functions.o\
	$(ODIR)/main.o
	
OFILES2=\
	$(ODIR)/pool.o\
	$(ODIR)/test.o

all: build/squirrels build/pool	

$(ODIR)/%.o: src/%.c 	
	mkdir -p build/obj
	$(CC) $(CFLAGS) -c $< -o $@

build/squirrels: $(OFILES)
	$(CC) $(CFLAGS)  $^ -o $@

build/pool: $(OFILES2) 
	$(CC) $(CFLAGS)  $^ -o $@



clean:
	rm -rf build
	