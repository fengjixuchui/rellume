/**
 * This file is part of Rellume.
 *
 * (c) 2016-2019, Alexis Engelke <alexis.engelke@googlemail.com>
 *
 * Rellume is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License (LGPL)
 * as published by the Free Software Foundation, either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Rellume is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Rellume.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 **/

#ifndef RELLUME_LIFTER_PRIVATE_H
#define RELLUME_LIFTER_PRIVATE_H

#include "basicblock.h"
#include "config.h"
#include "facet.h"
#include "function-info.h"
#include "instr.h"
#include "regfile.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Operator.h>
#include <vector>


namespace rellume {

enum Alignment {
    /// Implicit alignment -- MAX for SSE operand, 1 otherwise
    ALIGN_IMP = -1,
    /// Maximum alignment, alignment is set to the size of the value
    ALIGN_MAX = 0,
    /// No alignment (1 byte)
    ALIGN_NONE = 1,
    ALIGN_MAXIMUM = ALIGN_MAX,
    ALIGN_1 = ALIGN_NONE,
};

enum class Condition {
    O = 0, NO = 1, C = 2, NC = 3, Z = 4, NZ = 5, BE = 6, A = 7,
    S = 8, NS = 9, P = 10, NP = 11, L = 12, GE = 13, LE = 14, G = 15,
};

/**
 * \brief The LLVM state of the back-end.
 **/
class LifterBase {
protected:
    LifterBase(FunctionInfo& fi, const LLConfig& cfg, ArchBasicBlock& ab)
            : fi(fi), cfg(cfg), ablock(ab),
              regfile(ablock.GetInsertBlock()->GetRegFile()),
              irb(regfile->GetInsertBlock()) {
        // Set fast-math flags. Newer LLVM supports FastMathFlags::getFast().
        if (cfg.enableFastMath) {
            llvm::FastMathFlags fmf;
            fmf.setFast();
            irb.setFastMathFlags(fmf);
        }
    }

public:
    LifterBase(LifterBase&& rhs);
    LifterBase& operator=(LifterBase&& rhs);

    LifterBase(const LifterBase&) = delete;
    LifterBase& operator=(const LifterBase&) = delete;

protected:
    FunctionInfo& fi;
    const LLConfig& cfg;
private:
    ArchBasicBlock& ablock;

protected:
    /// Current register file
    RegFile* regfile;

    llvm::IRBuilder<> irb;


    llvm::Module* GetModule() {
        return irb.GetInsertBlock()->getModule();
    }

    X86Reg MapReg(const LLReg reg);

    llvm::Value* GetReg(X86Reg reg, Facet facet) {
        return regfile->GetReg(reg, facet);
    }
    void SetReg(X86Reg reg, Facet facet, llvm::Value* value) {
        fi.ModifyReg(reg);
        regfile->SetReg(reg, facet, value, true); // clear all other facets
    }
    void SetRegFacet(X86Reg reg, Facet facet, llvm::Value* value) {
        // TODO: be more accurate about flags
        // Currently, when a single flag is modified, all other flags are marked
        // as modified as well.
        fi.ModifyReg(reg);
        regfile->SetReg(reg, facet, value, false);
    }
    llvm::Value* GetFlag(Facet facet) {
        return GetReg(X86Reg::EFLAGS, facet);
    }
    void SetFlag(Facet facet, llvm::Value* value) {
        SetRegFacet(X86Reg::EFLAGS, facet, value);
    }
    void SetFlagUndef(std::initializer_list<Facet> facets) {
        llvm::Value* undef = llvm::UndefValue::get(irb.getInt1Ty());
        for (const auto facet : facets)
            SetRegFacet(X86Reg::EFLAGS, facet, undef);
    }

    void SetInsertBlock(BasicBlock* block) {
        ablock.SetInsertBlock(block);
        regfile = block->GetRegFile();
        irb.SetInsertPoint(regfile->GetInsertBlock());
    }

    // Operand handling implemented in lloperand.cc
private:
    llvm::Value* OpAddrConst(uint64_t addr, llvm::PointerType* ptr_ty);
protected:
    llvm::Value* OpAddr(const Instr::Op op, llvm::Type* element_type, unsigned seg);
    llvm::Value* OpLoad(const Instr::Op op, Facet facet, Alignment alignment = ALIGN_NONE, unsigned force_seg = 7);
    void OpStoreGp(X86Reg reg, llvm::Value* v) {
        OpStoreGp(reg, Facet::In(v->getType()->getIntegerBitWidth()), v);
    }
    void OpStoreGp(X86Reg reg, Facet facet, llvm::Value* value);
    void OpStoreGp(const Instr::Op op, llvm::Value* value, Alignment alignment = ALIGN_NONE);
    void OpStoreVec(const Instr::Op op, llvm::Value* value, bool avx = false, Alignment alignment = ALIGN_IMP);
    void StackPush(llvm::Value* value);
    llvm::Value* StackPop(const X86Reg sp_src_reg = X86Reg::GP(LL_RI_SP));

