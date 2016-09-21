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

static void avc_interleaved_chroma_hz_2x2_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              UWORD32 hz_coeff0,
                                              UWORD32 hz_coeff1)
{
    v16u8 src0, src1, src2, src3;
    v16i8 mask = { 0, 2, 2, 4, 16, 18, 18, 20, 0, 2, 2, 4, 16, 18, 18, 20 };
    v16i8 tmp0, tmp1, out, mask1;
    v8u16 res0, res1;
    v16i8 coeff0 = __msa_fill_b(hz_coeff0);
    v16i8 coeff1 = __msa_fill_b(hz_coeff1);
    v16u8 coeff = (v16u8)__msa_ilvr_b(coeff0, coeff1);

    mask1 = ADDVI_B(mask, 1);

    LD_UB2(src, src_stride, src0, src1);
    VSHF_B2_UB(src0, src1, src0, src1, mask, mask1, src2, src3);
    DOTP_UB2_UH(src2, src3, coeff, coeff, res0, res1);
    SLLI_H2_UH(res0, res1, 3);
    SRARI_H2_UH(res0, res1, 6);
    SAT_UH2_UH(res0, res1, 7);
    PCKEV_B2_SB(res0, res0, res1, res1, tmp0, tmp1);

    out = __msa_ilvr_b(tmp1, tmp0);

    ST4x2_UB(out, dst, dst_stride);
}

static void avc_interleaved_chroma_hz_2x4_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              UWORD32 hz_coeff0,
                                              UWORD32 hz_coeff1)
{
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 mask = { 0, 2, 2, 4, 16, 18, 18, 20, 0, 2, 2, 4, 16, 18, 18, 20 };
    v16i8 tmp0, tmp1, out, mask1;
    v8u16 res0, res1;
    v16i8 coeff0 = __msa_fill_b(hz_coeff0);
    v16i8 coeff1 = __msa_fill_b(hz_coeff1);
    v16u8 coeff = (v16u8)__msa_ilvr_b(coeff0, coeff1);

    mask1 = ADDVI_B(mask, 1);

    LD_UB4(src, src_stride, src0, src1, src2, src3);
    VSHF_B2_UB(src0, src1, src2, src3, mask, mask, src4, src5);
    VSHF_B2_UB(src0, src1, src2, src3, mask1, mask1, src6, src7);
    ILVR_D2_UB(src5, src4, src7, src6, src4, src6);
    DOTP_UB2_UH(src4, src6, coeff, coeff, res0, res1);
    SLLI_H2_UH(res0, res1, 3);
    SRARI_H2_UH(res0, res1, 6);
    SAT_UH2_UH(res0, res1, 7);
    PCKEV_B2_SB(res0, res0, res1, res1, tmp0, tmp1);
    out = (v16i8)__msa_ilvr_b(tmp1, tmp0);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
}

static void avc_interleaved_chroma_hz_2w_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride,
                                             UWORD32 hz_coeff0,
                                             UWORD32 hz_coeff1,
                                             WORD32 height)
{
    if(2 == height)
    {
        avc_interleaved_chroma_hz_2x2_msa(src, src_stride, dst, dst_stride,
                                          hz_coeff0, hz_coeff1);
    }
    else
    {
        avc_interleaved_chroma_hz_2x4_msa(src, src_stride, dst, dst_stride,
                                          hz_coeff0, hz_coeff1);
    }
}

static void avc_interleaved_chroma_hz_4x2_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              UWORD32 hz_coeff0,
                                              UWORD32 hz_coeff1)
{
    v16u8 src0, src1, src2, src3;
    v16i8 mask = { 0, 2, 2, 4, 4, 6, 6, 8, 16, 18, 18, 20, 20, 22, 22, 24 };
    v16i8 tmp0, tmp1, out, mask1;
    v8u16 res0, res1;
    v16i8 coeff0 = __msa_fill_b(hz_coeff0);
    v16i8 coeff1 = __msa_fill_b(hz_coeff1);
    v16u8 coeff = (v16u8)__msa_ilvr_b(coeff0, coeff1);

    mask1 = ADDVI_B(mask, 1);

    LD_UB2(src, src_stride, src0, src1);
    VSHF_B2_UB(src0, src1, src0, src1, mask, mask1, src2, src3);
    DOTP_UB2_UH(src2, src3, coeff, coeff, res0, res1);
    SLLI_H2_UH(res0, res1, 3);
    SRARI_H2_UH(res0, res1, 6);
    SAT_UH2_UH(res0, res1, 7);
    PCKEV_B2_SB(res0, res0, res1, res1, tmp0, tmp1);

    out = __msa_ilvr_b(tmp1, tmp0);

    ST8x2_UB(out, dst, dst_stride);
}

