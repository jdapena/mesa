/*
 * Copyright © 2016 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "compiler/nir/nir_builder.h"
#include "brw_nir.h"

struct fix_64bits_state {
   nir_builder builder;
   bool impl_progress;
   void *mem_ctx;
   nir_function_impl *impl;
};

static bool
fix_64bits_block(nir_block *block, void *void_state)
{
   struct fix_64bits_state *state = void_state;

   unsigned offset = 0;

   nir_foreach_instr_safe(block, instr) {
      if (instr->type != nir_instr_type_intrinsic)
         continue;

      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
      if (intrin->intrinsic != nir_intrinsic_load_input)
         continue;

      assert(intrin->dest.is_ssa);

      nir_intrinsic_set_base(intrin, nir_intrinsic_base(intrin) + offset);

      if (intrin->dest.ssa.bit_size != 64)
         continue;

      unsigned num_components = intrin->num_components * 2;

      intrin->num_components = MIN2(num_components, 4);
      intrin->dest.ssa.num_components = intrin->num_components;

      nir_print_instr(instr, stderr); printf("\n");

      num_components -= intrin->num_components;

      if (num_components > 0) {
         nir_intrinsic_instr *load =
            nir_intrinsic_instr_create(state->mem_ctx,
                                       nir_intrinsic_load_input);
         load->num_components = num_components;
         nir_intrinsic_set_base(load, nir_intrinsic_base(intrin) + 1);
         nir_ssa_dest_init(&load->instr, &load->dest,
                           num_components,
                           intrin->dest.ssa.bit_size, NULL);
         load->dest.ssa.index = state->impl->ssa_alloc;
         state->impl->ssa_alloc++;
         nir_src_copy(&load->src[0], &intrin->src[0], state->mem_ctx);

         offset++;

         nir_print_instr(&load->instr, stderr); printf("\n");
         nir_instr_insert_after(instr, &load->instr);

         nir_foreach_use_safe(&intrin->dest.ssa, src) {
            if (src->parent_instr->type != nir_instr_type_alu)
               continue;

            if (src->parent_instr->block != block)
               continue;

            nir_alu_instr *alu = nir_instr_as_alu(src->parent_instr);
            for (unsigned i = 0; i < 1; i++) {
               nir_alu_src *alu_src = &alu->src[i];

               if (!alu_src->src.is_ssa || &alu_src->src != src)
                  continue;

               if (alu_src->swizzle[0] == SWIZZLE_Z ||
                   alu_src->swizzle[0] == SWIZZLE_W) {
                  nir_src new_src = nir_src_for_ssa(&load->dest.ssa);
                  nir_instr_rewrite_src(src->parent_instr,
                                        &(alu->src[i].src),
                                        new_src);
                  if (alu_src->swizzle[0] == SWIZZLE_Z)
                     alu_src->swizzle[0] = SWIZZLE_X;
                  else
                     alu_src->swizzle[0] = SWIZZLE_Y;
                  nir_print_instr(src->parent_instr, stderr); printf("\n");
               }
            }
         }

         state->impl_progress = true;
      }
   }

   return true;
}

bool
brw_nir_remap_64bits_attrs(nir_shader *shader)
{
   struct fix_64bits_state state = {0, };
   bool progress = false;

   nir_foreach_function(shader, func) {
      if (!func->impl)
         continue;

      nir_builder_init(&state.builder, func->impl);
      state.mem_ctx = ralloc_parent(func->impl);
      state.impl = func->impl;

      nir_foreach_block(func->impl, fix_64bits_block, &state);

      if (state.impl_progress) {
         progress = true;
      }
   }

   return progress;
}
