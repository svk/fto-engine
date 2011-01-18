#ifndef H_SISE
#define H_SISE

/* This is just a convenient format for human-readable
   or semi-human-readable serialization. I won't
   (at least initially, until they're needed) be
   supporting things like bignums or rationals.
*/

/* There are a number of obvious time and space
   optimizations that could be made, but clearly
   serialization can be done much faster in C++
   in other ways anyway. Decent is the enemy of
   quick and dirty.
*/

#include <list>

#include <ostream>
#include <sstream>
#include <queue>

#include <string>

#include <stdexcept>

/* Not meant to be resistant towards malicious
   activity, vulnerable to DOS by stack overflow
   or memory exhaustion. Secure if needed by
   imposing limits & investigate less obvious things.
*/

#include <ostream>
#include <iostream>

#include <set>
#include <map>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <sys/select.h>

namespace Sise {
    struct ParseError : public std::runtime_error {
        ParseError(std::string s) : std::runtime_error(s) {}
    };

    enum Type {
        TYPE_CONS,
        TYPE_INT,
        TYPE_STRING,
        TYPE_SYMBOL
    };

    struct SExpTypeError {
        Type expected;
        Type got;

        SExpTypeError(Type,Type);
    };


    struct Cons;
    struct Int;
    struct String;
    struct Symbol;

    class SExp {
        Type type;

        protected:
            explicit SExp(Type);

        public:
            virtual ~SExp(void);

            bool isType(Type) const;
            Cons* asCons(void);
            String* asString(void);
            Int* asInt(void);
            Symbol* asSymbol(void);

            virtual void output(std::ostream&) = 0;
    };

    class Cons : public SExp {
        SExp *carPtr;
        SExp *cdrPtr;

        public:
            Cons(void);
            explicit Cons(SExp*);
            Cons(SExp*, SExp*);
            ~Cons(void);

            SExp *getcar(void) const;
            SExp *getcdr(void) const;
            void setcar(SExp*);
            void setcdr(SExp*);

            SExp *nthcar(int);
            SExp *nthcdr(int);

            void output(std::ostream&);
    };

    template <class T, Type X>
    class PodType : public SExp {
        protected:
            const T data;

        public:
            explicit PodType(T data) : SExp(X), data ( data ) {}
            T get(void) const { return data; }
            operator T(void) { return data; }
    };

    class Int : public PodType<int,TYPE_INT> {
        public:
            explicit Int(int data) : PodType<int,TYPE_INT>(data) {}

            void output(std::ostream&);
    };

    class Symbol : public PodType<std::string,TYPE_SYMBOL> {
        public:
            explicit Symbol(std::string data) : PodType<std::string,TYPE_SYMBOL>(data) {}

            void output(std::ostream&);
    };

    class String : public PodType<std::string,TYPE_STRING> {
        public:
            explicit String(std::string data) : PodType<std::string,TYPE_STRING>(data) {}

            void output(std::ostream&);
    };

    class SExpParser {
        public:
            virtual void feedEnd(void) {}
            virtual bool feed(char) = 0;
            virtual bool done(void) const = 0;
            virtual SExp *get(void) = 0;
    };

    class NumberParser : public SExpParser {
        // expects everything
        private:
            char buffer[128];
            int length;
            bool isDone;

        public:
            NumberParser(void);
            void feedEnd(void) { isDone = true; }
            bool feed(char);
            bool done(void) const;
            SExp *get(void);
    };

    class SymbolParser : public SExpParser {
        // expects initial " cut off
        private:
            bool isDone;
            std::ostringstream oss;

        public:
            SymbolParser(void);

            void feedEnd(void) { isDone = true; }
            bool feed(char);
            bool done(void) const;
            SExp *get(void);
    };

    class StringParser : public SExpParser {
        // expects initial " cut off
        private:
            bool quoted, isDone;
            std::ostringstream oss;

        public:
            StringParser(void);

