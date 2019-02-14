CC=gcc
CFLAHS=-I.
DEPS = task.h
LIBS = -lm -lpthread `allegro-config --cflags --libs`
OBJ = main.o task.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $/*.o *~ core $(INCDIR)/*~
