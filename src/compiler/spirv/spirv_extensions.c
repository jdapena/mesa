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

#include "spirv.h"
#include "spirv_extensions.h"

const char *
spirv_extensions_to_string(SpvExtension ext)
{
   switch (ext) {
   case SPV_AMD_shader_explicit_vertex_parameter: return "SPV_AMD_shader_explicit_vertex_parameter";
   case SPV_AMD_shader_trinary_minmax: return "SPV_AMD_shader_trinary_minmax";
   case SPV_AMD_gcn_shader: return "SPV_AMD_gcn_shader";
   case SPV_KHR_shader_ballot: return "SPV_KHR_shader_ballot";
   case SPV_AMD_shader_ballot: return "SPV_AMD_shader_ballot";
   case SPV_AMD_gpu_shader_half_float: return "SPV_AMD_gpu_shader_half_float";
   case SPV_KHR_shader_draw_parameters: return "SPV_KHR_shader_draw_parameters";
   case SPV_KHR_subgroup_vote: return "SPV_KHR_subgroup_vote";
   case SPV_KHR_16bit_storage: return "SPV_KHR_16bit_storage";
   case SPV_KHR_device_group: return "SPV_KHR_device_group";
   case SPV_KHR_multiview: return "SPV_KHR_multiview";
   case SPV_NVX_multiview_per_view_attributes: return "SPV_NVX_multiview_per_view_attributes";
   case SPV_NV_viewport_array2: return "SPV_NV_viewport_array2";
   case SPV_NV_stereo_view_rendering: return "SPV_NV_stereo_view_rendering";
   case SPV_NV_sample_mask_override_coverage: return "SPV_NV_sample_mask_override_coverage";
   case SPV_NV_geometry_shader_passthrough: return "SPV_NV_geometry_shader_passthrough";
   case SPV_AMD_texture_gather_bias_lod: return "SPV_AMD_texture_gather_bias_lod";
   case SPV_KHR_storage_buffer_storage_class: return "SPV_KHR_storage_buffer_storage_class";
   case SPV_KHR_variable_pointers: return "SPV_KHR_variable_pointers";
   case SPV_AMD_gpu_shader_int16: return "SPV_AMD_gpu_shader_int16";
   case SPV_KHR_post_depth_coverage: return "SPV_KHR_post_depth_coverage";
   case SPV_KHR_shader_atomic_counter_ops: return "SPV_KHR_shader_atomic_counter_ops";
   case SPV_EXT_shader_stencil_export: return "SPV_EXT_shader_stencil_export";
   case SPV_EXT_shader_viewport_index_layer: return "SPV_EXT_shader_viewport_index_layer";
   case SPV_AMD_shader_image_load_store_lod: return "SPV_AMD_shader_image_load_store_lod";
   case SPV_AMD_shader_fragment_mask: return "SPV_AMD_shader_fragment_mask";
   default: return "unknown";
   }

   return "unknown";
}

/**
 * Sets the supported flags for known SPIR-V extensions based on the
 * capabilites supported (spirv capabilities based on the spirv to nir
 * support).
 *
 * One could argue that makes more sense in the other way around, as from the
 * spec pov capabilities are enable for a given extension. But from our pov,
 * we support or not (depending on the driver) some given capability, and
 * spirv_to_nir check for capabilities not extensions. Also we usually fill
 * first the supported capabilities, that are not always related to an
 * extension.
 */
void
fill_supported_spirv_extensions(struct spirv_supported_extensions *ext,
                                const struct nir_spirv_supported_capabilities *cap)
{
   for (unsigned i = 0; i < SPV_EXTENSIONS_COUNT; i++)
      ext->supported[i] = false;

   ext->count = 0;

   ext->supported[SPV_KHR_shader_draw_parameters] = cap->draw_parameters;
   ext->supported[SPV_KHR_multiview] = cap->multiview;
   ext->supported[SPV_KHR_variable_pointers] = cap->variable_pointers;

   for (unsigned i = 0; i < SPV_EXTENSIONS_COUNT; i++)
      if (ext->supported[i]) ext->count++;
}
