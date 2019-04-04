MF=	Makefile
IDIR =./include
CC=mpicc 
CFLAGS=-I$(IDIR) -g -Wall -Ofast 

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
	$(ODIR)/actor.o\
	$(ODIR)/argtable3.o\
	$(ODIR)/squirrel.o\
	$(ODIR)/cell.o\
	$(ODIR)/globalClock.o\
	$(ODIR)/simulation.o\


all: build/testPool	 build/squirrels2

qsub:
	qsub scripts/squirrels.pbs

validation:
	./scripts/validation.sh

run:
	mpirun -n 220 ./build/squirrels2

$(ODIR)/%.o: src/%.c 
	mkdir -p build/obj
	$(CC) $(CFLAGS) -c $< -o $@ -lm



build/squirrels2:$(OFILES3)
	$(CC) $(CFLAGS)  $^ -o $@ -lm

build/testPool: $(OFILES2) 
	$(CC) $(CFLAGS)  $^ -o $@ -lm



clean:
	rm -rf build
	
