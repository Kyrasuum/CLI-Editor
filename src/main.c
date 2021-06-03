/*////////////////////////////
    Includes
*/////////////////////////////
    #include <argp.h>
    #include <ctype.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <ncurses.h>
    #include <sys/stat.h>

/*////////////////////////////
    Defines
*/////////////////////////////
    #define EXPECTED_ARGUMENTS  0
    #define STATE_FILE_EDIT     0
    #define STATE_FILE_SELECT   1
    #define PROG_DOC            "Lightweight text editor"
    #define ARGS_DOC            "FILE DIR"
    #define COLOR_GRAY          COLOR_WHITE+1
    #define COLOR_DARK_GRAY     COLOR_WHITE+2
    #define COLOR_TAN           COLOR_WHITE+3
    #define PAIR_DARK_GRAY      1
    #define PAIR_RED            2
    #define PAIR_GRAY           3
    #define PAIR_BLACK          4
    #define PAIR_TAN            5
    #define RGB_DARK_GRAY       100,100,100
    #define RGB_GRAY            150,150,150
    #define RGB_RED             784,0,0
    #define RGB_BLACK           0,0,0
    #define RGB_TAN             200,200,200

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
    struct state{
        u_int64_t               cursx;      //cursor x
        u_int64_t               cursy;      //cursor y
        u_int64_t               state;      //current program working state
        u_int64_t               margin_top; //top margin for file view
        u_int64_t               scrollx;    //horizontal scroll within file
        u_int64_t               scrolly;    //vertical scroll within file
        u_int64_t               line_count; //size of current file buffer
        char **                 buffer;     //current file buffer
        u_int64_t *             buffer_size;//length of each line number
        char *                  file;       //current file
        char *                  dir;        //current working directory
    };
    struct arguments{
      char *                    file;
      char *                    dir;
    };
    struct node;
    struct node{
        struct node *           next;
        void *                  data;
    };

/*////////////////////////////
    Typedefs
*/////////////////////////////
    typedef struct state        state_t;
    typedef struct node         node_t;
    typedef struct argp_option  argp_option_t;
    typedef struct argp_state   argp_state_t;
    typedef struct arguments    args_t;
    typedef struct argp         argp_t;
    typedef struct stat         stat_t;

/*////////////////////////////
    Prototypes
*/////////////////////////////
    //Output
        static void editorRefreshScreen();
    //File
        static void closeFile();
        static void openFile(char * fname);
        static void getFileContents();
        static int64_t getLineLength(u_int64_t line);
    //Cursor
        static void moveBegin();
        static void moveEOL();
        static void moveLeft();
        static void moveRight();
        static void moveDown();
        static void moveUp();
    //Input
        static void editorProcessKeypress();
    //Execution Flow
        static void initProgram();
        static void exitFunc();
        static void die(const char *s);
        static error_t parse_opt (int key, char * p_arg, argp_state_t * p_state);
    //Terminal
        static void initCurses();
        static void exitCurses();

/*////////////////////////////
    Globals
*/////////////////////////////
    state_t     program;
    char *      DEFAULT_FILE =          NULL;
    char *      DEFAULT_DIR =           NULL;
    u_int64_t   DEFAULT_STATE =         STATE_FILE_EDIT;
    u_int64_t   DEFAULT_SCROLL =        0;
    u_int64_t   FILE_BROWSER_WIDTH =    64;
    u_int64_t   MAX_PATH_SIZE =         1024;
    u_int64_t   MAX_LINE_SIZE =         512;
    u_int64_t   SCROLLX_BUFFER =        5;
    u_int64_t   SCROLLY_BUFFER =        3;

