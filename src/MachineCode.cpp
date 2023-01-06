#include "MachineCode.h"
extern FILE *yyout;

// 合法的第二操作数循环移位偶数位后可以用8bit表示
bool isShifterOperandVal(unsigned bin_val)
{
    for (int i = 0; i < 32; i += 2)
    {
        unsigned shift_val = (bin_val >> i) | (bin_val << (32 - i)); // 循环右移i位
        if (!(shift_val & 0xFFFFFF00))
            return true;
    }
    return false;
}

MachineOperand::MachineOperand(int tp, double val, Type *valType)
{
    this->type = tp;
    if (tp == MachineOperand::IMM)
        this->val = val;
    else
        this->reg_no = (int)val;
    this->valType = valType;
}

MachineOperand::MachineOperand(std::string label)
{
    this->type = MachineOperand::LABEL;
    this->label = label;
    // this->valType = TypeSystem::intType;
}

bool MachineOperand::operator==(const MachineOperand &a) const
{
    if (this->type != a.type)
        return false;
    if (this->type == IMM)
        return this->val == a.val;
    return this->reg_no == a.reg_no;
}

bool MachineOperand::operator<(const MachineOperand &a) const
{
    if (this->type == a.type)
    {
        if (this->type == IMM)
            return this->val < a.val;
        // assert(this->type == VREG || this->type == REG);
        return this->reg_no < a.reg_no; // 不太理解比较label的意义
    }
    return this->type < a.type;
}

void MachineOperand::printReg()
{
    if (valType->isFloat())
    {
        switch (reg_no)
        {
        case 32:
            fprintf(yyout, "FPSCR");
            break;
        default:
            fprintf(yyout, "s%d", reg_no);
            break;
        }
    }
    else
    {
        switch (reg_no)
        {
        case 11:
            fprintf(yyout, "fp");
            break;
        case 13:
            fprintf(yyout, "sp");
            break;
        case 14:
            fprintf(yyout, "lr");
            break;
        case 15:
            fprintf(yyout, "pc");
            break;
        default:
            fprintf(yyout, "r%d", reg_no);
            break;
        }
    }
}

void MachineOperand::output()
{
    /* HINT：print operand
     * Example:
     * immediate num 1 -> print #1;
     * register 1 -> print r1;
     * label addr_a -> print addr_a; */
    switch (this->type)
    {
    case IMM:
        if (valType->isFloat())
        {
            float float_val = (float)this->val;
            fprintf(yyout, "#%u", reinterpret_cast<unsigned &>(float_val));
        }
        else
            fprintf(yyout, "#%d", (int)this->val);
        break;
    case VREG:
        fprintf(yyout, "v%d", this->reg_no); // 不用区分浮点？
        break;
    case REG:
        printReg();
        break;
    case LABEL:
        if (this->label.substr(0, 2) == ".L")
            fprintf(yyout, "%s", this->label.c_str());
        else if (this->label.substr(0, 1) == "@")
            fprintf(yyout, "%s", this->label.c_str() + 1);
        else
            fprintf(yyout, "addr_%d_%s", parent->getParent()->getParent()->getParent()->getLtorgNo(), /*this->label.substr(0, 1) == "@" ? this->label.c_str() + 1 : */ this->label.c_str());
    default:
        break;
    }
}

bool MachineOperand::isIllegalShifterOperand()
{
    assert(this->isImm());
    unsigned bin_val;
    if (valType->isFloat())
        bin_val = reinterpret_cast<unsigned &>(this->val);
    else
    {
        signed signed_val = (int)(this->val);
        bin_val = reinterpret_cast<unsigned &>(signed_val);
    }
    return !isShifterOperandVal(bin_val) && !isShifterOperandVal(~bin_val + 1);
}

