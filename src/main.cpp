/*////////////////////////////
    Includes
*/////////////////////////////
    #include <argp.h>
    #include <stdlib.h>
    #include "editor/editor.hpp"

/*////////////////////////////
    Defines
*/////////////////////////////
    #define EXPECTED_ARGUMENTS  0
    #define PROG_DOC            "Lightweight text editor"
    #define ARGS_DOC            "FILE DIR"
    #define DEFAULT_FILE        NULL
    #define DEFAULT_DIR         NULL

/*////////////////////////////
    Structs
*/////////////////////////////
    struct arguments{
        char *                  file;
        char *                  dir;
    };

/*////////////////////////////
    Typedefs
*/////////////////////////////
    typedef struct argp_option  argp_option_t;
    typedef struct argp_state   argp_state_t;
    typedef struct arguments    args_t;
    typedef struct argp         argp_t;

/*////////////////////////////
    Prototypes
*/////////////////////////////
    static error_t parse_opt (int key, char * p_arg, argp_state_t * p_state);

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
                    { 0,            0,   0,         0,  0,                      0}
                };
            
                //defaults
                args_t args;
                args.file = DEFAULT_FILE;
                args.dir = DEFAULT_DIR;
                
                //process input
                argp_t argp = {options, parse_opt, ARGS_DOC, PROG_DOC, 0, 0, 0};
                argp_parse(&argp, argc, argv, 0, 0, &args);

                editor program = editor(args.file, args.dir);
                program.run();
                //exit
                return EXIT_SUCCESS;
            }

    /*////////////////////////////
        Private Functions
    */////////////////////////////
        /*////////////////////////////
            Parse Options
                source: http://crasseux.com/books/ctutorial/argp-example.html
                uses argp.h library and processes command line input
        */////////////////////////////
            static error_t parse_opt (int key, char * p_arg, argp_state_t * p_state){
                args_t * p_input = (args_t*)p_state->input;
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
