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

/* User include files */
#include "ih264_typedefs.h"
#include "ih264_macros.h"
#include "ih264_platform_macros.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_macros_msa.h"

static void avg_src_width4_msa(UWORD8 *src1_ptr, WORD32 src1_stride,
                               UWORD8 *src2_ptr, WORD32 src2_stride,
                               UWORD8 *dst, WORD32 dst_stride,
                               WORD32 height)
{
    WORD32 cnt;
    UWORD32 src0_w, src1_w, src2_w, src3_w;
    v16u8 src1 = { 0 };
    v16u8 src2 = { 0 };
    v16u8 res;

    if(0 == (height % 4))
    {
        for(cnt = (height >> 2); cnt--;)
        {
            LW4(src1_ptr, src1_stride, src0_w, src1_w, src2_w, src3_w);
            src1_ptr += (4 * src1_stride);
            INSERT_W4_UB(src0_w, src1_w, src2_w, src3_w, src1);

            LW4(src2_ptr, src2_stride, src0_w, src1_w, src2_w, src3_w);
            src2_ptr += (4 * src2_stride);
            INSERT_W4_UB(src0_w, src1_w, src2_w, src3_w, src2);

            res = __msa_aver_u_b(src1, src2);

            ST4x4_UB(res, res, 0, 1, 2, 3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == (height % 2))
    {
        for(cnt = (height >> 1); cnt--;)
        {
            src0_w = LW(src1_ptr);
            src1_w = LW(src1_ptr + src1_stride);
            src2_w = LW(src2_ptr);
            src3_w = LW(src2_ptr + src2_stride);

            INSERT_W2_UB(src0_w, src1_w, src1);
            INSERT_W2_UB(src2_w, src3_w, src2);

            res = __msa_aver_u_b(src1, src2);

            ST4x2_UB(res, dst, dst_stride);
            dst += (2 * dst_stride);
        }
    }
}

static void avg_src_width8_msa(UWORD8 *src1_ptr, WORD32 src1_stride,
                               UWORD8 *src2_ptr, WORD32 src2_stride,
                               UWORD8 *dst, WORD32 dst_stride,
                               WORD32 height)
{
    WORD32 cnt;
    UWORD64 src0_d, src1_d, src2_d, src3_d;
    v16u8 src0 = { 0 };
    v16u8 src1 = { 0 };
    v16u8 src2 = { 0 };
    v16u8 src3 = { 0 };
    v16u8 res0, res1;

    if(0 == (height % 4))
    {
        for(cnt = (height >> 2); cnt--;)
        {
            LD4(src1_ptr, src1_stride, src0_d, src1_d, src2_d, src3_d);
            src1_ptr += (4 * src1_stride);
            INSERT_D2_UB(src0_d, src1_d, src0);
            INSERT_D2_UB(src2_d, src3_d, src1);

            LD4(src2_ptr, src2_stride, src0_d, src1_d, src2_d, src3_d);
            src2_ptr += (4 * src2_stride);
            INSERT_D2_UB(src0_d, src1_d, src2);
            INSERT_D2_UB(src2_d, src3_d, src3);

            AVER_UB2_UB(src0, src2, src1, src3, res0, res1);
            ST8x4_UB(res0, res1, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == (height % 2))
    {
        for(cnt = (height >> 1); cnt--;)
        {
            LD2(src1_ptr, src1_stride, src0_d, src1_d);
            LD2(src2_ptr, src2_stride, src2_d, src3_d);
            INSERT_D2_UB(src0_d, src1_d, src0);
            INSERT_D2_UB(src2_d, src3_d, src1);

            res0 = __msa_aver_u_b(src0, src1);

            ST8x2_UB(res0, dst, dst_stride);
            dst += (2 * dst_stride);
        }
    }
}

static void avg_src_width16_msa(UWORD8 *src1_ptr, WORD32 src1_stride,
                                UWORD8 *src2_ptr, WORD32 src2_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD32 height)
{
    WORD32 cnt;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16u8 src8, src9, src10, src11, src12, src13, src14, src15;

    if(0 == (height % 8))
    {
        for(cnt = (height >> 3); cnt--;)
        {
            LD_UB8(src1_ptr, src1_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src1_ptr += (8 * src1_stride);
            LD_UB8(src2_ptr, src2_stride,
                   src8, src9, src10, src11, src12, src13, src14, src15);
            src2_ptr += (8 * src2_stride);

            AVER_ST16x4_UB(src0, src8, src1, src9, src2, src10, src3, src11,
                           dst, dst_stride);
            dst += (4 * dst_stride);
            AVER_ST16x4_UB(src4, src12, src5, src13, src6, src14, src7, src15,
                           dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == (height % 4))
    {
        for(cnt = (height >> 2); cnt--;)
        {
            LD_UB4(src1_ptr, src1_stride, src0, src1, src2, src3);
            src1_ptr += (4 * src1_stride);
            LD_UB4(src2_ptr, src2_stride, src4, src5, src6, src7);
            src2_ptr += (4 * src2_stride);

            AVER_ST16x4_UB(src0, src4, src1, src5, src2, src6, src3, src7,
                           dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
}

void ih264_inter_pred_luma_bilinear_msa(UWORD8 *src1, UWORD8 *src2, UWORD8 *dst,
                                        WORD32 src1_stride, WORD32 src2_stride,
                                        WORD32 dst_stride, WORD32 height,
                                        WORD32 width)
{
    if(4 == width)
    {
        avg_src_width4_msa(src1, src1_stride, src2, src2_stride, dst,
                           dst_stride, height);
    }
    else if(8 == width)
    {
        avg_src_width8_msa(src1, src1_stride, src2, src2_stride, dst,
                           dst_stride, height);
    }
    else if(16 == width)
    {
        avg_src_width16_msa(src1, src1_stride, src2, src2_stride, dst,
                            dst_stride, height);
    }
}
