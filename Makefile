MF=	Makefile
IDIR =./include
CC=mpicc 
CFLAGS=-I$(IDIR) -lm -g -Wall

ODIR=build/obj
OFILES=\
	$(ODIR)/ran2.o\
	$(ODIR)/squirrel-functions.o\
	$(ODIR)/main.o\
	$(ODIR)/pool.o

	
OFILES2=\
	$(ODIR)/pool.o\
	$(ODIR)/test.o

OFILES3=\
	$(ODIR)/ran2.o\
	$(ODIR)/squirrel-functions.o\
	$(ODIR)/main2.o\
	$(ODIR)/pool.o\
	$(ODIR)/actor.o

all: build/squirrels build/testPool	 build/squirrels2

$(ODIR)/%.o: src/%.c	
	mkdir -p build/obj
	$(CC) $(CFLAGS) -c $< -o $@

build/squirrels: $(OFILES)
	$(CC) $(CFLAGS)  $^ -o $@

build/squirrels2:$(OFILES3)
	$(CC) $(CFLAGS)  $^ -o $@

build/testPool: $(OFILES2) 
	$(CC) $(CFLAGS)  $^ -o $@



clean:
	rm -rf build
	