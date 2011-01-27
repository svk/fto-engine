#ifndef H_SPROTO
#define H_SPROTO

#include "Sise.h"

#include <stdexcept>
#include <string>

// Sise protocol? Slow-game protocol?

#define SPROTO_STANDARD_PORT 8990

namespace SProto {

    class SProtoSocket : public Sise::ConsSocket {
        private:
            bool closing;
        public:
            SProtoSocket( Sise::RawSocket );
            virtual ~SProtoSocket(void) {}

            void close(void);

            void send( Sise::SExp* );
            void delsend( Sise::SExp* );
            void delsendPacket( const std::string&, Sise::SExp* );
            void delsendResponse( const std::string&, const std::string& );
    };

    class ClientCore {
        public:
            virtual void handle( const std::string&, Sise::SExp* ) = 0;
    };

    class Client : public SProtoSocket {
        private:
            enum IdState {
                IDST_UNIDENTIFIED,
                IDST_IDENTIFYING,
                IDST_IDENTIFIED
            };
            IdState idState;
            std::string username, password, passwordhash;

            bool autoregister;
            int triedAutoregister;

            ClientCore *clientCore;

        public:
            Client(Sise::RawSocket, ClientCore* = 0);

            void setAutoRegister(void) { autoregister = true; }

            void setCore(ClientCore*);

            void identify( const std::string&, const std::string& );

            void handle( const std::string&, Sise::SExp* );
            bool identified(void) const { return idState == IDST_IDENTIFIED; }
    };

    class RemoteClient;
    class Server;

    class SubServer {
        protected:
            Server& server;
        public:
            SubServer(const std::string&, Server&);
            virtual ~SubServer(void) {};

            virtual bool handle( RemoteClient*, const std::string&, Sise::SExp* ) = 0;

            virtual void saveSubserver(void) const {};
            virtual void restoreSubserver(void) {};
    };

    class RemoteClient : public SProtoSocket {
        public:
            enum State {
                ST_SILENT,
                ST_VERSION_OK,
            };

        private:
            Server& server;
            State state;
            const std::string netId;
            std::string username;
            std::vector<RemoteClient*>& rclients;

            bool loggingIn;
            std::string desiredUsername, challenge;

            typedef std::pair<std::string,std::string> ChannelId;
            std::set<ChannelId> channels;

        public:
            RemoteClient(Sise::RawSocket,Server&, const std::string&, std::vector<RemoteClient*>&);
            virtual ~RemoteClient(void);

            bool hasUsername(void) const;
            std::string getUsername(void) const;
            void setUsername(const std::string& un) { username = un; }

            bool getLoggingIn(std::string& du, std::string& ch) {
                if( !loggingIn ) return false;
                du = desiredUsername;
                ch = challenge;
                return true;
            }
            void setLoggingIn(const std::string& du, const std::string& ch) {
                loggingIn = true;
                desiredUsername = du;
                challenge = ch;
            }
            void setNotLoggingIn(void) {
                loggingIn = false;
            }


            const std::string& getNetId(void) const { return netId; }

            void handle( const std::string&, Sise::SExp* );

            void setState(State);
            State getState(void) const;

            void enterChannel(const std::string&, const std::string&);
            void leaveChannel(const std::string&, const std::string&);
            bool isInChannel(const std::string&, const std::string&);
    };

    class Persistable {
        private:
            std::string filename;
        public:
            Persistable(const std::string&);
            
            virtual Sise::SExp* toSexp(void) const = 0;
            virtual void fromSexp(Sise::SExp*) = 0;

            void save(void) const;
            void restore(void);
    };

    class DirectoryPersistable : public Sise::NamedSexpHandler {
        // note -- these names are trusted!
        // do not allow non-sanitized user input in the file names
        // the intention is things like game ID numbers
        private:
            std::string dirname;
            std::string extension;
    
        protected:
            void writeNamedSexp(const std::string&, Sise::SExp*);
            void clearFiles(void);

        public:
            DirectoryPersistable(const std::string&);
            
            virtual void handleNamedSExp(const std::string&, Sise::SExp*) = 0;
            virtual void save(void) const = 0;
            void restore(void);
    };

    class UsersInfo : public Persistable,
                      public SubServer {
        public:
            struct UserInfo {
                std::string passwordhash;
                bool isAdmin;

                UserInfo(void) {}
                UserInfo(const std::string&, bool = false);
            };

        private:
            typedef std::map<std::string, UserInfo> UsersMap;
            UsersMap users;

        public:
            UserInfo& operator[](const std::string&);
            const UserInfo& operator[](const std::string&) const;

            UsersInfo(Server& server) :
                Persistable( "./persist/users.lisp" ),
                SubServer( "user", server )
            {}

            Sise::SExp* toSexp(void) const;
            void fromSexp(Sise::SExp*);

            std::string usernameAvailable(const std::string&);
            std::string solveChallenge( const std::string&, const std::string& );
            std::string registerUsername( const std::string&, const std::string& );

            bool isAdministrator(const std::string&) const;

            bool handle( RemoteClient*, const std::string&, Sise::SExp* );

            void saveSubserver(void) const { save(); }
            void restoreSubserver(void) { restore(); }
    };

    class Server;

    class AdminSubserver : public SubServer {
        public:
            AdminSubserver(Server& server) : SubServer("admin",server) {}

            bool handle( RemoteClient*, const std::string&, Sise::SExp* );
    };

    class DebugSubserver : public SubServer {
        public:
            DebugSubserver(Server& server) : SubServer("debug",server) {}

            bool handle( RemoteClient*, const std::string&, Sise::SExp* );
    };
    
    class ChatSubserver : public SubServer {
        public:
            ChatSubserver(Server& server) : SubServer("chat",server) {}

            bool handle( RemoteClient*, const std::string&, Sise::SExp* );
    };

    class Server : public Sise::ConsSocketManager,
                   public Sise::SocketGreeter {
        public:
            typedef std::vector<RemoteClient*> RClientList;

        private:
            RClientList rclients;
            typedef std::map<std::string,SubServer*> SubserverMap;
            SubserverMap subservers;

            bool running;

            UsersInfo users;

            // not all the subservers are internally owned like this;
            // it's fine to add more (games)
            DebugSubserver ssDebug;
            AdminSubserver ssAdmin;
            ChatSubserver ssChat;

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

            UsersInfo& getUsers(void) { return users; }
            const UsersInfo& getUsers(void) const { return users; }

            void handle( RemoteClient*, const std::string&, Sise::SExp* );

            void save(void);
            void restore(void);

            RClientList& getClients(void) { return rclients; }

            RemoteClient* getConnectedUser( const std::string& );
    };

    struct NoSuchUserException : public std::runtime_error {
        NoSuchUserException(void) : std::runtime_error( "no such user" ) {}
    };
    
    std::string getHash(const std::string&);
    std::string makeChallengeResponse( const std::string&, const std::string&, const std::string&);
    std::string makePasswordHash( const std::string&, const std::string&);

    Sise::SExp *prepareChatMessage(const std::string&, const std::string&);
    std::string getChatMessageBody(Sise::SExp *);
    std::string getChatMessageOrigin(Sise::SExp *);
};

#endif
