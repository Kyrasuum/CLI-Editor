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
    #include <sys/mman.h>
    #include <memory>
/*////////////////////////////
    Defines
*/////////////////////////////
    #define STATE_FILE_EDIT     0
    #define STATE_FILE_SELECT   1
    #define MAX_CMD_BUFFER      256
    #define MAX_LINE_SIZE       256

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
            void * val;\
            if (NULL == (mem_ptr))\
            {\
                val = calloc((count), (size));\
            }\
            else\
            {\
                val = realloc((mem_ptr), (count) * (size));\
            }\
            val;\
        }\
        )
    #define remmap(mem_ptr, count, size, old_count, old_size, ...) (\
    {\
        void * val = mmap(NULL, count * size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);\
        if (NULL == val)\
        {\
            __VA_ARGS__;\
        }\
        memset(val, 0, count * size);\
        if (NULL != (mem_ptr))\
        {\
            memcpy(val, mem_ptr, old_count * old_size);\
            if (0 != munmap(mem_ptr, old_count * old_size))\
            {\
                __VA_ARGS__;\
            }\
        }\
        val;\
    }\
    )
    #define node_data(node, node_type) (((__typeof__(node_type)*) ((node)->data))[0])

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
        editor::editor(){
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
        
            this->files = NULL;
            this->files_cnt = 0;
            this->curr_f = NULL;
            this->dir = NULL;
        }
        editor::editor(char * tgts, char * dir):editor(){
            if (NULL != tgts){
                //verifies file exists and attempts to resolve any special characters like * into multiple files
                std::string cmd = std::string("ls ").append(std::string(tgts));
                std::string res = GetStdoutFromCommand(cmd);
                size_t pos = res.find("\n");
                std::string sub_res = res.substr(0);
                while (pos != std::string::npos){
                    std::string fpath = sub_res.substr(0, pos);
                    openfile((char*)fpath.c_str());
                    std::string sub_res = sub_res.substr(pos);
                    pos = sub_res.find("\n");
                }
            }
            this->dir = dir;
        }
    //Destructors
        editor::~editor(){
            ImTui_ImplText_Shutdown();
            ImTui_ImplNcurses_Shutdown();
        }
    /*////////////////////////////
        Die
            exit program and display error
    */////////////////////////////
        void editor::die(const char * str){
            ImTui_ImplText_Shutdown();
            ImTui_ImplNcurses_Shutdown();
            perror(str);
            exit(EXIT_FAILURE);
        }
    /*////////////////////////////
        File functions
    */////////////////////////////    
        /*////////////////////////////
            Open a file
                sets up memory for a file
        */////////////////////////////
            void editor::openfile(char * fpath){
                if (NULL == fpath){
                    return;
                }
                
                file_t * nfile = (file_t*)calloc(1, sizeof(file_t));
                if (NULL == nfile){
                    fprintf(stderr, "%s", "Error allocating memory for file");
                    return;
                }
                nfile->fpath = fpath;
                nfile->lines = 0;
                nfile->buff = NULL;
                nfile->buff_len = NULL;
                nfile->modified = 0;
                nfile->curs_pos = ImVec2(0,0);

                this->files = (file_t**)recalloc(this->files, this->files_cnt + 1, sizeof(file_t*));
                if (NULL == this->files){
                    fprintf(stderr, "%s", "Error allocating memory in files array");
                    free(nfile);
                    return;
                }
                readfile(nfile);
                if (NULL != nfile){
                    this->files[this->files_cnt++] = nfile;
                    this->curr_f = nfile;
                }
            }
        /*////////////////////////////
            Read File
                Gets file contents
        */////////////////////////////
            void editor::readfile(file_t * &file){
                if (NULL == file){
                    return;
                }

                //open file
                int file_desc = open(file->fpath, O_RDONLY);
                if (-1 == file_desc){
                    free(file);
                    return;
                }
                //setup for loop
                file->buff = (char**)remmap(file->buff, file->lines + 1, sizeof(char*), file->lines, sizeof(char*),
                    {
                        die("getLineContents - remmap buff");
                    }
                );

                file->buff_len = (uint64_t*)remmap(file->buff_len, file->lines + 1, sizeof(uint64_t), file->lines, sizeof(uint64_t),
                    {
                        die("getLineContents - remmap buff_len");
                    }
                );

                file->buff[file->lines] = (char*)remmap(file->buff[file->lines], MAX_LINE_SIZE, sizeof(char), 0, 0,
                    {
                        die("getLineContents - remmap buff[]");
                    }
                );
                file->lines++;

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
                    memcpy(file->buff[file->lines - 1] + file->buff_len[file->lines - 1], &buffer, 1);
                    file->buff_len[file->lines - 1]++;
                    //check for end of lines
                    if (buffer == '\n'){
                        file->buff = (char**)remmap(file->buff, file->lines + 1, sizeof(char*), file->lines, sizeof(char*),
                            {
                                die("getLineContents - remmap buff");
                            }
                        );

                        file->buff_len = (uint64_t*)remmap(file->buff_len, file->lines + 1, sizeof(uint64_t), file->lines, sizeof(uint64_t),
                            {
                                die("getLineContents - remmap buff_len");
                            }
                        );

                        file->buff[file->lines] = (char*)remmap(file->buff[file->lines], MAX_LINE_SIZE, sizeof(char), 0, 0,
                            {
                                die("getLineContents - remmap buff[]");
                            }
                        );
                        file->lines++;
                    }
                }
                //cleanup
                close(file_desc);
            }
        /*////////////////////////////
            Close the currently open file
        */////////////////////////////
            void editor::closefile(file_t * file){
                if (NULL == file){
                    return;
                }
                
                uint64_t index = 0;
                while (index < this->files_cnt && file != this->files[index]){
                    index++;
                }
                if (index >= this->files_cnt){
                    return;
                }
                this->files_cnt--;

                if (this->curr_f == file){
                    if (this->files_cnt == index){
                        this->curr_f = this->files[index - 1];
                    }else{
                        this->curr_f = this->files[index + 1];
                    }
                }
                free(file);
                while (index < this->files_cnt){
                    this->files[index] = this->files[index+1];
                    index++;
                }
            }
    
    /*////////////////////////////
        Run
            executes every frame
    */////////////////////////////
        void editor::run(){
            auto screen = ImTui_ImplNcurses_Init(true);
            ImTui_ImplText_Init();
            //base style
            ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.784f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            //draw gui each frame
            uint64_t curs_delta = 0;
            for (;;){
                ImTui_ImplNcurses_NewFrame();
                ImTui_ImplText_NewFrame();
        
                ImGui::NewFrame();

                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
                io.ConfigInputTextCursorBlink = true;

                auto wSize = io.DisplaySize;
                ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
                // - 1 for status bar at bottom
                ImGui::SetNextWindowSize(ImVec2(wSize.x, wSize.y - 1), ImGuiCond_Always);

                ImGui::Begin("Main", nullptr,
                    // ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_MenuBar |
                    ImGuiWindowFlags_NoTitleBar
                );
                //menu bar
                if (ImGui::BeginMenuBar()){
                    if (ImGui::BeginMenu("File ")){
                        if (ImGui::MenuItem(" Open", "Ctrl+O")){}
                        if (ImGui::MenuItem(" Save", "Ctrl+S")){}
                        if (ImGui::MenuItem(" Close", "Ctrl+W")){}
                        if (ImGui::MenuItem(" Quit", "Ctrl+Q")){break;}
                        ImGui::EndMenu();
                    }
                    if (ImGui::Button("Tab ")){}
                    ImGui::EndMenuBar();
                }
                
                if (io.KeysDown[CTRL_KEY('q')]){break;}
                

                //draw file contents
                float scrollY = ImGui::GetScrollY();
                if (NULL != this->curr_f){
                    uint8_t indent = numDigits(this->curr_f->lines+1);
                    for (uint64_t iter = 0; iter < this->curr_f->lines; iter++){
                        ImDrawList* draw_list = ImGui::GetWindowDrawList();
                        //create line numbering variables
                        uint8_t used = numDigits(iter+1);
                        std::string spaces(indent-used, ' ');
                        std::string line = string_format("%s%ld:", spaces.c_str(), iter + 1);
                        ImGui::Text("%s", line.c_str());
                        ImGui::SameLine();
                        ImVec2 p0 = ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y);
                        ImVec2 p1 = ImVec2(p0.x + line.size() + 2, ImGui::GetItemRectMax().y);
                        //Draw line numbering (avoid drawing over status bar)
                        if (iter - scrollY < wSize.y - 2){
                            draw_list->PushClipRect(p0, p1, IM_COL32(33, 33, 33, 255));
                            draw_list->AddRectFilled(p0, p1, IM_COL32(33, 33, 33, 255));
                            draw_list->AddText(p0, IM_COL32(175, 175, 175, 255), line.c_str());
                            draw_list->PopClipRect();
                        }
                        
                        //draw line contents
                        ImGui::Text("%s", this->curr_f->buff[iter]);
                        if (ImGui::IsItemClicked()){
                            this->curr_f->curs_pos.y = iter;
                            for (uint64_t iter_c = 0; iter_c < this->curr_f->buff_len[iter]; iter_c++){
                                ImVec2 p2 = ImVec2(p1.x + iter_c, p0.y);
                                ImVec2 p3 = ImVec2(p2.x + 1, p1.y);
                                ImVec2 mouse = ImGui::GetMousePos();
                                if (mouse.x >= p2.x && mouse.y >= p2.y && mouse.x <= p3.x && mouse.y <= p3.y){
                                    this->curr_f->curs_pos.x = iter_c;
                                }
                            }
                        }
                        //draw invisble button for remainder to allow clicks here
                        ImGui::SameLine();
                        ImGui::InvisibleButton("##ENDLINE", ImVec2(wSize.x, 1));
                        if (ImGui::IsItemClicked()){
                            this->curr_f->curs_pos.y = iter;
                            this->curr_f->curs_pos.x = this->curr_f->buff_len[iter];
                        }
                        
                        //draw cursor
                        curs_delta += 1;
                        if (iter + scrollY == this->curr_f->curs_pos.y){
                            ImVec2 p2 = ImVec2(p1.x + this->curr_f->curs_pos.x, p0.y + scrollY);
                            ImVec2 p3 = ImVec2(p2.x + 1, p1.y + scrollY);
                            if (curs_delta > 35 * ImGui::GetIO().Framerate){
                                draw_list->PushClipRect(p2, p3, IM_COL32(200, 200, 200, 255));
                                draw_list->AddRectFilled(p2, p3, IM_COL32(200, 200, 200, 255));
                                draw_list->AddText(ImVec2(p2.x - 1, p2.y), IM_COL32(0,0,0,255), string_format("%c", this->curr_f->buff[iter + (int)scrollY][(int)this->curr_f->curs_pos.x]).c_str());
                                draw_list->PopClipRect();
                                if (curs_delta > 70 * ImGui::GetIO().Framerate){
                                    curs_delta = 0;
                                }
                            }
                        }
                    }
                }
                ImGui::End();

                //draw bottom status bar
                ImGui::SetNextWindowPos(ImVec2(0, wSize.y-1), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(wSize.x, 1), ImGuiCond_Always);
                
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::Begin("StatusBar", nullptr,
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoTitleBar
                );
                ImGui::Text("Time per frame %.3f ms/frame (%.1f FPS)\tScroll: %f", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate, scrollY);
                ImGui::End();
                ImGui::PopStyleColor(1);

                ImGui::Render();
        
                ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
                ImTui_ImplNcurses_DrawScreen(true);
            }

        }
    /*////////////////////////////
        Helpers
    */////////////////////////////
        //Execute Command
            std::string editor::GetStdoutFromCommand(std::string cmd){
                std::string data;
                FILE * stream;
                char buffer[MAX_CMD_BUFFER];
                cmd.append(" 2>&1");

                stream = popen(cmd.c_str(), "r");
                if (stream){
                    while (!feof(stream)){
                        if (fgets(buffer, MAX_CMD_BUFFER, stream) != NULL){
                            data.append(buffer);
                        }
                    }
                    pclose(stream);
                }
                return data;
            }
        //Printf to string
            template<typename ... Args> std::string editor::string_format(const std::string& format, Args ... args){
                //Extra space for '\0'
                int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;
                if(0 >= size_s){
                    die("Error during formatting.");
                }
                auto size = static_cast<size_t>(size_s);
                auto buf = std::make_unique<char[]>(size);
                std::snprintf(buf.get(), size, format.c_str(), args ...);
                //We don't want the '\0' inside
                return std::string(buf.get(), buf.get() + size - 1);
            }
        //Count digits
            template <class T> int editor::numDigits(T number){
                int digits = 0;
                if (number < 0) digits = 1;
                while (number){
                    number /= 10;
                    digits++;
                }
                return digits;
            }
//End of file
