/*////////////////////////////
    Includes
*/////////////////////////////
    #include <string>
    #include "../imtui/imtui.hpp"
    #include "../imtui/imtui-ncurses.hpp"

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
    struct file{
        char *      fpath;              //file path for file
        char **     buff;               //text buffer for file line by line
        uint64_t *  buff_len;           //length of each line in buffer
        uint64_t    lines;              //Number of lines in file
        uint8_t     modified;           //Modified flag for file
        ImVec2      curs_pos;           //Position of cursor
    };

/*////////////////////////////
    Typedefs
*/////////////////////////////
    typedef struct node         node_t;
    typedef struct file         file_t;

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
                file_t **       files;      //Currently open files
                uint64_t        files_cnt;  //Number of currently open files
                file_t *        curr_f;     //Currently viewed file 
                char *          dir;        //Current working directory
            //functions
                //helpers
                    template <class T> int numDigits(T number);
                    std::string GetStdoutFromCommand(std::string cmd);
                    template<typename ... Args> std::string string_format( const std::string& format, Args ... args );
                void die(const char * str);
                //files
                    void openfile(char * fpath);
                    void readfile(file_t * &file);
                    void closefile(file_t * file);
        public:
            //constructors
                editor(char * tgts, char * dir);
            //destructors
                ~editor();
            //run
                void run();
    };
//End of file