void MachineInstruction::printCond()
{
    switch (cond)
    {
    case EQ:
        fprintf(yyout, "eq");
        break;
    case NE:
        fprintf(yyout, "ne");
        break;
    case LT:
        fprintf(yyout, "lt");
        break;
    case LE:
        fprintf(yyout, "le");
        break;
    case GT:
        fprintf(yyout, "gt");
        break;
    case GE:
        fprintf(yyout, "ge");
        break;
    default:
        break;
    }
}

BinaryMInstruction::BinaryMInstruction(
    MachineBlock *p, int op,
    MachineOperand *dst, MachineOperand *src1, MachineOperand *src2,
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::BINARY;
    this->op = op;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src1);
    this->use_list.push_back(src2);
    dst->setParent(this);
    src1->setParent(this);
    src2->setParent(this);
}

void BinaryMInstruction::output()
{
    if (def_list[0]->getValType()->isFloat())
    {
        switch (this->op)
        {
        case BinaryMInstruction::ADD:
            fprintf(yyout, "\tvadd.f32");
            break;
        case BinaryMInstruction::SUB:
            fprintf(yyout, "\tvsub.f32");
            break;
        case BinaryMInstruction::MUL:
            fprintf(yyout, "\tvmul.f32");
            break;
        case BinaryMInstruction::DIV:
            fprintf(yyout, "\tvdiv.f32");
            break;
        default:
            break;
        }
    }
    else
    {
        switch (this->op)
        {
        case BinaryMInstruction::ADD:
            fprintf(yyout, "\tadd");
            break;
        case BinaryMInstruction::SUB:
            fprintf(yyout, "\tsub");
            break;
        case BinaryMInstruction::MUL:
            fprintf(yyout, "\tmul");
            break;
        case BinaryMInstruction::DIV:
            fprintf(yyout, "\tsdiv");
            break;
        default:
            break;
        }
    }
    printCond();
    fprintf(yyout, " ");
    this->def_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[1]->output();
    fprintf(yyout, "\n");
}

LoadMInstruction::LoadMInstruction(MachineBlock *p,
                                   MachineOperand *dst, MachineOperand *src1, MachineOperand *src2,
                                   int cond)
{
    this->parent = p;
    this->type = MachineInstruction::LOAD;
    this->op = -1;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src1);
    if (src2)
        this->use_list.push_back(src2);
    dst->setParent(this);
    src1->setParent(this);
    if (src2)
        src2->setParent(this);
}

void LoadMInstruction::output()
{
    // 这段只针对栈中偏移更新前合法但更新后不合法的情况
    if (this->use_list.size() > 1 && this->use_list[1]->isIllegalShifterOperand())
    {
        fprintf(yyout, "\tldr ");
        this->def_list[0]->output();
        fprintf(yyout, ", =%d\n", (int)this->use_list[0]->getVal());
        this->use_list[1] = this->def_list[0];
    }

    // // 小的立即数用MOV优化一下，arm汇编器会自动做?
    // if ((this->use_list.size() == 1) && this->use_list[0]->isImm() && !this->use_list[0]->isIllegalShifterOperand())
    // {
    //     if (this->def_list[0]->isFloat())
    //         fprintf(yyout, "\tvmov.f32");
    //     else
    //         fprintf(yyout, "\tmov");
    //     printCond();
    //     fprintf(yyout, " ");
    //     this->def_list[0]->output();
    //     fprintf(yyout, ", ");
    //     this->use_list[0]->output();
    //     fprintf(yyout, "\n");
    //     return;
    // }

    if (this->def_list[0]->getValType()->isFloat())
        fprintf(yyout, "\tvldr.32");
    else if (this->def_list[0]->getValType()->isBool())
        fprintf(yyout, "\tldrb");
    else
        fprintf(yyout, "\tldr");
    printCond();
    fprintf(yyout, " ");
    this->def_list[0]->output();
    fprintf(yyout, ", ");

    // Load immediate num, eg: ldr r1, =8
    if (this->use_list[0]->isImm())
    {
        if (this->def_list[0]->getValType()->isFloat())
        {
            float float_val = (float)this->use_list[0]->getVal();
            fprintf(yyout, "=%u\n", reinterpret_cast<unsigned &>(float_val));
        }
        else
            fprintf(yyout, "=%d\n", (int)this->use_list[0]->getVal());
        return;
    }

    // Load address
    if (this->use_list[0]->isReg() || this->use_list[0]->isVReg())
        fprintf(yyout, "[");

    this->use_list[0]->output();
    if (this->use_list.size() > 1)
    {
        fprintf(yyout, ", ");
        this->use_list[1]->output();
    }

    if (this->use_list[0]->isReg() || this->use_list[0]->isVReg())
        fprintf(yyout, "]");
    fprintf(yyout, "\n");
}

