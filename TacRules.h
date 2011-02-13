#ifndef H_TAC_RULES
#define H_TAC_RULES

#include "Tac.h"
#include "HexTools.h"

#include "Sise.h"

#include <gmpxx.h>

#include "BoxRandom.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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
        ActivityPoints(void);
        ActivityPoints(const UnitType&, int, int, int); // usual is 1, 1, 1
        ActivityPoints(const ActivityPoints&);
        ActivityPoints(Sise::SExp*);

        const ActivityPoints& operator=(const ActivityPoints&);

        int getImmediateMovementEnergy(void) const;
        int getPotentialMovementEnergy(void) const;

        bool maySpendMovementEnergy(int) const;
        bool maySpendActionPoints(int) const;

        void spendMovementEnergy(int);
        void spendActionPoint(int);

        void forbidMovement(void);

        Sise::SExp *toSexp(void) const;
        static ActivityPoints fromSexp(Sise::SExp*);
};

class OpposedBooleanRoller {
    // basic case meant for: attack vs defense -> hit?
    //  suitable parameters p=0.75, c=0.06
    // also: attack vs defense -> bernoulli damage succeeeds?
    //  suitable parameters p=0.5, c=0.02
    private:
        const double p, c;

        mpq_class chance(int);
    public:
        OpposedBooleanRoller(const double p, const double c) :
            p ( p ), c ( c )
        {
        }

        virtual ~OpposedBooleanRoller(void) {}

        mpq_class chance(int att,int def) {
            return chance( att - def );
        }
};

class AttackRoller : public OpposedBooleanRoller {
    public:
        AttackRoller(void);
};

class DamageSuccessRoller : public OpposedBooleanRoller {
    public:
        DamageSuccessRoller(void);
};

struct AttackResult {
    enum Status {
        MISS,
        HIT
        // blocking?
    };

    Status status;
    int damage; // different types..?

    bool operator==(const AttackResult&) const;

    Sise::SExp *toSexp(void) const;
    static AttackResult fromSexp(Sise::SExp*);
};

Outcomes<AttackResult> makeAttack(int,int);
Outcomes<AttackResult> makeAttackBetween(const AttackCapability&, const DefenseCapability&);

struct BernoulliDamageDie : public NondeterministicTransform<AttackResult,AttackResult> {
    const mpq_class successP;
    const int firepower;

    BernoulliDamageDie(mpq_class successP, int firepower) : successP( successP ), firepower ( firepower ) {}

    Outcomes<AttackResult> transform(AttackResult x);
};

struct DamageResister : public DeterministicTransform<AttackResult,AttackResult> {
    const double resistance;

    DamageResister(double resistance) :
        resistance ( resistance )
    {
    }

    AttackResult transform(AttackResult x) {
        if( x.status == AttackResult::HIT ){
            x.damage = MAX( 0, (int)(((double)x.damage) * (1.0-resistance)) );
        }
        return x;
    }
};

struct DamageReducer : public DeterministicTransform<AttackResult,AttackResult> {
    const int reduction;

    DamageReducer(int reduction) :
        reduction ( reduction )
    {
    }

    AttackResult transform(AttackResult x){
        if( x.status == AttackResult::HIT ){
            x.damage = MAX( 0, x.damage - reduction );
        }
        return x;
    }
};

struct DamageDealer : public NondeterministicTransform<AttackResult,AttackResult> {
    const mpq_class successP;
    const int diceno, firepower;

    DamageDealer(int att, int def, int diceno, int firepower) :
        successP ( DamageSuccessRoller().chance( att, def ) ),
        diceno ( diceno ),
        firepower ( firepower )
    {
    }

    Outcomes<AttackResult> transform(AttackResult x);
};

int getDamageOfAttack(AttackResult);

void findAllAccessible(const UnitType&, const TileTypeMap&, int, int, int, HexTools::HexReceiver&);

};

#endif
