#ifndef H_TURNS
#define H_TURNS

#include <vector>
#include <map>

#include <string>

#include <sys/time.h>

class Timer {
    private:
        struct timeval t0;

    public:
        Timer(void);

        void reset(void);
        double getElapsedTime(void);
};

class FischerTurnManager {
    private:
        Timer clock;
        std::map<int,int> indices;
        std::map<int,double> increments;
        std::map<int,double> allocations;
        std::vector<int> participants;
        int index;
        bool running;

        void wrapIndex(void);
        void flushTime(void);
    public:
        FischerTurnManager(void);

        int skip(void);
        int next(void);
        int current(void);

        // obv. players can't pause unilaterally or
        // this would be meaningless..
        void start(void);
        void stop(void);

        void removeParticipant(int);
        void addParticipant(int, double, double);
        void addTime(int, double);
        double getCurrentRemainingTime(void);
        double getRemainingTime(int);

        int getNumberOfParticipants(void);
};

std::string formatTime(double);
std::string formatTimeCoarse(double);

#endif
