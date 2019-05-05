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

#ifndef LL_BASIC_BLOCK_H
#define LL_BASIC_BLOCK_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <llvm-c/Core.h>

#include <llbasicblock.h>

#include <llcommon-internal.h>
#include <llregfile-internal.h>


LLBasicBlock* ll_basic_block_new(LLVMBasicBlockRef, LLState* state);
void ll_basic_block_dispose(LLBasicBlock*);
void ll_basic_block_add_predecessor(LLBasicBlock*, LLBasicBlock*);
void ll_basic_block_add_phis(LLBasicBlock*);
void ll_basic_block_terminate(LLBasicBlock*);
void ll_basic_block_fill_phis(LLBasicBlock*);

#define ll_get_register(reg,facet,state) ll_basic_block_get_register(state->currentBB,facet,reg,state)
#define ll_clear_register(reg,state) ll_basic_block_clear_register(state->currentBB,reg,state)
#define ll_set_register(reg,facet,value,clear,state) ll_basic_block_set_register(state->currentBB,facet,reg,value,clear,state)
#define ll_get_flag(reg,state) ll_basic_block_get_flag(state->currentBB,reg)
#define ll_set_flag(reg,value,state) ll_basic_block_set_flag(state->currentBB,reg,value)
#define ll_get_flag_cache(state) ll_basic_block_get_flag_cache(state->currentBB)

LLVMValueRef ll_basic_block_get_register(LLBasicBlock*, RegisterFacet, LLReg, LLState*);
void ll_basic_block_clear_register(LLBasicBlock*, LLReg, LLState*);
void ll_basic_block_zero_register(LLBasicBlock*, LLReg, LLState*);
void ll_basic_block_rename_register(LLBasicBlock*, LLReg, LLReg, LLState*);
void ll_basic_block_set_register(LLBasicBlock*, RegisterFacet, LLReg, LLVMValueRef, bool, LLState*);
LLVMValueRef ll_basic_block_get_flag(LLBasicBlock*, int);
void ll_basic_block_set_flag(LLBasicBlock*, int, LLVMValueRef);
LLFlagCache* ll_basic_block_get_flag_cache(LLBasicBlock*);

LLVMBasicBlockRef ll_basic_block_llvm(LLBasicBlock*);

#endif