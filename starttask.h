#ifndef STARTTASK_H
#define STARTTASK_H

#include "task.h"

class StartTask : public Task
{
public:
    enum STARTITEM {
        CONTINUE = 0,
        NEW_FLAT,
        NEW_TERRAIN,
        NEW_GRAPH,
        EXIT,
        START_ITEM_MAX
    };

    StartTask();
    virtual ~StartTask();

    virtual void makeCurrent() override;
    virtual void render() override;
    virtual void logic(GLFix dt) override;

    void setHasSavedWorld(bool exists) { has_saved_world = exists; }

private:
    int selected_item = NEW_TERRAIN;
    bool has_saved_world = false;
};

extern StartTask start_task;

#endif // STARTTASK_H
