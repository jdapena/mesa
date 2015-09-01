/*
 * Copyright © 2015 Intel Corporation
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
 *
 * Authors:
 *    Eduardo Lima Mitev (elima@igalia.com)
 *
 */

#include "nir.h"

/*
 * Implements a pass that lowers vecN instructions by propagating the
 * components of their destinations, as the destination of the
 * instructions that defines the sources of the vecN instruction.
 *
 * This effectively coalesces registers and reduces indirection.
 *
 * If all the components of the destination register in the vecN
 * instruction can be propagated, the instruction is removed. Otherwise,
 * a new, reduced vecN instruction is emitted with the channels that
 * remained.
 *
 * By now, this pass will only propagate to ALU instructions, but it could
 * be extended to include load_const instructions or some intrinsics like
 * load_input.
 *
 * This pass works on a NIR shader in final form (after SSA), and is
 * expected to run before nir_lower_vec_to_movs().
 */

/**
 * Clone an ALU instruction and override the destination with the one given by
 * new_dest. It copies sources from original ALU to the new one, adjusting
 * their swizzles.
 *
 * Returns the new ALU instruction.
 */
static nir_alu_instr *
clone_alu_instr_and_override_dest(nir_alu_instr *alu_instr,
                                  nir_alu_dest *new_dest, unsigned index,
                                  void *mem_ctx)
{
   nir_alu_instr *new_alu_instr = nir_alu_instr_create(mem_ctx, alu_instr->op);

   /* Determine which dest channel was used in the parent ALU instruction */
   unsigned channel;
   for (unsigned i = 0; i < 4; i++) {
      if (alu_instr->dest.write_mask & (1 << i)) {
         channel = i;
         break;
      }
   }
   // assert(alu_instr->dest.write_mask == (1 << channel));

   for (unsigned i = 0; i < nir_op_infos[alu_instr->op].num_inputs; i++) {
      nir_alu_src_copy(&new_alu_instr->src[i], &alu_instr->src[i], mem_ctx);

      switch (alu_instr->op) {
      case nir_op_fdot2:
      case nir_op_fdot3:
      case nir_op_fdot4:
         continue;
      default:
         break;
      }

      new_alu_instr->src[i].swizzle[index] =
         alu_instr->src[i].swizzle[channel];
   }

   nir_alu_dest_copy(&new_alu_instr->dest, new_dest, mem_ctx);
   new_alu_instr->dest.write_mask = 1 << index;

   return new_alu_instr;
}

static bool
register_already_tracked(const nir_register *reg, nir_register *list[4],
                         unsigned num_items)
{
   for (unsigned i = 0; i < num_items; i++) {
      if (list[i] == reg)
         return true;
   }
   return false;
}

