#ifndef H_SPROTO
#define H_SPROTO

#include "Sise.h"

#include <stdexcept>
#include <string>

// Sise protocol? Slow-game protocol?

namespace SProto {

    class SProtoSocket : public Sise::ConsSocket {
        private:
            bool closing;
        public:
            SProtoSocket( Sise::RawSocket );
            virtual ~SProtoSocket(void) {}

            void close(void);

            void delsend( Sise::SExp* );
            void delsendPacket( const std::string&, Sise::SExp* );
    };

    class Client : public SProtoSocket {
        private:
        public:
            void identify( const std::string&, const std::string& );

            void handle( const std::string&, Sise::SExp* );
    };

    class RemoteClient;
    class Server;

    class SubServer {
        public:
            virtual void handle( RemoteClient*, const std::string&, Sise::SExp* ) = 0;
            virtual bool entering( RemoteClient* ) { return true; }
            virtual void leaving( RemoteClient* ) {}
    };

    class RemoteClient : public SProtoSocket {
        private:
            enum State {
                ST_SILENT,
                ST_VERSION_OK,
            };
            Server& server;
            State state;
            const std::string netId;
            std::string username;
            SubServer *subserver;
            std::vector<RemoteClient*>& rclients;

            bool loggingIn;
            std::string desiredUsername, loginChallenge;


        public:
            RemoteClient(Sise::RawSocket,Server&, const std::string&, std::vector<RemoteClient*>&);
            virtual ~RemoteClient(void);

            const std::string& getNetId(void) const { return netId; }

            void leave(void);

            void handle( const std::string&, Sise::SExp* );
    };

    class Server : public Sise::ConsSocketManager,
                   public Sise::SocketGreeter {
        private:
            std::vector<RemoteClient*> rclients;
            typedef std::map<std::string,SubServer*> SubserverMap;
            SubserverMap subservers;

            typedef std::map<std::string, std::string> UsersMap;
            UsersMap users; // username -> hashed pw

            bool running;

        public:
            Server(void);
            ~Server(void);

            SubServer* getSubServer(const std::string&);
            void setSubServer(const std::string&, SubServer*);

            void stopServer(void);
            bool isRunning(void) const { return running; }

            Sise::Socket* greet(Sise::RawSocket, struct sockaddr_storage*, socklen_t);

            std::string makeChallenge(void);
            std::string usernameAvailable(const std::string&);
            std::string solveChallenge( const std::string&, const std::string& );
            std::string registerUsername( const std::string&, const std::string& );
    };

    struct NoSuchUserException : public std::runtime_error {
        NoSuchUserException(void) : std::runtime_error( "no such user" ) {}
    };
    
    std::string getHash(const std::string&);
    std::string makeChallengeResponse( const std::string&, const std::string&, const std::string&);
    std::string makePasswordHash( const std::string&, const std::string&);
};

#endif
