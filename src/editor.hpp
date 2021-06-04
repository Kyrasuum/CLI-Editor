/*////////////////////////////
    Includes
*/////////////////////////////
    #define Uses_TKeys
    #define Uses_TApplication
    #define Uses_TEvent
    #define Uses_TRect
    #define Uses_TDialog
    #define Uses_TStaticText
    #define Uses_TButton
    #define Uses_TMenuBar
    #define Uses_TSubMenu
    #define Uses_TMenuItem
    #define Uses_TStatusLine
    #define Uses_TStatusItem
    #define Uses_TStatusDef
    #define Uses_TDeskTop
    #include "../deps/tvision/tv.h" 

/*////////////////////////////
    Defines
*/////////////////////////////

/*////////////////////////////
    Structs
*/////////////////////////////
    struct node;
    struct node{
        struct node *           next;
        void *                  data;
    };

/*////////////////////////////
    Typedefs
*/////////////////////////////
    typedef struct node         node_t;

/*////////////////////////////
    Globals
*/////////////////////////////

/*////////////////////////////
    Editor Class
*/////////////////////////////
    class editor: public TApplication{
        private:
        protected:
            //constructors
                editor();
            //member variables
                char *          file;       //Currently open
                char *          dir;        //Current working directory
            //functions
                void handleEvent(TEvent&);
                static TMenuBar * initMenuBar(TRect);
                static TStatusLine * initStatusLine(TRect);
        public:
            //constructors
                editor(char * file, char * dir);
            //destructors
                ~editor();
    };
//End of file
