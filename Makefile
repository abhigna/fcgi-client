CC	= gcc
DEFS = 
CFLAGS	=  -g -Wall
INCDIR = ./include
OBJ = fcgi.o fcgi_header.o

bin/fcgi: $(OBJ)
	$(CC) $(CFLAGS) $(DEFS) $(LDFLAGS) -o $@ $(OBJ)  $(LIBS)

clean:
	rm -rf *.o
	rm bin/fcgi
.c.o:
	$(CC) -c $(CFLAGS) $(DEFS) -I$(INCDIR) $<
