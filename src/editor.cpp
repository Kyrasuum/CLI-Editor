/*////////////////////////////
    Includes
*/////////////////////////////
    #include "editor.hpp"
    #include <ctype.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <sys/stat.h>

/*////////////////////////////
    Defines
*/////////////////////////////
    #define STATE_FILE_EDIT     0
    #define STATE_FILE_SELECT   1

/*////////////////////////////
    Macros
*/////////////////////////////
    #define CTRL_KEY(k) ((k) & 0x1f)
    #define free(pointer) (\
        {\
            (free(pointer));\
            ((pointer) = (NULL));\
        }\
        )
    #define recalloc(mem_ptr, count, size) (\
        {\
            if (NULL == (mem_ptr))\
            {\
                (mem_ptr) = calloc((count), (size));\
            }\
            else\
            {\
                (mem_ptr) = realloc((mem_ptr), (count) * (size));\
            }\
            mem_ptr;\
        }\
        )
    #define node_data(node, node_type) (((typeof(node_type)*) ((node)->data))[0])

/*////////////////////////////
    Structs
*/////////////////////////////

/*////////////////////////////
    Typedefs
*/////////////////////////////

/*////////////////////////////
    Globals
*/////////////////////////////

/*////////////////////////////
    Functions
*/////////////////////////////
    //Constructors
        editor::editor():TProgInit(
            &editor::initStatusLine,
            &editor::initMenuBar,
            &editor::initDeskTop
        ){
            this->file = NULL;
            this->dir = NULL;
        }
        editor::editor(char * file, char * dir):editor(){
            this->file = file;
            this->dir = dir;
        }
    //Destructors
        editor::~editor(){
        }
    //Handle Events
        void editor::handleEvent(TEvent& event){
            TApplication::handleEvent(event);
            if(event.what == evCommand){
                switch(event.message.command){
                    default:
                    break;
                }
            }
        }
    //Status Line
        TMenuBar * editor::initMenuBar(TRect rect){
            rect.b.y = rect.a.y+1;
            return new TMenuBar(rect, 
                *new TSubMenu( "~Esc~-Menu", kbEsc ) +
                *new TMenuItem( "~Q~uit", cmQuit, cmQuit, hcNoContext, "Ctrl-Q" )
            );
        }
    //Menu Bar
        TStatusLine * editor::initStatusLine(TRect rect){
            rect.a.y = rect.b.y-1;
            return new TStatusLine(rect, 
                *new TStatusDef( 0, 0xFFFF ) +
                *new TStatusItem( "~Ctrl-Q~ ~Q~uit", kbCtrlQ, cmQuit ) +
                *new TStatusItem( 0, kbEsc, cmMenu )
            );
        }
//End of file
