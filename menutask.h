#ifndef MENUTASK_H
#define MENUTASK_H

#include "gl.h"

#include "task.h"

class MenuTask : public Task
{
public:
    enum MENUITEM {
        RESUME = 0,
        SETTINGS,
        HELP,
        SAVE_WORLD,
        QUIT_TO_TITLE,
        MENU_ITEM_MAX
    };

    MenuTask();
    virtual ~MenuTask();

    virtual void makeCurrent() override;

    virtual void render() override;
    virtual void logic(GLFix dt) override;

private:
    int menu_selected_item = 0;
};

extern MenuTask menu_task;

#endif // MENUTASK_H