            bool feed(char);
            bool done(void) const;
            SExp *get(void);
    };

    class ListParser : public SExpParser {
        // expects initial ( cut off
        private:
            enum Phase {
                LIST_ITEMS,
                CDR_ITEM,
                WAITING_FOR_TERMINATION,
                DONE
            };

            Phase phase;
            std::list<SExp*> elements;
            SExp * terminatingCdr;

            SExpParser *subparser;

        public:
            ListParser(void);
            ~ListParser(void);

            bool feed(char);
            bool done(void) const;
            SExp *get(void);
    };

    class SExpStreamParser {
        private:
            std::queue<SExp*> rvs;
            SExpParser *parser;

        public:
            SExpStreamParser(void);
            ~SExpStreamParser(void);

            SExp *pop(void);
            bool empty(void) const;
            void feed(char);
            void end(void);
    };

    class List {
        private:
            std::vector<SExp*> list;
            
        public:
            List(void) : list() {
            }

            ~List(void) {
                for(std::vector<SExp*>::iterator i = list.begin(); i != list.end(); i++) {
                    delete *i;
                }
            }

            Cons *make(void) {
                Cons *rv = 0;
                for(std::vector<SExp*>::reverse_iterator i = list.rbegin(); i != list.rend(); i++) {
                    rv = new Cons( *i, rv );
                }
                list.clear();
                return rv;
            }

            List& operator()(SExp* val) {
                list.push_back( val );
                return *this;
            }
    };

    SExpParser *makeSExpParser(char);

    void outputSExp(SExp*,std::ostream&);

    typedef int RawSocket;
#define closesocket close
#define INVALID_SOCKET -1

    class OutputBuffer : public std::streambuf {
        private:
            int capacity;
            char *data;

        public:
            OutputBuffer(void);
            ~OutputBuffer(void);

            std::string debugGetString(void);

            void consume(int);

            bool tryFlushToSocket(RawSocket);
            bool hasWaiting(void) const;
            int getSize(void);

            int overflow(int);
    };

    class Socket {
        private:
            RawSocket sock;
            OutputBuffer outbuffer;

            SExpStreamParser instream;
            std::ostream outstream;

            bool errorstate;
            bool gcShutdownMode;


        public:
            Socket(RawSocket);
            virtual ~Socket(void);

            static Socket *connectTo(const std::string&, int);

            RawSocket getSocket(void);

            bool hasFatalError(void) const;
            bool wouldTransmit(void) const;

            void transmit(void);
            void receive(void);

            void gracefulShutdown(void);

            std::ostream& out(void);
            SExpStreamParser& in(void);
    };

    class SocketGreeter {
        public:
            virtual Socket* greet(RawSocket, struct sockaddr_storage*, socklen_t) = 0;
    };

    class SocketManager {
        private:
            SocketGreeter* greeter;

            typedef std::map<RawSocket,Socket*> WatchedMap;
            std::vector<RawSocket> listeners;
            fd_set fullWatched;
            WatchedMap watched;
            RawSocket maxWatched;
            RawSocket maxListener;

            bool doCheckStdin;
            bool stdinFlag;

            void watch( Socket* );
            void unwatch( Socket*, bool );

        public:
            typedef std::set<Socket*> SocketSet;

            SocketManager(void);
            ~SocketManager(void);

            void setGreeter(SocketGreeter*);

            void adopt( Socket* );
            bool addListener( int );

            void checkStdin(void);
            bool stdinFlagged(void) const;

            void pump(int, SocketSet*);

            int numberOfWatchedSockets(void);

            SocketSet debugGetSockets(void);
    };

    class ConsSocket : public Socket {
        public:
            ConsSocket( RawSocket x ) : Socket(x) {}

            virtual void handle( const std::string&,  SExp* ) = 0;
            void pump(void);
    };

    class ConsSocketManager : public SocketManager {
        public:
            void manage(int);
    };
};

#endif
