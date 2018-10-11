#pragma once

#include <string>
#include "smart_ptr.hpp"

namespace smart
{
namespace ut_smart_ptr
{
struct drawable
{
    virtual void load_image() = 0;
    virtual void draw(int active_player_x, int active_player_y) = 0;
    virtual ~drawable() {}

    std::string drawable_buffer;
};

class client_player;

void run_all();
}
}
