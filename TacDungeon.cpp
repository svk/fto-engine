#include "TacDungeon.h"

namespace Tac {

int PointCorridor::dx[6] ={ 3, 0, -3, -3, 0, 3};
int PointCorridor::dy[6] ={ 1, 2, 1, -1, -2, -1};

DungeonSketch::DungeonSketch(void) :
    sketch ( ST_NONE )
{
}

RoomNode::RoomNode(void) :
    connected ( false ),
    connections (),
    region ()
{
}

void RoomNode::connect(RoomNode *node) {
    node->connections.insert( this );
    connections.insert( node );
}

void RoomNode::add(int x, int y) {
    region.add( x, y );
}

void RoomNode::remove(int x, int y){
    region.remove( x, y );
}

void RoomNode::markConnected(void) {
    if( !connected ) {
        connected = true;
        for(std::set<RoomNode*>::iterator i = connections.begin(); i != connections.end(); i++){
            (*i)->markConnected();
        }
    }
}

bool RoomNode::isMarkedConnected(void) const {
    return connected;
}

int RoomNode::getRegionSize(void) const {
    return region.size();
}

void DungeonSketch::adoptRoom(RoomNode* room) {
    rooms.push_back( room );
}

DungeonSketch::~DungeonSketch(void) {
    for(std::vector<RoomNode*>::iterator i = rooms.begin(); i != rooms.end(); i++){
        delete *i;
    }
}

bool DungeonSketch::checkRoomAt(int cx, int cy, int R) const {
    using namespace HexTools;
    if( sketch.get(cx,cy) != ST_NONE ) return false;
    for(int r=1;r<=R;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int dx, dy;
        cartesianiseHexCoordinate( i, j, r, dx, dy );
        if( sketch.get(cx+dx,cy+dy) != ST_NONE ) return false;
    }
    return true;
}

void DungeonSketch::put(int x,int y, SketchTile st ){
    sketch.set( x, y, st );
}

HexTools::HexCoordinate SimpleLevelGenerator::placeRoomNear(int cx, int cy, int radius){
    using namespace HexTools;
    std::vector<HexCoordinate> rv;

    if( sketch.checkRoomAt( cx, cy, radius ) ) return HexCoordinate(cx,cy);

    for(int r=1;true;r++) {
        for(int i=0;i<6;i++) for(int j=0;j<r;j++){
            int dx, dy;
            cartesianiseHexCoordinate( i, j, r, dx, dy );
            if( sketch.checkRoomAt( cx + dx, cy + dy, radius ) ) {
                rv.push_back( HexCoordinate(cx + dx, cy + dy) );
            }
        }
        if( !rv.empty() ) break;
    }

    return rv[ abs( prng() ) % (int) rv.size() ];
}

RoomPainter::RoomPainter(int radius) :
    radius( radius )
{
}


BlankRoomPainter::BlankRoomPainter(int radius) :
    RoomPainter( radius )
{
}

RectangularRoomPainter::RectangularRoomPainter(int radius) :
    RoomPainter( radius )
{
    if( radius < 4 ) throw std::logic_error( "rectangular room too small" );
}


HollowHexagonRoomPainter::HollowHexagonRoomPainter(int radius, int thickness) :
    RoomPainter( radius ),
    thickness ( thickness )
{
    if( radius < (3 + thickness) ) throw std::logic_error( "hollow hexagon room too small" );
}


HexagonRoomPainter::HexagonRoomPainter(int radius) :
    RoomPainter( radius )
{
    if( radius < 4 ) throw std::logic_error( "hexagon room too small" );
}

void RectangularRoomPainter::paint(DungeonSketch& sketch, int cx, int cy) {
    using namespace HexTools;
    int R = getRadius();
    using namespace std;
    RoomNode *node = new RoomNode();
    sketch.adoptRoom( node );
    sketch.put( cx, cy, DungeonSketch::ST_META_DIGGABLE );
    for(int r=1;r<=getRadius();r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        sketch.put( cx + x, cy + y, DungeonSketch::ST_META_DIGGABLE );
    }
    R--;
    for(int i=-(R-1);i<=(R-1);i++) for(int j=-(R-1);j<(R-1);j++) { // don't touch this
        int x = cx + 3 * i, y = cy + 2 * j;
        int ci, cj, cr;
        if( (i%2) != 0 ) y++;
        HexTools::polariseHexCoordinate( x - cx, y - cy, ci, cj, cr );
        assert( cr <= getRadius() );
        if( abs(i) == (R-1) || abs(j) == (R-1) ) {
            if( !i || !j ) {
                sketch.put( cx + x, cy + y, DungeonSketch::ST_META_CONNECTOR );
                sketch.registerConnector( cx + x, cy + y, node );
            } else {
                sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_WALL );
            }
        } else{
            node->add( cx + x, cy + y );
            sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_FLOOR );
        }
    }
}