StoreMInstruction::StoreMInstruction(MachineBlock *p,
                                     MachineOperand *src1, MachineOperand *src2, MachineOperand *src3,
                                     int cond)
{
    this->parent = p;
    this->type = MachineInstruction::STORE;
    this->op = -1;
    this->cond = cond;
    this->use_list.push_back(src1);
    this->use_list.push_back(src2);
    if (src3)
        this->use_list.push_back(src3);
    src1->setParent(this);
    src2->setParent(this);
    if (src3)
        src3->setParent(this);
}

void StoreMInstruction::output()
{
    if (this->use_list[0]->getValType()->isFloat())
        fprintf(yyout, "\tvstr.32");
    else if (this->use_list[0]->getValType()->isBool())
        fprintf(yyout, "\tstrb");
    else
        fprintf(yyout, "\tstr");
    printCond();
    fprintf(yyout, " ");
    this->use_list[0]->output();
    fprintf(yyout, ", ");

    // Store address
    if (this->use_list[1]->isReg() || this->use_list[1]->isVReg())
        fprintf(yyout, "[");

    this->use_list[1]->output();
    if (this->use_list.size() > 2)
    {
        fprintf(yyout, ", ");
        this->use_list[2]->output();
    }

    if (this->use_list[1]->isReg() || this->use_list[1]->isVReg())
        fprintf(yyout, "]");
    fprintf(yyout, "\n");
}

MovMInstruction::MovMInstruction(MachineBlock *p, int op,
                                 MachineOperand *dst, MachineOperand *src,
                                 int cond)
{
    // assert(!src->isImm());
    this->parent = p;
    this->type = MachineInstruction::MOV;
    this->op = op;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src);
    dst->setParent(this);
    src->setParent(this);
}

void MovMInstruction::output()
{
    switch (this->op)
    {
    case MovMInstruction::MOV:
        if (use_list[0]->getValType()->isBool())
            fprintf(yyout, "\tmovw"); // move byte指令呢?
        else
        {
            assert(use_list[0]->getValType()->isInt());
            fprintf(yyout, "\tmov");
        }
        break;
    // case MovMInstruction::MVN:
    //     fprintf(yyout, "\tmvn");
    //     break;
    // case MovMInstruction::MOVT:
    //     fprintf(yyout, "\tmovt");
    //     break;
    case MovMInstruction::VMOV:
        fprintf(yyout, "\tvmov");
        break;
    // case MovMInstruction::VMOVF32:
    //     fprintf(yyout, "\tvmov.f32");
    //     break;
    default:
        break;
    }
    printCond();
    fprintf(yyout, " ");
    this->def_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[0]->output();
    fprintf(yyout, "\n");
}

BranchMInstruction::BranchMInstruction(MachineBlock *p, int op,
                                       MachineOperand *dst,
                                       int cond)
{
    this->type = MachineInstruction::BRANCH;
    this->op = op;
    this->parent = p;
    this->cond = cond;
    this->def_list.push_back(dst);
    dst->setParent(this);
}

void BranchMInstruction::output()
{
    switch (op)
    {
    case BL:
        fprintf(yyout, "\tbl");
        break;
    case B:
        fprintf(yyout, "\tb");
        break;
    // case BX:
    //     fprintf(yyout, "\tbx ");
    //     break;
    default:
        break;
    }
    printCond();
    fprintf(yyout, " ");
    this->def_list[0]->output();
    fprintf(yyout, "\n");
}

