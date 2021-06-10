/*! \file imtui-impl-ncurses.cpp
 *  \brief Enter description here.
 */

#include "imtui.hpp"
#include "imtui-ncurses.hpp"
#include "imtui-text.hpp"

#ifdef _WIN32
#define NCURSES_MOUSE_VERSION
#include <pdcurses.h>
#define set_escdelay(X)

#define KEY_OFFSET 0xec00
#define KEY_CODE_YES    (KEY_OFFSET + 0x00) /* If get_wch() gives a key code */
#define KEY_BREAK       (KEY_OFFSET + 0x01) /* Not on PC KBD */
#define KEY_DOWN        (KEY_OFFSET + 0x02) /* Down arrow key */
#define KEY_UP          (KEY_OFFSET + 0x03) /* Up arrow key */
#define KEY_LEFT        (KEY_OFFSET + 0x04) /* Left arrow key */
#define KEY_RIGHT       (KEY_OFFSET + 0x05) /* Right arrow key */
#define KEY_HOME        (KEY_OFFSET + 0x06) /* home key */
#define KEY_BACKSPACE   (8) /* not on pc */
#define KEY_F0          (KEY_OFFSET + 0x08) /* function keys; 64 reserved */

#else
#include <ncurses.h>
#include <gpm.h>
#endif

#define RELEASE_MASK    0x01
#define ALTFUNC_MASK    0x02
#define SHIFT_MOD_MASK  0x04
#define ALT_MOD_MASK    0x08
#define CTRL_MOD_MASK   0x10

#include <cmath>
#include <array>
#include <chrono>
#include <map>
#include <vector>
#include <string>
#include <thread>
#include <fcntl.h>
#include <unistd.h>

namespace {
    struct VSync {
        VSync(double fps_active = 60.0, double fps_idle = 60.0) :
            tStepActive_us(1000000.0/fps_active),
            tStepIdle_us(1000000.0/fps_idle) {}

        uint64_t tStepActive_us;
        uint64_t tStepIdle_us;
        uint64_t tLast_us = t_us();
        uint64_t tNext_us = tLast_us;

        inline uint64_t t_us() const {
            return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(); // duh ..
        }

        inline void wait(bool active) {
            uint64_t tNow_us = t_us();

            auto tStep_us = active ? tStepActive_us : tStepIdle_us;
            auto tNextCur_us = tNext_us + tStep_us;

            wtimeout(stdscr, 0);
            while (tNow_us < tNextCur_us - 100) {
                if (tNow_us + 0.5*tStepActive_us < tNextCur_us) {
                    int ch = wgetch(stdscr);

                    if (ch != ERR) {
                        ungetch(ch);
                        tNextCur_us = tNow_us;
                        wtimeout(stdscr, 1);

                        return;
                    }
                }

                std::this_thread::sleep_for(std::chrono::microseconds(
                        std::min((uint64_t)(0.9*tStepActive_us),
                                 (uint64_t) (0.9*(tNextCur_us - tNow_us))
                                 )));

                tNow_us = t_us();
            }

            tNext_us += tStep_us;
            wtimeout(stdscr, 1);
        }

        inline float delta_s() {
            uint64_t tNow_us = t_us();
            uint64_t res = tNow_us - tLast_us;
            tLast_us = tNow_us;

            return float(res)/1e6f;
        }
    };
}

static VSync g_vsync;
static ImTui::TScreen * g_screen = nullptr;

