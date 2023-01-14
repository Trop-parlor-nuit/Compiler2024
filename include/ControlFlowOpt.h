#ifndef __CONTROLFLOWOPT_H__
#define __CONTROLFLOWOPT_H__

#include "Unit.h"

/*
    control flow optimization for IR:
        1) 删除不可达的基本块。
        2) 如果仅有一个前驱且该前驱仅有一个后继，将基本块与前驱合并。
        3) 消除空的基本块和仅包含无条件分支的基本块。
*/
class ControlFlowOpt
{
private:
    Unit *unit;

public:
    ControlFlowOpt(Unit *unit) : unit(unit){};
    void pass();
};

#endif