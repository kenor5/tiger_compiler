//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
  public:
        X64RegManager();
   /**
         * Get general-purpose registers except RSI
         * NOTE: returned temp list should be in the order of calling convention
         * @return general-purpose registers
         */
        [[nodiscard]] temp::TempList *Registers() override;

        /**
         * Get registers which can be used to hold arguments
         * NOTE: returned temp list must be in the order of calling convention
         * @return argument registers
         */
        [[nodiscard]] temp::TempList *ArgRegs() override;

        /**
         * Get caller-saved registers
         * NOTE: returned registers must be in the order of calling convention
         * @return caller-saved registers
         */
        [[nodiscard]] temp::TempList *CallerSaves() override;

        /**
         * Get callee-saved registers
         * NOTE: returned registers must be in the order of calling convention
         * @return callee-saved registers
         */
        [[nodiscard]] temp::TempList *CalleeSaves() override;

        /**
         * Get return-sink registers
         * @return return-sink registers
         */
        [[nodiscard]] temp::TempList *ReturnSink() override;
        [[nodiscard]] temp::TempList *OperateRegs() override;

        [[nodiscard]] temp::Temp *Dx()override;
        /**
         * Get word size
         */
        [[nodiscard]] int WordSize() override;

        [[nodiscard]] temp::Temp *FramePointer() override;

        [[nodiscard]] temp::Temp *StackPointer() override;

        [[nodiscard]] temp::Temp *ReturnValue() override;

private:
  int reg_num;
        
};


class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */

  public:
    X64Frame (temp::Label *name) :Frame(name){}

    void setFormals(const std::list<bool> &esc) override;

    frame::Access *AllocLocal(bool esc) override;
};

 tree::Exp *ExternalCall(const std::string &name, tree::ExpList *args);
 Frame *NewFrame(temp::Label *label, const std::list<bool> &formals);
  tree::Stm *ProcEntryExit1(Frame *f, tree::Stm *stm);
  assem::InstrList *ProcEntryExit2(assem::InstrList *body);
  assem::Proc *ProcEntryExit3(frame::Frame *f, assem::InstrList *body);

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
