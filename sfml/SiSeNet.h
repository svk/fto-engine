#ifndef H_SISENET
#define H_SISENET

/* Not meant to be resistant towards malicious
   activity, vulnerable to DOS by stack overflow
   or memory exhaustion. Secure if needed by
   imposing limits & investigate less obvious things.
*/

#include "SiSExp.h"

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

namespace SiSeNet {
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

            SiSExp::SExpStreamParser instream;
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
            SiSExp::SExpStreamParser& in(void);
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

};

#endif