static void avc_interleaved_chroma_hz_4x4mul_msa(UWORD8 *src, WORD32 src_stride,
                                                 UWORD8 *dst, WORD32 dst_stride,
                                                 UWORD32 hz_coeff0,
                                                 UWORD32 hz_coeff1,
                                                 WORD32 height)
{
    UWORD32 row;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7, out0, out1;
    v16i8 mask0 = { 0, 2, 2, 4, 4, 6, 6, 8, 16, 18, 18, 20, 20, 22, 22, 24 };
    v16i8 tmp0, tmp1, mask1;
    v8u16 res0, res1, res2, res3;
    v16i8 coeff0 = __msa_fill_b(hz_coeff0);
    v16i8 coeff1 = __msa_fill_b(hz_coeff1);
    v16u8 coeff = (v16u8)__msa_ilvr_b(coeff0, coeff1);

    mask1 = ADDVI_B(mask0, 1);

    for(row = (height >> 2); row--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        VSHF_B2_UB(src0, src1, src2, src3, mask0, mask0, src4, src5);
        VSHF_B2_UB(src0, src1, src2, src3, mask1, mask1, src6, src7);
        DOTP_UB4_UH(src4, src5, src6, src7, coeff, coeff, coeff, coeff, res0,
                    res1, res2, res3);
        SLLI_H4_UH(res0, res1, res2, res3, 3);
        SRARI_H4_UH(res0, res1, res2, res3, 6);
        SAT_UH4_UH(res0, res1, res2, res3, 7);
        PCKEV_B2_SB(res1, res0, res3, res2, tmp0, tmp1);
        ILVRL_B2_UB(tmp1, tmp0, out0, out1);
        ST8x4_UB(out0, out1, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void avc_interleaved_chroma_hz_4w_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride,
                                             UWORD32 hz_coeff0,
                                             UWORD32 hz_coeff1,
                                             WORD32 height)
{
    if(2 == height)
    {
        avc_interleaved_chroma_hz_4x2_msa(src, src_stride, dst, dst_stride,
                                          hz_coeff0, hz_coeff1);
    }
    else
    {
        avc_interleaved_chroma_hz_4x4mul_msa(src, src_stride, dst, dst_stride,
                                             hz_coeff0, hz_coeff1, height);
    }
}

static void avc_interleaved_chroma_hz_8w_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride,
                                             UWORD32 hz_coeff0,
                                             UWORD32 hz_coeff1,
                                             WORD32 height)
{
    UWORD32 row;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16u8 src11, src12, src13, src14, src15, out0, out1, out2, out3;
    v16i8 mask0 = { 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14, 16 };
    v16i8 tmp0, tmp1, tmp2, tmp3, mask1;
    v8u16 res0, res1, res2, res3, res4, res5, res6, res7;
    v16i8 coeff0 = __msa_fill_b(hz_coeff0);
    v16i8 coeff1 = __msa_fill_b(hz_coeff1);
    v16u8 coeff = (v16u8)__msa_ilvr_b(coeff0, coeff1);

    mask1 = ADDVI_B(mask0, 1);

    for(row = (height >> 2); row--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        LD_UB4(src + 16, src_stride, src4, src5, src6, src7);
        src += (4 * src_stride);

        VSHF_B2_UB(src0, src4, src1, src5, mask0, mask0, src8, src9);
        VSHF_B2_UB(src2, src6, src3, src7, mask0, mask0, src10, src11);
        VSHF_B2_UB(src0, src4, src1, src5, mask1, mask1, src12, src13);
        VSHF_B2_UB(src2, src6, src3, src7, mask1, mask1, src14, src15);
        DOTP_UB4_UH(src8, src9, src10, src11, coeff, coeff, coeff, coeff, res0,
                    res1, res2, res3);
        DOTP_UB4_UH(src12, src13, src14, src15, coeff, coeff, coeff, coeff,
                    res4, res5, res6, res7);
        SLLI_H4_UH(res0, res1, res2, res3, 3);
        SLLI_H4_UH(res4, res5, res6, res7, 3);
        SRARI_H4_UH(res0, res1, res2, res3, 6);
        SRARI_H4_UH(res4, res5, res6, res7, 6);
        SAT_UH4_UH(res0, res1, res2, res3, 7);
        SAT_UH4_UH(res4, res5, res6, res7, 7);
        PCKEV_B2_SB(res1, res0, res3, res2, tmp0, tmp1);
        PCKEV_B2_SB(res5, res4, res7, res6, tmp2, tmp3);
        ILVRL_B2_UB(tmp2, tmp0, out0, out1);
        ILVRL_B2_UB(tmp3, tmp1, out2, out3);
        ST_UB4(out0, out1, out2, out3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void avc_interleaved_chroma_vt_2x2_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              UWORD32 vt_coeff0,
                                              UWORD32 vt_coeff1)
{
    UWORD32 out0, out1;
    v16u8 src0, src1, src2, src0_r, src1_r;
    v16i8 out;
    v8u16 res0, res1;
    v16i8 coeff0 = __msa_fill_b(vt_coeff0);
    v16i8 coeff1 = __msa_fill_b(vt_coeff1);
    v16u8 coeff = (v16u8)__msa_ilvr_b(coeff0, coeff1);

    LD_UB3(src, src_stride, src0, src1, src2);
    ILVR_B2_UB(src1, src0, src2, src1, src0_r, src1_r);
    DOTP_UB2_UH(src0_r, src1_r, coeff, coeff, res0, res1);
    SLLI_H2_UH(res0, res1, 3);
    SRARI_H2_UH(res0, res1, 6);
    SAT_UH2_UH(res0, res1, 7);

    out = __msa_pckev_b((v16i8)res1, (v16i8)res0);

    out0 = __msa_copy_u_w((v4i32)out, 0);
    out1 = __msa_copy_u_w((v4i32)out, 2);

    SW(out0, dst);
    dst += dst_stride;
    SW(out1, dst);
}

static void avc_interleaved_chroma_vt_2x4_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              UWORD32 vt_coeff0,
                                              UWORD32 vt_coeff1)
{
    v16u8 src0, src1, src2, src3, src4, out0, out1;
    v16u8 src0_r, src1_r, src2_r, src3_r;
    v8u16 res0, res1, res2, res3;
    v16i8 coeff0 = __msa_fill_b(vt_coeff0);
    v16i8 coeff1 = __msa_fill_b(vt_coeff1);
    v16u8 coeff = (v16u8)__msa_ilvr_b(coeff0, coeff1);

    LD_UB5(src, src_stride, src0, src1, src2, src3, src4);
    ILVR_B4_UB(src1, src0, src2, src1, src3, src2, src4, src3, src0_r, src1_r,
               src2_r, src3_r);
    DOTP_UB4_UH(src0_r, src1_r, src2_r, src3_r, coeff, coeff, coeff, coeff,
                res0, res1, res2, res3);
    SLLI_H4_UH(res0, res1, res2, res3, 3);
    SRARI_H4_UH(res0, res1, res2, res3, 6);
    SAT_UH4_UH(res0, res1, res2, res3, 7);
    PCKEV_B2_UB(res1, res0, res3, res2, out0, out1);
    ST4x4_UB(out0, out1, 0, 2, 0, 2, dst, dst_stride);
}

static void avc_interleaved_chroma_vt_2w_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride,
                                             UWORD32 vt_coeff0,
                                             UWORD32 vt_coeff1,
                                             WORD32 height)
{
    if(2 == height)
    {
        avc_interleaved_chroma_vt_2x2_msa(src, src_stride, dst, dst_stride,
                                          vt_coeff0, vt_coeff1);
    }
    else if(4 == height)
    {
        avc_interleaved_chroma_vt_2x4_msa(src, src_stride, dst, dst_stride,
                                          vt_coeff0, vt_coeff1);
    }
}

static void avc_interleaved_chroma_vt_4x2_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              UWORD32 vt_coeff0,
                                              UWORD32 vt_coeff1)
{
    v16u8 src0, src1, src2, src3, src4, out0, out1;
    v16u8 src0_r, src1_r, src2_r, src3_r;
    v8u16 res0, res1, res2, res3;
    v16i8 coeff0 = __msa_fill_b(vt_coeff0);
    v16i8 coeff1 = __msa_fill_b(vt_coeff1);
    v16u8 coeff = (v16u8)__msa_ilvr_b(coeff0, coeff1);

    LD_UB5(src, src_stride, src0, src1, src2, src3, src4);
    ILVR_B4_UB(src1, src0, src2, src1, src3, src2, src4, src3, src0_r, src1_r,
               src2_r, src3_r);
    DOTP_UB4_UH(src0_r, src1_r, src2_r, src3_r, coeff, coeff, coeff, coeff,
                res0, res1, res2, res3);
    SLLI_H4_UH(res0, res1, res2, res3, 3);
    SRARI_H4_UH(res0, res1, res2, res3, 6);
    SAT_UH4_UH(res0, res1, res2, res3, 7);
    PCKEV_B2_UB(res1, res0, res3, res2, out0, out1);
    ST8x4_UB(out0, out1, dst, dst_stride);
}

