CC=g++
CPPFLAGS=-g `freetype-config --cflags`

EXECUTABLES=test-hexfml test-coords test-typesetter

all: $(EXECUTABLES)

clean:
	rm -f *.o
	rm -f $(EXECUTABLES)

test-sftools: test-sftools.o hexfml.o anisprite.o typesetter.o sftools.o
	$(CC) $(CPPFLAGS) `freetype-config --libs` -lsfml-system -lsfml-graphics $^ -o $@

test-hexfml: test-hexfml.o hexfml.o anisprite.o typesetter.o sftools.o
	$(CC) $(CPPFLAGS) `freetype-config --libs` -lsfml-system -lsfml-graphics $^ -o $@

test-coords: test-coords.o hexfml.o
	$(CC) $(CPPFLAGS) -lsfml-system -lsfml-graphics $^ -o $@

test-typesetter: test-typesetter.o typesetter.o
	$(CC) -lsfml-graphics `freetype-config --libs` $(CPPFLAGS) $^ -o $@
