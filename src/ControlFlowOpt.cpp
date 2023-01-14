#include "ControlFlowOpt.h"
#include "Type.h"
#include <queue>

std::set<BasicBlock *> freeList;
std::set<Instruction *> freeInsts;

void ControlFlowOpt::pass()
{
    auto Funcs = std::vector<Function *>(unit->begin(), unit->end());
    for (auto func : Funcs)
    {
        std::map<BasicBlock *, bool> is_visited;
        for (auto bb : func->getBlockList())
            is_visited[bb] = false;
        std::queue<BasicBlock *> q;
        q.push(func->getEntry());
        is_visited[func->getEntry()] = true;
        while (!q.empty())
        {
            auto bb = q.front();
            std::vector<BasicBlock *> preds(bb->pred_begin(), bb->pred_end());
            std::vector<BasicBlock *> succs(bb->succ_begin(), bb->succ_end());
            if (bb->empty())
            {
                assert(0);
            }
            else if (bb->begin()->getNext() == bb->end() && bb->begin()->isUncond())
            {
                assert(bb->getNumOfSucc() == 1);
                for (auto pred : preds)
                {
                    pred->removeSucc(bb);
                    if (pred->rbegin()->isCond())
                    {
                        CondBrInstruction *branch = (CondBrInstruction *)(pred->rbegin());
                        if (branch->getTrueBranch() == bb)
                            branch->setTrueBranch(succs[0]); // toDO ： 真假分支一样
                        else
                            branch->setFalseBranch(succs[0]);
                    }
                    else
                    {
                        assert(pred->rbegin()->isUncond());
                        freeInsts.insert(pred->rbegin());
                        pred->remove(pred->rbegin());
                        pred->insertBefore(new UncondBrInstruction(bb, pred), pred->end());
                    }
                    pred->addSucc(succs[0]);
                    succs[0]->removePred(bb);
                    succs[0]->addPred(pred);
                }
                bb->getParent()->remove(bb);
                freeList.insert(bb);
            }
            else if (bb->getNumOfPred() == 1 && (*(bb->pred_begin()))->getNumOfSucc() == 1)
            {
                auto pred = *(bb->pred_begin());
                pred->removeSucc(bb);
                assert(pred->rbegin()->isUncond());
                freeInsts.insert(pred->rbegin());
                pred->remove(pred->rbegin());
                for (auto succ : succs)
                    pred->addSucc(succ);
                for (auto inst = bb->begin(); inst != bb->end(); inst = inst->getNext())
                {
                    bb->remove(inst);
                    pred->insertBefore(inst, pred->end());
                }
                for (auto succ : succs)
                {
                    succ->removePred(bb);
                    succ->addPred(pred);
                }
                bb->getParent()->remove(bb);
                freeList.insert(bb);
            }
            q.pop();
            for (auto succ : succs)
            {
                if (!is_visited[succ])
                {
                    q.push(succ);
                    is_visited[succ] = true;
                }
            }
        }
        for (auto bb : func->getBlockList())
            if (!is_visited[bb])
            {
                func->remove(bb);
                freeList.insert(bb);
            }
    }
    // for(auto bb : freeList)
    //     delete bb;
    // for(auto inst :freeInsts)
    //     delete inst;
}