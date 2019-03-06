MF=	Makefile
IDIR =./include
CC=icc 
CFLAGS=-I$(IDIR)

ODIR=build/obj
OFILES=\
	$(ODIR)/ran2.o\
	$(ODIR)/squirrel-functions.o\
	$(ODIR)/main.o
	

$(ODIR)/%.o: src/%.c 	
	mkdir -p build/obj
	$(CC) $(CFLAGS) -c $< -o $@

build/squirrels: $(OFILES)
	$(CC) $(CFLAGS)  $^ -o $@



all: build/squirrels

clean:
	rm -rf build
	