static void avc_interleaved_chroma_vt_4x4mul_msa(UWORD8 *src, WORD32 src_stride,
                                                 UWORD8 *dst, WORD32 dst_stride,
                                                 UWORD32 vt_coeff0,
                                                 UWORD32 vt_coeff1,
                                                 WORD32 height)
{
    UWORD32 row;
    v16u8 src0, src1, src2, src3, src4, out0, out1;
    v16u8 src0_r, src1_r, src2_r, src3_r;
    v8u16 res0, res1, res2, res3;
    v16i8 coeff0 = __msa_fill_b(vt_coeff0);
    v16i8 coeff1 = __msa_fill_b(vt_coeff1);
    v16u8 coeff = (v16u8)__msa_ilvr_b(coeff0, coeff1);

    src0 = LD_UB(src);
    src += src_stride;

    for(row = height >> 2; row--;)
    {
        LD_UB4(src, src_stride, src1, src2, src3, src4);
        src += (4 * src_stride);
        ILVR_B4_UB(src1, src0, src2, src1, src3, src2, src4, src3, src0_r,
                   src1_r, src2_r, src3_r);
        DOTP_UB4_UH(src0_r, src1_r, src2_r, src3_r, coeff, coeff, coeff, coeff,
                    res0, res1, res2, res3);
        SLLI_H4_UH(res0, res1, res2, res3, 3);
        SRARI_H4_UH(res0, res1, res2, res3, 6);
        SAT_UH4_UH(res0, res1, res2, res3, 7);
        PCKEV_B2_UB(res1, res0, res3, res2, out0, out1);
        ST8x4_UB(out0, out1, dst, dst_stride);
        dst += (4 * dst_stride);

        src0 = src4;
    }
}