/*////////////////////////////
    Functions
*/////////////////////////////
    /*////////////////////////////
        Public Functions
    */////////////////////////////
        /*////////////////////////////
            Main
        */////////////////////////////
            int main(int argc, char ** argv){
                //options we handle
                argp_option_t options[] = {
                    {"file",        'f', "FILE",    0,  "file to open",         0},
                    {"directory",   'd', "DIR",     0,  "working directory",    0},
                    { 0 }
                };
            
                //defaults
                args_t args;
                args.file = DEFAULT_FILE;
                args.dir = DEFAULT_DIR;
                
                //process input
                argp_t argp = {options, parse_opt, ARGS_DOC, PROG_DOC, 0, 0, 0};
                argp_parse(&argp, argc, argv, 0, 0, &args);

                //start screen
                atexit(exitFunc);
                initCurses();

                //setup initial state
                initProgram();
                program.file = args.file;
                program.dir = args.dir;

                //colors
                init_color(COLOR_DARK_GRAY, RGB_DARK_GRAY);
                init_color(COLOR_RED, RGB_RED);
                init_color(COLOR_GRAY, RGB_GRAY);
                init_color(COLOR_BLACK, RGB_BLACK);
                init_color(COLOR_TAN, RGB_TAN);
                init_pair(PAIR_DARK_GRAY, COLOR_WHITE, COLOR_DARK_GRAY);
                init_pair(PAIR_RED, COLOR_WHITE, COLOR_RED);
                init_pair(PAIR_BLACK, COLOR_WHITE, COLOR_BLACK);
                init_pair(PAIR_GRAY, COLOR_WHITE, COLOR_GRAY);
                init_pair(PAIR_TAN, COLOR_WHITE, COLOR_TAN);
                wbkgd(stdscr, COLOR_PAIR(PAIR_DARK_GRAY));
                move(program.cursy + program.margin_top, program.cursx);
                refresh();

                //check if directory was set
                if (program.dir == DEFAULT_DIR){
                    char * curr_wd = calloc(MAX_PATH_SIZE, sizeof(char));
                    if (NULL == curr_wd){
                        die("main - calloc");
                    }
                    if (NULL != getcwd(curr_wd, MAX_PATH_SIZE)){
                        program.dir = curr_wd;
                    }else{
                        die("main - getcwd");
                    }
                }

                if (program.file == DEFAULT_FILE){
                    program.state = STATE_FILE_SELECT;
                }else{
                    openFile(args.file);
                }

                //begin main loop
                while (1){
                    editorRefreshScreen();
                    editorProcessKeypress();
                    refresh();
                }
                //exit
                return EXIT_SUCCESS;
            }

    /*////////////////////////////
        Private Functions
    */////////////////////////////
        /*////////////////////////////
            Output Functions
        */////////////////////////////
            static void editorRefreshScreen(){
                //store cursor position
                u_int64_t x;
                u_int64_t y;
                getyx(stdscr, y, x);
                move(0, 0);
                
                //print each line of buffer
                for (u_int64_t curr_line = 0; curr_line < (u_int64_t)LINES; curr_line++){
                    move(curr_line, 0);
                    clrtoeol();
                    switch (program.state){
                        case STATE_FILE_SELECT:
                            //print top banner
                            if (curr_line < program.margin_top){
                                attron(COLOR_PAIR(PAIR_RED));
                                u_int64_t file_length = strnlen(program.file, MAX_PATH_SIZE);
                                u_int64_t dir_length = strnlen(program.dir, MAX_PATH_SIZE);
                                mvwprintw(stdscr, curr_line, 0, program.file);
                                for (u_int64_t blank = file_length; blank < ((u_int64_t)COLS - dir_length) / 2; blank++){
                                    mvwprintw(stdscr, curr_line, blank, " ");
                                }
                                mvwprintw(stdscr, curr_line, (COLS - dir_length) / 2, program.dir);
                                for (u_int64_t blank = ((u_int64_t)COLS + dir_length) / 2; blank < (u_int64_t)COLS; blank++){
                                    mvwprintw(stdscr, curr_line, blank, " ");
                                }
                                attroff(COLOR_PAIR(PAIR_RED));
                                continue;
                            }
                            if (NULL != program.file){
                                attron(COLOR_PAIR(PAIR_TAN));
                                for (u_int64_t blank = 0; blank < FILE_BROWSER_WIDTH; blank++){
                                    mvwprintw(stdscr, curr_line, blank, " ");
                                }
                                attroff(COLOR_PAIR(PAIR_TAN));
                                if (curr_line + program.scrolly < program.line_count){
                                    //check if on current cursors line 
                                    if (curr_line == program.cursy - program.scrolly + program.margin_top){
                                        attron(COLOR_PAIR(PAIR_GRAY));
                                        mvwprintw(stdscr, curr_line, FILE_BROWSER_WIDTH, program.buffer[curr_line + program.scrolly - program.margin_top] + program.scrollx);
                                        for (u_int64_t blank = getLineLength(curr_line + program.scrolly - program.margin_top) + FILE_BROWSER_WIDTH; blank < (u_int64_t)COLS; blank++){
                                            mvwprintw(stdscr, curr_line, blank, " ");
                                        }
                                        attroff(COLOR_PAIR(PAIR_GRAY));
                                    //normal print
                                    }else{
                                        mvwprintw(stdscr, curr_line,  FILE_BROWSER_WIDTH, program.buffer[curr_line + program.scrolly - program.margin_top] + program.scrollx);
                                    }
                                }
                            }
                        break;
                        
                        case STATE_FILE_EDIT:
                            //print top banner
                            if (curr_line < program.margin_top){
                                attron(COLOR_PAIR(PAIR_RED));
                                u_int64_t file_length = strnlen(program.file, MAX_PATH_SIZE);
                                u_int64_t dir_length = strnlen(program.dir, MAX_PATH_SIZE);
                                mvwprintw(stdscr, curr_line, 0, program.file);
                                for (u_int64_t blank = file_length; blank < ((u_int64_t)COLS - dir_length) / 2; blank++){
                                    mvwprintw(stdscr, curr_line, blank, " ");
                                }
                                mvwprintw(stdscr, curr_line, (COLS - dir_length) / 2, program.dir);
                                for (u_int64_t blank = ((u_int64_t)COLS + dir_length) / 2; blank < (u_int64_t)COLS; blank++){
                                    mvwprintw(stdscr, curr_line, blank, " ");
                                }
                                attroff(COLOR_PAIR(PAIR_RED));
                                continue;
                            }
                            if (curr_line + program.scrolly < program.line_count){
                                //check if on current cursors line 
                                if (curr_line == program.cursy - program.scrolly + program.margin_top){
                                    attron(COLOR_PAIR(PAIR_GRAY));
                                    mvwprintw(stdscr, curr_line, 0, program.buffer[curr_line + program.scrolly - program.margin_top] + program.scrollx);
                                    for (u_int64_t blank = getLineLength(curr_line + program.scrolly - program.margin_top); blank < (u_int64_t)COLS; blank++){
                                        mvwprintw(stdscr, curr_line, blank, " ");
                                    }
                                    attroff(COLOR_PAIR(PAIR_GRAY));
                                //normal print
                                }else{
                                    mvwprintw(stdscr, curr_line, 0, program.buffer[curr_line + program.scrolly - program.margin_top] + program.scrollx);
                                }
                            }
                        break;
                    }
                }
                //restore cursor position
                move(y, x);
            }
        /*////////////////////////////
            File Functions
        */////////////////////////////
            /*////////////////////////////
                Close the currently open file
            */////////////////////////////
                static void closeFile(){
                    program.file = NULL;
                    for (u_int64_t iter = 0; iter < program.line_count; iter++){
                        free(program.buffer[iter]);
                    }
                    free(program.buffer);
                    free(program.buffer_size);
                    program.cursx = 0;
                    program.cursy = 0;
                    program.scrolly = DEFAULT_SCROLL;
                    program.scrollx = 0;
                    program.line_count = 0;
                    program.buffer_size = 0;
                    program.margin_top = 1;
                }
            /*////////////////////////////
                Open a file
            */////////////////////////////
                static void openFile(char * fname){
                    if (NULL != program.file){
                        closeFile();
                    }
                    program.file = fname;
                    getFileContents();
                }
            /*////////////////////////////
                Get file contents
            */////////////////////////////
                static void getFileContents(){
                    if (NULL == program.file){
                        return;
                    }
                    //open file
                    int file_desc = open(program.file, O_RDONLY);
                    if (-1 == file_desc){
                        return;
                    }
                    //setup for loop
                    program.line_count++;
                    program.buffer = recalloc(program.buffer, program.line_count, sizeof(char*));
                    if (NULL == program.buffer){
                        die("getLineContents - calloc");
                    }
                    program.buffer_size = recalloc(program.buffer_size, program.line_count, sizeof(u_int64_t));
                    if (NULL == program.buffer_size){
                        die("getLineContents - calloc");
                    }
                    program.buffer_size[program.line_count - 1] = 0;
                    program.buffer[program.line_count-1] = calloc(MAX_LINE_SIZE, sizeof(char));
                    if (NULL == program.buffer[program.line_count - 1]){
                        die("getLineContents - calloc");
                    }
                    u_int64_t offset = 0;
                    char buffer;
                    //read contents
                    for(;;){
                        //attempt to read
                        ssize_t read_size = pread(file_desc, &buffer, 1, offset);
                        if (0 >= read_size){
                            break;
                        }
                        //copy into buffer
                        offset++;
                        memcpy(program.buffer[program.line_count - 1] + program.buffer_size[program.line_count - 1], &buffer, 1);
                        program.buffer_size[program.line_count - 1]++;
                        //check for end of lines
                        if (buffer == '\n'){
                            program.line_count++;
                            program.buffer = recalloc(program.buffer, program.line_count, sizeof(char*));
                            if (NULL == program.buffer){
                                die("getLineContents - calloc");
                            }
                            program.buffer_size = recalloc(program.buffer_size, program.line_count, sizeof(u_int64_t));
                            if (NULL == program.buffer_size){
                                die("getLineContents - calloc");
                            }
                            program.buffer_size[program.line_count - 1] = 0;
                            program.buffer[program.line_count-1] = calloc(MAX_LINE_SIZE, sizeof(char));
                            if (NULL == program.buffer[program.line_count - 1]){
                                die("getLineContents - calloc");
                            }
                        }
                    }
                    //cleanup
                    close(file_desc);
                }
            /*////////////////////////////
                Get line length
                    returns the length of a specific lineg
            */////////////////////////////
                static int64_t getLineLength(u_int64_t line){
                    if (NULL == program.file){
                        return -1;
                    }
                    if (line >= program.line_count){
                        return -1;
                    }
                    if (0 == program.buffer_size[line]){
                        return 0;
                    }
                    return program.buffer_size[line] - 1;
                }
        /*////////////////////////////
            Cursor Functions
        */////////////////////////////
            /*////////////////////////////
                Move to beginning of line
            */////////////////////////////
                static void moveBegin(){
                    program.cursx = 0;
                    program.scrollx = 0;
                    move(program.cursy - program.scrolly + program.margin_top, program.cursx);
                }
            /*////////////////////////////
                Move to end of line
            */////////////////////////////
                static void moveEOL(){
                    int64_t length = getLineLength(program.cursy);
                    if (0 > length){
                        return;
                    }
                    program.cursx = length;
                    while ((program.cursx - program.scrollx > (u_int64_t)COLS - SCROLLX_BUFFER) && (length - program.scrollx >= (u_int64_t)COLS) && (UINT64_MAX > program.scrollx)){
                        program.scrollx++;
                    }
                    move(program.cursy - program.scrolly + program.margin_top, program.cursx - program.scrollx);
                }
            /*////////////////////////////
                Move cursor left
            */////////////////////////////
                static void moveLeft(){
                    int64_t length = getLineLength(program.cursy);
                    //if we were previously beyond line length
                    if ((0 <= length) && (program.cursx > (u_int64_t)length)){
                        program.cursx = length;
                    }
                    //if there is more to our left
                    if (0 < program.cursx){
                        program.cursx--;
                        //scroll left
                        if ((0 < program.scrollx) && (program.cursx - program.scrollx < SCROLLX_BUFFER)){
                            program.scrollx--;
                        }
                        move(program.cursy - program.scrolly + program.margin_top, program.cursx - program.scrollx);
                    }else{
                        //go to end of previous line
                        if (0 < program.cursy){
                            moveUp();
                            moveEOL();
                        }
                    }
                }
            /*////////////////////////////
                Move cursor right
            */////////////////////////////
                static void moveRight(){
                    int64_t length = getLineLength(program.cursy);
                    //there is more to right
                    if ((0 <= length) && (program.cursx + 1 <= (u_int64_t)length)){
                        program.cursx++;
                        //scroll right
                        if (((u_int64_t)COLS < program.cursx + SCROLLX_BUFFER) && (length - program.scrollx >= (u_int64_t)COLS) && (UINT64_MAX > program.scrollx)){
                            program.scrollx++;
                        }
                        move(program.cursy - program.scrolly + program.margin_top, program.cursx - program.scrollx);
                    }else{
                        //go to start of next line
                        if (program.cursy < program.line_count){
                            moveDown(program);
                            moveBegin(program);
                        }
                    }
                }
            /*////////////////////////////
                Move cursor down
            */////////////////////////////
                static void moveDown(){
                    int64_t length = getLineLength(program.cursy + 1);
                    if (0 <= length){
                        program.cursy++;
                        //if we need to scroll down
                        if ((UINT64_MAX > program.scrolly) && (program.cursy - program.scrolly + program.margin_top > (u_int64_t)LINES - SCROLLY_BUFFER)){
                            program.scrolly++;
                        }
                        //if our x cursor was greater than line length
                        if (program.cursx > (u_int64_t)length){
                            //move our horizontal scroll left
                            while ((0 < program.scrollx) && (program.scrollx > (u_int64_t)length - SCROLLX_BUFFER)){
                                program.scrollx--;
                            }
                            move(program.cursy - program.scrolly + program.margin_top, length - program.scrollx);
                        }else{//if our x cursor wasnt greater than line length
                            //move our horizontal scroll right
                            while ((program.cursx - program.scrollx > (u_int64_t)COLS - SCROLLX_BUFFER) && (length - program.scrollx >= (u_int64_t)COLS) && (UINT64_MAX > program.scrollx)){
                                program.scrollx++;
                            }
                            move(program.cursy - program.scrolly + program.margin_top, program.cursx - program.scrollx);
                        }
                    }
                }
            /*////////////////////////////
                Move cursor up
            */////////////////////////////
                static void moveUp(){
                    if (0 < program.cursy){
                        program.cursy--;
                        //if we need to scroll down
                        if ((0 < program.scrolly) && (program.cursy - program.scrolly + program.margin_top < SCROLLY_BUFFER)){
                            program.scrolly--;
                        }
                        int64_t length = getLineLength(program.cursy);
                        //if our x cursor was greater than line length
                        if ((0 <= length) && (program.cursx > (u_int64_t)length)){
                            //move our horizontal scroll left
                            while ((0 < program.scrollx) && (program.scrollx > (u_int64_t)length - SCROLLX_BUFFER)){
                                program.scrollx--;
                            }
                            move(program.cursy - program.scrolly + program.margin_top, length - program.scrollx);
                        }else{//if our x cursor was greater than line length
                            //move our horizontal scroll right
                            while ((program.cursx - program.scrollx > (u_int64_t)COLS - SCROLLX_BUFFER) && (length - program.scrollx >= (u_int64_t)COLS) && (UINT64_MAX > program.scrollx)){
                                program.scrollx++;
                            }
                            move(program.cursy - program.scrolly + program.margin_top, program.cursx - program.scrollx);
                        }
                    }
                }
        /*////////////////////////////
            Input Functions
        */////////////////////////////
            /*////////////////////////////
                Process Key for editor
                    gets input and processes it
            */////////////////////////////
                static void editorProcessKeypress(){
                    int64_t input = getch();
                    MEVENT event;
                    //keybinds that are always valid
                    switch (input){
                        case CTRL_KEY('q'):
                            exit(EXIT_SUCCESS);
                        break;
                        default:
                        break;
                    }
                    if ((NULL == program.file) || (STATE_FILE_SELECT == program.state)){
                        //when no file is open
                        switch (input){
                            case KEY_MOUSE:
                                if (getmouse(&event) == OK){
                                    //left mouse button on down
                                    if(event.bstate & BUTTON1_PRESSED){
                                        int x = strnlen(program.file, MAX_PATH_SIZE);
                                        int y = program.margin_top;
                                        wmouse_trafo(stdscr, &y, &x, FALSE);
                                        if ((event.y < y) && (event.x < x)){
                                            program.state = STATE_FILE_EDIT;
                                            move(program.cursy - program.scrolly + program.margin_top, program.cursx - program.scrollx);
                                        }
                                    }
                                    //middle mouse button on down
                                    else if(event.bstate & BUTTON2_PRESSED){
                                    }
                                    //right mouse button on down
                                    else if(event.bstate & BUTTON3_PRESSED){
                                    }
                                }
                            break;
                            default:
                            break;
                        }
                    }
                    else if ((NULL != program.file) && (STATE_FILE_EDIT == program.state)){
                        //keybinds specific to open files
                        switch (input){
                            case KEY_LEFT:
                                moveLeft();
                            break;
                            case KEY_RIGHT:
                                moveRight();
                            break;
                            case KEY_DOWN:
                                moveDown();
                            break;
                            case KEY_UP:
                                moveUp();
                            break;
                            case KEY_ENTER:
                            break;
                            case KEY_BACKSPACE:
                            break;
                            case KEY_DC: //delete key
                            break;
                            case KEY_MOUSE:
                                if (getmouse(&event) == OK){
                                    //left mouse button on down
                                    if(event.bstate & BUTTON1_PRESSED){
                                        int x = strnlen(program.file, MAX_PATH_SIZE);
                                        int y = program.margin_top;
                                        wmouse_trafo(stdscr, &y, &x, FALSE);
                                        if ((event.y < y) && (event.x < x)){
                                            program.state = STATE_FILE_SELECT;
                                            move(program.cursy - program.scrolly + program.margin_top, program.cursx - program.scrollx + FILE_BROWSER_WIDTH);
                                        }
                                    }
                                    //middle mouse button on down
                                    else if(event.bstate & BUTTON2_PRESSED){
                                    }
                                    //right mouse button on down
                                    else if(event.bstate & BUTTON3_PRESSED){
                                    }
                                }
                            break;
                            default:
                            break;
                        }
                    }
                }

        /*////////////////////////////
            Execution Flow
        */////////////////////////////
            /*////////////////////////////
                Init, initializes program state to defaults
            */////////////////////////////
                static void initProgram(){
                    program.file = DEFAULT_FILE;
                    program.dir = DEFAULT_DIR;
                    program.cursx = 0;
                    program.cursy = 0;
                    program.scrolly = DEFAULT_SCROLL;
                    program.scrollx = 0;
                    program.line_count = 0;
                    program.buffer = NULL;
                    program.buffer_size = 0;
                    program.margin_top = 1;
                }
            /*////////////////////////////
                At Exit
                    performs program exiting process
            */////////////////////////////
                static void exitFunc(){
                    exitCurses();
                }
            /*////////////////////////////
                Die, exit program and display error
            */////////////////////////////
                static void die(const char *s){
                    exitCurses();
                    perror(s);
                    exit(EXIT_FAILURE);
                }
            /*////////////////////////////
                Parse Options
                    source: http://crasseux.com/books/ctutorial/argp-example.html
                    uses argp.h library and processes command line input
            */////////////////////////////
                static error_t parse_opt (int key, char * p_arg, argp_state_t * p_state){
                    args_t * p_input = p_state->input;
                    static u_int8_t args_count = EXPECTED_ARGUMENTS;
                    switch (key){
                        case 'f':
                            p_input->file = p_arg;
                            args_count--;
                        break;
                        case 'd':
                            p_input->dir = p_arg;
                            args_count--;
                        break;
                        case ARGP_KEY_ARG:
                            switch (p_state->arg_num){
                                case 0:
                                    p_input->file = p_arg;
                                    args_count--;
                                break;
                                case 1:
                                    p_input->dir = p_arg;
                                    args_count--;
                                break;
                                default:
                                    argp_usage(p_state);
                                break;
                            }
                        break;
                        case ARGP_KEY_END:
                        break;
                        default:
                            return ARGP_ERR_UNKNOWN;
                        break;
                    }
                    return 0;
                }
        /*////////////////////////////
            Terminal Functions
        */////////////////////////////
            /*////////////////////////////
                Disable Raw Mode for terminal
                    used to restore terminal to previous state
            */////////////////////////////
                static void exitCurses(){
                    endwin();
                }
            /*////////////////////////////
                Enable Raw Mode for terminal
                    transitions terminal to state used by program
            */////////////////////////////
                static void initCurses(){
                    initscr();              //start screen
                    start_color();          //initialize colors
                    noecho();               //dont echo keystrokes
                    cbreak();               //disable line buffering
                    raw();                  //
                    keypad(stdscr, TRUE);   //enable special keys
                    mousemask(ALL_MOUSE_EVENTS, NULL);
                    mouseinterval(1);
                }
//End of file