    // llflags.cc
    void FlagCalcZ(llvm::Value* value) {
        auto zero = llvm::Constant::getNullValue(value->getType());
        SetFlag(Facet::ZF, irb.CreateICmpEQ(value, zero));
    }
    void FlagCalcS(llvm::Value* value) {
        auto zero = llvm::Constant::getNullValue(value->getType());
        SetFlag(Facet::SF, irb.CreateICmpSLT(value, zero));
    }
    void FlagCalcP(llvm::Value* value);
    void FlagCalcA(llvm::Value* res, llvm::Value* lhs, llvm::Value* rhs);
    void FlagCalcCAdd(llvm::Value* res, llvm::Value* lhs, llvm::Value* rhs) {
        SetFlag(Facet::CF, irb.CreateICmpULT(res, lhs));
    }
    void FlagCalcCSub(llvm::Value* res, llvm::Value* lhs, llvm::Value* rhs) {
        SetFlag(Facet::CF, irb.CreateICmpULT(lhs, rhs));
    }
    void FlagCalcOAdd(llvm::Value* res, llvm::Value* lhs, llvm::Value* rhs);
    void FlagCalcOSub(llvm::Value* res, llvm::Value* lhs, llvm::Value* rhs);
    void FlagCalcAdd(llvm::Value* res, llvm::Value* lhs, llvm::Value* rhs) {
        FlagCalcZ(res);
        FlagCalcS(res);
        FlagCalcP(res);
        FlagCalcA(res, lhs, rhs);
        FlagCalcCAdd(res, lhs, rhs);
        FlagCalcOAdd(res, lhs, rhs);
    }
    void FlagCalcSub(llvm::Value* res, llvm::Value* lhs, llvm::Value* rhs) {
        SetFlag(Facet::ZF, irb.CreateICmpEQ(lhs, rhs));
        FlagCalcS(res);
        FlagCalcP(res);
        FlagCalcA(res, lhs, rhs);
        FlagCalcCSub(res, lhs, rhs);
        FlagCalcOSub(res, lhs, rhs);
    }

    llvm::Value* FlagCond(Condition cond);
    llvm::Value* FlagAsReg(unsigned size);
    void FlagFromReg(llvm::Value* val);

    struct RepInfo {
        enum RepMode { NO_REP, REP, REPZ, REPNZ };
        RepMode mode;
        BasicBlock* loop_block;
        BasicBlock* cont_block;

        llvm::Value* di;
        llvm::Value* si;
    };
    RepInfo RepBegin(const Instr& inst);
    void RepEnd(RepInfo info);

    // Helper function for older LLVM versions
    llvm::Value* CreateUnaryIntrinsic(llvm::Intrinsic::ID id, llvm::Value* v) {
        // TODO: remove this helper function
        return irb.CreateUnaryIntrinsic(id, v);
    }
};

class Lifter : public LifterBase {
public:
    Lifter(FunctionInfo& fi, const LLConfig& cfg, ArchBasicBlock& ab) :
            LifterBase(fi, cfg, ab) {}

    // llinstruction-gp.cc
    bool Lift(const Instr&);

private:
    void LiftOverride(const Instr&, llvm::Function* override);

    void LiftMovgp(const Instr&, llvm::Instruction::CastOps cast);
    void LiftArith(const Instr&, bool sub);
    void LiftCmpxchg(const Instr&);
    void LiftXchg(const Instr&);
    void LiftAndOrXor(const Instr& inst, llvm::Instruction::BinaryOps op,
                      bool writeback = true);
    void LiftNot(const Instr&);
    void LiftNeg(const Instr&);
    void LiftIncDec(const Instr&);
    void LiftShift(const Instr&, llvm::Instruction::BinaryOps);
    void LiftRotate(const Instr&);
    void LiftShiftdouble(const Instr&);
    void LiftMul(const Instr&);
    void LiftDiv(const Instr&);
    void LiftLea(const Instr&);
    void LiftXlat(const Instr&);
    void LiftCmovcc(const Instr& inst, Condition cond);
    void LiftSetcc(const Instr& inst, Condition cond);
    void LiftCext(const Instr& inst);
    void LiftCsep(const Instr& inst);
    void LiftBitscan(const Instr& inst, bool trailing);
    void LiftBittest(const Instr& inst);
    void LiftMovbe(const Instr& inst);
    void LiftBswap(const Instr& inst);

