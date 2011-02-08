#ifndef H_TAC_RULES
#define H_TAC_RULES

#include "Tac.h"
#include "HexTools.h"


namespace Tac {

class TileTypeMap {
    public:
        virtual const TileType* getTileTypeAt(int,int) const = 0;
};

class ActivityPoints {
    private:
        int speed;

        int movementPoints;
        int actionPoints;
        int flexPoints;

        int movementEnergy;

    public:
        ActivityPoints(const UnitType&, int, int, int); // usual is 1, 1, 1
        ActivityPoints(const ActivityPoints&);

        const ActivityPoints& operator=(const ActivityPoints&);

        int getImmediateMovementEnergy(void) const;
        int getPotentialMovementEnergy(void) const;

        bool maySpendMovementEnergy(int) const;
        bool maySpendActionPoints(int) const;

        void spendMovementEnergy(int);
        void spendActionPoint(int);
};

void findAllAccessible(const UnitType&, const TileTypeMap&, int, int, int, HexTools::HexReceiver&);


};

#endif