ImTui::TScreen * ImTui_ImplNcurses_Init(bool mouseSupport, float fps_active, float fps_idle) {
    if (g_screen == nullptr) {
        g_screen = new ImTui::TScreen();
    }

    if (fps_idle < 0.0) {
        fps_idle = fps_active;
    }
    fps_idle = std::min(fps_active, fps_idle);
    g_vsync = VSync(fps_active, fps_idle);



    initscr();              //start screen
    // use_default_colors();
    start_color();          //initialize colors
    cbreak();               //disable line buffering
    noecho();               //dont echo keystrokes
    raw();
    curs_set(0);
    nodelay(stdscr, true);
    wtimeout(stdscr, 1);
    set_escdelay(25);
    keypad(stdscr, true);
    static int ttflags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, ttflags | O_NONBLOCK);

    if (mouseSupport) {
        mouseinterval(0);
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        printf("\033[?1003h\n");
        // printf("\033[?1000h\n");
    }

    if (!can_change_color()){
        endwin();
        return NULL;
    }

    ImGui::GetIO().KeyMap[ImGuiKey_Tab]         = 9;
    ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow]   = 260;
    ImGui::GetIO().KeyMap[ImGuiKey_RightArrow]  = 261;
    ImGui::GetIO().KeyMap[ImGuiKey_UpArrow]     = 259;
    ImGui::GetIO().KeyMap[ImGuiKey_DownArrow]   = 258;
    ImGui::GetIO().KeyMap[ImGuiKey_PageUp]      = 339;
    ImGui::GetIO().KeyMap[ImGuiKey_PageDown]    = 338;
    ImGui::GetIO().KeyMap[ImGuiKey_Home]        = 262;
    ImGui::GetIO().KeyMap[ImGuiKey_End]         = 360;
    ImGui::GetIO().KeyMap[ImGuiKey_Insert]      = 331;
    ImGui::GetIO().KeyMap[ImGuiKey_Delete]      = 330;
    ImGui::GetIO().KeyMap[ImGuiKey_Backspace]   = 263;
    ImGui::GetIO().KeyMap[ImGuiKey_Space]       = 32;
    ImGui::GetIO().KeyMap[ImGuiKey_Enter]       = 10;
    ImGui::GetIO().KeyMap[ImGuiKey_Escape]      = 27;
    ImGui::GetIO().KeyMap[ImGuiKey_KeyPadEnter] = 343;

    ImGui::GetIO().KeyRepeatDelay = 0.050;
    ImGui::GetIO().KeyRepeatRate = 0.050;

	int screenSizeX = 0;
	int screenSizeY = 0;

	getmaxyx(stdscr, screenSizeY, screenSizeX);
	ImGui::GetIO().DisplaySize = ImVec2(screenSizeX, screenSizeY);

    return g_screen;
}

