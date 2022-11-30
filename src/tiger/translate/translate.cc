#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
// #include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  return new tr::Access(level, level->frame_->AllocLocal(escape));
}

Level *NewLevel(Level *parent, temp::Label *label, std::list<bool> escape) {
  auto l = escape;
  l.push_front(true); // static link
  frame::Frame *f = new frame::X64Frame(label);
  f->setFormals(l);
  return new Level(f, parent);
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    /* TODO: Put your lab5 code here */
    return this->exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(this->exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();

    tree::CjumpStm *s = new tree::CjumpStm(
      tree::NE_OP, exp_, new tree::ConstExp(0), t, f
    );

    std::list<temp::Label **> patch_list_t;
    std::list<temp::Label **> patch_list_f;
    patch_list_t.push_back(&(s->true_label_));
    patch_list_f.push_back(&(s->false_label_));
    PatchList t_(patch_list_t);
    PatchList f_(patch_list_f);

    return tr::Cx(t_,f_, s);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(
      this->stm_, new tree::ConstExp(0)
    );
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    /* TODO: Put your lab5 code here */
    return this->stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    assert(false);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */

    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t  = temp::LabelFactory::NewLabel();
    temp::Label *f  = temp::LabelFactory::NewLabel();
    this->cx_.trues_.DoPatch(t);
    this->cx_.falses_.DoPatch(f);

    return new tree::EseqExp(
      new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
      new tree::EseqExp(
        this->cx_.stm_,
        new tree::EseqExp(
          new tree::LabelStm(f),
          new tree::EseqExp(
            new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(0)),
            new tree::EseqExp(
              new tree::LabelStm(t), new tree::TempExp(r)
            )
          )
        )
      )
    );
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    /* TODO: Put your lab5 code here */
    return this->cx_;
  }
};

void ProgTr::Translate() {

  FillBaseTEnv();
  FillBaseVEnv();


  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *root = this->absyn_tree_->Translate(
    venv_.get(),
    tenv_.get(),
    this->main_level_.get(),
    temp::LabelFactory::NewLabel(),
    errormsg_.get()
    );

 frags->PushBack(new frame::ProcFrag(root->exp_->UnNx(), main_level_->frame_));
}

} // namespace tr