static void avc_interleaved_chroma_vt_4w_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride,
                                             UWORD32 vt_coeff0,
                                             UWORD32 vt_coeff1,
                                             WORD32 height)
{
    if(2 == height)
    {
        avc_interleaved_chroma_vt_4x2_msa(src, src_stride, dst, dst_stride,
                                          vt_coeff0, vt_coeff1);
    }
    else
    {
        avc_interleaved_chroma_vt_4x4mul_msa(src, src_stride, dst, dst_stride,
                                             vt_coeff0, vt_coeff1, height);
    }
}

static void avc_interleaved_chroma_vt_8w_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride,
                                             UWORD32 vt_coeff0,
                                             UWORD32 vt_coeff1,
                                             WORD32 height)
{
    UWORD32 row;
    v16u8 src0, src1, src2, src3, src4, out0, out1, out2, out3;
    v16u8 tmp0, tmp1, tmp2, tmp3;
    v16u8 src0_r, src1_r, src2_r, src3_r, src0_l, src1_l, src2_l, src3_l;
    v8u16 res0, res1, res2, res3, res4, res5, res6, res7;
    v16i8 coeff0 = __msa_fill_b(vt_coeff0);
    v16i8 coeff1 = __msa_fill_b(vt_coeff1);
    v16u8 coeff = (v16u8)__msa_ilvr_b(coeff0, coeff1);

    src0 = LD_UB(src);
    src += src_stride;

    for(row = height >> 2; row--;)
    {
        LD_UB4(src, src_stride, src1, src2, src3, src4);
        src += (4 * src_stride);

        ILVRL_B2_UB(src1, src0, src0_r, src0_l);
        ILVRL_B2_UB(src2, src1, src1_r, src1_l);
        ILVRL_B2_UB(src3, src2, src2_r, src2_l);
        ILVRL_B2_UB(src4, src3, src3_r, src3_l);
        DOTP_UB4_UH(src0_r, src1_r, src2_r, src3_r, coeff, coeff, coeff, coeff,
                    res0, res1, res2, res3);
        DOTP_UB4_UH(src0_l, src1_l, src2_l, src3_l, coeff, coeff, coeff, coeff,
                    res4, res5, res6, res7);
        SLLI_H4_UH(res0, res1, res2, res3, 3);
        SLLI_H4_UH(res4, res5, res6, res7, 3);
        SRARI_H4_UH(res0, res1, res2, res3, 6);
        SRARI_H4_UH(res4, res5, res6, res7, 6);
        SAT_UH4_UH(res0, res1, res2, res3, 7);
        SAT_UH4_UH(res4, res5, res6, res7, 7);
        PCKEV_B2_UB(res1, res0, res3, res2, tmp0, tmp1);
        PCKEV_B2_UB(res5, res4, res7, res6, tmp2, tmp3);

        out0 = (v16u8)__msa_ilvr_d((v2i64)tmp2, (v2i64)tmp0);
        out1 = (v16u8)__msa_ilvl_d((v2i64)tmp2, (v2i64)tmp0);
        out2 = (v16u8)__msa_ilvr_d((v2i64)tmp3, (v2i64)tmp1);
        out3 = (v16u8)__msa_ilvl_d((v2i64)tmp3, (v2i64)tmp1);

        ST_UB4(out0, out1, out2, out3, dst, dst_stride);
        dst += (4 * dst_stride);

        src0 = src4;
    }
}

static void avc_interleaved_chroma_hv_2x2_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              UWORD32 hz_coef0,
                                              UWORD32 hz_coef1,
                                              UWORD32 vt_coef0,
                                              UWORD32 vt_coef1)
{
    UWORD32 out0, out1;
    v16u8 src0, src1, src2, src3, src4;
    v8u16 res_hz0, res_hz1, res_hz2, res_hz3;
    v8u16 res_vt0, res_vt1, res_vt2, res_vt3;
    v16i8 mask0 = { 0, 2, 2, 4, 4, 6, 6, 8, 16, 18, 18, 20, 20, 22, 22, 24 };
    v16i8 mask1, res0, res1, out;
    v16i8 coeff_hz0 = __msa_fill_b(hz_coef0);
    v16i8 coeff_hz1 = __msa_fill_b(hz_coef1);
    v16u8 coeff_hz = (v16u8)__msa_ilvr_b(coeff_hz0, coeff_hz1);
    v8u16 coeff_vt0 = (v8u16)__msa_fill_h(vt_coef0);
    v8u16 coeff_vt1 = (v8u16)__msa_fill_h(vt_coef1);

    mask1 = ADDVI_B(mask0, 1);

    LD_UB3(src, src_stride, src0, src1, src2);
    VSHF_B2_UB(src0, src1, src1, src2, mask1, mask1, src3, src4);
    VSHF_B2_UB(src0, src1, src1, src2, mask0, mask0, src0, src1);
    DOTP_UB4_UH(src0, src1, src3, src4, coeff_hz, coeff_hz, coeff_hz, coeff_hz,
                res_hz0, res_hz1, res_hz2, res_hz3);
    MUL4(res_hz0, coeff_vt1, res_hz1, coeff_vt0, res_hz2, coeff_vt1, res_hz3,
         coeff_vt0, res_vt0, res_vt1, res_vt2, res_vt3);
    ADD2(res_vt0, res_vt1, res_vt2, res_vt3, res_vt0, res_vt2);
    SRARI_H2_UH(res_vt0, res_vt2, 6);
    SAT_UH2_UH(res_vt0, res_vt2, 7);
    PCKEV_B2_SB(res_vt0, res_vt0, res_vt2, res_vt2, res0, res1);

    out = __msa_ilvr_b(res1, res0);

    out0 = __msa_copy_u_w((v4i32)out, 0);
    out1 = __msa_copy_u_w((v4i32)out, 2);

    SW(out0, dst);
    dst += dst_stride;
    SW(out1, dst);
}

