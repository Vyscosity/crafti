#ifndef DEATHTASK_H
#define DEATHTASK_H

#include "gl.h"
#include "task.h"

class DeathTask : public Task
{
public:
    enum DEATHITEM
    {
        RESPAWN = 0,
        QUIT_TO_TITLE,
        DEATH_ITEM_MAX
    };

    DeathTask();
    virtual ~DeathTask();

    virtual void makeCurrent() override;
    virtual void render() override;
    virtual void logic(GLFix dt) override;

private:
    int death_selected_item = RESPAWN;
};

extern DeathTask death_task;

#endif // DEATHTASK_H