tree::Exp *StaricLink(tr::Level *cur, tr::Level *target) {
  tree::Exp *rt = new tree::TempExp(reg_manager->FramePointer());
  while (cur != target)
  {
    if (!cur || !cur->parent_) return nullptr;
    // first param -> static link
    auto access = cur->frame_->formals_.front();
    rt = access->toExp(rt);
    cur = cur->parent_;
  }
  return rt;
  
}

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return this->root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto entry = venv->Look(this->sym_);
  auto var_entry = (env::VarEntry *)entry;

  auto exp = new tr::ExExp(var_entry->access_->access_->toExp(
    StaricLink(level, var_entry->access_->level_)
  ));
  return new tr::ExpAndTy(exp, var_entry->ty_->ActualTy());

}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var_trans = this->var_->Translate(venv, tenv, level, label, errormsg);
  auto record_ty = (type::RecordTy *)(var_trans->ty_->ActualTy());

  auto lists = record_ty->fields_->GetList();
  int cnt = 0;
  for (auto l:lists) {
    if (l->name_->Name() == this->sym_->Name()) {
      auto exp = new tr::ExExp(
        new tree::MemExp(
          new tree::BinopExp(
            tree::BinOp::PLUS_OP, 
            var_trans->exp_->UnEx(),
            new tree::ConstExp(cnt * reg_manager->WordSize())
          )
        )
      );
      return new tr::ExpAndTy(exp, var_trans->ty_->ActualTy());
    }
    cnt ++;
  }
  return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var_trans = this->var_->Translate(venv, tenv, level, label, errormsg);
  auto sub_trans = this->subscript_->Translate(venv, tenv, level, label, errormsg);

  type::Ty *ty = var_trans->ty_->ActualTy();
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::MemExp(
       new tree::BinopExp(
        tree::BinOp::PLUS_OP, 
        var_trans->exp_->UnEx(),
        new tree::BinopExp(
          tree::BinOp::MUL_OP,
          sub_trans->exp_->UnEx(),
          new tree::ConstExp(reg_manager->WordSize())
        )
        )
      )
    ),
    ((type::ArrayTy *)ty)->ty_->ActualTy()
  );

}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return this->var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(this->val_)), type::IntTy::Instance());

}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto str_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(str_label, this->str_));
  auto name_label = new tr::ExExp(new tree::NameExp(str_label));

  return new tr::ExpAndTy(name_label, type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto entry = venv->Look(this->func_);
  if (!entry || entry->type() != 1) {
    errormsg->Error(pos_, "undefined func");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }

  auto func_entry = (env::FunEntry *)entry;
  auto arg_list = this->args_->GetList();
  // std::list<tr::Exp *> arg_list_exp;
  auto arg_list_exp = new tree::ExpList();
  for (auto arg: arg_list) {
    auto tmp = arg->Translate(venv, tenv, level, label, errormsg);
    arg_list_exp->Append(tmp->exp_->UnEx());
  }

  tree::Exp * static_link = StaricLink(level, func_entry->level_->parent_);

  tr::Exp *exp = nullptr;

  auto max = [](int a, int b)->int{
    if(a>b) return a;
    return b;
  };
  if (static_link) {
    arg_list_exp->Insert(static_link);
    level->frame_->arg_num = max(level->frame_->arg_num, arg_list.size()+1);
    exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(this->func_), arg_list_exp));
  }else {
    exp = new tr::ExExp(frame::ExternalCall(temp::LabelFactory::LabelString(this->func_), arg_list_exp));
    level->frame_->arg_num = max(level->frame_->arg_num, arg_list.size());
  }

  return new tr::ExpAndTy(
    exp,
    func_entry->result_
  );
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tree::CjumpStm *stm = nullptr;
  tree::BinopExp *exp = nullptr;
  tr::ExpAndTy *lft = this->left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *rht = this->right_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *lft_exp = lft->exp_;
  tr::Exp *rht_exp = rht->exp_;
  int pos = this->pos_;
  switch (this->oper_)
  {
  case absyn::PLUS_OP:
    exp = new tree::BinopExp(tree::BinOp::PLUS_OP, lft_exp->UnEx(), rht_exp->UnEx());
    break;
  case absyn::MINUS_OP:
    exp = new tree::BinopExp(tree::BinOp::MINUS_OP, lft_exp->UnEx(), rht_exp->UnEx());
    break;
  case absyn::TIMES_OP:
    exp = new tree::BinopExp(tree::BinOp::MUL_OP, lft_exp->UnEx(), rht_exp->UnEx());
    break;
  case absyn::DIVIDE_OP:
    exp = new tree::BinopExp(tree::BinOp::DIV_OP, lft_exp->UnEx(), rht_exp->UnEx());
    break;

  case absyn::EQ_OP:
    stm = new tree::CjumpStm(tree::RelOp::EQ_OP, lft_exp->UnEx(), rht_exp->UnEx(), nullptr, nullptr);
    break;
  case absyn::NEQ_OP:
    stm = new tree::CjumpStm(tree::RelOp::NE_OP, lft_exp->UnEx(), rht_exp->UnEx(), nullptr, nullptr);
    break;
  case absyn::LT_OP:
    stm = new tree::CjumpStm(tree::RelOp::LT_OP, lft_exp->UnEx(), rht_exp->UnEx(), nullptr, nullptr);
    break;
  case absyn::LE_OP:
    stm = new tree::CjumpStm(tree::RelOp::LE_OP, lft_exp->UnEx(), rht_exp->UnEx(), nullptr, nullptr);
    break;

  case absyn::GT_OP:
    stm = new tree::CjumpStm(tree::RelOp::GT_OP, lft_exp->UnEx(), rht_exp->UnEx(), nullptr, nullptr);
    break;
  case absyn::GE_OP:
    stm = new tree::CjumpStm(tree::RelOp::GE_OP, lft_exp->UnEx(), rht_exp->UnEx(), nullptr, nullptr);
    break;
  case absyn::AND_OP:{
    absyn::IfExp *e = new absyn::IfExp(this->pos_, this->left_, this->right_, new absyn::IntExp(pos, 0));
    return e->Translate(venv, tenv, level, label, errormsg);
    break;
  }
  case absyn::OR_OP:{
    absyn::IfExp *e = new absyn::IfExp(this->pos_, this->left_,new absyn::IntExp(pos, 1), this->right_);
    return e->Translate(venv, tenv, level, label, errormsg);
  }
  default:
    break;
  }

  if (exp) {
    return new tr::ExpAndTy(new tr::ExExp(exp), type::IntTy::Instance());
  }else if (stm) {
    std::list<temp::Label **> true_list;
    std::list<temp::Label **> false_list;
    true_list.push_back(&(stm->true_label_));
    false_list.push_back(&(stm->false_label_));
    tr::Exp * tmp = new tr::CxExp(tr::PatchList(true_list), 
      tr::PatchList(false_list),
      stm);
    return new tr::ExpAndTy(tmp, type::IntTy::Instance());
    
  }
  assert(0);
  return nullptr;
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto exp_list = new tree::ExpList();
  auto lists = this->fields_->GetList();
  for (auto l:lists) {
    auto exp_ = l->exp_->Translate(venv, tenv, level, label, errormsg);
    exp_list->Append(exp_->exp_->UnEx());
  }

  auto reg = temp::TempFactory::NewTemp();
  auto args = new tree::ExpList();
  int s = lists.size();
  args->Append(new tree::ConstExp(s * 8));
  tree::Stm *stm = new tree::MoveStm(new tree::TempExp(reg), frame::ExternalCall("alloc_record", args));

  int cnt = 0;
  auto exps = exp_list->GetList();
  for (auto e:exps) {
    auto loc =
        new tree::MemExp(
          new tree::BinopExp(
            tree::BinOp::PLUS_OP, 
            new tree::TempExp(reg),
            new tree::ConstExp(cnt * 8)
          )
        );
    stm = new tree::SeqStm(stm, new tree::MoveStm(loc, e));
    cnt++;
  }
  return new tr::ExpAndTy(
    new tr::ExExp(new tree::EseqExp(stm, new tree::TempExp(reg))),
    tenv->Look(this->typ_)->ActualTy()
  );
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty * ty = type::VoidTy::Instance();
  auto lists = this->seq_->GetList();
  auto cur = new tr::ExExp(new tree::ConstExp(0));
  for (auto l:lists) {
    auto e_t = l->Translate(venv, tenv, level, label, errormsg);
    ty = e_t->ty_->ActualTy();
    auto cur_exp = e_t->exp_;
    if (cur_exp)
      cur = new tr::ExExp(new tree::EseqExp(cur->UnNx(), cur_exp->UnEx()));
    else 
      cur = new tr::ExExp(new tree::EseqExp(cur->UnNx(), new tree::ConstExp(0)));
  }

  return new tr::ExpAndTy(cur, ty);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto v = this->var_->Translate(venv, tenv, level, label, errormsg);
  auto e = this->exp_->Translate(venv, tenv, level, label, errormsg);
  return new tr::ExpAndTy(
    new tr::NxExp(new tree::MoveStm(v->exp_->UnEx(), e->exp_->UnEx())),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto tst = this->test_->Translate(venv, tenv, level, label, errormsg);
  auto thn = this->then_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *ese(nullptr);
  if (this->elsee_)
    ese = this->elsee_->Translate(venv, tenv, level, label, errormsg);
  
  auto rt = temp::TempFactory::NewTemp();
  auto true_label = temp::LabelFactory::NewLabel();
  auto false_label = temp::LabelFactory::NewLabel();
  auto done_label = temp::LabelFactory::NewLabel();

  auto done_vec = new std::vector<temp::Label*>;
  done_vec->push_back(done_label);
  tr::Cx c = tst->exp_->UnCx(errormsg);

  c.trues_.DoPatch(true_label);
  c.falses_.DoPatch(false_label);

  tr::Exp *else_exp;
  if (this->elsee_) {
    else_exp = new tr::ExExp(
      new tree::EseqExp(
        c.stm_,
        new tree::EseqExp(
          new tree::LabelStm(true_label),
          new tree::EseqExp(
            new tree::MoveStm(new tree::TempExp(rt), thn->exp_->UnEx()),
            new tree::EseqExp(
              new tree::JumpStm(new tree::NameExp(done_label), done_vec),
              new tree::EseqExp(
                new tree::LabelStm(false_label),
                new tree::EseqExp(
                  new tree::MoveStm(new tree::TempExp(rt), ese->exp_->UnEx()),
                  new tree::EseqExp(
                    new tree::JumpStm(new tree::NameExp(done_label), done_vec),
                    new tree::EseqExp(
                      new tree::LabelStm(done_label),
                      new tree::TempExp(rt)
                    )
                  )
                )
              )
            )
          )
        )
      )
    );
  } else {
    else_exp = new tr::NxExp(
      new tree::SeqStm(
        c.stm_,
        new tree::SeqStm(
          new tree::LabelStm(true_label),
          new tree::SeqStm(
            thn->exp_->UnNx(),
            new tree::LabelStm(false_label)
          )
        )
      )
    );
  }
  return new tr::ExpAndTy(else_exp, thn->ty_->ActualTy());
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto test_label = temp::LabelFactory::NewLabel();
  auto done_label = temp::LabelFactory::NewLabel();
  auto body_label = temp::LabelFactory::NewLabel();
  auto tst = this->test_->Translate(venv, tenv, level, label, errormsg);
  auto bdy = this->body_->Translate(venv, tenv, level, done_label, errormsg);
  tr::Cx c = tst->exp_->UnCx(errormsg);
  c.trues_.DoPatch(body_label);
  c.falses_.DoPatch(done_label);

  auto test_vec = new std::vector<temp::Label *>;
  test_vec->push_back(test_label);
  return new tr::ExpAndTy(
    new tr::NxExp(new tree::SeqStm(
      new tree::LabelStm(test_label),
      new tree::SeqStm(
          c.stm_, 
          new tree::SeqStm(
           new tree::LabelStm(body_label),
           new tree::SeqStm(
               bdy->exp_->UnNx(),
               new tree::SeqStm(
                   new tree::JumpStm(
                     new tree::NameExp(test_label),
                     test_vec),
                   new tree::LabelStm(done_label))))))),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto body_label = temp::LabelFactory::NewLabel();
  auto test_label = temp::LabelFactory::NewLabel();
  auto done_label = temp::LabelFactory::NewLabel();
  auto lo_trans = this->lo_->Translate(venv, tenv, level, label, errormsg);
  auto hi_trans = this->hi_->Translate(venv, tenv, level, label, errormsg);
  tr::Access *acc_i = tr::Access::AllocLocal(level, this->escape_);
  venv->BeginScope();
  venv->Enter(this->var_, new env::VarEntry(acc_i, lo_trans->ty_, true));
  auto bdy_trans = this->body_->Translate(venv, tenv, level, done_label, errormsg);
  venv->EndScope();


  tree::Exp *sp = acc_i->access_->toExp(new tree::TempExp(reg_manager->FramePointer()));
  return new tr::ExpAndTy(
    new tr::NxExp(new tree::SeqStm(
      new tree::MoveStm(sp, lo_trans->exp_->UnEx()),
      new tree::SeqStm(
          new tree::LabelStm(test_label),
          new tree::SeqStm(
              new tree::CjumpStm(tree::RelOp::LE_OP, sp, hi_trans->exp_->UnEx(),
                                 body_label, done_label),
              new tree::SeqStm(
                  new tree::LabelStm(body_label),
                  new tree::SeqStm(
                      bdy_trans->exp_->UnNx(),
                      new tree::SeqStm(
                          new tree::MoveStm(
                              sp, 
                              new tree::BinopExp(
                                tree::BinOp::PLUS_OP, 
                                sp,
                                new tree::ConstExp(1))),
                          new tree::SeqStm(
                              new tree::JumpStm(
                                new tree::NameExp(test_label),
                                new std::vector<temp::Label *>(1, test_label)),
                              new tree::LabelStm(done_label))))))))),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::NxExp(
      new tree::JumpStm(
        new tree::NameExp(label),
        new std::vector<temp::Label *>(1, label)
      )
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto dec_list = this->decs_->GetList();
  tr::Exp *rt = nullptr;
  std::list<tr::Exp *> dec_trans;
  for (auto dec:dec_list) {
    auto tmp = dec->Translate(venv, tenv, level, label,errormsg);
    dec_trans.push_back(tmp);
    
  }
  rt = dec_trans.front();
  for (auto dec_tran:dec_trans) {
    if (dec_tran) rt = new tr::ExExp(new tree::EseqExp(rt->UnNx(), dec_tran->UnEx()));
    else rt = new tr::ExExp(new tree::EseqExp(rt->UnNx(), new tree::ConstExp(0)));
  }

  tr::ExpAndTy *bdy_trans = this->body_->Translate(venv, tenv, level, label, errormsg);
  
  if (!rt) return bdy_trans;
  else return new tr::ExpAndTy(
    new tr::ExExp(new tree::EseqExp(rt->UnNx(), bdy_trans->exp_->UnEx())),
    bdy_trans->ty_
  );
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto size_init = this->size_->Translate(venv, tenv, level, label, errormsg);
  auto init_init = this->init_->Translate(venv, tenv, level, label, errormsg);

  tree::Exp* array_init = frame::ExternalCall(
    "init_array",
    new tree::ExpList({size_init->exp_->UnEx(), init_init->exp_->UnEx()})
  );
  return new tr::ExpAndTy(new tr::ExExp(array_init), tenv->Look(this->typ_)->ActualTy());
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto func_list = this->functions_->GetList();
  for (auto func: func_list) {
    auto func_params = func->params_->GetList();
    std::list<bool> escape_l;
    for (auto field: func_params) {
      escape_l.push_back(field->escape_);
    }

    auto func_label = temp::LabelFactory::NamedLabel(func->name_->Name());
    auto func_level = tr::NewLevel(level, func_label, escape_l);
    auto formal_list = func->params_->MakeFormalTyList(tenv, errormsg);
    if (func->result_) {
      type::Ty *result_ty = tenv->Look(func->result_);
      venv->Enter(func->name_, new env::FunEntry(func_level, func_label, formal_list, result_ty));
    }else {
      venv->Enter(func->name_, new env::FunEntry(func_level, func_label, formal_list, type::VoidTy::Instance()));

    }
  }
  // body
  for (auto func: func_list) {
    auto func_entry = (env::FunEntry*)(venv->Look(func->name_));
    auto field_list = func->params_->GetList();
    auto ty_list = func->params_->MakeFormalTyList(tenv, errormsg)->GetList();
    auto access_list = func_entry->level_->frame_->formals_;

    auto it_acc = std::next(access_list.begin());
    
    auto it_ty = ty_list.begin();
    auto it_field = field_list.begin();

    venv->BeginScope();
    while (it_field != field_list.end()) {

      venv->Enter((*it_field)->name_, new env::VarEntry(new tr::Access(func_entry->level_, *it_acc), *it_ty));
      it_acc ++;
      it_ty++;
      it_field++;
    }
    tr::ExpAndTy *body_trans = func->body_->Translate(venv, tenv, func_entry->level_, label, errormsg);
     // seg fault
    venv->EndScope();
    tree::Stm* stm;
    // if (body_trans)
      stm = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), body_trans->exp_->UnEx());
    // else stm = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), new tree::ConstExp(0));
    stm = frame::ProcEntryExit1(func_entry->level_->frame_, stm);
    frags->PushBack(new frame::ProcFrag(stm, func_entry->level_->frame_));
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* init_trans = init_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *init_ty = init_trans->ty_->ActualTy();
  if (this->typ_) init_ty = tenv->Look(this->typ_)->ActualTy();

  auto *access = tr::Access::AllocLocal(level, this->escape_);
  venv->Enter(this->var_, new env::VarEntry(access, init_ty));

  tree::Exp *fp = StaricLink(level, access->level_);
  tr::Exp * var_trans = new tr::ExExp(access->access_->toExp(fp));


  return new tr::NxExp(new tree::MoveStm(
    var_trans->UnEx(), init_trans->exp_->UnEx()
  ));


}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto dec_list = this->types_->GetList();
  for (auto dec: dec_list) {
    for (auto dec1: dec_list) 
      if (dec != dec1 && dec->name_->Name() == dec1->name_->Name())
        errormsg->Error(pos_, "two type have same name");
    tenv->Enter(dec->name_, new type::NameTy(dec->name_, nullptr));
  }

  for (auto dec: dec_list) {
    type::Ty *ty = dec->ty_->Translate(tenv, errormsg);
    auto tar = (type::NameTy*)(tenv->Look(dec->name_));
    tar->ty_ = ty;
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::NameTy(this->name_, tenv->Look(this->name_));
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::RecordTy(this->record_->MakeFieldList(tenv, errormsg));

}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto ty = tenv->Look(this->array_);
  if (ty) return new type::ArrayTy(ty);
  return type::IntTy::Instance();
}

} // namespace absyn
