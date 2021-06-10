/*! \file imtui.h
 *  \brief Enter description here.
 */

#pragma once

// simply expose the existing Dear ImGui API
#include "../../deps/imgui/imgui.h"

#include "imtui-text.hpp"

#include <cstring>
#include <cstdint>

namespace ImTui {

using TChar = unsigned char;
using TColor = uint32_t;

// single screen cell
// 0x000000000000FFFF - char
// 0x000000FFFFFF0000 - foreground color
// 0xFFFFFF0000000000 - background color
using TCell = uint64_t;

struct TScreen {
    int nx = 0;
    int ny = 0;

    int nmax = 0;

    TCell * data = nullptr;

    ~TScreen() {
        if (data) delete [] data;
    }

    inline int size() const { return nx*ny; }

    inline void clear() {
        if (data) {
            memset(data, 0, nx*ny*sizeof(TCell));
        }
    }

    inline void resize(int pnx, int pny) {
        nx = pnx;
        ny = pny;
        if (nx*ny <= nmax) return;

        if (data) delete [] data;

        nmax = nx*ny;
        data = new TCell[nmax];
    }
};

}