void BlankRoomPainter::paint(DungeonSketch& sketch, int cx, int cy) {
    using namespace HexTools;
    int R = getRadius();
    sketch.put(cx,cy, DungeonSketch::ST_META_DIGGABLE );
    for(int r=1;r<=R;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        sketch.put( cx + x, cy + y, DungeonSketch::ST_META_DIGGABLE );
    }
}

void HollowHexagonRoomPainter::paint(DungeonSketch& sketch, int cx, int cy) {
    using namespace HexTools;
    int R = getRadius();
    RoomNode *node = new RoomNode();
    sketch.adoptRoom( node );
    sketch.put(cx,cy, DungeonSketch::ST_NORMAL_WALL );
    for(int r=1;r<=R;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        if( r == R ) {
            sketch.put( cx + x, cy + y, DungeonSketch::ST_META_DIGGABLE );
        } else if( r == (R-1) ) {
            if( j == (R-1)/2 ) {
                sketch.put( cx + x, cy + y, DungeonSketch::ST_META_CONNECTOR );
                sketch.registerConnector( cx + x, cy + y, node );
            } else {
                node->add( cx + x, cy + y );
                sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_WALL );
            }
        } else if( r < ((R-1)-thickness)) {
            sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_WALL );
        } else {
            node->add( cx + x, cy + y );
            sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_FLOOR );
        }
    }
}

void HexagonRoomPainter::paint(DungeonSketch& sketch, int cx, int cy) {
    using namespace HexTools;
    int R = getRadius();
    RoomNode *node = new RoomNode();
    sketch.adoptRoom( node );
    node->add( cx, cy );
    sketch.put(cx,cy, DungeonSketch::ST_NORMAL_FLOOR );
    for(int r=1;r<=R;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        if( r == R ) {
            sketch.put( cx + x, cy + y, DungeonSketch::ST_META_DIGGABLE );
        } else if( r == (R-1) ) {
            if( j == (R-1)/2 ) {
                sketch.registerConnector( cx + x, cy + y, node );
                sketch.put( cx + x, cy + y, DungeonSketch::ST_META_CONNECTOR );
            } else {
                sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_WALL );
            }
        } else {
            node->add( cx + x, cy + y );
            sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_FLOOR );
        }
    }
}

PointCorridor::PointCorridor(DungeonSketch& sketch, int cx, int cy) :
    sketch ( &sketch ),
    cx ( cx ),
    cy ( cy ),
    checked ( false )
{
    for(int i=0;i<6;i++) {
        activeDir[i] = false;
    }
}

bool PointCorridor::check(void) {
    int n = 0;
    length = 0;
    using namespace std;
    if( !sketch ) {
        cerr << "oops" << endl;
    }
    DungeonSketch::SketchTile mptt = sketch->get(cx,cy);
    if( mptt != DungeonSketch::ST_NONE && mptt != DungeonSketch::ST_META_DIGGABLE ) return false;
    for(int i=0;i<6;i++) {
        int tries = sketch->getMaxRadius() * 2 + 1;
        int x = cx, y = cy;
        int llen = 0;
        while( tries-- > 0 ) {
            DungeonSketch::SketchTile tt = sketch->get(x,y);
            if( tt != DungeonSketch::ST_NONE && tt != DungeonSketch::ST_META_DIGGABLE ) break;
            x += dx[i];
            y += dy[i];
            ++llen;
        }
        using namespace std;
        switch( sketch->get(x,y) ) {
            case DungeonSketch::ST_META_CONNECTOR:
                ++n;
                length += llen;
                activeDir[i] = true;
                node[i] = sketch->getConnectorRoom(x,y);
                break;
            default:
                activeDir[i] = false;
                break;
        }
    }
    checked = true;
    using namespace std;
    return n > 1;
}

void PointCorridor::dig(void) {
    if( !checked ) {
        check();
    }
    using namespace std;
    std::set< RoomNode* > ends;
    for(int i=0;i<6;i++) if(activeDir[i] ){
        int x = cx, y = cy;
        while( sketch->get(x,y) != DungeonSketch::ST_META_CONNECTOR ) {
            for(int k=0;k<6;k++) {
                int sx = x + dx[k], sy = y + dy[k];
                if( sketch->get(sx, sy) == DungeonSketch::ST_NONE
                    || sketch->get(sx, sy) == DungeonSketch::ST_META_DIGGABLE ) {
                    sketch->put( sx, sy, DungeonSketch::ST_NORMAL_WALL );
                }
            }
            sketch->put(x, y, DungeonSketch::ST_NORMAL_CORRIDOR );
            using namespace std;
            x += dx[i];
            y += dy[i];
        }
        RoomNode* rv = sketch->getConnectorRoom( x, y );
        if( !rv ) {
            cerr << "warning: unregistered connector at " << x << " " << y << endl;
        } else {
            ends.insert( rv );
        }
        sketch->put( x, y, DungeonSketch::ST_NORMAL_DOORWAY );
    }

    for(std::set<RoomNode*>::iterator i = ends.begin(); i != ends.end(); i++) {
        for(std::set<RoomNode*>::iterator j = ends.begin(); j != ends.end(); j++) {
            if( *i != *j ) {
                (*i)->connect( *j );
            }
        }
    }

}

