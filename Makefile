CC = gcc
CCFLAGS = -Wall

BINDIR = bin
BINNAME = $(BINDIR)/cid
OBJECTS = $(shell ls -1 | grep .*.c | cut -d'.' -f 1 | sed 's/$$/.o/g')

$(BINDIR):
	mkdir $(BINDIR)

$(BINNAME): $(OBJECTS) $(BINDIR)
	$(CC) -o $(BINNAME) $(OBJECTS)

run: $(BINNAME)
	./$(BINNAME)

clean:
	rm -rf $(OBJECTS) $(BINDIR)