bool ImTui_ImplNcurses_NewFrame() {
    bool hasInput = false;

	int screenSizeX = 0;
	int screenSizeY = 0;

	getmaxyx(stdscr, screenSizeY, screenSizeX);
	ImGui::GetIO().DisplaySize = ImVec2(screenSizeX, screenSizeY);

    static int mx = screenSizeX / 2;
    static int my = screenSizeY / 2;
    static int lbut = 0;
    static int rbut = 0;
    static char input[3];

    input[2] = 0;

    auto & keysDown = ImGui::GetIO().KeysDown;
    std::fill(keysDown, keysDown + 512, 0);

    ImGui::GetIO().KeyAlt = false;
    ImGui::GetIO().KeyCtrl = false;
    ImGui::GetIO().KeyShift = false;
    ImGui::GetIO().MouseWheel = 0;

    while (true) {
        int64_t key = getchar();

        if (key <= 0){
            // lbut = 0;
            // rbut = 0;
            break;
        }
        
        if (key == 27){
            int64_t seq[7] = {0};
            seq[0] = key;
            ssize_t i = 1;
            while (i < 7 && 0 < seq[i-1]){
                seq[i] = getchar();
                i++;
            }
            i = 1;

            switch (seq[i++]){
                case -1:
                    fprintf(stderr, "escape key\r\n");
                    continue;
                break;
                case 8:
                    switch (seq[i++]){
                        case -1:
                            fprintf(stderr, "ctrl alt backspace\r\n");
                            ImGui::GetIO().KeyCtrl = true;
                            ImGui::GetIO().KeyAlt = true;
                            continue;
                        break;
                    }
                break;
                case 13:
                    switch (seq[i++]){
                        case -1:
                            fprintf(stderr, "alt enter key\r\n");
                            ImGui::GetIO().KeyAlt = true;
                            continue;
                        break;
                    }
                break;
                case 79:
                    if (seq[i] == 50){
                        i++;
                        ImGui::GetIO().KeyShift = true;
                    }else if (seq[i] == 51){
                        i++;
                        ImGui::GetIO().KeyAlt = true;
                    }else if (seq[i] == 52){
                        i++;
                        ImGui::GetIO().KeyShift = true;
                        ImGui::GetIO().KeyAlt = true;
                    }else if (seq[i] == 53){
                        i++;
                        ImGui::GetIO().KeyCtrl = true;
                    }else if (seq[i] == 54){
                        i++;
                        ImGui::GetIO().KeyCtrl = true;
                        ImGui::GetIO().KeyShift = true;
                    }else if (seq[i] == 55){
                        i++;
                        ImGui::GetIO().KeyCtrl = true;
                        ImGui::GetIO().KeyAlt = true;
                    }else if (seq[i] == 56){
                        i++;
                        ImGui::GetIO().KeyCtrl = true;
                        ImGui::GetIO().KeyAlt = true;
                        ImGui::GetIO().KeyShift = true;
                    }
                    switch (seq[i++]){
                        case 65:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "up arrow\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 66:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "down arrow\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 67:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "right arrow\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 68:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "left arrow\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 69:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "center arrow\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 70:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "end key\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 72:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "home key\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 77:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "numpad enter key\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 106:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "numpad multiply key\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 107:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "numpad plus key\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 109:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "numpad minus key\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 111:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "numpad divide key\r\n");
                                    continue;
                                break;
                            }
                        break;
                        case 126:
                            switch (seq[i++]){
                                case -1:
                                    fprintf(stderr, "numpad delete key\r\n");
                                    continue;
                                break;
                            }
                        break;
                    }
                break;
                case 91:
                    switch (seq[i++]){
                        case 49:
                            if (59 == seq[i]){
                                i++;
                                if (seq[i] == 50){
                                    ImGui::GetIO().KeyShift = true;
                                }else if (seq[i] == 51){
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 52){
                                    ImGui::GetIO().KeyShift = true;
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 53){
                                    ImGui::GetIO().KeyCtrl = true;
                                }else if (seq[i] == 54){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyShift = true;
                                }else if (seq[i] == 55){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 56){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyAlt = true;
                                    ImGui::GetIO().KeyShift = true;
                                }
                                i++;
                            }
                            switch (seq[i++]){
                                case 65:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "up arrow\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 66:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "down arrow\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 67:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "right arrow\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 68:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "left arrow\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 69:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "center arrow\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 70:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "numpad end key\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 72:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "numpad home key\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 77:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "numpad enter key\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 106:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "numpad multiply key\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 107:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "numpad plus key\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 109:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "numpad minus key\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                                case 111:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "numpad divide key\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                            }
                        break;
                        case 50:
                            if (59 == seq[i]){
                                i++;
                                if (seq[i] == 50){
                                    ImGui::GetIO().KeyShift = true;
                                }else if (seq[i] == 51){
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 52){
                                    ImGui::GetIO().KeyShift = true;
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 53){
                                    ImGui::GetIO().KeyCtrl = true;
                                }else if (seq[i] == 54){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyShift = true;
                                }else if (seq[i] == 55){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 56){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyAlt = true;
                                    ImGui::GetIO().KeyShift = true;
                                }
                                i++;
                            }
                            switch (seq[i++]){
                                case 126:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "insert key\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                            }
                        break;
                        case 51:
                            if (59 == seq[i]){
                                i++;
                                if (seq[i] == 50){
                                    ImGui::GetIO().KeyShift = true;
                                }else if (seq[i] == 51){
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 52){
                                    ImGui::GetIO().KeyShift = true;
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 53){
                                    ImGui::GetIO().KeyCtrl = true;
                                }else if (seq[i] == 54){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyShift = true;
                                }else if (seq[i] == 55){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 56){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyAlt = true;
                                    ImGui::GetIO().KeyShift = true;
                                }
                                i++;
                            }
                            switch (seq[i++]){
                                case 126:
                                    switch (seq[i++]){
                                        case -1:
                                            ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Delete]] = true;
                                            continue;
                                        break;
                                    }
                                break;
                            }
                        break;
                        case 53:
                            if (59 == seq[i]){
                                i++;
                                if (seq[i] == 50){
                                    ImGui::GetIO().KeyShift = true;
                                }else if (seq[i] == 51){
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 52){
                                    ImGui::GetIO().KeyShift = true;
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 53){
                                    ImGui::GetIO().KeyCtrl = true;
                                }else if (seq[i] == 54){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyShift = true;
                                }else if (seq[i] == 55){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 56){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyAlt = true;
                                    ImGui::GetIO().KeyShift = true;
                                }
                                i++;
                            }
                            switch (seq[i++]){
                                case 126:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "pg up key\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                            }
                        break;
                        case 54:
                            if (59 == seq[i]){
                                i++;
                                if (seq[i] == 50){
                                    ImGui::GetIO().KeyShift = true;
                                }else if (seq[i] == 51){
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 52){
                                    ImGui::GetIO().KeyShift = true;
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 53){
                                    ImGui::GetIO().KeyCtrl = true;
                                }else if (seq[i] == 54){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyShift = true;
                                }else if (seq[i] == 55){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyAlt = true;
                                }else if (seq[i] == 56){
                                    ImGui::GetIO().KeyCtrl = true;
                                    ImGui::GetIO().KeyAlt = true;
                                    ImGui::GetIO().KeyShift = true;
                                }
                                i++;
                            }
                            switch (seq[i++]){
                                case 126:
                                    switch (seq[i++]){
                                        case -1:
                                            fprintf(stderr, "pg dn key\r\n");
                                            continue;
                                        break;
                                    }
                                break;
                            }
                        break;
                        case 77:
                            if (seq[i] > 0){
                                mx = seq[++i] - 33;
                                my = seq[++i] - 33;
                                
                                if (seq[i-2] & SHIFT_MOD_MASK){
                                    ImGui::GetIO().KeyShift = true;
                                    seq[i-2] -= SHIFT_MOD_MASK;
                                }
                                if (seq[i-2] & ALT_MOD_MASK){
                                    ImGui::GetIO().KeyAlt = true;
                                    seq[i-2] -= ALT_MOD_MASK;
                                }
                                if (seq[i-2] & CTRL_MOD_MASK){
                                    ImGui::GetIO().KeyCtrl = true;
                                    seq[i-2] -= CTRL_MOD_MASK;
                                }
                                uint8_t mod  = seq[i-2];
                                uint8_t type = seq[i-2] >> 4;

                                if (type == 0x2){//button press
                                    if (mod & RELEASE_MASK){
                                        if (1 == rbut && mod & ALTFUNC_MASK){
                                            rbut = 0;
                                        }else if (1 == lbut){
                                            lbut = 0;
                                        }
                                    }else{
                                        if (mod & ALTFUNC_MASK){
                                            rbut = 1;
                                        }else{
                                            lbut = 1;
                                        }
                                    }
                                }else if (type == 0x4){//mouse move
                                }else if (type == 0x6){//scroll wheel
                                    if (mod & RELEASE_MASK){
                                        ImGui::GetIO().MouseWheel -= 0.2;
                                    }else{
                                        ImGui::GetIO().MouseWheel += 0.2;
                                    }
                                }else{
                                    fprintf(stderr, "unknown mouse event: %ld\r\n", seq[i-2]);
                                }
                                continue;
                            }
                        break;
                        case 90:
                            switch (seq[i++]){
                                case -1:
                                    ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Tab]] = true;
                                    ImGui::GetIO().KeyShift = true;
                                    continue;
                                break;
                            }
                        break;
                    }
                break;
                case 127:
                    switch (seq[i++]){
                        case -1:
                            ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Backspace]] = true;
                            ImGui::GetIO().KeyAlt = true;
                            continue;
                        break;
                    }
                break;
            }

            fprintf(stderr, "unknown sequence %ld %ld %ld %ld %ld %ld %ld\r\n", seq[0], seq[1], seq[2], seq[3], seq[4], seq[5], seq[6]);
            continue;
        }else if (key == 127){
            ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Backspace]] = true;
            continue;
        }else if (key == 8){
            ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Backspace]] = true;
            ImGui::GetIO().KeyCtrl = true;
            continue;
        }else if (key == 13){
            ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Enter]] = true;
            continue;
        }else if (key == 9){
            ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Tab]] = true;
            continue;
        }else{
            input[0] = (key & 0x000000FF);
            input[1] = (key & 0x0000FF00) >> 8;
            if (key < 127) {
                ImGui::GetIO().AddInputCharactersUTF8(input);
            }
            keysDown[key] = true;
        }
        hasInput = true;
    }

    ImGui::GetIO().MousePos.x = mx;
    ImGui::GetIO().MousePos.y = my;
    ImGui::GetIO().MouseDown[0] = lbut;
    ImGui::GetIO().MouseDown[1] = rbut;

    ImGui::GetIO().DeltaTime = g_vsync.delta_s();

    return hasInput;
}

