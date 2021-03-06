#ifndef H_TYPESETTER
#define H_TYPESETTER

#include <ft2build.h>
#include FT_FREETYPE_H

#include <list>

#include <vector>
#include <stdint.h>

#include <string>

#include <SFML/Graphics.hpp>

typedef uint32_t ColRGBA;
#define COL_RED(col)   ((col&0xff000000)>>24)
#define COL_GREEN(col) ((col&0x00ff0000)>>16)
#define COL_BLUE(col)  ((col&0x0000ff00)>>8)
#define COL_ALPHA(col) ((col&0x000000ff)>>0)
#define MAKE_COL(r,g,b,a) (((r)<<24)|((g)<<16)|((b)<<8)|(a))


class PixelFilter {
    public:
        virtual ColRGBA operator()(ColRGBA) const = 0;
};

class ImageBuffer {
    // RGBA, the format loadable by 
    private:
        int width, height;
        ColRGBA *data;

    public:
        ImageBuffer(int,int);
        ~ImageBuffer(void);

        int getWidth(void) { return width; }
        int getHeight(void) { return height; }
        
        sf::Color getColourAt(int,int);

        void setPixel(int,int, ColRGBA);
        void putFTGraymap(int, int, FT_Bitmap*, const sf::Color&);

        void writeP6(FILE *);
};

class FreetypeLibrary {
    private:
        FT_Library library;
        static FreetypeLibrary *singleton;
    public:
        FreetypeLibrary(void);
        ~FreetypeLibrary(void);

        static FT_Library getSingleton(void);
};

class FreetypeFace {
    // not thread-safe!
    private:
        FT_Face face;
        int pixelSize;

    public:
        FreetypeFace(const std::string&, int);
        ~FreetypeFace(void);

        int getWidthOf(uint32_t);
        int getHeightOf(uint32_t);

        int getHeight(void);

        FT_Face getFace(void) const { return face; }
};

/* There's a lot of copying coming up -- not optimized
   for large amounts of text, and ESPECIALLY not for
   storing them in this form. Store either rendered,
   or non-wrapped.
*/

struct FormattedCharacter {
    // For efficient usage, only construct these at the
    // last moment from the more space-efficient state-delta
    // stream.
    // This contains sufficient information to actually paint
    // the character to a texture (and adjust the pen position).

    FreetypeFace* face;
    sf::Color colour;
    uint32_t character;

    FormattedCharacter(FreetypeFace&, const sf::Color&, uint32_t);
    FormattedCharacter(const FormattedCharacter&);
    const FormattedCharacter& operator=(const FormattedCharacter&);

    int getWidth(void);
    int getHeight(void);
    int getAscender(void);

    int render(int,int,ImageBuffer&) const;
};

class FormattedWord {
    private:
        int width, height;
        int maxAscender;
        typedef std::vector<FormattedCharacter> FCList;
        FCList components;
    
    public:
        FormattedWord(void);
        FormattedWord(const FormattedWord&);

        const FormattedWord& operator=(const FormattedWord&);

        void addCharacter(const FormattedCharacter&);

        int getLength(void) const { return components.size(); }

        int getWidth(void) const { return width; }
        int getHeight(void) const { return height; }
        int getAscender(void) const { return maxAscender; }
        void clear(void);

        std::string getRawText(void) const;

        int render(int,int,ImageBuffer&) const;

};

class FormattedLine {
    private:
        int wordWidth, height;
        int maxAscender;
        typedef std::vector<FormattedWord> FWList;
        FWList components;

    public:
        FormattedLine(void);

        int getWordWidth(void) const { return wordWidth; }
        int getHeight(void) const { return height; }
        int getBreaks(void) const;
        void clear(void);

        void addWord(const FormattedWord&);

        std::string getRawText(void) const;

        void renderLeftJustified(int,int,int,ImageBuffer&) const;
        void renderCentered(int,int,int,int,ImageBuffer&) const;
        void renderPadded(int,int,int,int,ImageBuffer&) const;
        int getWidthWithSpacing(int) const;
};

class LineRenderer {
    public:
        virtual void render(const FormattedLine&, bool) = 0;
};

class LineBuilder {
    private:
        std::vector<FormattedCharacter> characters;

        bool useCaret;
        FormattedCharacter *caret;

    public:
        LineBuilder(void);
        ~LineBuilder(void);

