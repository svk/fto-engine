CC=g++
CPPFLAGS=-g -I/usr/include/freetype2 -Wall -O0

SFML_LIBS=-lsfml-system -lsfml-graphics -lsfml-audio
CORE_LIBS=-lboost_filesystem -lboost_program_options -lssl
LIBS=$(SFML_LIBS) $(CORE_LIBS) `freetype-config --libs`

EXECUTABLES=test-hexfml test-coords test-typesetter test-sexp test-sisenet test-sftools spserver spclient spguient test-hexfml test-hexplorer test-fov test-tacclient test-tacserver

all: $(EXECUTABLES)

clean:
	rm -f *.o
	rm -f $(EXECUTABLES)

test-sftools: test-sftools.o hexfml.o HexTools.o anisprite.o typesetter.o sftools.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-hexfml: test-hexfml.o HexTools.o hexfml.o anisprite.o typesetter.o sftools.o HtGo.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-hexplorer: test-hexplorer.o HexTools.o hexfml.o anisprite.o typesetter.o sftools.o HtGo.o myabort.o mtrand.o HexFov.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-coords: test-coords.o hexfml.o HexTools.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-typesetter: test-typesetter.o typesetter.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-sexp: test-sexp.o Sise.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-sisenet: test-sisenet.o Sise.o myabort.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

spserver: Sise.o spserver.o SProto.o myabort.o Nash.o NashServer.o HexTools.o HexFov.o HexTools.o myabort.o mtrand.o Tac.o TacServer.o
	$(CC) $(CPPFLAGS) $(CORE_LIBS) $^ -o $@

spclient: spclient.o Sise.o SProto.o myabort.o
	$(CC) $(CPPFLAGS) $(CORE_LIBS) $^ -o $@

spguient: spguient.o Sise.o SProto.o typesetter.o sftools.o myabort.o Nash.o NashClient.o sftools.o hexfml.o HexTools.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-fov: test-fov.o HexFov.o HexTools.o myabort.o
	$(CC) $(CPPFLAGS) $^ -o $@

test-tacclient: test-tacclient.o HexFov.o HexTools.o myabort.o TacClient.o sftools.o hexfml.o mtrand.o TacClientAction.o typesetter.o anisprite.o Tac.o Sise.o SProto.o
	$(CC) $(CPPFLAGS) $(LIBS) $^ -o $@

test-tacserver: test-tacserver.o HexFov.o HexTools.o myabort.o mtrand.o Tac.o TacServer.o
	$(CC) $(CPPFLAGS) $(CORE_LIBS) $^ -o $@
