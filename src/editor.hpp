/*////////////////////////////
    Includes
*/////////////////////////////
    #include "../deps/imtui/imtui.h"
    #include "../deps/imtui/imtui-impl-ncurses.h"

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
    class editor{
        private:
        protected:
            //constructors
                editor();
            //member variables
                char *          file;       //Currently open
                char *          dir;        //Current working directory
            //functions
        public:
            //constructors
                editor(char * file, char * dir);
            //destructors
                ~editor();
            //run
                void run();
    };
//End of file