    void LiftLahf(const Instr& inst) {
        OpStoreGp(X86Reg::RAX, Facet::I8H, FlagAsReg(8));
    }
    void LiftSahf(const Instr& inst) {
        FlagFromReg(GetReg(X86Reg::RAX, Facet::I8H));
    }
    void LiftPush(const Instr& inst) {
        StackPush(OpLoad(inst.op(0), Facet::I));
    }
    void LiftPushf(const Instr& inst) {
        StackPush(FlagAsReg(inst.opsz() * 8));
    }
    void LiftPop(const Instr& inst) {
        OpStoreGp(inst.op(0), StackPop());
    }
    void LiftPopf(const Instr& inst) {
        FlagFromReg(StackPop());
    }
    void LiftLeave(const Instr& inst) {
        llvm::Value* val = StackPop(X86Reg::RBP);
        OpStoreGp(X86Reg::RBP, val);
    }

    void LiftJmp(const Instr& inst);
    void LiftJcc(const Instr& inst, Condition cond);
    void LiftJcxz(const Instr& inst);
    void LiftLoop(const Instr& inst);
    void LiftCall(const Instr& inst);
    void LiftRet(const Instr& inst);
    void LiftUnreachable(const Instr& inst);

    void LiftClc(const Instr& inst) { SetFlag(Facet::CF, irb.getFalse()); }
    void LiftStc(const Instr& inst) { SetFlag(Facet::CF, irb.getTrue()); }
    void LiftCmc(const Instr& inst) {
        SetFlag(Facet::CF, irb.CreateNot(GetFlag(Facet::CF)));
    }

    void LiftCld(const Instr& inst) { SetFlag(Facet::DF, irb.getFalse()); }
    void LiftStd(const Instr& inst) { SetFlag(Facet::DF, irb.getTrue()); }
    void LiftLods(const Instr& inst);
    void LiftStos(const Instr& inst);
    void LiftMovs(const Instr& inst);
    void LiftScas(const Instr& inst);
    void LiftCmps(const Instr& inst);

    // llinstruction-sse.cc
    void LiftFence(const Instr&);
    void LiftPrefetch(const Instr&, unsigned rw, unsigned locality);
    void LiftFxsave(const Instr&);
    void LiftFxrstor(const Instr&);
    void LiftFstcw(const Instr&);
    void LiftFstsw(const Instr&);
    void LiftStmxcsr(const Instr&);
    void LiftSseMovq(const Instr&, Facet type);
    void LiftSseBinOp(const Instr&, llvm::Instruction::BinaryOps op,
                      Facet type);
    void LiftSseMovScalar(const Instr&, Facet);
    void LiftSseMovdq(const Instr&, Facet, Alignment);
    void LiftSseMovntStore(const Instr&, Facet);
    void LiftSseMovlp(const Instr&);
    void LiftSseMovhps(const Instr&);
    void LiftSseMovhpd(const Instr&);
    void LiftSseAndn(const Instr&, Facet op_type);
    void LiftSseComis(const Instr&, Facet);
    void LiftSseCmp(const Instr&, Facet op_type);
    void LiftSseMinmax(const Instr&, llvm::CmpInst::Predicate, Facet);
    void LiftSseSqrt(const Instr&, Facet op_type);
    void LiftSseCvt(const Instr&, Facet src_type, Facet dst_type);
    void LiftSseUnpck(const Instr&, Facet type);
    void LiftSseShufpd(const Instr&);
    void LiftSseShufps(const Instr&);
    void LiftSsePshufd(const Instr&);
    void LiftSsePshufw(const Instr&, unsigned off);
    void LiftSseInsertps(const Instr&);
    void LiftSsePinsr(const Instr&, Facet, Facet, unsigned);
    void LiftSsePextr(const Instr&, Facet, unsigned);
    void LiftSsePshiftElement(const Instr&, llvm::Instruction::BinaryOps op, Facet op_type);
    void LiftSsePshiftBytes(const Instr&);
    void LiftSsePavg(const Instr&, Facet);
    void LiftSsePmulhw(const Instr&, llvm::Instruction::CastOps cast);
    void LiftSsePaddsubSaturate(const Instr& inst,
                                llvm::Instruction::BinaryOps calc_op, bool sign,
                                Facet op_ty);
    void LiftSsePack(const Instr&, Facet, bool sign);
    void LiftSsePcmp(const Instr&, llvm::CmpInst::Predicate, Facet);
    void LiftSsePminmax(const Instr&, llvm::CmpInst::Predicate, Facet);
    void LiftSseMovmsk(const Instr&, Facet op_type);
};

} // namespace

#endif