CmpMInstruction::CmpMInstruction(MachineBlock *p,
                                 MachineOperand *src1, MachineOperand *src2,
                                 int cond)
{
    this->type = MachineInstruction::CMP;
    this->parent = p;
    this->op = cond;
    this->use_list.push_back(src1);
    this->use_list.push_back(src2);
    src1->setParent(this);
    src2->setParent(this);
}

void CmpMInstruction::output()
{
    if (this->use_list[0]->getValType()->isFloat())
        fprintf(yyout, "\tvcmp.f32");
    else
        fprintf(yyout, "\tcmp");
    printCond();
    fprintf(yyout, " ");
    this->use_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[1]->output();
    fprintf(yyout, "\n");
}

StackMInstruction::StackMInstruction(MachineBlock *p, int op,
                                     MachineOperand *src,
                                     int cond)
{
    this->parent = p;
    this->type = MachineInstruction::STACK;
    this->op = op;
    this->cond = cond;
    this->use_list.push_back(src);
    src->setParent(this);
}

StackMInstruction::StackMInstruction(MachineBlock *p, int op,
                                     std::vector<MachineOperand *> src,
                                     int cond)
{
    this->parent = p;
    this->type = MachineInstruction::STACK;
    this->op = op;
    this->cond = cond;
    this->use_list = src;
    for (auto ope : use_list)
    {
        ope->setParent(this);
    }
}

void StackMInstruction::output()
{
    assert(this->cond == NONE);
    std::string op_str;
    if (this->use_list[0]->getValType()->isFloat())
    {
        switch (op)
        {
        case PUSH:
            op_str = "\tvpush {";
            break;
        case POP:
            op_str = "\tvpop {";
            break;
        }
    }
    else
    {
        switch (op)
        {
        case PUSH:
            op_str = "\tpush {";
            break;
        case POP:
            op_str = "\tpop {";
            break;
        }
    }
    // 浮点寄存器可能会很多 每次只能push/pop16个
    size_t i = 0;
    while (i != use_list.size())
    {
        fprintf(yyout, "%s", op_str.c_str());
        this->use_list[i++]->output();
        for (size_t j = 1; i != use_list.size() && j < 16; i++, j++)
        {
            fprintf(yyout, ", ");
            this->use_list[i]->output();
        }
        fprintf(yyout, "}\n");
    }
}

ZextMInstruction::ZextMInstruction(MachineBlock *p, MachineOperand *dst, MachineOperand *src, int cond)
{
    this->parent = p;
    this->type = MachineInstruction::ZEXT;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src);
    dst->setParent(this);
    src->setParent(this);
}

void ZextMInstruction::output()
{
    fprintf(yyout, "\tuxtb");
    printCond();
    fprintf(yyout, " ");
    def_list[0]->output();
    fprintf(yyout, ", ");
    use_list[0]->output();
    fprintf(yyout, "\n");
}

VcvtMInstruction::VcvtMInstruction(MachineBlock *p,
                                   int op,
                                   MachineOperand *dst,
                                   MachineOperand *src,
                                   int cond)
{
    this->parent = p;
    this->type = MachineInstruction::VCVT;
    this->op = op;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src);
    dst->setParent(this);
    src->setParent(this);
}

void VcvtMInstruction::output()
{
    switch (this->op)
    {
    case VcvtMInstruction::F2S:
        fprintf(yyout, "\tvcvt.s32.f32");
        break;
    case VcvtMInstruction::S2F:
        fprintf(yyout, "\tvcvt.f32.s32");
        break;
    default:
        break;
    }
    printCond();
    fprintf(yyout, " ");
    this->def_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[0]->output();
    fprintf(yyout, "\n");
}

void MachineBlock::insertBefore(MachineInstruction *pos, MachineInstruction *inst)
{
    auto p = find(inst_list.begin(), inst_list.end(), pos);
    inst_list.insert(p, inst);
}

