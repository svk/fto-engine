CC=g++
CPPFLAGS=-g

test-hexfml: test-hexfml.o hexfml.o
	$(CC) $(CPPFLAGS) -lsfml-system -lsfml-graphics $^ -o $@

test-coords: test-coords.o hexfml.o
	$(CC) $(CPPFLAGS) -lsfml-system -lsfml-graphics $^ -o $@
