#ifndef H_ANGELIC
#define H_ANGELIC

namespace Angelic {

    struct Coordinate {
        int x, y;
        Coordinate();
        Coordinate(int,int);

        bool operator>(const Coordinate&) const;
        bool operator<(const Coordinate&) const;
        bool operator>=(const Coordinate&) const;
        bool operator<=(const Coordinate&) const;
        bool operator==(const Coordinate&) const;

        private:
            int quadrant(void) const;
            int compare(const Coordinate&) const;
    };

};

#endif