void MachineBlock::insertAfter(MachineInstruction *pos, MachineInstruction *inst)
{
    auto p = find(inst_list.begin(), inst_list.end(), pos);
    if (p == inst_list.end())
    {
        inst_list.push_back(inst);
        return;
    }
    inst_list.insert(p + 1, inst);
}

void MachineBlock::output()
{
    fprintf(yyout, ".L%d:\n", this->no);
    for (auto iter : inst_list)
        iter->output();
}

MachineFunction::MachineFunction(MachineUnit *p, SymbolEntry *sym_ptr)
{
    this->parent = p;
    this->sym_ptr = sym_ptr;
    this->stack_size = 0;
};

std::vector<MachineOperand *> MachineFunction::getSavedRegs()
{
    std::vector<MachineOperand *> regs;
    for (auto no : saved_regs)
    {
        MachineOperand *reg = nullptr;
        reg = new MachineOperand(MachineOperand::REG, no);
        regs.push_back(reg);
    }
    return regs;
}

std::vector<MachineOperand *> MachineFunction::getSavedFRegs()
{
    std::vector<MachineOperand *> fregs;
    for (auto no : saved_fregs)
    {
        MachineOperand *freg = nullptr;
        freg = new MachineOperand(MachineOperand::REG, no, TypeSystem::floatType);
        fregs.push_back(freg);
    }
    return fregs;
}

void MachineFunction::output()
{
    fprintf(yyout, "\t.global %s\n", this->sym_ptr->toStr().c_str() + 1);
    fprintf(yyout, "\t.type %s , %%function\n", this->sym_ptr->toStr().c_str() + 1);
    fprintf(yyout, "%s:\n", this->sym_ptr->toStr().c_str() + 1);
    // Save callee saved int registers
    fprintf(yyout, "\tpush {");
    std::vector<MachineOperand *> regs = getSavedRegs();
    for (auto reg : regs)
    {
        reg->output();
        fprintf(yyout, ", ");
    }
    // Save fp, lr
    fprintf(yyout, "fp, lr}\n");
    // Save callee saved float registers
    std::vector<MachineOperand *> fregs = getSavedFRegs();
    size_t i = 0;
    while (i != fregs.size())
    {
        fprintf(yyout, "\tvpush {");
        fregs[i++]->output();
        for (int j = 1; i != fregs.size() && j != 16; i++, j++)
        {
            fprintf(yyout, ", ");
            fregs[i]->output();
        }
        fprintf(yyout, "}\n");
    }
    // 更新additional args的偏移
    for (auto offset : additional_args_offset)
        offset->setVal(offset->getVal() + 4 * (regs.size() + fregs.size() + 2));
    // fp = sp
    fprintf(yyout, "\tmov fp, sp\n");
    // Allocate stack space for local variable
    if (stack_size)
    {
        if (!isShifterOperandVal(reinterpret_cast<unsigned &>(stack_size)))
        {
            assert((std::find(saved_regs.begin(), saved_regs.end(), 4)) != saved_regs.end());
            fprintf(yyout, "\tldr r4,=%d\n", stack_size);
            fprintf(yyout, "\tsub sp, sp, r4\n");
        }
        else
            fprintf(yyout, "\tsub sp, sp, #%d\n", stack_size);
    }
    // Traverse all the block in block_list to print assembly code.
    int cnt = 0;
    bool outputEndLabel = false;
    for (auto iter : block_list)
    {
        iter->output();
        // 生成一条跳转到结尾函数栈帧处理的无条件跳转语句
        if (iter->getInsts().empty() || (!(*(iter->end() - 1))->isBranch() && iter != *(block_list.end() - 1)))
        {
            outputEndLabel = true;
            std::string endLabel = ".L" + this->sym_ptr->toStr().erase(0, 1) + "_END";
            fprintf(yyout, "\tb %s\n", endLabel.c_str());
        }
        cnt += iter->getInsts().size();
        if (cnt > 300)
        {
            fprintf(yyout, "\tb .LiteralPool%d\n", parent->getLtorgNo());
            fprintf(yyout, ".LTORG\n");
            parent->printBridge();
            fprintf(yyout, ".LiteralPool%d:\n", parent->getLtorgNo() - 1);
            cnt = 0;
        }
    }
    // output endLabel
    if (outputEndLabel)
        fprintf(yyout, ".L%s_END:\n", this->sym_ptr->toStr().erase(0, 1).c_str()); // skip '@'
    // Restore callee saved registers
    if (stack_size)
    {
        if (!isShifterOperandVal(reinterpret_cast<unsigned &>(stack_size)))
        {
            assert((std::find(saved_regs.begin(), saved_regs.end(), 4)) != saved_regs.end());
            fprintf(yyout, "\tldr r4,=%d\n", stack_size);
            fprintf(yyout, "\tadd sp, sp, r4\n");
        }
        else
            fprintf(yyout, "\tadd sp, sp, #%d\n", stack_size);
    }
    // Restore saved registers and fp, lr
    i = 0;
    while (i != fregs.size())
    {
        fprintf(yyout, "\tvpush {");
        fregs[i++]->output();
        for (int j = 1; i != fregs.size() && j != 16; i++, j++)
        {
            fprintf(yyout, ", ");
            fregs[i]->output();
        }
        fprintf(yyout, "}\n");
    }
    fprintf(yyout, "\tpop {");
    for (auto reg : regs)
    {
        reg->output();
        fprintf(yyout, ", ");
    }
    fprintf(yyout, "fp, lr}\n");
    // Generate bx instruction
    fprintf(yyout, "\tbx lr\n\n");
}