static bool
lower_vec_and_coalesce_block(nir_block *block, void *mem_ctx)
{
   nir_foreach_instr_safe(block, instr) {
      if (instr->type != nir_instr_type_alu)
         continue;

      nir_alu_instr *vec = nir_instr_as_alu(instr);

      switch (vec->op) {
      case nir_op_vec2:
      case nir_op_vec3:
      case nir_op_vec4:
         break;
      default:
         continue; /* The loop */
      }

      /* Since we insert multiple MOVs, we have to be non-SSA. */
      assert(!vec->dest.dest.is_ssa);

      unsigned finished_write_mask = 0;
      nir_register *tracked_registers[4] = {0};
      unsigned num_tracked_registers = 0;

      for (unsigned i = 0; i < 4; i++) {
         if (!(vec->dest.write_mask & (1 << i)))
            continue;

         /* We don't propagate constants by now
          * @FIXME: we could also consider propagating destination of
          * load_const instructions.
          */
         if (vec->src[i].src.is_ssa)
            continue;

         nir_register *reg = vec->src[i].src.reg.reg;

         nir_foreach_def_safe(reg, src) {
            nir_instr *parent_instr = src->reg.parent_instr;

            /* the parent instruction must be in the same block as the vecX */
            if (parent_instr->block != block)
               continue;

            /* We only coalesce registers written by ALU instructions, by now.
             * @FIXME: consider other type of instructions, like intrinsics, etc.
             */
            if (parent_instr->type != nir_instr_type_alu)
               continue;

            nir_alu_instr *parent_alu_instr = nir_instr_as_alu(parent_instr);
            nir_register *parent_dest_reg = parent_alu_instr->dest.dest.reg.reg;

            /* We only override dest registers that are only used once, and in
             * this vecX instruction.
             * @FIXME: In the future we might consider registers used more than
             * once as sources of the same vecX instruction.
             */
            if (list_length(&parent_dest_reg->uses) != 1)
               continue;

            /* @FIXME: IMOVs ops cannot be propagated.
             * Need to investigate why!
             */
            if (parent_alu_instr->op == nir_op_imov)
               continue;

            nir_alu_instr *new_alu_instr =
               clone_alu_instr_and_override_dest(parent_alu_instr, &vec->dest,
                                                 i, mem_ctx);
            finished_write_mask |= new_alu_instr->dest.write_mask;

            /* Insert the new instruction with the overwritten destination,
             * in the block where the parent instruction is.
             */
            /*
            nir_instr_insert_before(&parent_alu_instr->instr,
                                    &new_alu_instr->instr);
            */

            /* Remove the old ALU instruction */
            nir_instr_remove(&parent_alu_instr->instr);
            ralloc_free(parent_alu_instr);

            /* Track the intermediate register, to remove it later if not used */
            if (!register_already_tracked(parent_dest_reg, tracked_registers,
                                          num_tracked_registers)) {
               tracked_registers[num_tracked_registers] = parent_dest_reg;
               num_tracked_registers++;
            }

            /* Insert the new instruction with the overwritten destination */
            nir_instr_insert_before(&vec->instr, &new_alu_instr->instr);
         }
      }

      if (finished_write_mask == 0)
         continue;

      nir_alu_instr *new_alu_instr = nir_alu_instr_create(mem_ctx, nir_op_vec4);
      nir_alu_dest_copy(&new_alu_instr->dest, &vec->dest, mem_ctx);
      new_alu_instr->dest.write_mask = 0;

      unsigned c = 0;
      for (unsigned i = 0; i < nir_op_infos[vec->op].num_inputs; i++) {
         if (!(vec->dest.write_mask & (1 << i)))
            continue;

         if (finished_write_mask & (1 << i))
            continue;

         nir_alu_src_copy(&new_alu_instr->src[c], &vec->src[i], mem_ctx);
         new_alu_instr->src[c].swizzle[i] = vec->src[i].swizzle[c];

         new_alu_instr->dest.write_mask |= (1 << i);

         c++;
      }

      switch (c) {
      case 0:
         ralloc_free(new_alu_instr);
         break;
      case 1:
         new_alu_instr->op = nir_op_imov;
         break;
      case 2:
         new_alu_instr->op = nir_op_vec2;
         break;
      case 3:
         new_alu_instr->op = nir_op_vec3;
         break;
      default:
         unreachable("Not reached");
      }

      if (c != 0)
         nir_instr_insert_before(&vec->instr, &new_alu_instr->instr);

      /* Remove the original vecN instruction */
      nir_instr_remove(&vec->instr);
      ralloc_free(vec);

      /* Remove the tracked registers if no longer used */
      for (unsigned i = 0; i < num_tracked_registers; i++) {
         if (list_length(&tracked_registers[i]->defs) == 0 &&
             list_length(&tracked_registers[i]->uses) == 0 &&
             list_length(&tracked_registers[i]->if_uses) == 0) {
            nir_reg_remove(tracked_registers[i]);
         }
      }
   }

   return true;
}

static void
nir_lower_vec_and_coalesce_impl(nir_function_impl *impl)
{
   nir_foreach_block(impl, lower_vec_and_coalesce_block, ralloc_parent(impl));
}

void
nir_lower_vec_and_coalesce(nir_shader *shader)
{
   nir_foreach_overload(shader, overload) {
      if (overload->impl)
         nir_lower_vec_and_coalesce_impl(overload->impl);
   }
}
