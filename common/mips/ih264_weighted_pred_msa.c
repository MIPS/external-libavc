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
#include "ih264_weighted_pred.h"
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

static void avc_wgt_luma_4x2_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD32 log2_denom, WORD32 weight,
                                 WORD32 offset_in)
{
    UWORD32 load0, load1, out0, out1;
    v16u8 zero = { 0 };
    v16u8 src0, src1;
    v4i32 dst0, dst1;
    v8u16 temp0, temp1, wgt, denom, offset, tp0, tp1;
    v8i16 vec0, vec1;

    wgt = (v8u16)__msa_fill_h(weight);
    offset = (v8u16)__msa_fill_h(offset_in);
    denom = (v8u16)__msa_fill_h(log2_denom);

    load0 = LW(src);
    src += src_stride;
    load1 = LW(src);

    src0 = (v16u8)__msa_fill_w(load0);
    src1 = (v16u8)__msa_fill_w(load1);

    ILVR_B2_UH(zero, src0, zero, src1, temp0, temp1);
    MUL2(wgt, temp0, wgt, temp1, temp0, temp1);
    ADDS_SH2_SH(temp0, offset, temp1, offset, vec0, vec1);
    MAXI_SH2_SH(vec0, vec1, 0);

    tp0 = (v8u16)__msa_srl_h(vec0, (v8i16)denom);
    tp1 = (v8u16)__msa_srl_h(vec1, (v8i16)denom);

    SAT_UH2_UH(tp0, tp1, 7);
    PCKEV_B2_SW(tp0, tp0, tp1, tp1, dst0, dst1);

    out0 = __msa_copy_u_w(dst0, 0);
    out1 = __msa_copy_u_w(dst1, 0);
    SW(out0, dst);
    dst += dst_stride;
    SW(out1, dst);
}

