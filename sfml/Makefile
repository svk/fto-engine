CC=g++
CPPFLAGS=-g `freetype-config --cflags` -Wall

SFML_LIBS=-lsfml-system -lsfml-graphics -lsfml-audio
CORE_LIBS=`freetype-config --libs` -lboost_filesystem -lboost_program_options -lssl
LIBS=$(SFML_LIBS) $(CORE_LIBS)

EXECUTABLES=test-hexfml test-coords test-typesetter test-sexp test-sisenet test-sftools spserver

all: $(EXECUTABLES)

clean:
	rm -f *.o
	rm -f $(EXECUTABLES)

test-sftools: test-sftools.o hexfml.o anisprite.o typesetter.o sftools.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-hexfml: test-hexfml.o hexfml.o anisprite.o typesetter.o sftools.o HtGo.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-coords: test-coords.o hexfml.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-typesetter: test-typesetter.o typesetter.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-sexp: test-sexp.o Sise.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-sisenet: test-sisenet.o Sise.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

spserver: Sise.o spserver.o SProto.o
	$(CC) $(CPPFLAGS) $(CORE_LIBS) $^ -o $@

spclient: spclient.o Sise.o SProto.o
	$(CC) $(CPPFLAGS) $(CORE_LIBS) $^ -o $@
