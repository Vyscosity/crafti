#include "graphtask.h"

#include <cstring>

#ifndef _TINSPIRE
#include <SDL/SDL.h>
#endif

#include "font.h"
#include "world.h"
#include "worldtask.h"
#include "starttask.h"

GraphTask graph_task;

namespace {
struct GraphPreset {
    const char *label;
    const char *expr;
};

static const GraphPreset k_graph_presets[] = {
    {"Sine Wave", "sin(x)"},
    {"Cosine Waves", "cos(x*y)"},
    {"Ripple", "sin(sqrt(x*x+y*y))"},
    {"Paraboloid", "(x*x+y*y)/120"},
    {"Saddle", "(x*x-y*y)/120"},
    {"Checker Wave", "sin(x)*cos(y)"},
    {"Complex z^2 (domain)", "c:z^2"},
    {"Complex z^3-1 (domain)", "c:z^3-1"},
    {"Complex 1/z (domain)", "c:1/z"},
    {"Complex sin(z) (domain)", "c:sin(z)"},
    {"Complex mobius (domain)", "c:(z^2+1)/(z^2-1)"},
    {"Implicit Sphere", "i:sphere"},
    {"Implicit Toroid", "i:toroid"},
};

static constexpr unsigned int k_graph_presets_count = sizeof(k_graph_presets) / sizeof(*k_graph_presets);
}

GraphTask::GraphTask()
{
}

GraphTask::~GraphTask()
{
}

void GraphTask::makeCurrent()
{
    std::strncpy(expression, world.graphExpression(), max_expr_len);
    expression[max_expr_len] = '\0';
    expression_len = std::strlen(expression);
    charset_index = 0;
    preset_index = 0;
    for(unsigned int i = 0; i < k_graph_presets_count; ++i)
    {
        if(std::strcmp(expression, k_graph_presets[i].expr) == 0)
        {
            preset_index = i;
            break;
        }
    }
    Task::makeCurrent();
}

void GraphTask::render()
{
    for(unsigned int i = 0; i < screen->width * screen->height; ++i)
        screen->bitmap[i] = 0x0000;

    drawStringCenter("Graphing Mode", 0xFFFF, *screen, SCREEN_WIDTH / 2, 8);
    drawStringCenter("Type z=f(x,y) and press Enter", 0xFFFF, *screen, SCREEN_WIDTH / 2, 24);

    drawRectangle(*screen, 12, 42, SCREEN_WIDTH - 24, 24, 0xFFFF);
    drawString("z=", 0xFFFF, *screen, 18, 49);
    drawString(expression, 0xFFFF, *screen, 38, 49);

    static const char charset[] = "xyz0123456789+-*/^()., sincoartbpe";
    const char selected = charset[charset_index];

    char selected_text[20] = "Current char: ' '";
    selected_text[15] = selected;
    drawStringCenter(selected_text, 0xFFFF, *screen, SCREEN_WIDTH / 2, 80);

    char preset_text[64];
    snprintf(preset_text, sizeof(preset_text), "Preset: %s", k_graph_presets[preset_index].label);
    drawStringCenter(preset_text, 0xFFFF, *screen, SCREEN_WIDTH / 2, 92);

    drawString("8/2: char  5/Space: append  7: backspace", 0xFFFF, *screen, 10, 106);
    drawString("4/6: preset  1: apply preset", 0xFFFF, *screen, 10, 120);
    char fill_text[48];
    snprintf(fill_text, sizeof(fill_text), "+/-: fill depth n = %d", world.graphFillDepth());
    drawString(fill_text, 0xFFFF, *screen, 10, 132);
    drawString("9: clear   Enter/T: start graph", 0xFFFF, *screen, 10, 146);
    drawString("ESC: back", 0xFFFF, *screen, 10, 160);

    drawString("Range: x,y in [-30,30]", 0xFFFF, *screen, 10, 174);
    drawString("Tip: c:* for domain, i:* for implicit", 0xFFFF, *screen, 10, 188);
}

