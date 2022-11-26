#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  this->fs_ = this->frame_->name_->Name();


  // store callee save
  auto instr_list = new assem::InstrList();
  auto callee_save = reg_manager->CalleeSaves()->GetList();
  std::list<std::pair<temp::Temp *, temp::Temp *>> tmp_store; // to restore
  for (auto reg: callee_save) {
    temp::Temp *store = temp::TempFactory::NewTemp();
    instr_list->Append(new assem::MoveInstr("movq `s0, `d0",
                                            new temp::TempList(store),
                                            new temp::TempList(reg)));
    tmp_store.push_front(std::pair<temp::Temp *, temp::Temp *>(reg, store));
                                            
  }

  auto trace_list = this->traces_.get()->GetStmList()->GetList();
  for (auto trace: trace_list) {
    trace->Munch(*instr_list, fs_);
  }

  // restore callee save
  for(auto restore_it: tmp_store) {
    temp::Temp *reg = restore_it.first;
    temp::Temp *store_reg = restore_it.second;

    instr_list->Append(new assem::MoveInstr("movq `s0, `d0",
                                            new temp::TempList(reg),
                                            new temp::TempList(store_reg)));
  }
  
  this->assem_instr_ = std::make_unique<cg::AssemInstr>(frame::ProcEntryExit2(instr_list));

}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->left_->Munch(instr_list, fs);
  this->right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(temp::LabelFactory::LabelString(this->label_), this->label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(
    new assem::OperInstr(
      "jmp `j0",
      nullptr,
      nullptr,
      new assem::Targets(this->jumps_)            
    )
  );
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *l = this->left_->Munch(instr_list, fs);
  temp::Temp *r = this->right_->Munch(instr_list, fs);

  std::string cmd;
  switch (this->op_)
  {
  case tree::RelOp::EQ_OP:
    cmd = "je";
    break;
  case tree::RelOp::NE_OP:
    cmd = "jne";
    break;
  case tree::RelOp::LT_OP:
    cmd = "jl";
    break;
  case tree::RelOp::GT_OP:
    cmd = "jg";
    break;
  case tree::RelOp::LE_OP:
    cmd = "jle";
    break;
  case tree::RelOp::GE_OP:
    cmd = "jge";
    break;
  default:
    break;
  }
  cmd.append(" `j0");
  instr_list.Append(new assem::OperInstr(
        "cmpq `s1, `s0",
        nullptr,
        new temp::TempList({l, r}),
        nullptr
  ));

  auto label = new std::vector<temp::Label*>;
  label->push_back(this->true_label_);
  label->push_back(this->false_label_);
  instr_list.Append(new assem::OperInstr(
    cmd,
    nullptr,
    nullptr,
    new assem::Targets(label)
  ));

}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if (this->dst_->type() == tree::Exp::Type::TEMP) {
    auto l = this->src_->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr(
      "movq `s0, `d0", 
      new temp::TempList(((tree::TempExp*)dst_)->temp_),
      new temp::TempList(l)
    ));
  }else if (this->dst_->type() == tree::Exp::Type::MEM) {
    auto l = this->src_->Munch(instr_list, fs);
    auto r = ((tree::MemExp*)this->dst_)->Munch(instr_list, fs);

    auto l_and_r = new temp::TempList();
    l_and_r->Append(l);
    l_and_r->Append(r);
    instr_list.Append(new assem::OperInstr(
      "movq `s0, (`s1)",
      nullptr,
      l_and_r,
      nullptr
    ));
  }else assert(0);
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *l = this->left_->Munch(instr_list, fs);
  temp::Temp *r = this->right_->Munch(instr_list, fs);

  temp::Temp *rt = temp::TempFactory::NewTemp();

  switch (this->op_)
  {
    case tree::PLUS_OP:
    { 
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(rt), new temp::TempList(l)));
      instr_list.Append(new assem::OperInstr("addq `s0, `d0", new temp::TempList(rt), new temp::TempList({r, rt}), nullptr));
      break;
    }
    case tree::MINUS_OP:
    { 
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(rt), new temp::TempList(l)));
      instr_list.Append(new assem::OperInstr("subq `s0, `d0", new temp::TempList(rt), new temp::TempList({r, rt}), nullptr));
      break;
    }
    case tree::MUL_OP:
    { 
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg_manager->ReturnValue()), new temp::TempList(l)));
      instr_list.Append(new assem::OperInstr("imulq `s0", new temp::TempList(reg_manager->ReturnValue()), new temp::TempList(r), nullptr));
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(rt), new temp::TempList(reg_manager->ReturnValue())));
      break;
    }
    case tree::DIV_OP:
    { 
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg_manager->ReturnValue()), new temp::TempList(l)));
      instr_list.Append(new assem::OperInstr("cqto", new temp::TempList({reg_manager->Dx(), reg_manager->ReturnValue()}), new temp::TempList(reg_manager->ReturnValue()), nullptr));
      instr_list.Append(new assem::OperInstr("idivq `s0", new temp::TempList({reg_manager->Dx(), reg_manager->ReturnValue()}), new temp::TempList(r), nullptr));
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(rt), new temp::TempList(reg_manager->ReturnValue())));
      break;
    }
    default:
      break;
  }
  return rt;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto rt = temp::TempFactory::NewTemp();
  auto exp_reg = this->exp_->Munch(instr_list, fs);
  instr_list.Append(new assem::OperInstr("movq (`s0), `d0", new temp::TempList(rt), new temp::TempList(exp_reg), nullptr));
  return rt;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto rt = this->temp_;
  if (this->temp_ == reg_manager->FramePointer()) {
    auto reg = temp::TempFactory::NewTemp();
    std::stringstream stream;
    stream << "leaq " << fs << "(`s0), `d0";
    instr_list.Append(new assem::OperInstr(stream.str(), new temp::TempList(reg), new temp::TempList(reg_manager->StackPointer()), nullptr)) ;
    rt = reg;
  }
  return rt;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::stringstream stream;
  auto rt = temp::TempFactory::NewTemp();
  stream << "leaq " << temp::LabelFactory::LabelString(this->name_) << "(%rip), `d0";
  instr_list.Append(new assem::OperInstr(stream.str(), new temp::TempList(rt), nullptr, nullptr));
  return rt;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::stringstream stream;
  auto rt = temp::TempFactory::NewTemp();
  stream << "movq $" << this->consti_ << ", `d0";
  instr_list.Append(new assem::OperInstr(stream.str(), new temp::TempList(rt), nullptr, nullptr));
  return rt;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto rt = temp::TempFactory::NewTemp();
  std::string label = temp::LabelFactory::LabelString(((tree::NameExp*)fun_)->name_);
  this->args_->MunchArgs(instr_list, fs);
  instr_list.Append(new assem::OperInstr("callq "+label, reg_manager->CallerSaves(), reg_manager->ArgRegs(), nullptr));
  instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(rt), new temp::TempList(reg_manager->ReturnValue())));
  return rt;
}
temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto rt = new temp::TempList();

  int i = 0;
  int word_size = reg_manager->WordSize();
  auto arg_reg = reg_manager->ArgRegs()->GetList();
  auto it_arg_reg = arg_reg.begin();
  for(auto exp: GetList()) {
    auto arg = exp->Munch(instr_list, fs);
    if (i<6) {
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(*it_arg_reg), new temp::TempList(arg)));
    }else {
      std::stringstream stream;
      stream << "movq `s0, " << (i-6)*word_size << "(`s1)";
      instr_list.Append(new assem::OperInstr(stream.str(),nullptr, new temp::TempList({arg, reg_manager->StackPointer()}), nullptr));

    }
    ++i;
    
  }
  return rt;
}

} // namespace tree
