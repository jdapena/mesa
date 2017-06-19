/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2017 Advanced Micro Devices, Inc.
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "nir_linker.h"

#include "main/mtypes.h"
#include "main/shaderobj.h"
#include "program/program.h"

#include "nir.h"
#include "compiler/glsl/glsl_symbol_table.h"
#include "compiler/glsl/linker.h"

#define validate(prog, cond, msg) \
   if (!(cond)) \
      validate_error(prog, __FILE__, __LINE__, #cond, msg);

static void
validate_error(gl_shader_program *prog, const char *filename, unsigned line,
               const char *cond, const char *msg)
{
   linker_error(prog, "SPIR-V link-time validation error:\n%s\nin %s:%u: %s",
                msg, filename, line, cond);
}

static ir_depth_layout
ir_from_nir_depth_layout(nir_depth_layout depth_layout)
{
   switch (depth_layout) {
   default:
      assert(false);
      /* fall-through */
   case nir_depth_layout_none: return ir_depth_layout_none;
   case nir_depth_layout_any: return ir_depth_layout_any;
   case nir_depth_layout_greater: return ir_depth_layout_greater;
   case nir_depth_layout_less: return ir_depth_layout_less;
   case nir_depth_layout_unchanged: return ir_depth_layout_unchanged;
   }
}

void
nir_to_ir_variable(void *mem_ctx, gl_shader_program *prog,
                   gl_linked_shader *linked, nir_variable *nir_var)
{
   ir_variable_mode mode;

   switch (nir_var->data.mode) {
   case nir_var_shader_in: mode = ir_var_shader_in; break;
   case nir_var_shader_out: mode = ir_var_shader_out; break;
   case nir_var_uniform: mode = ir_var_uniform; break;
   default:
      assert(!"not implemented");
   }

   const char *name = nir_var->name;

   validate(prog,
            nir_var->data.location >= 0 ||
            (nir_var->data.mode != nir_var_shader_in && nir_var->data.mode != nir_var_shader_out),
            "Input and output variables must be decorated with a Location");

   if (!name) {
      /* Need spec clarification: Is this allowed for default-block uniforms?
       * https://gitlab.khronos.org/opengl/API/issues/35
       */
      validate(prog, nir_var->data.location >= 0,
               "Default-block uniforms without Name must have a Location");
      name = "";
   }

   ir_variable *ir_var = new(linked) ir_variable(nir_var->type, name, mode);

   linked->ir->push_tail(ir_var);

   ir_var->data.read_only = nir_var->data.read_only;
   ir_var->data.centroid = nir_var->data.centroid;
   ir_var->data.sample = nir_var->data.sample;
   ir_var->data.patch = nir_var->data.patch;
   ir_var->data.invariant = nir_var->data.invariant;
   ir_var->data.interpolation = nir_var->data.interpolation;
   ir_var->data.origin_upper_left = nir_var->data.origin_upper_left;
   ir_var->data.pixel_center_integer = nir_var->data.pixel_center_integer;
   ir_var->data.location_frac = nir_var->data.location_frac;
   ir_var->data.fb_fetch_output = nir_var->data.fb_fetch_output;
   ir_var->data.depth_layout = ir_from_nir_depth_layout(nir_var->data.depth_layout);
   ir_var->data.location = nir_var->data.location;
   ir_var->data.index = nir_var->data.index;
   assert(nir_var->data.descriptor_set == 0);
   ir_var->data.binding = nir_var->data.binding;
   ir_var->data.offset = nir_var->data.offset;
   ir_var->data.memory_read_only = nir_var->data.image.read_only;
   ir_var->data.memory_write_only = nir_var->data.image.write_only;
   ir_var->data.memory_coherent = nir_var->data.image.coherent;
   ir_var->data.memory_volatile = nir_var->data.image._volatile;
   ir_var->data.memory_restrict = nir_var->data.image.restrict_flag;
   ir_var->data.image_format = nir_var->data.image.format;

   if (ir_var->data.location >= 0)
      ir_var->data.explicit_location = 1;
}
