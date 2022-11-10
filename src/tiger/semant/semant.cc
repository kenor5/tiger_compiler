#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"


namespace absyn {

#define INTTY type::IntTy::Instance();

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  
  env::EnvEntry *e = venv->Look(this->sym_);
  if (e && e->type() == 0) 
      return ((env::VarEntry *)e)->ty_->ActualTy();
  else
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());

  return INTTY;
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // type::RecordTy *exap;
  
  type::Ty *t = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (typeid(*t) == typeid(type::RecordTy)) {
    type::RecordTy *r = (type::RecordTy *)t;
    std::list<type::Field *> l = r->fields_->GetList();
    for (auto it = l.begin(); it != l.end(); it++) {
      if ((*it)->name_ == this->sym_) {
        return (*it)->ty_->ActualTy();
      }
    }
    errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
    return nullptr;
  }else 
    errormsg->Error(this->pos_, "not a record type");
  
  return INTTY;
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  
  type::Ty *t = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *t2 = this->subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*t) == typeid(type::ArrayTy) && typeid(*t2) == typeid(type::IntTy) ) {
    type::ArrayTy *a = (type::ArrayTy *)t;
    return a->ty_->ActualTy();

  }else 
    errormsg->Error(this->pos_, "array type required");
  
  return INTTY;
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return this->var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return INTTY;
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *e = venv->Look(this->func_);
  if (e && e->type() == 1) {
    env::FunEntry *f = (env::FunEntry *)e;

    std::list<type::Ty *> req = f->formals_->GetList();
    std::list<Exp *> l = this->args_->GetList();
    int size1 = req.size();
    
    

    auto it2 = req.begin();
    for (auto it1 = l.begin(); it1 != l.end() && it2 != req.end(); it1++, it2++) {
      if (!(*it2)->IsSameType((*it1)->SemAnalyze(venv, tenv, labelcount, errormsg))){
        errormsg->Error(this->pos_, "para type mismatch");
        break;
      }
    }
    if (size1 < l.size()) {
      errormsg->Error(this->pos_, "too many params in function %s", func_->Name().data());
    }

    if (size1 > l.size()) {
      errormsg->Error(this->pos_, "too few params in function %s", func_->Name().data());
    }
    return f->result_->ActualTy();

  }else 
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());

  return INTTY;
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *l = this->left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *r = this->right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (this->oper_ == Oper::PLUS_OP ||
  this->oper_ == Oper::MINUS_OP ||
  this->oper_ == Oper::DIVIDE_OP ||
  this->oper_ == Oper::TIMES_OP 
  )
  {
    if (typeid(type::IntTy) != typeid(*l))
      
      errormsg->Error(left_->pos_, "integer required");
    if (typeid(type::IntTy) != typeid(*r))
    errormsg->Error(right_->pos_, "integer required");
      
  }else{
    if(!l->IsSameType(r))
      errormsg->Error(this->pos_, "same type required");
  }
  return INTTY;
  
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *t = tenv->Look(this->typ_);
  
  // type::RecordTy *r;
  if (t && typeid(*t->ActualTy()) == typeid(type::RecordTy)) {
    t = t->ActualTy();
    type::RecordTy *cur = (type::RecordTy *)t;
    std::list<type::Field *> l = cur->fields_->GetList();
    std::list<EField*> l2 = this->fields_->GetList();


    if (l.size() != l2.size())
      {errormsg->Error(this->pos_, "field nums mismatch");}
    auto it1 = l.begin();
    for (auto it2 = l2.begin();it2 != l2.end(); it1++, it2++) {
      if ((*it1)->name_ != (*it2)->name_ ){
        errormsg->Error(this->pos_, "record name mismatch");
        break;
      }

      if (!(*it1)->ty_->IsSameType((*it2)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg))) {
            errormsg->Error(this->pos_, "record type mismatch");
            break;
        }
    }
    return cur->ActualTy();

  }else 
    errormsg->Error(this->pos_, "undefined type rectype");
  return INTTY;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<Exp *> l = this->seq_->GetList();
  type::Ty *rt = type::VoidTy::Instance();

  for (auto &it:l) {
    rt = it->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return rt;
}
type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *t1 = this->var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *t2 = this->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);

  absyn::SimpleVar *s;

  if (typeid(*var_) == typeid(SimpleVar)) {
    SimpleVar *sv = (SimpleVar *) var_;
    env::EnvEntry *entry = venv->Look(sv->sym_);
    if (entry->readonly_) {
      errormsg->Error(sv->pos_, "loop variable can't be assigned");
    }
  }

  if (t1 == nullptr || t2 == nullptr) return type::VoidTy::Instance();
  if (!t1->IsSameType(t2)) {
    errormsg->Error(this->pos_, "unmatched assign exp");
  }
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *t1 = this->test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *t2 = this->then_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if(typeid(*t1) != typeid(type::IntTy)) {
    errormsg->Error(this->test_->pos_, "if test type err");
  }
  
  if (this->elsee_) {
    type::Ty *t3 = this->elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!t2->IsSameType(t3)) {
      errormsg->Error(this->elsee_->pos_, "then exp and else exp type mismatch");

    }
    return t2->ActualTy();
  }else {
    if (typeid(*t2) != typeid(type::VoidTy)) {
      errormsg->Error(this->then_->pos_, "if-then exp's body must produce no value");
    }
  }

  return type::VoidTy::Instance();
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *t1 = this->test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  

  if(typeid(*t1) != typeid(type::IntTy)) {
    errormsg->Error(this->test_->pos_, "test err");
  }

  if (this->body_) {
    type::Ty *t2 = this->body_->SemAnalyze(venv, tenv, labelcount+1, errormsg);
    if (typeid(*t2) != typeid(type::VoidTy)) {
        errormsg->Error(this->body_->pos_, "while body must produce no value");
      }
  }

   return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *t1 = this->lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *t2 = this->hi_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (typeid(*t1) != typeid(type::IntTy)) {
            errormsg->Error(lo_->pos_, "for exp's range type is not integer");
        }
  if (typeid(*t2) != typeid(type::IntTy)) {
      errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  }

  if (this->body_) {
    venv->BeginScope();
    venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));

    type::Ty *t3 = this->body_->SemAnalyze(venv, tenv, labelcount+1, errormsg);

    venv->EndScope();
    if (typeid(*t3) != typeid(type::VoidTy)) {
        errormsg->Error(this->body_->pos_, "for exp's body must produce no value");
      }
  }
    return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (labelcount == 0) {
            errormsg->Error(pos_, "break is not inside any loop");
        }
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<Dec*> l = this->decs_->GetList();
  type::Ty *rt = type::VoidTy::Instance();
  venv->BeginScope();
  tenv->BeginScope();
  for (auto it = l.begin();it != l.end(); it++) {
    (*it)->SemAnalyze(venv, tenv, labelcount, errormsg);
  }

  if (body_) rt = body_->SemAnalyze(venv, tenv, labelcount, errormsg);

  tenv->EndScope();
  venv->EndScope();
  return rt;

}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *t1 = tenv->Look(this->typ_);
  if (!t1) {
    errormsg->Error(this->pos_, "no array type");
    return INTTY;
  }

  // type::ArrayTy *t2 ;
  t1 = t1->ActualTy();
  if (typeid(*t1)!=typeid(type::ArrayTy)) {
    errormsg->Error(this->pos_, "type mismatch");
    return INTTY;
  }

  type::ArrayTy *t3 = (type::ArrayTy *)t1;
  type::Ty *t_size = this->size_->SemAnalyze(venv, tenv, labelcount, errormsg);
  // type::IntTy *t4;
  if (typeid(*t_size)!=typeid(type::IntTy)) {
    errormsg->Error(this->pos_, "array size not fit");
  }

  type::Ty *init_ty = this->init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!t3->ty_->IsSameType(init_ty)){
    errormsg->Error(this->pos_, "init type mismatch");
  }
  return t3->ActualTy();
  
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<FunDec *> l = this->functions_->GetList();
  for (auto &func: l) {

    if (venv->Look(func->name_)) {
      errormsg->Error(func->pos_, "two functions have the same name");
    }

    type::TyList *tl = func->params_->MakeFormalTyList(tenv, errormsg);

    if (func->result_)
      venv->Enter(func->name_, new env::FunEntry(tl, tenv->Look(func->result_)));
    else 
      venv->Enter(func->name_, new env::FunEntry(tl, type::VoidTy::Instance()));
  }

  for (auto &func: l) {
    venv->BeginScope();

    std::list<type::Ty*> tl = func->params_->MakeFormalTyList(tenv, errormsg)->GetList();
    std::list<type::Field *> fl = func->params_->MakeFieldList(tenv, errormsg)->GetList();
    assert(tl.size() == fl.size());
    auto it1 = tl.begin();

    for (auto it2 = fl.begin(); it2 != fl.end(); it2 ++, it1++) {
      venv->Enter((*it2)->name_, new env::VarEntry((*it1)));
    }

    type::Ty *ty = func->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    env::EnvEntry *entry = venv->Look(func->name_);
    if (typeid(*entry) != typeid(env::FunEntry) || ty != static_cast<env::FunEntry *>(entry)->result_->ActualTy()) {
        errormsg->Error(pos_, "procedure returns value");
    }

    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (venv->Look(this->var_)) {
    return;
  }
  
  type::Ty *init_t = this->init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (this->typ_) {
    type::Ty *t = tenv->Look(this->typ_);
    if (!t) {
      errormsg->Error(pos_, "undefined type of %s", this->typ_->Name().data());
    }else {
      if (!init_t->IsSameType(t))
        errormsg->Error(this->pos_, "type mismatch");
      venv->Enter(this->var_,new env::VarEntry(t, false));

    }

  }else {
    if (init_t->IsSameType(type::NilTy::Instance()) && typeid(*(init_t->ActualTy())) != typeid(type::RecordTy)) 
      errormsg->Error(pos_, "init should not be nil without type specified");
    venv->Enter(this->var_,new env::VarEntry(init_t));

  }
  
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<NameAndTy *> l = this->types_->GetList();
  for (auto &nat: l) {
    if (tenv->Look(nat->name_)) {
      errormsg->Error(this->pos_, "two types have the same name");
    }else {
      tenv->Enter(nat->name_, new type::NameTy(nat->name_, nullptr));
    }
  }

  for (auto &nat: l) {
    type::Ty *ty = tenv->Look(nat->name_);
    type::NameTy *nt = (type::NameTy *)ty;

    nt->ty_ = nat->ty_->SemAnalyze(tenv, errormsg);
  }

  for (auto &nat: l) {
    type::Ty *t = tenv->Look(nat->name_);
    auto ty = ((type::NameTy *)t)->ty_;
    while (typeid(*ty) == typeid(type::NameTy))
    {
      auto namety = (type::NameTy *)ty;
      if (namety->sym_ == nat->name_)
        goto flag;
      
      ty = namety->ty_;
    }
    
  }
  return;

  flag:
    errormsg->Error(this->pos_, "illegal type cycle");
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *t = tenv->Look(this->name_);
  if (t) return t;
  errormsg->Error(this->pos_, "undefined type");
  return INTTY;
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return new type::RecordTy(this->record_->MakeFieldList(tenv, errormsg));

}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *t = tenv->Look(this->array_);
  if (t) return t->ActualTy();
  errormsg->Error(this->pos_, "undefined type");
  return INTTY;
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
