#include "Turns.h"

#include <iostream>
#include <algorithm>

#include <sstream>
#include <iomanip>

FischerTurnManager::FischerTurnManager(void) :
    clock (),
    indices (),
    increments (),
    allocations (),
    participants (),
    index ( 0 ),
    running ( false )
{
}

void FischerTurnManager::addParticipant(int id, double seconds, double increment) {
    using namespace std;
    increments[id] = increment;
    allocations[id] = seconds;
    indices[id] = participants.size();
    participants.push_back( id );
}

void FischerTurnManager::addTime(int id, double seconds) {
    if( participants[id] < 0 ) {
        // you can never go into negative time
        // if you get, say, 30 seconds when you're
        // technically 10 seconds past, you still
        // have 30 seconds to move, not 20
        participants[ id ] = seconds;
    } else {
        participants[ id ] += seconds;
    }
}

double FischerTurnManager::getRemainingTime(int id) {
    flushTime();
    return allocations[ indices[ id ] ];
}

double FischerTurnManager::getCurrentRemainingTime(void) {
    flushTime();
    return allocations[participants[index]];
}

void FischerTurnManager::wrapIndex(void) {
    if( index >= (int) participants.size() ) {
        index = 0;
    }
}

int FischerTurnManager::next(void) {
    using namespace std;
    if( !participants.size() ) {
        return -1;
    }
    allocations[participants[index]] += increments[participants[index]];
    ++index;
    wrapIndex();
    return current();
}

int FischerTurnManager::current(void) {
    if( !participants.size() ) {
        return -1;
    }
    return participants[index];
}

void FischerTurnManager::start(void) {
    flushTime();
    running = true;
}

void FischerTurnManager::stop(void) {
    flushTime();
    running = false;
}

void FischerTurnManager::flushTime(void) {
    if( running ) {
        allocations[ participants[index] ] -= clock.getElapsedTime();
    }
    clock.reset();
}

void FischerTurnManager::removeParticipant(int id) {
    // because this is so terribly useful? yeah, I don't know
    // hackish because it's pretty much only useful for test code
    std::vector<int>::iterator i = find( participants.begin(), participants.end(), id );
    if( i == participants.end() ) {
        return;
    }
    participants.erase( i );
    increments.erase( id );
    allocations.erase( id );

    indices.clear();
    for(std::map<int,double>::iterator j = allocations.begin(); j != allocations.end(); j++) {
        indices[j->second] = j->first;
    }
    wrapIndex();
}

Timer::Timer(void) {
    reset();
}

void Timer::reset(void) {
    gettimeofday( &t0, 0 );
}

double Timer::getElapsedTime(void) {
    struct timeval t1, dt;
    gettimeofday( &t1, 0 );
    timersub( &t1, &t0, &dt );
    return dt.tv_sec + 0.000001 * dt.tv_usec;
}

std::string formatTimeCoarse(double seconds) {
    int t = (int)(0.5 + seconds * 1000);
    const int k[] = { 1000 * 60 * 60, 1000 * 60, 1000 };
    const int kn = 3;
    int r[kn];
    std::ostringstream oss;
    for(int i=0;i<kn;i++) {
        r[i] = t / k[i];
        t -= r[i] * k[i];
    }
    for(int i=0;i<kn;i++) {
        if( i != 0 ) oss << ":";
        oss << std::setfill('0') << std::setw(2);
        oss << r[i];
    }
    return oss.str();
}


std::string formatTime(double seconds) {
    int t = (int)(0.5 + seconds * 1000);
    const int k[] = { 1000 * 60 * 60 * 24, 1000 * 60 * 60, 1000 * 60, 1000, 1 };
    const int kn = 5;
    const std::string sx[] = { "d", "h", "m", "s", "ms" };
    int r[kn];
    std::ostringstream oss;
    for(int i=0;i<kn;i++) {
        r[i] = t / k[i];
        t -= r[i] * k[i];
    }
    for(int i=0;i<kn;i++) if( r[i] ) {
        if( i != 0 ) oss << " ";
        oss << r[i] << sx[i];
    }
    return oss.str();
}

int FischerTurnManager::getNumberOfParticipants(void) {
    return participants.size();
}