void DungeonSketch::registerConnector(int x, int y, RoomNode *room) {
    rConnectors[ HexTools::HexCoordinate(x,y) ] = room;
}

RoomNode* DungeonSketch::getConnectorRoom(int x, int y) {
    std::map< HexTools::HexCoordinate, RoomNode* >::const_iterator i = rConnectors.find( HexTools::HexCoordinate(x,y) );
    if( i == rConnectors.end() ) return 0;
    return i->second;
}

LevelGenerator::LevelGenerator(MTRand_int32& prng) :
    prng ( prng ),
    sketch ()
{
}

SimpleLevelGenerator::PainterEntry::PainterEntry(RoomPainter* p ,int w ,bool c, bool a) :
    painter ( p ),
    weight ( w ),
    countTowardsTarget ( c ),
    adopted ( a )
{
}

SimpleLevelGenerator::~SimpleLevelGenerator(void) {
    for(std::vector<PainterEntry>::iterator i = entries.begin(); i != entries.end(); i++) {
        if( i->adopted ) {
            delete i->painter;
        }
    }
}

SimpleLevelGenerator::SimpleLevelGenerator( MTRand_int32& prng ) :
    LevelGenerator ( prng ),
    // defaults
    entries (),
    roomTarget ( 4 ),
    ensureConnectedness ( true ),
    forbidTrivialLoops ( true ),
    maxCorridors ( 100 ),
    useCorridorLengthLimit ( true ),
    corridorLengthLimit ( 30 ),
    useRadiusLimit ( false ),
    radiusLimit ( -1 ),
    shortestCorridorFirst ( false ),
    stopWhenConnected ( false ),
    swcExtraCorridors ( -1 ),
    pointCorridorCandidatesGenerated ( false ),
    pccs (),
    corridorCount ( 0 )
{
}

void SimpleLevelGenerator::adoptPainter(RoomPainter* painter,int weight,bool countTT) {
    entries.push_back( PainterEntry( painter, weight, countTT, true ) );
}

SimpleLevelGenerator::PainterEntry& SimpleLevelGenerator::selectPainter(void) {
    int sum = 0;
    for(std::vector<PainterEntry>::iterator i = entries.begin(); i != entries.end(); i++) {
        sum += i->weight;
    }
    int r = prng( sum );
    for(std::vector<PainterEntry>::iterator i = entries.begin(); i != entries.end(); i++) {
        r -= i->weight;
        if( r < 0 ) {
            return *i;
        }
    }
    assert( false );
    return *entries.begin();
}

void SimpleLevelGenerator::generateRooms(void) {
    using namespace std;
    int count = 0;
    cerr << "hmm+?" << endl;
    while( count < roomTarget ) {
        PainterEntry& entry = selectPainter();
        HexTools::HexCoordinate coords = placeRoomNear( 0, 0, entry.painter->getRadius() );
        cerr << "painting " << entry.painter << endl;
        entry.painter->paint( sketch, coords.first, coords.second );
        if( entry.countTowardsTarget ) {
            ++count;
        }
    }
}

PointCorridor::PointCorridor(const PointCorridor& that) :
    sketch ( that.sketch ),
    cx ( that.cx ),
    cy ( that.cy ),
    checked ( that.checked ),
    length ( that.length )
{
    using namespace std;
    for(int i=0;i<6;i++) {
        activeDir[i] = that.activeDir[i];
        node[i] = that.node[i];
    }
}

const PointCorridor& PointCorridor::operator=(const PointCorridor& that) {
    using namespace std;
    if( this != &that ){
        sketch = that.sketch;
        cx = that.cx;
        cy = that.cy;
        checked = that.checked;
        length = that.length;
        for(int i=0;i<6;i++) {
            activeDir[i] = that.activeDir[i];
            node[i] = that.node[i];
        }
    }
    return *this;
}

bool RoomNode::isDirectlyConnected(RoomNode* t) const {
    return connections.find( t ) != connections.end();
}