void GraphTask::logic(GLFix /*dt*/)
{
    static const char charset[] = "xyz0123456789+-*/^()., sincoartbpe";
    const unsigned int charset_len = sizeof(charset) - 1;

    bool desktop_enter_down = false;
    bool desktop_space_down = false;
    bool desktop_t_down = false;
#ifndef _TINSPIRE
    const Uint8 *keys = SDL_GetKeyState(nullptr);
    desktop_enter_down = keys[SDLK_RETURN] != 0 || keys[SDLK_KP_ENTER] != 0;
    desktop_space_down = keys[SDLK_SPACE] != 0;
    desktop_t_down = keys[SDLK_t] != 0;
#endif

    const bool submit_down = keyPressed(KEY_NSPIRE_ENTER) || desktop_enter_down || desktop_t_down;
    const bool append_down =
#ifdef _TINSPIRE
        keyPressed(KEY_NSPIRE_5);
#else
        desktop_space_down || (keyPressed(KEY_NSPIRE_5) && !desktop_enter_down);
#endif

    if(key_held_down)
    {
        key_held_down = keyPressed(KEY_NSPIRE_ESC) || keyPressed(KEY_NSPIRE_UP) || keyPressed(KEY_NSPIRE_DOWN)
            || keyPressed(KEY_NSPIRE_8) || keyPressed(KEY_NSPIRE_2)
            || keyPressed(KEY_NSPIRE_4) || keyPressed(KEY_NSPIRE_6)
            || keyPressed(KEY_NSPIRE_1) || keyPressed(KEY_NSPIRE_PLUS) || keyPressed(KEY_NSPIRE_MINUS)
            || keyPressed(KEY_NSPIRE_7) || keyPressed(KEY_NSPIRE_9)
            || append_down || submit_down;
        return;
    }

    if(keyPressed(KEY_NSPIRE_ESC))
    {
        start_task.makeCurrent();
        key_held_down = true;
        return;
    }

    if(keyPressed(KEY_NSPIRE_UP) || keyPressed(KEY_NSPIRE_8))
    {
        if(charset_index == 0)
            charset_index = charset_len - 1;
        else
            --charset_index;
        key_held_down = true;
        return;
    }

    if(keyPressed(KEY_NSPIRE_DOWN) || keyPressed(KEY_NSPIRE_2))
    {
        ++charset_index;
        if(charset_index >= charset_len)
            charset_index = 0;
        key_held_down = true;
        return;
    }

    if(keyPressed(KEY_NSPIRE_LEFT) || keyPressed(KEY_NSPIRE_4))
    {
        if(preset_index == 0)
            preset_index = k_graph_presets_count - 1;
        else
            --preset_index;
        key_held_down = true;
        return;
    }

    if(keyPressed(KEY_NSPIRE_RIGHT) || keyPressed(KEY_NSPIRE_6))
    {
        ++preset_index;
        if(preset_index >= k_graph_presets_count)
            preset_index = 0;
        key_held_down = true;
        return;
    }

    if(keyPressed(KEY_NSPIRE_1))
    {
        std::strncpy(expression, k_graph_presets[preset_index].expr, max_expr_len);
        expression[max_expr_len] = '\0';
        expression_len = std::strlen(expression);
        key_held_down = true;
        return;
    }

    if(keyPressed(KEY_NSPIRE_PLUS))
    {
        world.setGraphFillDepth(world.graphFillDepth() + 1);
        key_held_down = true;
        return;
    }

    if(keyPressed(KEY_NSPIRE_MINUS))
    {
        world.setGraphFillDepth(world.graphFillDepth() - 1);
        key_held_down = true;
        return;
    }

    if(keyPressed(KEY_NSPIRE_7))
    {
        if(expression_len > 0)
            expression[--expression_len] = '\0';
        key_held_down = true;
        return;
    }

    if(keyPressed(KEY_NSPIRE_9))
    {
        expression[0] = '\0';
        expression_len = 0;
        key_held_down = true;
        return;
    }

    if(append_down)
    {
        if(expression_len < max_expr_len)
        {
            expression[expression_len++] = charset[charset_index];
            expression[expression_len] = '\0';
        }
        key_held_down = true;
        return;
    }

    if(submit_down)
    {
        if(expression_len == 0)
        {
            std::strcpy(expression, "sin(x)");
            expression_len = 6;
        }

        if(!world.setGraphExpression(expression))
        {
            world_task.setMessage("Invalid graph function");
            key_held_down = true;
            return;
        }

        world.setWorldType(World::WorldType::Graph);
        world_task.resetWorld();
        world_task.makeCurrent();
        key_held_down = true;
    }
}
