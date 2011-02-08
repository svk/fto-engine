#include "Turns.h"

#include <algorithm>

FischerTurnManager::FischerTurnManager(void) :
    clock (),
    participants (),
    index ( 0 ),
    running ( false )
{
}

void FischerTurnManager::addParticipant(int id, double seconds) {
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
    return allocations[index];
}

void FischerTurnManager::wrapIndex(void) {
    if( index >= participants.size() ) {
        index = 0;
    }
}

int FischerTurnManager::next(void) {
    ++index;
    wrapIndex();
    return current();
}

int FischerTurnManager::current(void) {
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
        allocations[ participants[index] ] -= clock.GetElapsedTime();
    }
    clock.Reset();
}

void FischerTurnManager::removeParticipant(int id) {
    // because this is so terribly useful? yeah, I don't know
    // hackish because it's pretty much only useful for test code
    std::vector<int>::iterator i = find( participants.begin(), participants.end(), id );
    if( i == participants.end() ) {
        return;
    }
    participants.erase( i );
    allocations.erase( id );

    indices.clear();
    for(std::map<int,double>::iterator j = allocations.begin(); j != allocations.end(); j++) {
        indices[j->second] = j->first;
    }
    wrapIndex();
}

