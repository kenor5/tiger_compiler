#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */

  X64RegManager::X64RegManager() {
    std::vector<std::string *> names;
    reg_num = 16;
    std::string reg_list[] = {"%rax",
                        "%rbx",
                        "%rcx",
                        "%rdx",
                        "%rsi",
                        "%rdi",
                        "%rbp",
                        "%rsp",
                        "%r8",
                        "%r9",
                        "%r10",
                        "%r11",
                        "%r12",
                        "%r13",
                        "%r14",
                        "%r15"};
    for (int i = 0; i < reg_num; i++) {
      names.push_back(new std::string(reg_list[i]));
    }

    for (auto name: names) {
      auto reg = temp::TempFactory::NewTemp();
      regs_.push_back(reg);
      temp_map_->Enter(reg, name);
    }
  }

  temp::TempList *X64RegManager::Registers() {
    auto rt = new temp::TempList();
    for (int i = 0; i < reg_num; i++) {
      if (i == 7) continue;
      rt->Append(GetRegister(i));
    }

    return rt;
  }

  temp::TempList *X64RegManager::ArgRegs() {
    int list[] = {5, 4, 3, 2, 8, 9};
    auto rt = new temp::TempList();
    for (auto i:list) 
      rt->Append(GetRegister(i));
    
    return rt;
  }

  temp::TempList *X64RegManager::CallerSaves() {
    int list[] = {0, 2, 3, 4, 5, 8, 9, 10, 11};
    auto rt = new temp::TempList();
    for (auto i:list) 
      rt->Append(GetRegister(i));
    
    return rt;
  }

  temp::TempList *X64RegManager::CalleeSaves() {
    int list[] = {1, 6, 12, 13, 14, 15};
    auto rt = new temp::TempList();
    for (auto i:list) 
      rt->Append(GetRegister(i));
    
    return rt;
  }

  temp::TempList *X64RegManager::ReturnSink() {
    int list[] = {0, 7};
    auto rt = new temp::TempList();
    for (auto i:list) 
      rt->Append(GetRegister(i));
    
    return rt;
    
  }

  temp::TempList *X64RegManager::OperateRegs(){
    int list[] = {3, 0};
    auto rt = new temp::TempList();
    for (auto i:list) 
      rt->Append(GetRegister(i));
    
    return rt;
  }

  int X64RegManager::WordSize() {
        return 8;
  }

  temp::Temp *X64RegManager::FramePointer() {
      return GetRegister(6); //%rbp
  }

  temp::Temp *X64RegManager::StackPointer() {
      return GetRegister(7); //%rsp
  }

  temp::Temp *X64RegManager::ReturnValue() {
      return GetRegister(0); //%rax
  }

  temp::Temp *X64RegManager::Dx() {
      return GetRegister(3); //%rdx
  }



class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */

  tree::Exp *toExp(tree::Exp *fp = nullptr) const override {
    auto bopExp = new tree::BinopExp(tree::BinOp::PLUS_OP, fp,
                                   new tree::ConstExp(offset));
    return new tree::MemExp(bopExp);
  }
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */

  tree::Exp *toExp(tree::Exp *fp = nullptr) const override {
      return new tree::TempExp(this->reg);
    }
};



void X64Frame::setFormals(const std::list<bool> &esc) {
  for (auto l:esc) {
    this->formals_.push_back(AllocLocal(l));
  }

  auto arg_reg = reg_manager->ArgRegs()->GetList();
  auto it_arg = arg_reg.begin();
  int i = 0;
  for (auto access: formals_) {
    tree::Stm * stm;
    tree::Exp* dst = access->toExp(new tree::TempExp(reg_manager->FramePointer()));
    if (it_arg != arg_reg.end()) {
      stm = new tree::MoveStm(dst, new tree::TempExp(*it_arg));
      it_arg++;

    }else {
      stm = new tree::MoveStm(
        dst,
        new tree::MemExp(
          new tree::BinopExp(
            tree::BinOp::PLUS_OP,
            new tree::TempExp(reg_manager->FramePointer()),
            new tree::ConstExp((i+1)*8)
          )
        )
      );
      i++;
    }
    if (!view_shift)
      view_shift = stm;
    else view_shift = new tree::SeqStm(view_shift, stm);

  }
  
}

frame::Access *X64Frame::AllocLocal(bool esc) {
  Access* rt = nullptr;
  if (esc) {
    this->offset_ -= 8;
    rt = new InFrameAccess(offset_);
  }else rt = new InRegAccess(temp::TempFactory::NewTemp());
  return rt;
}
/* TODO: Put your lab5 code here */

tree::Exp *ExternalCall(const std::string &name, tree::ExpList *args) {
   temp::Label *label = temp::LabelFactory::NamedLabel(name);
   return new tree::CallExp(new tree::NameExp(label), args);
 }


Frame *NewFrame(temp::Label *label, const std::list<bool> &formals) {
  
}
tree::Stm *ProcEntryExit1(Frame *f, tree::Stm *stm) {
  return new tree::SeqStm(f->view_shift, stm);
}
assem::InstrList *ProcEntryExit2(assem::InstrList *body){
  body->Append(new assem::OperInstr("", new temp::TempList(), reg_manager->ReturnSink(), nullptr));
  return body;

}
assem::Proc *ProcEntryExit3(frame::Frame *f, assem::InstrList *body){
  int argRegs = reg_manager->ArgRegs()->GetList().size();
        int ws = reg_manager->WordSize();

        // 传统的做法：
        // call 的时候先把返回地址压入栈中
        // 然后把 %rbp 压入栈中
        // 再把 %rsp 赋值给 %rbp，这样 %rbp 指向的对象就是保存的旧值
        // 再将调用者保护寄存器压入栈中
        // ...

        // 现在的做法：
        // call 的时候保存一个栈帧大小，假设为k
        // 那么 %rsp + k 就是帧指针，即 FP = SP + k
        //

        // 栈帧尺寸伪指令 ".set xx_framesize, size"
        std::string prolog =
            ".set " + f->name_->Name() + "_framesize, " + std::to_string(-(f->offset_)) + "\n";

        // 当前过程标签 "name:"
        prolog += f->name_->Name() + ":\n";

        // 当参数超过6个时需要额外的栈空间存这些参数
        int size_for_args = std::max(f->arg_num - argRegs, 0) * ws;
        // "subq $size, %rsp"
        prolog += "subq $" + std::to_string(size_for_args - f->offset_) + ", %rsp\n";

        // 出口恢复栈指针
        //"addq $size, %rsp"
        std::string epilog = "addq $" + std::to_string(size_for_args - f->offset_) + ", %rsp\n";

        // 返回指令
        epilog += "retq\n";
        return new assem::Proc(prolog, body, epilog);
}

} // namespace frame

