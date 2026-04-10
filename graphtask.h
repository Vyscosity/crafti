#ifndef GRAPHTASK_H
#define GRAPHTASK_H

#include "task.h"

class GraphTask : public Task
{
public:
    GraphTask();
    virtual ~GraphTask() override;

    virtual void makeCurrent() override;
    virtual void render() override;
    virtual void logic(GLFix dt) override;

private:
    static constexpr unsigned int max_expr_len = 63;

    char expression[max_expr_len + 1] = "sin(x)";
    unsigned int expression_len = 6;
    unsigned int charset_index = 0;
    unsigned int preset_index = 0;
};

extern GraphTask graph_task;

#endif // GRAPHTASK_H
