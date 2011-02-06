#ifndef H_MANAGER
#define H_MANAGER

#include "mtrand.h"

#include <map>
#include <set>
#include <algorithm>

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

#endif
