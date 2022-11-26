#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc
                 

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */
  this->root_->Traverse(env, 0);
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  // for simple var, we need to compare current depth and declaration depth
  esc::EscapeEntry *ent = env->Look(this->sym_);
  if (ent && ent->depth_ < depth) 
    *ent->escape_ = true;
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
  // esc::EscapeEntry *ent = env->Look(this->sym_);
  // if (ent && ent->depth_ < depth) 
  //   *ent->escape_ = true;
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->subscript_->Traverse(env, depth);
  this->var_->Traverse(env, depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto lists = this->args_->GetList();
  for (auto l:lists) {
    l->Traverse(env, depth);
  }
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->left_->Traverse(env, depth);
  this->right_->Traverse(env, depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto lists = this->fields_->GetList();
  for (auto l:lists) {
    l->exp_->Traverse(env, depth);
  }
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto lists = this->seq_->GetList();
  for (auto l:lists) {
    l->Traverse(env, depth);
  }
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->exp_->Traverse(env, depth);
  this->var_->Traverse(env, depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env, depth);
  this->then_->Traverse(env, depth);
  if (elsee_) this->elsee_->Traverse(env, depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env, depth);
  if (this->body_) this->body_->Traverse(env, depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->hi_->Traverse(env, depth);
  this->lo_->Traverse(env, depth);
  if (this->body_) {
    env->BeginScope();

    this->escape_ = false;
    env->Enter(var_, new esc::EscapeEntry(depth, &this->escape_));
    this->body_->Traverse(env, depth);
    env->EndScope();
  }
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

    env->BeginScope();
    auto lists = this->decs_->GetList();
    for (auto l:lists) 
      l->Traverse(env, depth);
    
    
    if (this->body_)
      this->body_->Traverse(env, depth);
    env->EndScope();
  
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->init_->Traverse(env, depth);
  this->size_->Traverse(env, depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto dec_lists = this->functions_->GetList();
  for (auto l: dec_lists) {
    env->BeginScope();

    auto var_lists = l->params_->GetList();
    for (auto l1: var_lists) {
      l1->escape_ = false;
      env->Enter(l1->name_, new esc::EscapeEntry(depth+1, &l1->escape_));
    }

    if (l->body_) l->body_->Traverse(env, depth+1);
    env->EndScope();
  }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->init_->Traverse(env, depth);
  this->escape_ = false;
  env->Enter(this->var_, new esc::EscapeEntry(depth, &this->escape_));
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto t_lists = this->types_->GetList();
  for (auto l: t_lists) {

    if (typeid(*(l->ty_)) == typeid(RecordTy)) {

      RecordTy * r = (RecordTy *)(l->ty_);
      auto rlists = r->record_->GetList();
      for (auto l1: rlists) 
        l1->escape_ = true;
      
    }
  }
}

} // namespace absyn
