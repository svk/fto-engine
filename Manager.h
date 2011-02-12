#ifndef H_MANAGER
#define H_MANAGER

#include "mtrand.h"

#include <map>
#include <set>
#include <algorithm>
#include <vector>

#include <string>

#include "Sise.h"

template<class T>
class ResourceManager {
    // not thread-safe for assignment
    private:
        typedef std::map<std::string,T*> ResourceContainer;
        ResourceContainer resources;
    
    public:
        ResourceManager(void) {}
        ~ResourceManager(void) {
            for(typename ResourceContainer::iterator i=resources.begin();i != resources.end(); i++) {
                delete i->second;
            }
        }

        void bind(const std::string& str, T* resource) {
            typename ResourceContainer::iterator i = resources.find( str );
            if( i != resources.end() ) {
                delete i->second;
            }
            resources[ str ] = resource;
        }

        T& operator[](const std::string& str) {
            return *resources[ str ];
        }
};

template
<class T>
class RandomVariantsCollection {
    private:
        MTRand_int32 prng;
        std::vector<T*> elts;
    public:
        RandomVariantsCollection(void) :
            prng (),
            elts ()
        {
        }

        virtual ~RandomVariantsCollection(void) {
            for(typename std::vector<T*>::iterator i = elts.begin(); i != elts.end(); i++) {
                delete *i;
            }
        }

        void adopt(T* elt) {
            elts.push_back( elt );
        }

        T& choose(void) {
            return *elts[abs( prng() ) % elts.size()];
        }
};

template
<class T>
void fillManagerFromFile(const std::string& filename, ResourceManager<T>& uts) {
    using namespace Sise;
    using namespace std;
    SExpStreamParser streamParser;
    ifstream is ( filename.c_str(), ios::in );
    char byte;
    if( !is.good() ) {
        throw FileInputError();
    }
    while( !is.eof() ) {
        is.read( &byte, 1 );
        streamParser.feed( byte );
    }
    streamParser.end();
    while( !streamParser.empty() ) {
        Cons *data = asProperCons( streamParser.pop() );
        std::string name = * asSymbol(data->getcar() );
        T *ut = new T( data );
        delete data;
        uts.bind( name, ut );
    }
}

#endif