static void avc_interleaved_chroma_hv_2x4_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              UWORD32 hz_coef0,
                                              UWORD32 hz_coef1,
                                              UWORD32 vt_coef0,
                                              UWORD32 vt_coef1)
{
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v16u8 out0, out1;
    v8u16 res_hz0, res_hz1, res_hz2, res_hz3;
    v8u16 res_vt0, res_vt1, res_vt2, res_vt3;
    v16i8 mask0 = { 0, 2, 2, 4, 4, 6, 6, 8, 16, 18, 18, 20, 20, 22, 22, 24 };
    v16i8 mask1;
    v8i16 res0, res1, res2, res3;
    v16i8 coeff_hz0 = __msa_fill_b(hz_coef0);
    v16i8 coeff_hz1 = __msa_fill_b(hz_coef1);
    v16u8 coeff_hz = (v16u8)__msa_ilvr_b(coeff_hz0, coeff_hz1);
    v8u16 coeff_vt0 = (v8u16)__msa_fill_h(vt_coef0);
    v8u16 coeff_vt1 = (v8u16)__msa_fill_h(vt_coef1);

    mask1 = ADDVI_B(mask0, 1);

    LD_UB5(src, src_stride, src0, src1, src2, src3, src4);
    VSHF_B2_UB(src0, src1, src1, src2, mask1, mask1, src5, src6);
    VSHF_B2_UB(src2, src3, src3, src4, mask1, mask1, src7, src8);
    VSHF_B2_UB(src0, src1, src1, src2, mask0, mask0, src0, src1);
    VSHF_B2_UB(src2, src3, src3, src4, mask0, mask0, src2, src3);
    DOTP_UB4_UH(src0, src1, src2, src3, coeff_hz, coeff_hz, coeff_hz, coeff_hz,
                res_hz0, res_hz1, res_hz2, res_hz3);
    MUL4(res_hz0, coeff_vt1, res_hz1, coeff_vt0, res_hz2, coeff_vt1, res_hz3,
         coeff_vt0, res_vt0, res_vt1, res_vt2, res_vt3);
    ADD2(res_vt0, res_vt1, res_vt2, res_vt3, res_vt0, res_vt1);
    SRARI_H2_UH(res_vt0, res_vt1, 6);
    SAT_UH2_UH(res_vt0, res_vt1, 7);
    PCKEV_B2_SH(res_vt0, res_vt0, res_vt1, res_vt1, res0, res1);
    DOTP_UB4_UH(src5, src6, src7, src8, coeff_hz, coeff_hz, coeff_hz, coeff_hz,
                res_hz0, res_hz1, res_hz2, res_hz3);
    MUL4(res_hz0, coeff_vt1, res_hz1, coeff_vt0, res_hz2, coeff_vt1, res_hz3,
         coeff_vt0, res_vt0, res_vt1, res_vt2, res_vt3);
    ADD2(res_vt0, res_vt1, res_vt2, res_vt3, res_vt0, res_vt1);
    SRARI_H2_UH(res_vt0, res_vt1, 6);
    SAT_UH2_UH(res_vt0, res_vt1, 7);
    PCKEV_B2_SH(res_vt0, res_vt0, res_vt1, res_vt1, res2, res3);
    ILVR_B2_UB(res2, res0, res3, res1, out0, out1);
    ST4x4_UB(out0, out1, 0, 2, 0, 2, dst, dst_stride);
}

static void avc_interleaved_chroma_hv_2w_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride,
                                             UWORD32 hz_coef0, UWORD32 hz_coef1,
                                             UWORD32 vt_coef0, UWORD32 vt_coef1,
                                             WORD32 height)
{
    if(2 == height)
    {
        avc_interleaved_chroma_hv_2x2_msa(src, src_stride, dst, dst_stride,
                                          hz_coef0, hz_coef1, vt_coef0,
                                          vt_coef1);
    }
    else if(4 == height)
    {
        avc_interleaved_chroma_hv_2x4_msa(src, src_stride, dst, dst_stride,
                                          hz_coef0, hz_coef1, vt_coef0,
                                          vt_coef1);
    }
}

