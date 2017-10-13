/*
 * Copyright © 2017 Intel Corporation
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

#ifndef _SPIRV_EXTENSIONS_H_
#define _SPIRV_EXTENSIONS_H_

#include "compiler/nir/nir.h"
#include "nir_spirv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SpvExtension_ {
   SPV_AMD_shader_explicit_vertex_parameter = 0,
   SPV_AMD_shader_trinary_minmax,
   SPV_AMD_gcn_shader,
   SPV_KHR_shader_ballot,
   SPV_AMD_shader_ballot,
   SPV_AMD_gpu_shader_half_float,
   SPV_KHR_shader_draw_parameters,
   SPV_KHR_subgroup_vote,
   SPV_KHR_16bit_storage,
   SPV_KHR_device_group,
   SPV_KHR_multiview,
   SPV_NVX_multiview_per_view_attributes,
   SPV_NV_viewport_array2,
   SPV_NV_stereo_view_rendering,
   SPV_NV_sample_mask_override_coverage,
   SPV_NV_geometry_shader_passthrough,
   SPV_AMD_texture_gather_bias_lod,
   SPV_KHR_storage_buffer_storage_class,
   SPV_KHR_variable_pointers,
   SPV_AMD_gpu_shader_int16,
   SPV_KHR_post_depth_coverage,
   SPV_KHR_shader_atomic_counter_ops,
   SPV_EXT_shader_stencil_export,
   SPV_EXT_shader_viewport_index_layer,
   SPV_AMD_shader_image_load_store_lod,
   SPV_AMD_shader_fragment_mask,
   SPV_EXTENSIONS_COUNT
} SpvExtension;

struct spirv_supported_extensions {
   /** Flags the supported extensions. Array to make it easier to iterate. */
   bool supported[SPV_EXTENSIONS_COUNT];

   /** Number of supported extensions */
   unsigned int count;
};

const char *spirv_extensions_to_string(SpvExtension ext);

void fill_supported_spirv_extensions(struct spirv_supported_extensions *ext,
                                     const struct nir_spirv_supported_capabilities *cap);

#ifdef __cplusplus
}
#endif

#endif /* SPIRV_EXTENSIONS */