// state
int nActiveFrames = 10;
ImTui::TScreen screenPrev;

std::pair<bool, int16_t> * colPairs = (std::pair<bool, int16_t> *)calloc(0xFFFFFFFF, sizeof(std::tuple<bool, int16_t>));
std::pair<bool, int16_t> * cols = (std::pair<bool, int16_t> *)calloc(0xFFFFFFFF, sizeof(std::pair<bool, int16_t>));
std::vector<uint8_t> curs;
int nCols = 1;
int nPairs = 1;

void ImTui_ImplNcurses_DrawScreen(bool active) {
    if (active) nActiveFrames = 10;

    wrefresh(stdscr);

    int nx = g_screen->nx;
    int ny = g_screen->ny;

    bool compare = true;

    if (screenPrev.nx != nx || screenPrev.ny != ny) {
        screenPrev.resize(nx, ny);
        compare = false;
    }

    int ic = 0;
    curs.resize(nx + 1);
    
    for (int y = 0; y < ny; ++y) {
        bool isSame = compare;
        if (compare) {
            for (int x = 0; x < nx; ++x) {
                if (screenPrev.data[y*nx + x] != g_screen->data[y*nx + x]) {
                    isSame = false;
                    break;
                }
            }
        }
        if (isSame) continue;
        uint32_t lastp = 0xFFFFFFFF;

        move(y, 0);
        for (int x = 0; x < nx; ++x) {
            uint64_t cell = g_screen->data[y*nx + x];
            uint16_t ch = (cell & 0x000000000000FFFF) >> 00;

            uint32_t fg = (cell & 0x000000FFFFFF0000) >> 16;
            uint32_t bg = (cell & 0xFFFFFF0000000000) >> 40;
            
            if (!cols[fg].first){
                uint16_t fgb = std::round(3.9216f * ((fg & 0xFF0000) >> 16));
                uint16_t fgg = std::round(3.9216f * ((fg & 0x00FF00) >> 8));
                uint16_t fgr = std::round(3.9216f * ((fg & 0x0000FF)));

                cols[fg].first = true;
                cols[fg].second = nCols;

                init_color(nCols, fgr, fgg, fgb);
                nCols++;
                if (nCols >= COLORS){
                    nCols = 1;
                }
            }
            
            if (!cols[bg].first){
                uint16_t bgb = std::round(3.9216f * ((bg & 0xFF0000) >> 16));
                uint16_t bgg = std::round(3.9216f * ((bg & 0x00FF00) >> 8));
                uint16_t bgr = std::round(3.9216f * ((bg & 0x0000FF)));

                cols[bg].first = true;
                cols[bg].second = nCols;

                init_color(nCols, bgr, bgg, bgb);
                nCols++;
                if (nCols >= COLORS){
                    nCols = 1;
                }
            }            

            uint32_t p = (cols[fg].second << 16) | cols[bg].second;

            if (!colPairs[p].first){
                colPairs[p].first = true;
                colPairs[p].second = nPairs;
            
                init_pair(nPairs, cols[fg].second, cols[bg].second);
                nPairs++;
                if (nPairs >= COLOR_PAIRS){
                    nPairs = 1;
                }
            }

            if (lastp != p){
                if (curs.size() > 0){
                    curs[ic] = 0;
                    addstr((char *) curs.data());
                    ic = 0;
                    curs[0] = 0;
                }
                attron(COLOR_PAIR(colPairs[p].second));
                lastp = p;
            }
            curs[ic++] = ch > 0 ? ch : '.';
        }

        if (curs.size() > 0) {
            curs[ic] = 0;
            addstr((char *) curs.data());
            ic = 0;
            curs[0] = 0;
        }

        if (compare) {
            memcpy(screenPrev.data + y*nx, g_screen->data + y*nx, nx*sizeof(ImTui::TCell));
        }
    }

    if (!compare) {
        memcpy(screenPrev.data, g_screen->data, nx*ny*sizeof(ImTui::TCell));
    }

    g_vsync.wait(nActiveFrames --> 0);
}

bool ImTui_ImplNcurses_ProcessEvent() {
    return true;
}

void ImTui_ImplNcurses_Shutdown() {
    // ref #11 : https://github.com/ggerganov/imtui/issues/11
    printf("\033[?1003l\n"); // Disable mouse movement events, as l = low

    endwin();

    if (g_screen) {
        delete g_screen;
    }
    free(cols);
    free(colPairs);
    
    g_screen = nullptr;
}
