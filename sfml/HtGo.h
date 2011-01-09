#ifndef H_HTGO
#define H_HTGO

#include "hexfml.h"

#include <set>
#include <vector>
#include <map>
#include <utility>

#include <iostream>

struct HtGoTile {
    int coreX, coreY;
    std::string positionalLabel;

    enum TileState {
        BLANK,
        WHITE,
        BLACK
    };
    TileState state;

    HtGoTile(void) :
        state ( BLANK )
    {
    }

    HtGoTile(const HtGoTile& that) :
        coreX ( that.coreX ),
        coreY ( that.coreY ),
        positionalLabel ( that.positionalLabel ),
        state ( that.state )
    {
        using namespace std;
    }

    const HtGoTile& operator=(const HtGoTile& that) {
        if( this != &that ) {
            coreX = that.coreX;
            coreY = that.coreY;
            positionalLabel = that.positionalLabel;
            state = that.state;
        }
        using namespace std;
        return *this;
    }
};

class PointSet {
    private:
        typedef std::set<std::pair<int,int> > InternalPointSet;
        InternalPointSet ips;

    public:
        typedef InternalPointSet::const_iterator const_iterator;

        const_iterator begin(void) const { return ips.begin(); }
        const_iterator end(void) const { return ips.end(); }
        int size(void) const { return ips.size(); }
        bool empty(void) const { return ips.empty(); }

        void add(const HtGoTile&);
        void remove(const HtGoTile&);
        bool has(const HtGoTile&) const;

        std::pair<int,int> pop(void);
};

class HexTorusGoMap {
    private:
        int radius;
        HexMap<HtGoTile> coreMap;

            /* Actually ko seems to be impossible in
               hex-go, but I didn't realize this initially
               so it's still supported.
            */
        bool koActive;
        int koCoreX, koCoreY;

        HtGoTile& getIJR(int,int,int);

        HtGoTile& getNeighbourOf(int,int,int);
        HtGoTile& getNeighbourOf(const HtGoTile&,int);

    public:
        explicit HexTorusGoMap(int);
        HexTorusGoMap(const HexTorusGoMap&);

        HtGoTile& get(int,int);
        HtGoTile& get(std::pair<int,int>);

        const HexTorusGoMap& operator=(const HexTorusGoMap&);

        PointSet groupOf(int,int);
        PointSet groupOf(const HtGoTile&);
        PointSet libertiesOf(const PointSet&);

        int put(int,int, HtGoTile::TileState);
        bool putWouldBeLegal(int,int,HtGoTile::TileState);
        int removeGroup(const PointSet&);

        void debugLabelCore(void);
};

#endif
