#ifndef H_NASHSERVER
#define H_NASHSERVER

#include "Nash.h"
#include "SProto.h"

#include "Turns.h"

namespace Nash {

class NashGame {
    private:
        SProto::Server& server;

        int gameId;
        NashBoard board;
        std::string whitePlayer, blackPlayer;

        std::string channelName;

        int moveno;

        bool gameRunning;
//        bool whiteToMove;
        mutable FischerTurnManager turns;
        bool swapAllowed;

        void broadcast(Sise::SExp*);
        void sendTo(const std::string&, Sise::SExp*);
        void delbroadcast(Sise::SExp*);
        void delsendTo(const std::string&, Sise::SExp*);

        void broadcastMove(int,int,NashTile::Colour);
        void broadcastInfo(void);
        void broadcastMessage(const std::string&);

        void requestMove(void);

        std::string playerToMove(void) const;

        void swap(const std::string&);
        void move(const std::string&, int, int);

        void respondIllegal(const std::string&);

        void declareWin(NashTile::Colour);

    public:
        NashGame(SProto::Server&, int, int, const std::string, const std::string);

        void tick(double);

        bool handle(const std::string&, const std::string&, Sise::SExp*);
        bool done(void) const;
};


class NashSubserver : public SProto::Persistable,
                      public SProto::SubServer {
    private:
        int nextGameId;
        typedef std::map<int,NashGame*> GameMap;
        GameMap games;

        typedef std::set<std::string> PlayerList;
        PlayerList players;

        std::map<std::string, std::string> challenges; // challenger -> challengee

    public:
        NashSubserver(SProto::Server&);
        ~NashSubserver(void);

        void tick(double);

        bool handle(SProto::RemoteClient*,const std::string&,Sise::SExp*);

        Sise::SExp* toSexp(void) const;
        void fromSexp(Sise::SExp*);
        void saveSubserver(void) const { save(); }
        void restoreSubserver(void) { restore(); }

        void pruneUsers(void);
};

};
#endif
