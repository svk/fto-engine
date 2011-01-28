CC=g++
CPPFLAGS=-g -I/usr/include/freetype2 -Wall

SFML_LIBS=-lsfml-system -lsfml-graphics -lsfml-audio
CORE_LIBS=`freetype-config --libs` -lboost_filesystem -lboost_program_options -lssl
LIBS=$(SFML_LIBS) $(CORE_LIBS)

EXECUTABLES=test-hexfml test-coords test-typesetter test-sexp test-sisenet test-sftools spserver spclient spguient

all: $(EXECUTABLES)

clean:
	rm -f *.o
	rm -f $(EXECUTABLES)

test-sftools: test-sftools.o hexfml.o HexTools.o anisprite.o typesetter.o sftools.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-hexfml: test-hexfml.o HexTools.o hexfml.o anisprite.o typesetter.o sftools.o HtGo.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-coords: test-coords.o hexfml.o HexTools.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-typesetter: test-typesetter.o typesetter.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-sexp: test-sexp.o Sise.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-sisenet: test-sisenet.o Sise.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

spserver: Sise.o spserver.o SProto.o myabort.o Nash.o NashServer.o HexTools.o
	$(CC) $(CPPFLAGS) $(CORE_LIBS) $^ -o $@

spclient: spclient.o Sise.o SProto.o myabort.o
	$(CC) $(CPPFLAGS) $(CORE_LIBS) $^ -o $@

spguient: spguient.o Sise.o SProto.o typesetter.o sftools.o myabort.o Nash.o NashClient.o sftools.o hexfml.o HexTools.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@