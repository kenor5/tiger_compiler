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



} // namespace frame