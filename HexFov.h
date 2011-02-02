#ifndef H_HEXFOV
#define H_HEXFOV

#include <map>
#include <utility>

namespace HexTools {

class HexOpacityMap {
    public:
        virtual bool isOpaque(int,int) const = 0;
};

class HexLightReceiver {
    public:
        virtual void setLighted(int,int) = 0;
};

struct Angle {
    int x, y;
    Angle();
    Angle(int,int);

    Angle(const Angle&);
    const Angle& operator=(const Angle&);

    bool operator>(const Angle&) const;
    bool operator<(const Angle&) const;
    bool operator>=(const Angle&) const;
    bool operator<=(const Angle&) const;
    bool operator==(const Angle&) const;

    private:
        int quadrant(void) const;
        int compare(const Angle&) const;
};

struct Sector {
    Angle begin, end;
    bool empty;
    bool containsBranchCut;

    void recheckBranchCut(void);

    bool intersect(const Sector&);
    void adjacentUnion(const Sector&);

    Sector(const Angle&, const Angle&);

    bool contains(const Angle&) const;
};

bool sectorIntersection( const Angle&, const Angle&,
                         const Angle&, const Angle&,
                         Angle&, Angle& );
void sectorAdjacentUnion( const Angle&, const Angle&,
                          const Angle&, const Angle&,
                          Angle&, Angle& );

class LightedTileQueue {
    private:
        typedef std::map< std::pair<int,int>, std::pair<Angle,Angle> > LtqMap;
        LtqMap q;

    public:
        bool empty(void) const;
        void add(int,int,const Angle&,const Angle&);
        void getFront(int&,int&,Angle&,Angle&);
        void popFront(void);
};

class HexFovNorthBeam {
    // won't be making a class for each direction, but I'll be writing
    // north first and then translating to something general by symmetry
    // (but still a beam!)
    // this class assumes center at (0,0) and uses translators
    private:
        HexOpacityMap& map;
        int cx, cy;

        HexLightReceiver& receiver;

        LightedTileQueue *current, *primary, *secondary;

        bool popNext(int&,int&,Angle&,Angle&);
        void passFrom(int,int,const Angle&,const Angle&);

        void setLighted(int,int);
        bool isOpaque(int,int) const;

    public:
        HexFovNorthBeam( HexOpacityMap&, HexLightReceiver&); 
        ~HexFovNorthBeam(void);

        void calculate(void);
};


};

#endif
