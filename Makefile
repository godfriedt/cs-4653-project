CC = gcc
CFLAGS = -g -Wall
LFLAGS = -L./lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
OBJECTS = main.o cards.o drawing.o gameloop.o

clean_build: clean all

all: $(OBJECTS)
	$(CC) $^ $(LFLAGS)  -o reveng 
	rm -rf *.o
	./reveng

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm *.o || true
