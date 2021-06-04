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

            bool demo = true;
            int nframes = 0;
        
            for (;;) {
                ImTui_ImplNcurses_NewFrame();
                ImTui_ImplText_NewFrame();
        
                ImGui::NewFrame();

                ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
                io.ConfigInputTextCursorBlink = true;

                auto wSize = ImGui::GetIO().DisplaySize;
                ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
                ImGui::SetNextWindowSize(wSize, ImGuiCond_Always);

                ImGui::Begin("Hello, world!", nullptr,
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_MenuBar |
                    ImGuiWindowFlags_NoTitleBar
                );

                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("File"))
                    {
                        if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
                        if (ImGui::MenuItem("Save", "Ctrl+S"))   { /* Do stuff */ }
                        if (ImGui::MenuItem("Close", "Ctrl+W"))  { /* Do stuff */ }
                        if (ImGui::MenuItem("Quit", "Ctrl+Q"))  { break; }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }

                ImGui::Text("Mouse Pos : x = %g, y = %g", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
                ImGui::Text("Time per frame %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();

                ImGui::PopStyleColor(1);
                ImGui::PopStyleColor(1);
                ImGui::PopStyleColor(1);
                ImGui::PopStyleColor(1);
        
                ImGui::Render();
        
                ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
                ImTui_ImplNcurses_DrawScreen();
            }

        }
//End of file
