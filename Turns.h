#ifndef H_TURNS
#define H_TURNS

#include <vector>
#include <map>

#include <SFML/System.hpp>

class FischerTurnManager {
    private:
        sf::Clock clock;
        std::map<int,int> indices;
        std::map<int,double> allocations;
        std::vector<int> participants;
        int index;
        bool running;

        void wrapIndex(void);
        void flushTime(void);
    public:
        FischerTurnManager(void);

        void tick(double);
        int next(void);
        int current(void);

        // obv. players can't pause unilaterally or
        // this would be meaningless..
        void start(void);
        void stop(void);

        void removeParticipant(int);
        void addParticipant(int, double);
        void addTime(int, double);
        double getCurrentRemainingTime(void);
        double getRemainingTime(int);
};

#endif
