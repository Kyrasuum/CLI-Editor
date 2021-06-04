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
    #include <sys/stat.h>

/*////////////////////////////
    Defines
*/////////////////////////////
    #define EXPECTED_ARGUMENTS  0
    #define STATE_FILE_EDIT     0
    #define STATE_FILE_SELECT   1
    #define PROG_DOC            "Lightweight text editor"
    #define ARGS_DOC            "FILE DIR"

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
    typedef struct node         node_t;
    typedef struct argp_option  argp_option_t;
    typedef struct argp_state   argp_state_t;
    typedef struct arguments    args_t;
    typedef struct argp         argp_t;
    typedef struct stat         stat_t;

/*////////////////////////////
    Prototypes
*/////////////////////////////
    //File
        static void closeFile();
        static void openFile(char * fname);
        static void getFileContents();
        static int64_t getLineLength(u_int64_t line);
    //Execution Flow
        static void exitFunc();
        static void die(const char *s);
        static error_t parse_opt (int key, char * p_arg, argp_state_t * p_state);
    //Terminal

/*////////////////////////////
    Globals
*/////////////////////////////
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

                atexit(exitFunc);

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
                }
                //exit
                return EXIT_SUCCESS;
            }

    /*////////////////////////////
        Private Functions
    */////////////////////////////
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
            Execution Flow
        */////////////////////////////
            /*////////////////////////////
                At Exit
                    performs program exiting process
            */////////////////////////////
                static void exitFunc(){
                }
            /*////////////////////////////
                Die, exit program and display error
            */////////////////////////////
                static void die(const char *s){
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
//End of file