void MachineUnit::printGlobalDecl()
{
    // TODO: Array
    // print global variable declaration code;
    if (!global_var_list.empty())
        fprintf(yyout, "\t.data\n");
    for (auto var : global_var_list)
    {
        fprintf(yyout, "\t.global %s\n", var->toStr().c_str());
        fprintf(yyout, "\t.align 4\n");
        fprintf(yyout, "\t.size %s, %d\n", var->toStr().c_str(), /*var->getType()->getSize()*/ 4);
        fprintf(yyout, "%s:\n", var->toStr().c_str());
        if (var->getType()->isInt())
            fprintf(yyout, "\t.word %d\n", int(var->getValue()));
        else
        {
            float value = float(var->getValue());
            fprintf(yyout, "\t.word %u\n", reinterpret_cast<unsigned &>(value));
        }
    }
}

void MachineUnit::output()
{
    // TODO
    /* Hint:
     * 1. You need to print global variable/const declarition code;
     * 2. Traverse all the function in func_list to print assembly code;
     * 3. Don't forget print bridge label at the end of assembly code!! */
    fprintf(yyout, "\t.arch armv8-a\n");
    // fprintf(yyout, "\t.fpu vfpv3-d16\n");
    fprintf(yyout, "\t.arch_extension crc\n");
    fprintf(yyout, "\t.arm\n");
    printGlobalDecl();
    fprintf(yyout, "\t.text\n");
    int cnt = 0;
    for (auto iter : func_list)
    {
        iter->output();

        // literal pool
        for (auto bb : iter->getBlocks())
            cnt += bb->getInsts().size();
        if (cnt > 300)
        {
            fprintf(yyout, "\tb .LiteralPool%d\n", LtorgNo);
            fprintf(yyout, ".LTORG\n");
            printBridge();
            fprintf(yyout, ".LiteralPool%d:\n", LtorgNo - 1);
            cnt = 0;
        }
    }
    printBridge();
}

void MachineUnit::printBridge()
{
    for (auto sym_ptr : global_var_list)
    {
        fprintf(yyout, "addr_%d_%s:\n", LtorgNo, sym_ptr->toStr().c_str());
        fprintf(yyout, "\t.word %s\n", sym_ptr->toStr().c_str());
    }
    LtorgNo++;
}