bool SimpleLevelGenerator::checkPCC( PointCorridor& corr ) {
    if( !corr.check() ) {
        return false;
    }
    if( useCorridorLengthLimit ) {
        if( corr.getLength() > corridorLengthLimit ) return false;
    }
    if( forbidTrivialLoops ) {
        bool hasTrivialLoops = false;
        std::vector<RoomNode*> ends;
        for(int i=0;i<6;i++) if( corr.isActive(i)) {
            ends.push_back( corr.getNode(i) );
        }
        for(int i=0;i<(int)ends.size();i++) for(int j=0;j<(int)ends.size();j++) if( i != j ) {
            if( ends[i] == ends[j] ) {
                hasTrivialLoops = true;
            }
            if( ends[i]->isDirectlyConnected( ends[j]) ) {
                hasTrivialLoops = true;
            }
        }
        if( hasTrivialLoops ) return false;
    }
    return true;
}

void SimpleLevelGenerator::generatePCCs(void) {
    pointCorridorCandidatesGenerated = true;
    int sz = sketch.getMaxRadius();
    for(int r=0;r<=sz;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        HexTools::cartesianiseHexCoordinate( i, j, r, x, y );
        PointCorridor pcc ( sketch, x, y );
        if( checkPCC( pcc ) ) {
            pccs.push_back( pcc );
        }
    }
}

bool SimpleLevelGenerator::generatePointCorridor(void) {
    using namespace std;
    if( corridorCount >= maxCorridors ) {
        return false;
    }

    if( !pointCorridorCandidatesGenerated ) {
        generatePCCs();
    }

    int shortestCorridorLength = 0x7fffffff;
    int shortestCorridorN = 0;

    cerr << "pc?3 ";
    // pccs need pruning
    int counttest = 0;
    for(std::vector<PointCorridor>::iterator i = pccs.begin(); i != pccs.end();) {
        ++counttest;
        if( !checkPCC( *i ) ) {
            i = pccs.erase( i );
        } else {
            if( i->getLength() < shortestCorridorLength ) {
                shortestCorridorLength = i->getLength();
                shortestCorridorN = 1;
            } else if( i->getLength() == shortestCorridorLength ) {
                shortestCorridorN++;
            }
            i++;
        }
    }

    if( !pccs.size() ) return false;

    int n = (shortestCorridorFirst) ? shortestCorridorN : (int) pccs.size();
    int r = prng(n);
    std::vector<PointCorridor>::iterator i = pccs.begin();
    while( true ) {
        assert( i != pccs.end() );
        if( !shortestCorridorFirst || i->getLength() == shortestCorridorLength ) {
            --r;
        }
        if( r < 0 ) {
            break;
        }
        i++;
    }

    i->dig();
    ++corridorCount;

    return true;
}

void SimpleLevelGenerator::finalChecks(void){
    std::vector<RoomNode*> rooms = sketch.getRooms();
    do {
        if( rooms.size() < 1 ) {
            throw LevelGenerationFailure( "no rooms" );
        }

        if( useRadiusLimit ) {
            if( sketch.getMaxRadius() > radiusLimit ) {
                throw LevelGenerationFailure( "too large" );
            }
        }

        if( ensureConnectedness ) {
            if( !isConnected() ) throw LevelGenerationFailure( "not connected" );
        }
    } while(false);
}

bool SimpleLevelGenerator::isConnected(void) {
    std::vector<RoomNode*> rooms = sketch.getRooms();
    bool notConnected = false;
    for(std::vector<RoomNode*>::iterator i = rooms.begin(); i != rooms.end(); i++) {
        (*i)->clearConnectedMark();
    }
    rooms[0]->markConnected();
    for(std::vector<RoomNode*>::iterator i = rooms.begin(); i != rooms.end(); i++) {
        if( !(*i)->isMarkedConnected() ) {
            notConnected = true;
        }
    }
    return !notConnected;
}

void SimpleLevelGenerator::generate(void) {
    generateRooms();
    while( generatePointCorridor() ) {
        if( stopWhenConnected && isConnected() ) {
            break;
        }
    }
    if( stopWhenConnected ) {
        for(int i=0;i<swcExtraCorridors;i++) {
            generatePointCorridor();
        }
    }
    finalChecks();
}

void SimpleLevelGenerator::setSWCExtraCorridors(int n) {
    swcExtraCorridors = n;
}

void SimpleLevelGenerator::setRoomTarget(int n) {
    roomTarget = n;
}

void SimpleLevelGenerator::setStopWhenConnected(bool tf) {
    stopWhenConnected = tf;
}

void SimpleLevelGenerator::setShortestCorridorsFirst(bool tf) {
    shortestCorridorFirst = tf;
}

}