static void avc_interleaved_chroma_hv_4x2_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              UWORD32 hz_coef0,
                                              UWORD32 hz_coef1,
                                              UWORD32 vt_coef0,
                                              UWORD32 vt_coef1)
{
    v16u8 src0, src1, src2, src3, src4;
    v8u16 res_hz0, res_hz1, res_hz2, res_hz3;
    v8u16 res_vt0, res_vt1, res_vt2, res_vt3;
    v16i8 mask0 = { 0, 2, 2, 4, 4, 6, 6, 8, 16, 18, 18, 20, 20, 22, 22, 24 };
    v16i8 mask1, res0, res1, out;
    v16i8 coeff_hz0 = __msa_fill_b(hz_coef0);
    v16i8 coeff_hz1 = __msa_fill_b(hz_coef1);
    v16u8 coeff_hz = (v16u8)__msa_ilvr_b(coeff_hz0, coeff_hz1);
    v8u16 coeff_vt0 = (v8u16)__msa_fill_h(vt_coef0);
    v8u16 coeff_vt1 = (v8u16)__msa_fill_h(vt_coef1);

    mask1 = ADDVI_B(mask0, 1);

    LD_UB3(src, src_stride, src0, src1, src2);
    VSHF_B2_UB(src0, src1, src1, src2, mask1, mask1, src3, src4);
    VSHF_B2_UB(src0, src1, src1, src2, mask0, mask0, src0, src1);
    DOTP_UB4_UH(src0, src1, src3, src4, coeff_hz, coeff_hz, coeff_hz, coeff_hz,
                res_hz0, res_hz1, res_hz2, res_hz3);
    MUL4(res_hz0, coeff_vt1, res_hz1, coeff_vt0, res_hz2, coeff_vt1, res_hz3,
         coeff_vt0, res_vt0, res_vt1, res_vt2, res_vt3);
    ADD2(res_vt0, res_vt1, res_vt2, res_vt3, res_vt0, res_vt2);
    SRARI_H2_UH(res_vt0, res_vt2, 6);
    SAT_UH2_UH(res_vt0, res_vt2, 7);
    PCKEV_B2_SB(res_vt0, res_vt0, res_vt2, res_vt2, res0, res1);

    out = __msa_ilvr_b(res1, res0);

    ST8x2_UB(out, dst, dst_stride);
}

static void avc_interleaved_chroma_hv_4x4mul_msa(UWORD8 *src, WORD32 src_stride,
                                                 UWORD8 *dst, WORD32 dst_stride,
                                                 UWORD32 hz_coef0,
                                                 UWORD32 hz_coef1,
                                                 UWORD32 vt_coef0,
                                                 UWORD32 vt_coef1,
                                                 WORD32 height)
{
    UWORD32 row;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v8u16 res_hz0, res_hz1, res_hz2, res_hz3;
    v8u16 res_vt0, res_vt1, res_vt2, res_vt3;
    v8u16 out_vt0, out_vt1, out_vt2, out_vt3;
    v16i8 mask0 = { 0, 2, 2, 4, 4, 6, 6, 8, 16, 18, 18, 20, 20, 22, 22, 24 };
    v16i8 mask1, res0, res1, res2, res3;
    v16u8 out0, out1;
    v16i8 coeff_hz0 = __msa_fill_b(hz_coef0);
    v16i8 coeff_hz1 = __msa_fill_b(hz_coef1);
    v16u8 coeff_hz = (v16u8)__msa_ilvr_b(coeff_hz0, coeff_hz1);
    v8u16 coeff_vt0 = (v8u16)__msa_fill_h(vt_coef0);
    v8u16 coeff_vt1 = (v8u16)__msa_fill_h(vt_coef1);

    mask1 = ADDVI_B(mask0, 1);

    src0 = LD_UB(src);
    src += src_stride;

    for(row = (height >> 2); row--;)
    {
        LD_UB4(src, src_stride, src1, src2, src3, src4);
        src += (4 * src_stride);

        VSHF_B2_UB(src0, src1, src1, src2, mask1, mask1, src5, src6);
        VSHF_B2_UB(src2, src3, src3, src4, mask1, mask1, src7, src8);
        VSHF_B2_UB(src0, src1, src1, src2, mask0, mask0, src0, src1);
        VSHF_B2_UB(src2, src3, src3, src4, mask0, mask0, src2, src3);
        DOTP_UB4_UH(src0, src1, src2, src3, coeff_hz, coeff_hz, coeff_hz,
                    coeff_hz, res_hz0, res_hz1, res_hz2, res_hz3);
        MUL4(res_hz0, coeff_vt1, res_hz1, coeff_vt0, res_hz2, coeff_vt1,
             res_hz3, coeff_vt0, res_vt0, res_vt1, res_vt2, res_vt3);
        ADD2(res_vt0, res_vt1, res_vt2, res_vt3, out_vt0, out_vt1);
        SRARI_H2_UH(out_vt0, out_vt1, 6);
        SAT_UH2_UH(out_vt0, out_vt1, 7);
        PCKEV_B2_SB(out_vt0, out_vt0, out_vt1, out_vt1, res0, res1);

        DOTP_UB4_UH(src5, src6, src7, src8, coeff_hz, coeff_hz, coeff_hz,
                    coeff_hz, res_hz0, res_hz1, res_hz2, res_hz3);
        MUL4(res_hz0, coeff_vt1, res_hz1, coeff_vt0, res_hz2, coeff_vt1,
             res_hz3, coeff_vt0, res_vt0, res_vt1, res_vt2, res_vt3);
        ADD2(res_vt0, res_vt1, res_vt2, res_vt3, out_vt2, out_vt3);
        SRARI_H2_UH(out_vt2, out_vt3, 6);
        SAT_UH2_UH(out_vt2, out_vt3, 7);
        PCKEV_B2_SB(out_vt2, out_vt2, out_vt3, out_vt3, res2, res3);

        ILVR_B2_UB(res2, res0, res3, res1, out0, out1);

        ST8x4_UB(out0, out1, dst, dst_stride);
        dst += (4 * dst_stride);
        src0 = src4;
    }
}

