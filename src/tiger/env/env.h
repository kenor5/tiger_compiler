#ifndef TIGER_ENV_ENV_H_
#define TIGER_ENV_ENV_H_

#include "tiger/semant/types.h"
#include "tiger/symbol/symbol.h"

namespace env {
class EnvEntry {
public:
  bool readonly_;

  explicit EnvEntry(bool readonly = true) : readonly_(readonly) {}
  virtual ~EnvEntry() = default;
  virtual bool type() = 0;
};

class VarEntry : public EnvEntry {
public:
  type::Ty *ty_;

  // For lab4(semantic analysis) only
  explicit VarEntry(type::Ty *ty, bool readonly = false)
      : EnvEntry(readonly), ty_(ty){};
  bool type() {return 0;};
};

class FunEntry : public EnvEntry {
public:
  type::TyList *formals_;
  type::Ty *result_;

  // For lab4(semantic analysis) only
  FunEntry(type::TyList *formals, type::Ty *result)
      : formals_(formals), result_(result){}
  bool type() {return 1;};
};

using VEnv = sym::Table<env::EnvEntry>;
using TEnv = sym::Table<type::Ty>;
using VEnvPtr = sym::Table<env::EnvEntry> *;
using TEnvPtr = sym::Table<type::Ty> *;
} // namespace env

#endif // TIGER_ENV_ENV_H_
