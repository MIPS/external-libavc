/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Imagination Technologies
*/

#include "ih264_defs.h"
#include "ih264_typedefs.h"
#include "ih264_macros.h"
#include "ih264_platform_macros.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_macros_msa.h"

static void intra_predict_vert_16x8_msa(UWORD8 *src, UWORD8 *dst,
                                        WORD32 dst_stride)
{
    v16u8 out = LD_UB(src);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

static void avc_intra_predict_hz_interleaved_chroma_msa(UWORD8 *src,
                                                        UWORD8 *dst,
                                                        WORD32 dst_stride)
{
    WORD32 inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 data;

    data = LD_SH(src);

    inp0 = __msa_copy_u_h(data, 0);
    inp1 = __msa_copy_u_h(data, 1);
    inp2 = __msa_copy_u_h(data, 2);
    inp3 = __msa_copy_u_h(data, 3);
    inp4 = __msa_copy_u_h(data, 4);
    inp5 = __msa_copy_u_h(data, 5);
    inp6 = __msa_copy_u_h(data, 6);
    inp7 = __msa_copy_u_h(data, 7);

    src0 = (v16u8)__msa_fill_h(inp0);
    src1 = (v16u8)__msa_fill_h(inp1);
    src2 = (v16u8)__msa_fill_h(inp2);
    src3 = (v16u8)__msa_fill_h(inp3);
    src4 = (v16u8)__msa_fill_h(inp4);
    src5 = (v16u8)__msa_fill_h(inp5);
    src6 = (v16u8)__msa_fill_h(inp6);
    src7 = (v16u8)__msa_fill_h(inp7);

    ST_UB8(src0, src1, src2, src3, src4, src5, src6, src7, dst, dst_stride);
}

void ih264_intra_pred_chroma_8x8_mode_horz_msa(UWORD8 *src, UWORD8 *dst,
                                               WORD32 src_stride,
                                               WORD32 dst_stride,
                                               WORD32 ngbr_avail)
{
    UNUSED(src_stride);
    UNUSED(ngbr_avail);

    avc_intra_predict_hz_interleaved_chroma_msa(src, dst + 7 * dst_stride,
                                                -dst_stride);
}

void ih264_intra_pred_chroma_8x8_mode_vert_msa(UWORD8 *src, UWORD8 *dst,
                                               WORD32 src_stride,
                                               WORD32 dst_stride,
                                               WORD32 ngbr_avail)
{
    UNUSED(src_stride);
    UNUSED(ngbr_avail);

    intra_predict_vert_16x8_msa(src + 2 * BLK8x8SIZE + 2, dst, dst_stride);
}
