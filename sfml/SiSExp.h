#ifndef H_SISEXP
#define H_SISEXP

/* This is just a convenient format for human-readable
   or semi-human-readable serialization. I won't
   (at least initially, until they're needed) be
   supporting things like bignums or rationals.
*/

/* There are a number of obvious time and space
   optimizations that could be made, but clearly
   serialization can be done much faster in C++
   in other ways anyway. Decent is the enemy of
   quick and dirty.
*/

#include <string>

namespace SiSExp {
    enum Type {
        TYPE_CONS,
        TYPE_INT,
        TYPE_STRING
    };

    struct SExpTypeError {
        Type expected;
        Type got;

        SExpTypeError(Type,Type);
    };


    struct Cons;

    class SExp {
        Type type;
        int refcount;

        protected:
            SExp(Type);

        public:
            virtual ~SExp(void);

            void incref(void);
            void decref(void);

            bool isType(Type) const;
            Cons* asCons(void);
    };

    class Cons {
        SExp *carPtr;
        SExp *cdrPtr;

        public:
            Cons(void);
            ~Cons(void);

            SExp *getcar(void);
            SExp *getcdr(void);
            void setcar(SExp*);
            void setcdr(SExp*);
    };

    template <class T, Type X>
    class PodType : public SExp{
        protected:
            const T data;

        public:
            PodType(T data) : SExp(X), data ( data ) {}
            T get(void) const { return data; }
    };

    class Int : public PodType<int,TYPE_INT> {
        public:
            Int(int data) : PodType<int,TYPE_INT>(data) {}
    };

    class String : public PodType<std::string,TYPE_STRING> {
        public:
            String(std::string data) : PodType<std::string,TYPE_STRING>(data) {}
    };
    
};

#endif