        void setCaret(const FormattedCharacter&);
        void addCharacter(const FormattedCharacter&);
        void backspace(void);

        FormattedLine createLine(void);

        void clear(void);
};

class WordWrapper {
    private:
        LineRenderer& renderer;

        FormattedLine currentLine;
        FormattedWord currentWord;

        int widthBound;
        int minimumWordSpacing;

        void endCurrentLine(bool);
        void handleNewWord(void);
    public:
        WordWrapper(LineRenderer&, int, int);

        void feed(const FormattedCharacter&);
        void end(void);

        void setWidthBound(int wB) { widthBound = wB; }
};

enum TsChartype {
    TSCT_NORMAL,
    TSCT_BREAK,
    TSCT_IGNORE,
    TSCT_NEWLINE
};

TsChartype classifyCharacter( uint32_t );

enum TextJustifyMode {
    TJM_LEFT,
    TJM_PAD,
    TJM_CENTER
};

class SfmlRectangularRenderer : public LineRenderer {
    private:
        int x, totalHeight;
        int width;
        int spacing;
        TextJustifyMode mode;

        typedef std::vector<ImageBuffer*> LineList;
        LineList lines;

    public:
        SfmlRectangularRenderer(int,int,TextJustifyMode);
        ~SfmlRectangularRenderer(void);

        void render(const FormattedLine&, bool);
        sf::Image* createImage(void);

        void setWidth(int);
};

class LabelSprite {
    private:
        sf::Image *image;
        sf::Sprite *sprite;

        void construct(int, const FormattedLine&);
    public:
        LabelSprite(const std::string&,sf::Color,FreetypeFace&);
        LabelSprite(FreetypeFace&, const FormattedLine&);
        ~LabelSprite(void);

        sf::Sprite& getSprite(void) { return *sprite; }

        void restrictToWidth(int);
        void noRestrictToWidth(void);

        void setPosition(int,int);

        int getWidth(void) const;
        int getHeight(void) const;

        void draw(sf::RenderTarget&) const;

        void setAlpha(int);
};

struct ChatLine {
    std::string username;
    sf::Color usernameColour;
    std::string text;
    sf::Color textColour;

    ChatLine(const std::string&, sf::Color, const std::string&, sf::Color);
};

class ChatLineSprite {
    private:
        int padding, height;
        LabelSprite *nameSprite;
        sf::Image *image;
        sf::Sprite *textSprite;

        void construct(int,const std::string&,sf::Color,const std::string&,sf::Color,FreetypeFace&);
    public:
        ChatLineSprite(int,const ChatLine&,FreetypeFace&);
        ChatLineSprite(int,const std::string&,sf::Color,const std::string&,sf::Color,FreetypeFace&);
        ~ChatLineSprite(void);

        int getHeight(void) const;
        int getOffset(void) const;

        void setPosition(int,int);

        void draw(sf::RenderWindow&) const;
};

class ChatInputLine {
    private:
        int width;

        sf::Color colour;
        FreetypeFace& face;

        LineBuilder builder;

        std::vector<FormattedCharacter> characters;

        LabelSprite *sprite;

        bool ready;
        int x, y;

        void update(void);

    public:
        ChatInputLine(int width, FreetypeFace&, sf::Color, FormattedCharacter);
        ~ChatInputLine(void);

        void clear(void);

        void setWidth(int);
        void add(uint32_t);
        void backspace(void);
        bool isDone(void) const { return ready; }

        void setPosition(int,int);

        int getHeight(void) const;

        void textEntered( uint32_t );

        std::string getString(void);
        void draw(sf::RenderWindow&);
};

class ChatBox {
    // no manual scrolling for now! infinite inaccessible text cache though
    private:
        FreetypeFace& face;

        typedef std::vector< ChatLine > ChatLineList;
        ChatLineList chatlines;

        sf::Color bgColour;
        int x, y;
        int width, height;

        typedef std::list< ChatLineSprite* > SpriteList;
        SpriteList renderedLinesCache;

        ChatLineSprite* render(const ChatLine&);
        void rebuildCache(void);
        void clearCache(void);

    public:
        ChatBox(int, int, int, int, FreetypeFace&, sf::Color);
        ~ChatBox(void);

        void setPosition(int,int);
        void resize(int, int);

        int getHeight(void) const { return height; }
        int getWidth(void) const { return width; }
        
        void add(const ChatLine&);
        void draw(sf::RenderWindow&);
};


#endif
