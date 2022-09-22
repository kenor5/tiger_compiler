#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int left = this->stm1->MaxArgs();
  int right = this->stm2->MaxArgs();
  return left > right ? left : right;
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  Table *rt = this->stm1->Interp(t);
  rt = this->stm2->Interp(rt);
  return rt;
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return this->exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *rt = this->exp->Interp(t);
  rt->t = rt->t->Update(this->id, rt->i);
  // if (t==nullptr)std::cerr<<"cao";
  // rt->t->show();
  // std::cerr << this->id << ": " << rt->i << std::endl;
  return rt->t;
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int left = this->exps->NumExps();
  int right = this->exps->MaxArgs();
  return left > right ? left : right;

}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *rt = this->exps->Interp(t);


  return rt->t;
}

int A::EseqExp::MaxArgs() const {
  int left = this->exp->MaxArgs();
  int right = this->stm->MaxArgs();
  return left > right ? left : right;

}

int A::IdExp::MaxArgs() const {
  return 0;

}

int A::NumExp::MaxArgs() const {
  return 0;

}

int A::OpExp::MaxArgs() const {
  int left = this->left->MaxArgs();
  int right = this->right->MaxArgs();
  return left > right ? left : right;

}

int A::PairExpList::MaxArgs() const {
  int left = this->exp->MaxArgs();
  int right = this->tail->MaxArgs();
  return left > right ? left : right;

}

int A::LastExpList::MaxArgs() const {
  return this->exp->MaxArgs();

}


IntAndTable *A::IdExp::Interp(Table *t)const{
  return new IntAndTable(t->Lookup(this->id), t);

}

IntAndTable *A::EseqExp::Interp(Table *t)const{
  Table *r = this->stm->Interp(t);
  IntAndTable *rt = this->exp->Interp(r);
  return rt;

}

IntAndTable *A::NumExp::Interp(Table *t)const{
  return new IntAndTable(this->num, t);

}

IntAndTable *A::OpExp::Interp(Table *t)const{
  IntAndTable *left = this->left->Interp(t);
  IntAndTable *right = this->right->Interp(left->t);
  switch (this->oper)
  {
  case PLUS:
    return new IntAndTable((left->i)+(right->i), right->t);
  case MINUS:
    return new IntAndTable((left->i)-(right->i), right->t);
  case TIMES:
    return new IntAndTable((left->i)*(right->i), right->t);
  case DIV:
    return new IntAndTable((left->i)/(right->i), right->t);
  default:
    break;
  }

}

IntAndTable *A::PairExpList::Interp(Table *t)const{
  IntAndTable *left = this->exp->Interp(t);
  std::cout << left->i << " ";

  return this->tail->Interp(left->t);


}

IntAndTable *A::LastExpList::Interp(Table *t)const{
  IntAndTable *rt =  this->exp->Interp(t);
  std::cout << rt->i << std::endl;
  return rt;

}

int A::PairExpList::NumExps() const{
  return 1 + this->tail->NumExps();

}

int A::LastExpList::NumExps() const{
  return 1;
}





int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
}  // namespace A
