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

#include <list>

#include <ostream>
#include <sstream>
#include <queue>

#include <string>

#include <stdexcept>

namespace SiSExp {

    struct ParseError : public std::runtime_error {
        ParseError(std::string s) : std::runtime_error(s) {}
    };

    enum Type {
        TYPE_CONS,
        TYPE_INT,
        TYPE_STRING,
        TYPE_SYMBOL
    };

    struct SExpTypeError {
        Type expected;
        Type got;

        SExpTypeError(Type,Type);
    };


    struct Cons;
    struct Int;
    struct String;
    struct Symbol;

    class SExp {
        Type type;

        protected:
            explicit SExp(Type);

        public:
            virtual ~SExp(void);

            bool isType(Type) const;
            Cons* asCons(void);
            String* asString(void);
            Int* asInt(void);
            Symbol* asSymbol(void);

            virtual void output(std::ostream&) = 0;
    };

    class Cons : public SExp {
        SExp *carPtr;
        SExp *cdrPtr;

        public:
            Cons(void);
            explicit Cons(SExp*);
            Cons(SExp*, SExp*);
            ~Cons(void);

            SExp *getcar(void) const;
            SExp *getcdr(void) const;
            void setcar(SExp*);
            void setcdr(SExp*);

            SExp *nthcar(int);
            SExp *nthcdr(int);

            void output(std::ostream&);
    };

    template <class T, Type X>
    class PodType : public SExp {
        protected:
            const T data;

        public:
            explicit PodType(T data) : SExp(X), data ( data ) {}
            T get(void) const { return data; }
    };

    class Int : public PodType<int,TYPE_INT> {
        public:
            explicit Int(int data) : PodType<int,TYPE_INT>(data) {}

            void output(std::ostream&);
    };

    class Symbol : public PodType<std::string,TYPE_SYMBOL> {
        public:
            explicit Symbol(std::string data) : PodType<std::string,TYPE_SYMBOL>(data) {}

            void output(std::ostream&);
    };

    class String : public PodType<std::string,TYPE_STRING> {
        public:
            explicit String(std::string data) : PodType<std::string,TYPE_STRING>(data) {}

            void output(std::ostream&);
    };

    class SExpParser {
        public:
            virtual void feedEnd(void) {}
            virtual bool feed(char) = 0;
            virtual bool done(void) const = 0;
            virtual SExp *get(void) = 0;
    };

    class NumberParser : public SExpParser {
        // expects everything
        private:
            char buffer[128];
            int length;
            bool isDone;

        public:
            NumberParser(void);
            void feedEnd(void) { isDone = true; }
            bool feed(char);
            bool done(void) const;
            SExp *get(void);
    };

    class SymbolParser : public SExpParser {
        // expects initial " cut off
        private:
            bool isDone;
            std::ostringstream oss;

        public:
            SymbolParser(void);

            void feedEnd(void) { isDone = true; }
            bool feed(char);
            bool done(void) const;
            SExp *get(void);
    };

    class StringParser : public SExpParser {
        // expects initial " cut off
        private:
            bool quoted, isDone;
            std::ostringstream oss;

        public:
            StringParser(void);

            bool feed(char);
            bool done(void) const;
            SExp *get(void);
    };

    class ListParser : public SExpParser {
        // expects initial ( cut off
        private:
            enum Phase {
                LIST_ITEMS,
                CDR_ITEM,
                WAITING_FOR_TERMINATION,
                DONE
            };

            Phase phase;
            std::list<SExp*> elements;
            SExp * terminatingCdr;

            SExpParser *subparser;

        public:
            ListParser(void);
            ~ListParser(void);

            bool feed(char);
            bool done(void) const;
            SExp *get(void);
    };

    class SExpStreamParser {
        private:
            std::queue<SExp*> rvs;
            SExpParser *parser;

        public:
            SExpStreamParser(void);
            ~SExpStreamParser(void);

            SExp *pop(void);
            bool empty(void) const;
            void feed(char);
            void end(void);
    };

    SExpParser *makeSExpParser(char);

    void outputSExp(SExp*,std::ostream&);
};

#endif
