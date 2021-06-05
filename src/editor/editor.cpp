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
        editor::editor(){
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
        
            this->file = NULL;
            this->dir = NULL;
        }
        editor::editor(char * file, char * dir):editor(){
            this->file = file;
            this->dir = dir;
        }
    //Destructors
        editor::~editor(){
            ImTui_ImplText_Shutdown();
            ImTui_ImplNcurses_Shutdown();
        }
    //Run
        void editor::run(){
            auto screen = ImTui_ImplNcurses_Init(true);
            ImTui_ImplText_Init();


            ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.784f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.47f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.45f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 1.0f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));

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
                ImGui::SetNextWindowSize(ImVec2(wSize.x, wSize.y-1), ImGuiCond_Always);

                ImGui::Begin("Main", nullptr,
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_MenuBar |
                    ImGuiWindowFlags_NoTitleBar
                );

                if (ImGui::BeginMenuBar()){
                    if (ImGui::BeginMenu("File ")){
                        if (ImGui::MenuItem(" Open", "Ctrl+O")){}
                        if (ImGui::MenuItem(" Save", "Ctrl+S")){}
                        if (ImGui::MenuItem(" Close", "Ctrl+W")){}
                        if (ImGui::MenuItem(" Quit", "Ctrl+Q")){break;}
                        ImGui::EndMenu();
                    }
                    ImGui::Button("Tab ");
                    ImGui::EndMenuBar();
                }
                
                if (io.KeysDown[CTRL_KEY('q')]){break;}
                

                
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::BeginChild("File");
                for (u_int64_t iter = 0; iter < 512; iter++){
                    if (io.KeysDown[iter]){
                        ImGui::Text("Key Pressed %d", iter);
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor(1);
                
                ImGui::End();
                ImGui::SetNextWindowPos(ImVec2(0, wSize.y-1), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(wSize.x, 1), ImGuiCond_Always);
                
                ImGui::Begin("StatusBar", nullptr,
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoTitleBar
                );
                ImGui::Text("Mouse Pos : x = %g, y = %g\tTime per frame %.3f ms/frame (%.1f FPS)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y, 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();

                ImGui::Render();
        
                ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
                ImTui_ImplNcurses_DrawScreen();
            }

        }
//End of file
