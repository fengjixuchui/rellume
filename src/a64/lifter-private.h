/**
 * This file is part of Rellume.
 *
 * (c) 2016-2019, Alexis Engelke <alexis.engelke@googlemail.com>
 * (c) 2020, Dominik Okwieka <dominik.okwieka@t-online.de>
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

#ifndef RELLUME_A64_LIFTER_H
#define RELLUME_A64_LIFTER_H

#include "basicblock.h"
#include "config.h"
#include "function-info.h"
#include "instr.h"
#include "regfile.h"
#include "lifter-base.h"

namespace rellume::aarch64 {

class Lifter : public LifterBase {
public:
    Lifter(FunctionInfo& fi, const LLConfig& cfg, ArchBasicBlock& ab) :
            LifterBase(fi, cfg, ab) {}

    bool Lift(const Instr&);

private:
    llvm::Value* GetGp(farmdec::Reg, bool w32, bool ptr = false);
    void SetGp(farmdec::Reg, bool w32, llvm::Value* val);

    void FlagCalcAdd(llvm::Value* res, llvm::Value* lhs, llvm::Value* rhs);
    void FlagCalcSub(llvm::Value* res, llvm::Value* lhs, llvm::Value* rhs);
    void FlagCalcLogic(llvm::Value* res);

    llvm::Value* Shift(llvm::Value* v, farmdec::Shift sh, uint32_t amount);
    llvm::Value* Extend(llvm::Value* v, bool w32, farmdec::ExtendType ext, uint32_t lsl);
    llvm::IntegerType* TypeOf(farmdec::Size sz);
    llvm::Type* TypeOf(farmdec::FPSize fsz);
    llvm::AtomicOrdering Ordering(farmdec::MemOrdering);
    llvm::Value* IsTrue(farmdec::Cond);

    llvm::Value* Extract(llvm::Value* v, bool w32, unsigned lsb, unsigned width);
    llvm::Value* MoveField(llvm::Value* v, bool w32, unsigned lsb, unsigned width);

    llvm::Value* PCRel(uint64_t off);

    // Dispatches to the correct addressing mode variant.
    llvm::Value* Addr(llvm::Type* elemty, farmdec::Inst inst);

    // The various addressing modes. See farmdec::AddrMode.
    llvm::Value* Addr(llvm::Type* elemty, farmdec::Reg base);
    llvm::Value* Addr(llvm::Type* elemty, farmdec::Reg base, uint64_t off);
    llvm::Value* Addr(llvm::Type* elemty, farmdec::Reg base, farmdec::Reg off, uint32_t lsl);
    llvm::Value* Addr(llvm::Type* elemty, farmdec::Reg base, farmdec::Reg off, farmdec::ExtendType ext, uint32_t lsl);

    void LiftCCmp(llvm::Value* lhs, llvm::Value* rhs, farmdec::Cond cond, uint8_t nzcv, bool ccmn);
};

} // namespace

#endif