CC=g++
CPPFLAGS=-g `freetype-config --cflags`

test-hexfml: test-hexfml.o hexfml.o anisprite.o
	$(CC) $(CPPFLAGS) -lsfml-system -lsfml-graphics $^ -o $@

test-coords: test-coords.o hexfml.o
	$(CC) $(CPPFLAGS) -lsfml-system -lsfml-graphics $^ -o $@

test-typesetter: test-typesetter.o typesetter.o
	$(CC) -lsfml-graphics `freetype-config --libs` $(CPPFLAGS) $^ -o $@