static void avc_interleaved_chroma_hv_4w_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride,
                                             UWORD32 hz_coef0, UWORD32 hz_coef1,
                                             UWORD32 vt_coef0, UWORD32 vt_coef1,
                                             WORD32 height)
{
    if(2 == height)
    {
        avc_interleaved_chroma_hv_4x2_msa(src, src_stride, dst, dst_stride,
                                          hz_coef0, hz_coef1,
                                          vt_coef0, vt_coef1);
    }
    else
    {
        avc_interleaved_chroma_hv_4x4mul_msa(src, src_stride, dst, dst_stride,
                                             hz_coef0, hz_coef1,
                                             vt_coef0, vt_coef1, height);
    }
}

static void avc_interleaved_chroma_hv_8w_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride,
                                             UWORD32 hz_coef0, UWORD32 hz_coef1,
                                             UWORD32 vt_coef0, UWORD32 vt_coef1,
                                             WORD32 height)
{
    UWORD32 row;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9;
    v16u8 src10, src11, src12, src13, src14;
    v8u16 res_hz0, res_hz1, res_hz2, res_hz3, res_hz4, res_hz5;
    v8u16 res_vt0, res_vt1, res_vt2, res_vt3;
    v16i8 mask = { 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14, 16 };
    v16i8 tmp0, tmp1, tmp2, tmp3, mask1;
    v16u8 out0, out1, out2, out3;
    v16i8 coeff_hz0 = __msa_fill_b(hz_coef0);
    v16i8 coeff_hz1 = __msa_fill_b(hz_coef1);
    v16u8 coeff_hz = (v16u8)__msa_ilvr_b(coeff_hz0, coeff_hz1);
    v8u16 coeff_vt0 = (v8u16)__msa_fill_h(vt_coef0);
    v8u16 coeff_vt1 = (v8u16)__msa_fill_h(vt_coef1);

    mask1 = ADDVI_B(mask, 1);

    LD_UB2(src, 16, src0, src13);
    src += src_stride;

    VSHF_B2_UB(src0, src13, src0, src13, mask1, mask, src14, src0);
    DOTP_UB2_UH(src0, src14, coeff_hz, coeff_hz, res_hz0, res_hz5);

    for(row = (height >> 2); row--;)
    {
        LD_UB4(src, src_stride, src1, src2, src3, src4);
        LD_UB4(src + 16, src_stride, src5, src6, src7, src8);
        src += (4 * src_stride);

        VSHF_B2_UB(src1, src5, src2, src6, mask, mask, src9, src10);
        VSHF_B2_UB(src3, src7, src4, src8, mask, mask, src11, src12);
        DOTP_UB4_UH(src9, src10, src11, src12, coeff_hz, coeff_hz, coeff_hz,
                    coeff_hz, res_hz1, res_hz2, res_hz3, res_hz4);
        MUL4(res_hz1, coeff_vt0, res_hz2, coeff_vt0, res_hz3, coeff_vt0,
             res_hz4, coeff_vt0, res_vt0, res_vt1, res_vt2, res_vt3);

        res_vt0 += (res_hz0 * coeff_vt1);
        res_vt1 += (res_hz1 * coeff_vt1);
        res_vt2 += (res_hz2 * coeff_vt1);
        res_vt3 += (res_hz3 * coeff_vt1);

        SRARI_H4_UH(res_vt0, res_vt1, res_vt2, res_vt3, 6);
        SAT_UH4_UH(res_vt0, res_vt1, res_vt2, res_vt3, 7);
        PCKEV_B2_SB(res_vt1, res_vt0, res_vt3, res_vt2, tmp0, tmp1);
        res_hz0 = res_hz4;

        VSHF_B2_UB(src1, src5, src2, src6, mask1, mask1, src5, src6);
        VSHF_B2_UB(src3, src7, src4, src8, mask1, mask1, src7, src8);
        DOTP_UB4_UH(src5, src6, src7, src8, coeff_hz, coeff_hz, coeff_hz,
                    coeff_hz, res_hz1, res_hz2, res_hz3, res_hz4);
        MUL4(res_hz1, coeff_vt0, res_hz2, coeff_vt0, res_hz3, coeff_vt0,
             res_hz4, coeff_vt0, res_vt0, res_vt1, res_vt2, res_vt3);

        res_vt0 += (res_hz5 * coeff_vt1);
        res_vt1 += (res_hz1 * coeff_vt1);
        res_vt2 += (res_hz2 * coeff_vt1);
        res_vt3 += (res_hz3 * coeff_vt1);

        SRARI_H4_UH(res_vt0, res_vt1, res_vt2, res_vt3, 6);
        SAT_UH4_UH(res_vt0, res_vt1, res_vt2, res_vt3, 7);
        PCKEV_B2_SB(res_vt1, res_vt0, res_vt3, res_vt2, tmp2, tmp3);
        res_hz5 = res_hz4;

        ILVRL_B2_UB(tmp2, tmp0, out0, out1);
        ILVRL_B2_UB(tmp3, tmp1, out2, out3);
        ST_UB4(out0, out1, out2, out3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void copy_width8_msa(UWORD8 *src, WORD32 src_stride,
                            UWORD8 *dst, WORD32 dst_stride,
                            WORD32 height)
{
    WORD32 cnt;
    UWORD64 out0, out1, out2, out3, out4, out5, out6, out7;

    if(0 == height % 8)
    {
        for(cnt = height >> 3; cnt--;)
        {
            LD4(src, src_stride, out0, out1, out2, out3);
            src += (4 * src_stride);
            LD4(src, src_stride, out4, out5, out6, out7);
            src += (4 * src_stride);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
            SD4(out4, out5, out6, out7, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == height % 4)
    {
        for(cnt = (height / 4); cnt--;)
        {
            LD4(src, src_stride, out0, out1, out2, out3);
            src += (4 * src_stride);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == height % 2)
    {
        for(cnt = (height / 2); cnt--;)
        {
            out0 = LD(src);
            src += src_stride;
            out1 = LD(src);
            src += src_stride;

            SD(out0, dst);
            dst += dst_stride;
            SD(out1, dst);
            dst += dst_stride;
        }
    }
}

static void copy_16multx8mult_msa(UWORD8 *src, WORD32 src_stride,
                                  UWORD8 *dst, WORD32 dst_stride,
                                  WORD32 height, WORD32 width)
{
    WORD32 cnt, loop_cnt;
    UWORD8 *src_tmp, *dst_tmp;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    for(cnt = (width >> 4); cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        for(loop_cnt = (height >> 3); loop_cnt--;)
        {
            LD_UB8(src_tmp, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src_tmp += (8 * src_stride);

            ST_UB8(src0, src1, src2, src3, src4, src5, src6, src7,
                   dst_tmp, dst_stride);
            dst_tmp += (8 * dst_stride);
        }

        src += 16;
        dst += 16;
    }
}

static void copy_width16_msa(UWORD8 *src, WORD32 src_stride,
                             UWORD8 *dst, WORD32 dst_stride,
                             WORD32 height)
{
    WORD32 cnt;
    v16u8 src0, src1, src2, src3;

    if(0 == height % 8)
    {
        copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 16);
    }
    else if(0 == height % 4)
    {
        for(cnt = (height >> 2); cnt--;)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);
            src += (4 * src_stride);

            ST_UB4(src0, src1, src2, src3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
}

void ih264_inter_pred_chroma_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                 WORD32 src_strd, WORD32 dst_strd,
                                 WORD32 dx, WORD32 dy, WORD32 ht, WORD32 wd)
{
    if(dx && dy)
    {
        if(8 == wd)
        {
            avc_interleaved_chroma_hv_8w_msa(pu1_src, src_strd, pu1_dst,
                                             dst_strd, dx, (8 - dx), dy,
                                             (8 - dy), ht);
        }
        else if(4 == wd)
        {
            avc_interleaved_chroma_hv_4w_msa(pu1_src, src_strd, pu1_dst,
                                             dst_strd, dx, (8 - dx), dy,
                                             (8 - dy), ht);
        }
        else if(2 == wd)
        {
            avc_interleaved_chroma_hv_2w_msa(pu1_src, src_strd, pu1_dst,
                                             dst_strd, dx, (8 - dx), dy,
                                             (8 - dy), ht);
        }
    }
    else if(dx)
    {
        if(8 == wd)
        {
            avc_interleaved_chroma_hz_8w_msa(pu1_src, src_strd, pu1_dst,
                                             dst_strd, dx, (8 - dx), ht);
        }
        else if(4 == wd)
        {
            avc_interleaved_chroma_hz_4w_msa(pu1_src, src_strd, pu1_dst,
                                             dst_strd, dx, (8 - dx), ht);
        }
        else if(2 == wd)
        {
            avc_interleaved_chroma_hz_2w_msa(pu1_src, src_strd, pu1_dst,
                                             dst_strd, dx, (8 - dx), ht);
        }
    }
    else if(dy)
    {
        if(8 == wd)
        {
            avc_interleaved_chroma_vt_8w_msa(pu1_src, src_strd, pu1_dst,
                                             dst_strd, dy, (8 - dy), ht);
        }
        else if(4 == wd)
        {
            avc_interleaved_chroma_vt_4w_msa(pu1_src, src_strd, pu1_dst,
                                             dst_strd, dy, (8 - dy), ht);
        }
        else if(2 == wd)
        {
            avc_interleaved_chroma_vt_2w_msa(pu1_src, src_strd, pu1_dst,
                                             dst_strd, dy, (8 - dy), ht);
        }
    }
    else
    {
        if(8 == wd)
        {
            copy_width16_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
        }
        else if(4 == wd)
        {
            copy_width8_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
        }
        else if(2 == wd)
        {
            if(2 == ht)
            {
                UWORD32 tmp0, tmp1;

                tmp0 = LW(pu1_src);
                tmp1 = LW(pu1_src + src_strd);
                SW(tmp0, pu1_dst);
                SW(tmp1, pu1_dst + dst_strd);
            }
            else
            {
                UWORD32 tmp0, tmp1, tmp2, tmp3;

                LW4(pu1_src, src_strd, tmp0, tmp1, tmp2, tmp3);
                SW4(tmp0, tmp1, tmp2, tmp3, pu1_dst, dst_strd);
            }
        }
    }
}