static void avc_wgt_luma_4x4mult_msa(UWORD8 *src, WORD32 src_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD32 height, WORD32 log2_denom,
                                     WORD32 weight, WORD32 offset_in)
{
    UWORD8 cnt;
    UWORD32 load0, load1, load2, load3;
    v16u8 zero = { 0 };
    v16u8 src0, src1, src2, src3;
    v8u16 temp0, temp1, temp2, temp3;
    v8u16 wgt, denom, offset;

    wgt = (v8u16)__msa_fill_h(weight);
    offset = (v8u16)__msa_fill_h(offset_in);
    denom = (v8u16)__msa_fill_h(log2_denom);

    for(cnt = (height >> 2); cnt--;)
    {
        LW4(src, src_stride, load0, load1, load2, load3);
        src += 4 * src_stride;

        src0 = (v16u8)__msa_fill_w(load0);
        src1 = (v16u8)__msa_fill_w(load1);
        src2 = (v16u8)__msa_fill_w(load2);
        src3 = (v16u8)__msa_fill_w(load3);

        ILVR_B4_UH(zero, src0, zero, src1, zero, src2, zero, src3, temp0, temp1,
                   temp2, temp3);
        MUL4(wgt, temp0, wgt, temp1, wgt, temp2, wgt, temp3, temp0, temp1,
             temp2, temp3);
        ADDS_SH4_UH(temp0, offset, temp1, offset, temp2, offset, temp3, offset,
                    temp0, temp1, temp2, temp3);
        MAXI_SH4_UH(temp0, temp1, temp2, temp3, 0);
        SRL_H4_UH(temp0, temp1, temp2, temp3, denom);
        SAT_UH4_UH(temp0, temp1, temp2, temp3, 7);
        PCKEV_ST4x4_UB(temp0, temp1, temp2, temp3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void avc_wgt_luma_4w_msa(UWORD8 *src, WORD32 src_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD32 height, WORD32 log2_denom,
                                WORD32 weight, WORD32 offset_in)
{
    if(2 == height)
    {
        avc_wgt_luma_4x2_msa(src, src_stride, dst, dst_stride, log2_denom,
                             weight, offset_in);
    }
    else
    {
        avc_wgt_luma_4x4mult_msa(src, src_stride, dst, dst_stride,
                                 height, log2_denom, weight, offset_in);
    }
}

static void avc_wgt_luma_8w_msa(UWORD8 *src, WORD32 src_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD32 height, WORD32 log2_denom,
                                WORD32 weight, WORD32 offset_in)
{
    UWORD8 cnt;
    v16u8 zero = { 0 };
    v16u8 src0, src1, src2, src3;
    v16i8 out0, out1;
    v8u16 temp0, temp1, temp2, temp3, wgt, denom, offset;

    wgt = (v8u16)__msa_fill_h(weight);
    offset = (v8u16)__msa_fill_h(offset_in);
    denom = (v8u16)__msa_fill_h(log2_denom);

    for(cnt = (height >> 2); cnt--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        src += 4 * src_stride;

        ILVR_B4_UH(zero, src0, zero, src1, zero, src2, zero, src3, temp0, temp1,
                   temp2, temp3);
        MUL4(wgt, temp0, wgt, temp1, wgt, temp2, wgt, temp3, temp0, temp1,
             temp2, temp3);
        ADDS_SH4_UH(temp0, offset, temp1, offset, temp2, offset, temp3, offset,
                    temp0, temp1, temp2, temp3);
        MAXI_SH4_UH(temp0, temp1, temp2, temp3, 0);
        SRL_H4_UH(temp0, temp1, temp2, temp3, denom);
        SAT_UH4_UH(temp0, temp1, temp2, temp3, 7);
        PCKEV_B2_SB(temp1, temp0, temp3, temp2, out0, out1);
        ST8x4_UB(out0, out1, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void avc_wgt_luma_16w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD32 height, WORD32 log2_denom,
                                 WORD32 weight, WORD32 offset_in)
{
    UWORD8 cnt;
    v16i8 zero = { 0 };
    v16u8 src0, src1, src2, src3, dst0, dst1, dst2, dst3;
    v8u16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    v8u16 wgt, denom, offset;

    wgt = (v8u16)__msa_fill_h(weight);
    offset = (v8u16)__msa_fill_h(offset_in);
    denom = (v8u16)__msa_fill_h(log2_denom);

    for(cnt = (height >> 2); cnt--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        src += 4 * src_stride;

        ILVR_B4_UH(zero, src0, zero, src1, zero, src2, zero, src3, temp0, temp2,
                   temp4, temp6);
        ILVL_B4_UH(zero, src0, zero, src1, zero, src2, zero, src3, temp1, temp3,
                   temp5, temp7);
        MUL4(wgt, temp0, wgt, temp1, wgt, temp2, wgt, temp3, temp0, temp1,
             temp2, temp3);
        MUL4(wgt, temp4, wgt, temp5, wgt, temp6, wgt, temp7, temp4, temp5,
             temp6, temp7);
        ADDS_SH4_UH(temp0, offset, temp1, offset, temp2, offset, temp3, offset,
                    temp0, temp1, temp2, temp3);
        ADDS_SH4_UH(temp4, offset, temp5, offset, temp6, offset, temp7, offset,
                    temp4, temp5, temp6, temp7);
        MAXI_SH4_UH(temp0, temp1, temp2, temp3, 0);
        MAXI_SH4_UH(temp4, temp5, temp6, temp7, 0);
        SRL_H4_UH(temp0, temp1, temp2, temp3, denom);
        SRL_H4_UH(temp4, temp5, temp6, temp7, denom);
        SAT_UH4_UH(temp0, temp1, temp2, temp3, 7);
        SAT_UH4_UH(temp4, temp5, temp6, temp7, 7);
        PCKEV_B4_UB(temp1, temp0, temp3, temp2, temp5, temp4, temp7, temp6,
                    dst0, dst1, dst2, dst3);

        ST_UB4(dst0, dst1, dst2, dst3, dst, dst_stride);
        dst += 4 * dst_stride;
    }
}

static void avc_biwgt_luma_4x2_msa(UWORD8 *src1_in, WORD32 src1_stride,
                                   UWORD8 *src2_in, WORD32 src2_stride,
                                   UWORD8 *dst, WORD32 dst_stride,
                                   WORD32 log2_denom, WORD32 src1_weight,
                                   WORD32 src2_weight, WORD32 offset_in)
{
    UWORD32 load0, load1, out0, out1;
    v16u8 in0, in1, in2, in3;
    v8i16 temp0, temp1, temp2, temp3;
    v16i8 zero = { 0 };
    v8i16 denom = __msa_fill_h(log2_denom);
    v8i16 offset = __msa_fill_h(offset_in);
    v8i16 src1_wgt = __msa_fill_h(src1_weight);
    v8i16 src2_wgt = __msa_fill_h(src2_weight);

    load0 = LW(src1_in);
    load1 = LW(src1_in + src1_stride);
    in0 = (v16u8)__msa_fill_w(load0);
    in1 = (v16u8)__msa_fill_w(load1);

    load0 = LW(src2_in);
    load1 = LW(src2_in + src2_stride);
    in2 = (v16u8)__msa_fill_w(load0);
    in3 = (v16u8)__msa_fill_w(load1);

    ILVR_B4_SH(zero, in0, zero, in1, zero, in2, zero, in3,
               temp0, temp1, temp2, temp3);
    temp0 = (temp0 * src1_wgt) + (temp2 * src2_wgt) + offset;
    temp1 = (temp1 * src1_wgt) + (temp3 * src2_wgt) + offset;

    temp0 >>= denom;
    temp1 >>= denom;

    CLIP_SH2_0_255(temp0, temp1);
    PCKEV_B2_UB(temp0, temp0, temp1, temp1, in0, in1);

    out0 = __msa_copy_u_w((v4i32)in0, 0);
    out1 = __msa_copy_u_w((v4i32)in1, 0);
    SW(out0, dst);
    dst += dst_stride;
    SW(out1, dst);
}

static void avc_biwgt_luma_4x4mult_msa(UWORD8 *src1_in, WORD32 src1_stride,
                                       UWORD8 *src2_in, WORD32 src2_stride,
                                       UWORD8 *dst, WORD32 dst_stride,
                                       WORD32 height, WORD32 log2_denom,
                                       WORD32 src1_weight, WORD32 src2_weight,
                                       WORD32 offset_in)
{
    UWORD8 cnt;
    UWORD32 load0, load1, load2, load3;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    v16i8 zero = { 0 };
    v8i16 denom = __msa_fill_h(log2_denom);
    v8i16 offset = __msa_fill_h(offset_in);
    v8i16 src1_wgt = __msa_fill_h(src1_weight);
    v8i16 src2_wgt = __msa_fill_h(src2_weight);

    for(cnt = (height >> 2); cnt--;)
    {
        LW4(src1_in, src1_stride, load0, load1, load2, load3);
        src1_in += (4 * src1_stride);
        src0 = (v16u8)__msa_fill_w(load0);
        src1 = (v16u8)__msa_fill_w(load1);
        src2 = (v16u8)__msa_fill_w(load2);
        src3 = (v16u8)__msa_fill_w(load3);

        LW4(src2_in, src2_stride, load0, load1, load2, load3);
        src2_in += (4 * src2_stride);
        src4 = (v16u8)__msa_fill_w(load0);
        src5 = (v16u8)__msa_fill_w(load1);
        src6 = (v16u8)__msa_fill_w(load2);
        src7 = (v16u8)__msa_fill_w(load3);

        ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
                   temp0, temp1, temp2, temp3);
        ILVR_B4_SH(zero, src4, zero, src5, zero, src6, zero, src7,
                   temp4, temp5, temp6, temp7);

        temp0 = (temp0 * src1_wgt) + (temp4 * src2_wgt) + offset;
        temp1 = (temp1 * src1_wgt) + (temp5 * src2_wgt) + offset;
        temp2 = (temp2 * src1_wgt) + (temp6 * src2_wgt) + offset;
        temp3 = (temp3 * src1_wgt) + (temp7 * src2_wgt) + offset;

        SRA_4V(temp0, temp1, temp2, temp3, denom);
        CLIP_SH4_0_255(temp0, temp1, temp2, temp3);
        PCKEV_ST4x4_UB(temp0, temp1, temp2, temp3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void avc_biwgt_luma_4w_msa(UWORD8 *src1_in, WORD32 src1_stride,
                                  UWORD8 *src2_in, WORD32 src2_stride,
                                  UWORD8 *dst, WORD32 dst_stride,
                                  WORD32 height, WORD32 log2_denom,
                                  WORD32 src1_weight, WORD32 src2_weight,
                                  WORD32 offset_in)
{
    if(2 == height)
    {
        avc_biwgt_luma_4x2_msa(src1_in, src1_stride, src2_in, src2_stride,
                               dst, dst_stride, log2_denom, src1_weight,
                               src2_weight, offset_in);
    }
    else
    {
        avc_biwgt_luma_4x4mult_msa(src1_in, src1_stride, src2_in, src2_stride,
                                   dst, dst_stride, height, log2_denom,
                                   src1_weight, src2_weight, offset_in);
    }
}

static void avc_biwgt_luma_8w_msa(UWORD8 *src1_in, WORD32 src1_stride,
                                  UWORD8 *src2_in, WORD32 src2_stride,
                                  UWORD8 *dst, WORD32 dst_stride,
                                  WORD32 height, WORD32 log2_denom,
                                  WORD32 src1_weight, WORD32 src2_weight,
                                  WORD32 offset_in)
{
    UWORD8 cnt;
    UWORD64 out0, out1, out2, out3;
    v16u8 src0, src1, src2, src3;
    v16u8 dst0, dst1, dst2, dst3;
    v8i16 temp0, temp1, temp2, temp3;
    v8i16 res0, res1, res2, res3;
    v16i8 zero = { 0 };
    v8i16 denom = __msa_fill_h(log2_denom);
    v8i16 offset = __msa_fill_h(offset_in);
    v8i16 src1_wgt = __msa_fill_h(src1_weight);
    v8i16 src2_wgt = __msa_fill_h(src2_weight);

    for(cnt = (height >> 2); cnt--;)
    {
        LD_UB4(src1_in, src1_stride, src0, src1, src2, src3);
        src1_in += (4 * src1_stride);
        LD_UB4(src2_in, src2_stride, dst0, dst1, dst2, dst3);
        src2_in += (4 * src2_stride);
        ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
                   temp0, temp1, temp2, temp3);
        ILVR_B4_SH(zero, dst0, zero, dst1, zero, dst2, zero, dst3,
                   res0, res1, res2, res3);
        res0 = (temp0 * src1_wgt) + (res0 * src2_wgt) + offset;
        res1 = (temp1 * src1_wgt) + (res1 * src2_wgt) + offset;
        res2 = (temp2 * src1_wgt) + (res2 * src2_wgt) + offset;
        res3 = (temp3 * src1_wgt) + (res3 * src2_wgt) + offset;
        SRA_4V(res0, res1, res2, res3, denom);
        CLIP_SH4_0_255(res0, res1, res2, res3);
        PCKEV_B4_UB(res0, res0, res1, res1, res2, res2, res3, res3,
                    dst0, dst1, dst2, dst3);

        out0 = __msa_copy_u_d((v2i64)dst0, 0);
        out1 = __msa_copy_u_d((v2i64)dst1, 0);
        out2 = __msa_copy_u_d((v2i64)dst2, 0);
        out3 = __msa_copy_u_d((v2i64)dst3, 0);
        SD4(out0, out1, out2, out3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void avc_biwgt_luma_16w_msa(UWORD8 *src1_in, WORD32 src1_stride,
                                   UWORD8 *src2_in, WORD32 src2_stride,
                                   UWORD8 *dst, WORD32 dst_stride,
                                   WORD32 height, WORD32 log2_denom,
                                   WORD32 src1_weight, WORD32 src2_weight,
                                   WORD32 offset_in)
{
    UWORD8 cnt;
    v16u8 src0, src1, src2, src3;
    v16u8 dst0, dst1, dst2, dst3;
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    v8i16 res0, res1, res2, res3, res4, res5, res6, res7;
    v16i8 zero = { 0 };
    v8i16 denom = __msa_fill_h(log2_denom);
    v8i16 offset = __msa_fill_h(offset_in);
    v8i16 src1_wgt = __msa_fill_h(src1_weight);
    v8i16 src2_wgt = __msa_fill_h(src2_weight);

    for(cnt = (height >> 2); cnt--;)
    {
        LD_UB4(src1_in, src1_stride, src0, src1, src2, src3);
        src1_in += (4 * src1_stride);
        LD_UB4(src2_in, src2_stride, dst0, dst1, dst2, dst3);
        src2_in += (4 * src2_stride);
        ILVRL_B2_SH(zero, src0, temp1, temp0);
        ILVRL_B2_SH(zero, src1, temp3, temp2);
        ILVRL_B2_SH(zero, src2, temp5, temp4);
        ILVRL_B2_SH(zero, src3, temp7, temp6);
        ILVRL_B2_SH(zero, dst0, res1, res0);
        ILVRL_B2_SH(zero, dst1, res3, res2);
        ILVRL_B2_SH(zero, dst2, res5, res4);
        ILVRL_B2_SH(zero, dst3, res7, res6);

        res0 = (temp0 * src1_wgt) + (res0 * src2_wgt) + offset;
        res1 = (temp1 * src1_wgt) + (res1 * src2_wgt) + offset;
        res2 = (temp2 * src1_wgt) + (res2 * src2_wgt) + offset;
        res3 = (temp3 * src1_wgt) + (res3 * src2_wgt) + offset;
        res4 = (temp4 * src1_wgt) + (res4 * src2_wgt) + offset;
        res5 = (temp5 * src1_wgt) + (res5 * src2_wgt) + offset;
        res6 = (temp6 * src1_wgt) + (res6 * src2_wgt) + offset;
        res7 = (temp7 * src1_wgt) + (res7 * src2_wgt) + offset;

        SRA_4V(res0, res1, res2, res3, denom);
        SRA_4V(res4, res5, res6, res7, denom);
        CLIP_SH4_0_255(res0, res1, res2, res3);
        CLIP_SH4_0_255(res4, res5, res6, res7);
        PCKEV_B4_UB(res0, res1, res2, res3, res4, res5, res6, res7,
                    dst0, dst1, dst2, dst3);
        ST_UB4(dst0, dst1, dst2, dst3, dst, dst_stride);
        dst += 4 * dst_stride;
    }
}

static void avc_wgt_interleaved_chroma_2x2_msa(UWORD8 *src,
                                               WORD32 src_stride,
                                               UWORD8 *dst,
                                               WORD32 dst_stride,
                                               WORD32 log2_denom,
                                               WORD32 weight,
                                               WORD32 offset_in)
{
    UWORD32 load0, load1, out0, out1;
    v16u8 zero = { 0 };
    v16u8 src0, src1;
    v4i32 dst0, dst1;
    v8u16 temp0, temp1, wgt, denom, offset, tp0, tp1;
    v8i16 vec0, vec1;

    wgt = (v8u16)__msa_fill_w(weight);
    offset = (v8u16)__msa_fill_w(offset_in);
    denom = (v8u16)__msa_fill_h(log2_denom);

    load0 = LW(src);
    src += src_stride;
    load1 = LW(src);

    src0 = (v16u8)__msa_fill_w(load0);
    src1 = (v16u8)__msa_fill_w(load1);

    ILVR_B2_UH(zero, src0, zero, src1, temp0, temp1);
    MUL2(wgt, temp0, wgt, temp1, temp0, temp1);
    ADDS_SH2_SH(temp0, offset, temp1, offset, vec0, vec1);
    MAXI_SH2_SH(vec0, vec1, 0);

    tp0 = (v8u16)__msa_srl_h(vec0, (v8i16)denom);
    tp1 = (v8u16)__msa_srl_h(vec1, (v8i16)denom);

    SAT_UH2_UH(tp0, tp1, 7);
    PCKEV_B2_SW(tp0, tp0, tp1, tp1, dst0, dst1);

    out0 = __msa_copy_u_w(dst0, 0);
    out1 = __msa_copy_u_w(dst1, 0);
    SW(out0, dst);
    dst += dst_stride;
    SW(out1, dst);
}

static void avc_wgt_interleaved_chroma_2x4_msa(UWORD8 *src,
                                               WORD32 src_stride,
                                               UWORD8 *dst,
                                               WORD32 dst_stride,
                                               WORD32 log2_denom,
                                               WORD32 weight,
                                               WORD32 offset_in)
{
    UWORD32 load0, load1, load2, load3;
    v16u8 zero = { 0 };
    v16u8 src0, src1, src2, src3;
    v8u16 temp0, temp1, temp2, temp3;
    v8u16 wgt, denom, offset;

    wgt = (v8u16)__msa_fill_w(weight);
    offset = (v8u16)__msa_fill_w(offset_in);
    denom = (v8u16)__msa_fill_h(log2_denom);

    LW4(src, src_stride, load0, load1, load2, load3);

    src0 = (v16u8)__msa_fill_w(load0);
    src1 = (v16u8)__msa_fill_w(load1);
    src2 = (v16u8)__msa_fill_w(load2);
    src3 = (v16u8)__msa_fill_w(load3);

    ILVR_B4_UH(zero, src0, zero, src1, zero, src2, zero, src3, temp0, temp1,
               temp2, temp3);
    MUL4(wgt, temp0, wgt, temp1, wgt, temp2, wgt, temp3, temp0, temp1,
         temp2, temp3);
    ADDS_SH4_UH(temp0, offset, temp1, offset, temp2, offset, temp3, offset,
                temp0, temp1, temp2, temp3);
    MAXI_SH4_UH(temp0, temp1, temp2, temp3, 0);
    SRL_H4_UH(temp0, temp1, temp2, temp3, denom);
    SAT_UH4_UH(temp0, temp1, temp2, temp3, 7);
    PCKEV_ST4x4_UB(temp0, temp1, temp2, temp3, dst, dst_stride);
}

static void avc_wgt_interleaved_chroma_2w_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              WORD32 height, WORD32 log2_denom,
                                              WORD32 weight, WORD32 offset_in)
{
    if(2 == height)
    {
        avc_wgt_interleaved_chroma_2x2_msa(src, src_stride, dst, dst_stride,
                                           log2_denom, weight, offset_in);
    }
    else
    {
        avc_wgt_interleaved_chroma_2x4_msa(src, src_stride, dst, dst_stride,
                                           log2_denom, weight, offset_in);
    }
}

static void avc_wgt_interleaved_chroma_4w_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              WORD32 height, WORD32 log2_denom,
                                              WORD32 weight, WORD32 offset_in)
{
    UWORD8 cnt;
    v16u8 zero = { 0 };
    v16u8 src0, src1, src2, src3;
    v16i8 out0, out1;
    v8i16 temp0, temp1, temp2, temp3, wgt, denom, offset;

    wgt = (v8i16)__msa_fill_w(weight);
    offset = (v8i16)__msa_fill_w(offset_in);
    denom = __msa_fill_h(log2_denom);

    if(0 == (height % 4))
    {
        for(cnt = (height >> 2); cnt--;)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);
            src += 4 * src_stride;
            ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3, temp0,
                       temp1, temp2, temp3);
            MUL4(wgt, temp0, wgt, temp1, wgt, temp2, wgt, temp3, temp0, temp1,
                 temp2, temp3);
            ADDS_SH4_SH(temp0, offset, temp1, offset, temp2, offset, temp3,
                        offset, temp0, temp1, temp2, temp3);
            MAXI_SH4_SH(temp0, temp1, temp2, temp3, 0);
            SRL_H4_SH(temp0, temp1, temp2, temp3, denom);
            SAT_UH4_SH(temp0, temp1, temp2, temp3, 7);
            PCKEV_B2_SB(temp1, temp0, temp3, temp2, out0, out1);
            ST8x4_UB(out0, out1, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else
    {
        LD_UB2(src, src_stride, src0, src1);
        ILVR_B2_SH(zero, src0, zero, src1, temp0, temp1);
        MUL2(wgt, temp0, wgt, temp1, temp0, temp1);
        ADDS_SH2_SH(temp0, offset, temp1, offset, temp0, temp1);
        MAXI_SH2_SH(temp0, temp1, 0);
        temp0 = __msa_srl_h((v8i16)temp0, (v8i16)denom);
        temp1 = __msa_srl_h((v8i16)temp1, (v8i16)denom);
        SAT_UH2_SH(temp0, temp1, 7);
        out0 = __msa_pckev_b((v16i8)temp1, (v16i8)temp0);
        ST8x2_UB(out0, dst, dst_stride);
    }
}

static void avc_wgt_interleaved_chroma_8w_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              WORD32 height, WORD32 log2_denom,
                                              WORD32 weight, WORD32 offset_in)
{
    UWORD8 cnt;
    v16i8 zero = { 0 };
    v16u8 src0, src1, src2, src3, dst0, dst1, dst2, dst3;
    v8u16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    v8u16 wgt, denom, offset;

    wgt = (v8u16)__msa_fill_w(weight);
    offset = (v8u16)__msa_fill_w(offset_in);
    denom = (v8u16)__msa_fill_h(log2_denom);

    for(cnt = (height >> 2); cnt--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        src += 4 * src_stride;
        ILVR_B4_UH(zero, src0, zero, src1, zero, src2, zero, src3, temp0, temp2,
                   temp4, temp6);
        ILVL_B4_UH(zero, src0, zero, src1, zero, src2, zero, src3, temp1, temp3,
                   temp5, temp7);
        MUL4(wgt, temp0, wgt, temp1, wgt, temp2, wgt, temp3, temp0, temp1,
             temp2, temp3);
        MUL4(wgt, temp4, wgt, temp5, wgt, temp6, wgt, temp7, temp4, temp5,
             temp6, temp7);
        ADDS_SH4_UH(temp0, offset, temp1, offset, temp2, offset, temp3, offset,
                    temp0, temp1, temp2, temp3);
        ADDS_SH4_UH(temp4, offset, temp5, offset, temp6, offset, temp7, offset,
                    temp4, temp5, temp6, temp7);
        MAXI_SH4_UH(temp0, temp1, temp2, temp3, 0);
        MAXI_SH4_UH(temp4, temp5, temp6, temp7, 0);
        SRL_H4_UH(temp0, temp1, temp2, temp3, denom);
        SRL_H4_UH(temp4, temp5, temp6, temp7, denom);
        SAT_UH4_UH(temp0, temp1, temp2, temp3, 7);
        SAT_UH4_UH(temp4, temp5, temp6, temp7, 7);
        PCKEV_B4_UB(temp1, temp0, temp3, temp2, temp5, temp4, temp7, temp6,
                    dst0, dst1, dst2, dst3);
        ST_UB4(dst0, dst1, dst2, dst3, dst, dst_stride);
        dst += 4 * dst_stride;
    }
}

static void avc_biwgt_interleaved_chroma_2x2_msa(UWORD8 *src1_in,
                                                 WORD32 src1_stride,
                                                 UWORD8 *src2_in,
                                                 WORD32 src2_stride,
                                                 UWORD8 *dst,
                                                 WORD32 dst_stride,
                                                 WORD32 log2_denom,
                                                 WORD32 src1_weight,
                                                 WORD32 src2_weight,
                                                 WORD32 offset_in)
{
    UWORD32 load0, load1, out0, out1;
    v16u8 in0, in1, in2, in3;
    v16i8 zero = { 0 };
    v8i16 temp0, temp1, temp2, temp3;
    v8i16 denom, offset, src1_wgt, src2_wgt;

    denom = __msa_fill_h(log2_denom);
    offset = (v8i16)__msa_fill_w(offset_in);
    src1_wgt = (v8i16)__msa_fill_w(src1_weight);
    src2_wgt = (v8i16)__msa_fill_w(src2_weight);

    load0 = LW(src1_in);
    load1 = LW(src1_in + src1_stride);
    in0 = (v16u8)__msa_fill_w(load0);
    in1 = (v16u8)__msa_fill_w(load1);
    load0 = LW(src2_in);
    load1 = LW(src2_in + src2_stride);
    in2 = (v16u8)__msa_fill_w(load0);
    in3 = (v16u8)__msa_fill_w(load1);
    ILVR_B4_SH(zero, in0, zero, in1, zero, in2, zero, in3, temp0, temp1, temp2,
               temp3);
    temp0 = (temp0 * src1_wgt) + (temp2 * src2_wgt) + offset;
    temp1 = (temp1 * src1_wgt) + (temp3 * src2_wgt) + offset;
    temp0 >>= denom;
    temp1 >>= denom;
    CLIP_SH2_0_255(temp0, temp1);
    PCKEV_B2_UB(temp0, temp0, temp1, temp1, in0, in1);
    out0 = __msa_copy_u_w((v4i32)in0, 0);
    out1 = __msa_copy_u_w((v4i32)in1, 0);
    SW(out0, dst);
    dst += dst_stride;
    SW(out1, dst);
}

static void avc_biwgt_interleaved_chroma_2x4_msa(UWORD8 *src1_in,
                                                 WORD32 src1_stride,
                                                 UWORD8 *src2_in,
                                                 WORD32 src2_stride,
                                                 UWORD8 *dst,
                                                 WORD32 dst_stride,
                                                 WORD32 log2_denom,
                                                 WORD32 src1_weight,
                                                 WORD32 src2_weight,
                                                 WORD32 offset_in)
{
    UWORD32 load0, load1, load2, load3;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 zero = { 0 };
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    v8i16 denom, offset, src1_wgt, src2_wgt;

    denom = __msa_fill_h(log2_denom);
    offset = (v8i16)__msa_fill_w(offset_in);
    src1_wgt = (v8i16)__msa_fill_w(src1_weight);
    src2_wgt = (v8i16)__msa_fill_w(src2_weight);

    LW4(src1_in, src1_stride, load0, load1, load2, load3);
    src0 = (v16u8)__msa_fill_w(load0);
    src1 = (v16u8)__msa_fill_w(load1);
    src2 = (v16u8)__msa_fill_w(load2);
    src3 = (v16u8)__msa_fill_w(load3);
    LW4(src2_in, src2_stride, load0, load1, load2, load3);
    src4 = (v16u8)__msa_fill_w(load0);
    src5 = (v16u8)__msa_fill_w(load1);
    src6 = (v16u8)__msa_fill_w(load2);
    src7 = (v16u8)__msa_fill_w(load3);
    ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3, temp0, temp1,
               temp2, temp3);
    ILVR_B4_SH(zero, src4, zero, src5, zero, src6, zero, src7, temp4, temp5,
               temp6, temp7);
    temp0 = (temp0 * src1_wgt) + (temp4 * src2_wgt) + offset;
    temp1 = (temp1 * src1_wgt) + (temp5 * src2_wgt) + offset;
    temp2 = (temp2 * src1_wgt) + (temp6 * src2_wgt) + offset;
    temp3 = (temp3 * src1_wgt) + (temp7 * src2_wgt) + offset;
    SRA_4V(temp0, temp1, temp2, temp3, denom);
    CLIP_SH4_0_255(temp0, temp1, temp2, temp3);
    PCKEV_ST4x4_UB(temp0, temp1, temp2, temp3, dst, dst_stride);
}

static void avc_biwgt_interleaved_chroma_2w_msa(UWORD8 *src1_in,
                                                WORD32 src1_stride,
                                                UWORD8 *src2_in,
                                                WORD32 src2_stride,
                                                UWORD8 *dst,
                                                WORD32 dst_stride,
                                                WORD32 height,
                                                WORD32 log2_denom,
                                                WORD32 src1_weight,
                                                WORD32 src2_weight,
                                                WORD32 offset_in)
{
    if(2 == height)
    {
        avc_biwgt_interleaved_chroma_2x2_msa(src1_in, src1_stride, src2_in,
                                             src2_stride, dst, dst_stride,
                                             log2_denom, src1_weight,
                                             src2_weight, offset_in);
    }
    else
    {
        avc_biwgt_interleaved_chroma_2x4_msa(src1_in, src1_stride, src2_in,
                                             src2_stride, dst, dst_stride,
                                             log2_denom, src1_weight,
                                             src2_weight, offset_in);
    }
}

static void avc_biwgt_interleaved_chroma_4w_msa(UWORD8 *src1_in,
                                                WORD32 src1_stride,
                                                UWORD8 *src2_in,
                                                WORD32 src2_stride,
                                                UWORD8 *dst,
                                                WORD32 dst_stride,
                                                WORD32 height,
                                                WORD32 log2_denom,
                                                WORD32 src1_weight,
                                                WORD32 src2_weight,
                                                WORD32 offset_in)
{
    UWORD8 cnt;
    UWORD64 out0, out1, out2, out3;
    v16u8 src0, src1, src2, src3, dst0, dst1, dst2, dst3;
    v16i8 zero = { 0 };
    v8i16 temp0, temp1, temp2, temp3, res0, res1, res2, res3;
    v8i16 denom, offset, src1_wgt, src2_wgt;

    denom = __msa_fill_h(log2_denom);
    offset = (v8i16)__msa_fill_w(offset_in);
    src1_wgt = (v8i16)__msa_fill_w(src1_weight);
    src2_wgt = (v8i16)__msa_fill_w(src2_weight);

    if(0 == (height % 4))
    {
        for(cnt = (height >> 2); cnt--;)
        {
            LD_UB4(src1_in, src1_stride, src0, src1, src2, src3);
            src1_in += (4 * src1_stride);
            LD_UB4(src2_in, src2_stride, dst0, dst1, dst2, dst3);
            src2_in += (4 * src2_stride);
            ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3, temp0,
                       temp1, temp2, temp3);
            ILVR_B4_SH(zero, dst0, zero, dst1, zero, dst2, zero, dst3, res0,
                       res1, res2, res3);
            res0 = (temp0 * src1_wgt) + (res0 * src2_wgt) + offset;
            res1 = (temp1 * src1_wgt) + (res1 * src2_wgt) + offset;
            res2 = (temp2 * src1_wgt) + (res2 * src2_wgt) + offset;
            res3 = (temp3 * src1_wgt) + (res3 * src2_wgt) + offset;
            SRA_4V(res0, res1, res2, res3, denom);
            CLIP_SH4_0_255(res0, res1, res2, res3);
            PCKEV_B4_UB(res0, res0, res1, res1, res2, res2, res3, res3, dst0,
                        dst1, dst2, dst3);
            out0 = __msa_copy_u_d((v2i64)dst0, 0);
            out1 = __msa_copy_u_d((v2i64)dst1, 0);
            out2 = __msa_copy_u_d((v2i64)dst2, 0);
            out3 = __msa_copy_u_d((v2i64)dst3, 0);
            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else
    {
        LD_UB2(src1_in, src1_stride, src0, src1);
        LD_UB2(src2_in, src2_stride, dst0, dst1);
        ILVR_B4_SH(zero, src0, zero, src1, zero, dst0, zero, dst1, temp0,
                   temp1, res0, res1);
        res0 = (temp0 * src1_wgt) + (res0 * src2_wgt) + offset;
        res1 = (temp1 * src1_wgt) + (res1 * src2_wgt) + offset;
        res0 >>= denom;
        res1 >>= denom;
        CLIP_SH2_0_255(res0, res1);
        PCKEV_B2_UB(res0, res0, res1, res1, dst0, dst1);
        out0 = __msa_copy_u_d((v2i64)dst0, 0);
        out1 = __msa_copy_u_d((v2i64)dst1, 0);
        SD(out0, dst);
        dst += dst_stride;
        SD(out1, dst);
    }
}

static void avc_biwgt_interleaved_chroma_8w_msa(UWORD8 *src1_in,
                                                WORD32 src1_stride,
                                                UWORD8 *src2_in,
                                                WORD32 src2_stride,
                                                UWORD8 *dst,
                                                WORD32 dst_stride,
                                                WORD32 height,
                                                WORD32 log2_denom,
                                                WORD32 src1_weight,
                                                WORD32 src2_weight,
                                                WORD32 offset_in)
{
    UWORD8 cnt;
    v16u8 src0, src1, src2, src3, dst0, dst1, dst2, dst3;
    v16i8 zero = { 0 };
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    v8i16 res0, res1, res2, res3, res4, res5, res6, res7;
    v8i16 denom, offset, src1_wgt, src2_wgt;

    denom = __msa_fill_h(log2_denom);
    offset = (v8i16)__msa_fill_w(offset_in);
    src1_wgt = (v8i16)__msa_fill_w(src1_weight);
    src2_wgt = (v8i16)__msa_fill_w(src2_weight);

    for(cnt = (height >> 2); cnt--;)
    {
        LD_UB4(src1_in, src1_stride, src0, src1, src2, src3);
        src1_in += (4 * src1_stride);
        LD_UB4(src2_in, src2_stride, dst0, dst1, dst2, dst3);
        src2_in += (4 * src2_stride);
        ILVRL_B2_SH(zero, src0, temp1, temp0);
        ILVRL_B2_SH(zero, src1, temp3, temp2);
        ILVRL_B2_SH(zero, src2, temp5, temp4);
        ILVRL_B2_SH(zero, src3, temp7, temp6);
        ILVRL_B2_SH(zero, dst0, res1, res0);
        ILVRL_B2_SH(zero, dst1, res3, res2);
        ILVRL_B2_SH(zero, dst2, res5, res4);
        ILVRL_B2_SH(zero, dst3, res7, res6);
        res0 = (temp0 * src1_wgt) + (res0 * src2_wgt) + offset;
        res1 = (temp1 * src1_wgt) + (res1 * src2_wgt) + offset;
        res2 = (temp2 * src1_wgt) + (res2 * src2_wgt) + offset;
        res3 = (temp3 * src1_wgt) + (res3 * src2_wgt) + offset;
        res4 = (temp4 * src1_wgt) + (res4 * src2_wgt) + offset;
        res5 = (temp5 * src1_wgt) + (res5 * src2_wgt) + offset;
        res6 = (temp6 * src1_wgt) + (res6 * src2_wgt) + offset;
        res7 = (temp7 * src1_wgt) + (res7 * src2_wgt) + offset;
        SRA_4V(res0, res1, res2, res3, denom);
        SRA_4V(res4, res5, res6, res7, denom);
        CLIP_SH4_0_255(res0, res1, res2, res3);
        CLIP_SH4_0_255(res4, res5, res6, res7);
        PCKEV_B4_UB(res0, res1, res2, res3, res4, res5, res6, res7, dst0, dst1,
                    dst2, dst3);
        ST_UB4(dst0, dst1, dst2, dst3, dst, dst_stride);
        dst += 4 * dst_stride;
    }
}

void ih264_default_weighted_pred_luma_msa(UWORD8 *src1, UWORD8 *src2,
                                          UWORD8 *dst, WORD32 src1_stride,
                                          WORD32 src2_stride, WORD32 dst_stride,
                                          WORD32 height, WORD32 width)
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

void ih264_default_weighted_pred_chroma_msa(UWORD8 *src1, UWORD8 *src2,
                                            UWORD8 *dst, WORD32 src1_stride,
                                            WORD32 src2_stride,
                                            WORD32 dst_stride, WORD32 height,
                                            WORD32 width)
{
    if(2 == width)
    {
        avg_src_width4_msa(src1, src1_stride, src2, src2_stride, dst,
                           dst_stride, height);
    }
    else if(4 == width)
    {
        avg_src_width8_msa(src1, src1_stride, src2, src2_stride, dst,
                           dst_stride, height);
    }
    else if(8 == width)
    {
        avg_src_width16_msa(src1, src1_stride, src2, src2_stride, dst,
                            dst_stride, height);
    }
}

void ih264_weighted_pred_luma_msa(UWORD8 *src, UWORD8 *dst, WORD32 src_stride,
                                  WORD32 dst_stride, WORD32 log_wd,
                                  WORD32 weight, WORD32 offset, WORD32 height,
                                  WORD32 width)
{
    weight = (WORD16)(weight & 0xffff);
    offset = (WORD8)(offset & 0xff);

    offset <<= log_wd;

    if(log_wd)
    {
        offset += (1 << (log_wd - 1));
    }

    if(4 == width)
    {
        avc_wgt_luma_4w_msa(src, src_stride, dst, dst_stride, height, log_wd,
                            weight, offset);
    }
    else if(8 == width)
    {
        avc_wgt_luma_8w_msa(src, src_stride, dst, dst_stride, height, log_wd,
                            weight, offset);
    }
    else if(16 == width)
    {
        avc_wgt_luma_16w_msa(src, src_stride, dst, dst_stride, height, log_wd,
                             weight, offset);
    }
}

void ih264_weighted_pred_chroma_msa(UWORD8 *src, UWORD8 *dst, WORD32 src_stride,
                                    WORD32 dst_stride, WORD32 log2_denom,
                                    WORD32 weight, WORD32 offset, WORD32 height,
                                    WORD32 width)
{
    WORD32 offset_in, offset_in_u, offset_in_v;

    offset_in_u = (WORD8)(offset & 0xff);
    offset_in_v = (WORD8)(offset >> 8);

    offset_in_u <<= log2_denom;
    offset_in_v <<= log2_denom;

    if(log2_denom)
    {
        offset_in_u += (1 << (log2_denom - 1));
        offset_in_v += (1 << (log2_denom - 1));
    }

    offset_in = ((offset_in_v << 16) & 0xffff0000) | offset_in_u;

    if(2 == width)
    {
        avc_wgt_interleaved_chroma_2w_msa(src, src_stride, dst, dst_stride,
                                          height, log2_denom, weight,
                                          offset_in);
    }
    else if(4 == width)
    {
        avc_wgt_interleaved_chroma_4w_msa(src, src_stride, dst, dst_stride,
                                          height, log2_denom, weight,
                                          offset_in);
    }
    else if(8 == width)
    {
        avc_wgt_interleaved_chroma_8w_msa(src, src_stride, dst, dst_stride,
                                          height, log2_denom, weight,
                                          offset_in);
    }
}

void ih264_weighted_bi_pred_luma_msa(UWORD8 *src1, UWORD8 *src2,
                                     UWORD8 *dst, WORD32 src1_stride,
                                     WORD32 src2_stride, WORD32 dst_stride,
                                     WORD32 log_wd, WORD32 weight1,
                                     WORD32 weight2, WORD32 offset1,
                                     WORD32 offset2, WORD32 height,
                                     WORD32 width)
{
    WORD32 log2_denom, offset;

    offset1 = (WORD8)(offset1 & 0xff);
    offset2 = (WORD8)(offset2 & 0xff);
    weight1 = (WORD16)(weight1 & 0xffff);
    weight2 = (WORD16)(weight2 & 0xffff);
    offset = (offset1 + offset2 + 1) >> 1;

    log2_denom = log_wd + 1;
    offset = (1 << log_wd) + (offset << log2_denom);

    if(4 == width)
    {
        avc_biwgt_luma_4w_msa(src1, src1_stride, src2, src2_stride, dst,
                              dst_stride, height, log2_denom, weight1, weight2,
                              offset);
    }
    else if(8 == width)
    {
        avc_biwgt_luma_8w_msa(src1, src1_stride, src2, src2_stride, dst,
                              dst_stride, height, log2_denom, weight1, weight2,
                              offset);
    }
    else if(16 == width)
    {
        avc_biwgt_luma_16w_msa(src1, src1_stride, src2, src2_stride, dst,
                               dst_stride, height, log2_denom, weight1, weight2,
                               offset);
    }
}

void ih264_weighted_bi_pred_chroma_msa(UWORD8 *src1, UWORD8 *src2,
                                       UWORD8 *dst, WORD32 src1_stride,
                                       WORD32 src2_stride, WORD32 dst_stride,
                                       WORD32 log_wd, WORD32 weight1,
                                       WORD32 weight2, WORD32 offset1,
                                       WORD32 offset2, WORD32 height,
                                       WORD32 width)
{
    WORD32 weight1_u, weight1_v, weight2_u, weight2_v;
    WORD32 offset1_u, offset1_v, offset2_u, offset2_v, offset_u, offset_v;
    WORD32 offset_in, src1_weight, src2_weight, log2_denom;

    offset1_u = (WORD8)(offset1 & 0xff);
    offset1_v = (WORD8)(offset1 >> 8);
    offset2_u = (WORD8)(offset2 & 0xff);
    offset2_v = (WORD8)(offset2 >> 8);
    weight1_u = (WORD16)(weight1 & 0xffff);
    weight1_v = (WORD16)(weight1 >> 16);
    weight2_u = (WORD16)(weight2 & 0xffff);
    weight2_v = (WORD16)(weight2 >> 16);
    offset_u = (offset1_u + offset2_u + 1) >> 1;
    offset_v = (offset1_v + offset2_v + 1) >> 1;

    log2_denom = log_wd + 1;
    offset_u = (1 << log_wd) + (offset_u << log2_denom);
    offset_v = (1 << log_wd) + (offset_v << log2_denom);

    src1_weight = ((weight1_v << 16) & 0xffff0000) | weight1_u;
    src2_weight = ((weight2_v << 16) & 0xffff0000) | weight2_u;
    offset_in = ((offset_v << 16) & 0xffff0000) | offset_u;

    if(2 == width)
    {
        avc_biwgt_interleaved_chroma_2w_msa(src1, src1_stride, src2,
                                            src2_stride, dst, dst_stride,
                                            height, log2_denom, src1_weight,
                                            src2_weight, offset_in);
    }
    else if(4 == width)
    {
        avc_biwgt_interleaved_chroma_4w_msa(src1, src1_stride, src2,
                                            src2_stride, dst, dst_stride,
                                            height, log2_denom, src1_weight,
                                            src2_weight, offset_in);
    }
    else if(8 == width)
    {
        avc_biwgt_interleaved_chroma_8w_msa(src1, src1_stride, src2,
                                            src2_stride, dst, dst_stride,
                                            height, log2_denom, src1_weight,
                                            src2_weight, offset_in);
    }
}
