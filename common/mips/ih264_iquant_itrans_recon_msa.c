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

#include "ih264_typedefs.h"
#include "ih264_trans_macros.h"
#include "ih264_macros.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_macros_msa.h"

#define AVC_ITRANS_H(in0, in1, in2, in3, out0, out1, out2, out3)          \
{                                                                         \
    v8i16 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                                 \
                                                                          \
    tmp0_m = in0 + in2;                                                   \
    tmp1_m = in0 - in2;                                                   \
    tmp2_m = SRAI_H(in1, 1);                                              \
    tmp2_m -= in3;                                                        \
    tmp3_m = SRAI_H(in3, 1);                                              \
    tmp3_m += in1;                                                        \
                                                                          \
    BUTTERFLY_4(tmp0_m, tmp1_m, tmp2_m, tmp3_m, out0, out1, out2, out3);  \
}

#define AVC_ITRANS_W(in0, in1, in2, in3, out0, out1, out2, out3)          \
{                                                                         \
    v4i32 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                                 \
                                                                          \
    tmp0_m = in0 + in2;                                                   \
    tmp1_m = in0 - in2;                                                   \
    tmp2_m = SRAI_W(in1, 1);                                              \
    tmp2_m -= in3;                                                        \
    tmp3_m = SRAI_W(in3, 1);                                              \
    tmp3_m += in1;                                                        \
                                                                          \
    BUTTERFLY_4(tmp0_m, tmp1_m, tmp2_m, tmp3_m, out0, out1, out2, out3);  \
}

static void avc_idct4x4_addblk_dc_msa(WORD16 dc, UWORD8 *pred_ptr,
                                      WORD32 pred_stride,
                                      UWORD8 *dst, WORD32 dst_stride)
{
    UWORD32 src0, src1, src2, src3;
    v16u8 pred = { 0 };
    v16i8 out;
    v8i16 input_dc, pred_r, pred_l;

    LW4(pred_ptr, pred_stride, src0, src1, src2, src3);
    input_dc = __msa_fill_h(dc);
    INSERT_W4_UB(src0, src1, src2, src3, pred);
    input_dc = __msa_srari_h(input_dc, 6);
    UNPCK_UB_SH(pred, pred_r, pred_l);
    ADD2(pred_r, input_dc, pred_l, input_dc, pred_r, pred_l);
    CLIP_SH2_0_255(pred_r, pred_l);
    out = __msa_pckev_b((v16i8)pred_l, (v16i8)pred_r);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
}

static void avc_idct8_dc_addblk_msa(WORD16 dc, UWORD8 *pred, WORD32 pred_stride,
                                    UWORD8 *dst, WORD32 dst_stride)
{
    WORD32 dc_val;
    v16i8 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 dst0_r, dst1_r, dst2_r, dst3_r, dst4_r, dst5_r, dst6_r, dst7_r;
    v8i16 input_dc;
    v16i8 zeros = { 0 };

    dc_val = (dc + 32) >> 6;
    input_dc = __msa_fill_h(dc_val);

    LD_SB8(pred, pred_stride, dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7);
    ILVR_B4_SH(zeros, dst0, zeros, dst1, zeros, dst2, zeros, dst3, dst0_r,
               dst1_r, dst2_r, dst3_r);
    ILVR_B4_SH(zeros, dst4, zeros, dst5, zeros, dst6, zeros, dst7, dst4_r,
               dst5_r, dst6_r, dst7_r);
    ADD4(dst0_r, input_dc, dst1_r, input_dc, dst2_r, input_dc, dst3_r, input_dc,
         dst0_r, dst1_r, dst2_r, dst3_r);
    ADD4(dst4_r, input_dc, dst5_r, input_dc, dst6_r, input_dc, dst7_r, input_dc,
         dst4_r, dst5_r, dst6_r, dst7_r);
    CLIP_SH4_0_255(dst0_r, dst1_r, dst2_r, dst3_r);
    CLIP_SH4_0_255(dst4_r, dst5_r, dst6_r, dst7_r);
    PCKEV_B4_SB(dst1_r, dst0_r, dst3_r, dst2_r, dst5_r, dst4_r, dst7_r, dst6_r,
                dst0, dst1, dst2, dst3);
    ST8x4_UB(dst0, dst1, dst, dst_stride);
    dst += (4 * dst_stride);
    ST8x4_UB(dst2, dst3, dst, dst_stride);
}

static void avc_luma_iquant_itrans_addblk_4x4_msa(WORD16 *src, UWORD8 *pred,
                                                  WORD32 pred_stride,
                                                  UWORD8 *dst,
                                                  WORD32 dst_stride,
                                                  const UWORD16 *quant_scale,
                                                  const UWORD16 *wgt_scale,
                                                  UWORD32 qp_div_6,
                                                  WORD32 start_idx,
                                                  WORD16 dc_val)
{
    WORD16 rnd_fact = (qp_div_6 < 4) ? 1 << (3 - qp_div_6) : 0;
    UWORD32 pred0, pred1, pred2, pred3;
    v8i16 src0, src1, scale0, scale1, weigh0, weigh1;
    v8i16 hres0, hres1, hres2, hres3, vres0, vres1, vres2, vres3;
    v8i16 inp0, inp1, res0, res1;
    v4i32 hres0_w, hres1_w, hres2_w, hres3_w, src0_w, src1_w, src2_w, src3_w;
    v4i32 scale0_w, scale1_w, scale2_w, scale3_w;
    v4i32 weigh0_w, weigh1_w, weigh2_w, weigh3_w, rndfact;
    v4i32 qp_div_6_v = __msa_fill_w(qp_div_6);
    v16i8 dst0 = { 0 };
    v16i8 dst1 = { 0 };
    v16i8 zero = { 0 };

    LD_SH2(src, 8, src0, src1);
    LD_SH2(quant_scale, 8, scale0, scale1);
    LD_SH2(wgt_scale, 8, weigh0, weigh1);
    rndfact = __msa_fill_w(rnd_fact);

    UNPCK_SH_SW(src0, src0_w, src1_w);
    UNPCK_SH_SW(src1, src2_w, src3_w);
    UNPCK_SH_SW(scale0, scale0_w, scale1_w);
    UNPCK_SH_SW(scale1, scale2_w, scale3_w);
    UNPCK_SH_SW(weigh0, weigh0_w, weigh1_w);
    UNPCK_SH_SW(weigh1, weigh2_w, weigh3_w);

    MUL4(src0_w, scale0_w, src1_w, scale1_w, src2_w, scale2_w, src3_w, scale3_w,
         src0_w, src1_w, src2_w, src3_w);
    MUL4(src0_w, weigh0_w, src1_w, weigh1_w, src2_w, weigh2_w, src3_w, weigh3_w,
         src0_w, src1_w, src2_w, src3_w);
    ADD4(src0_w, rndfact, src1_w, rndfact, src2_w, rndfact, src3_w, rndfact,
         src0_w, src1_w, src2_w, src3_w);
    SLL_4V(src0_w, src1_w, src2_w, src3_w, qp_div_6_v);
    SRAI_W4_SW(src0_w, src1_w, src2_w, src3_w, 4);

    if(1 == start_idx)
    {
        src0_w = __msa_insert_w(src0_w, 0, dc_val);
    }

    TRANSPOSE4x4_SW_SW(src0_w, src1_w, src2_w, src3_w, src0_w, src1_w, src2_w,
                       src3_w);
    AVC_ITRANS_W(src0_w, src1_w, src2_w, src3_w, hres0_w, hres1_w, hres2_w,
                 hres3_w);

    PCKEV_H2_SH(hres1_w, hres0_w, hres3_w, hres2_w, hres0, hres2);

    hres1 = (v8i16)__msa_ilvl_d((v2i64)hres0, (v2i64)hres0);
    hres3 = (v8i16)__msa_ilvl_d((v2i64)hres2, (v2i64)hres2);

    TRANSPOSE4x4_SH_SH(hres0, hres1, hres2, hres3, hres0, hres1, hres2, hres3);
    AVC_ITRANS_H(hres0, hres1, hres2, hres3, vres0, vres1, vres2, vres3);
    SRARI_H4_SH(vres0, vres1, vres2, vres3, 6);
    ILVR_D2_SH(vres1, vres0, vres3, vres2, inp0, inp1);
    LW4(pred, pred_stride, pred0, pred1, pred2, pred3);
    INSERT_W2_SB(pred0, pred1, dst0);
    INSERT_W2_SB(pred2, pred3, dst1);
    ILVR_B2_SH(zero, dst0, zero, dst1, res0, res1);
    ADD2(res0, inp0, res1, inp1, res0, res1);
    CLIP_SH2_0_255(res0, res1);
    PCKEV_B2_SB(res0, res0, res1, res1, dst0, dst1);
    ST4x4_UB(dst0, dst1, 0, 1, 0, 1, dst, dst_stride);
}

static void avc_luma_iquant_itrans_addblk_8x8_msa(WORD16 *src, UWORD8 *pred,
                                                  WORD32 pred_stride,
                                                  UWORD8 *dst,
                                                  WORD32 dst_stride,
                                                  const UWORD16 *quant_scale,
                                                  const UWORD16 *wgt_scale,
                                                  UWORD32 qp_div)
{

    v16i8 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v16i8 zeros = { 0 };
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 scale0, scale1, scale2, scale3, weigh0, weigh1, weigh2, weigh3;
    v8i16 vec0, vec1, vec2, vec3;
    v8i16 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    v8i16 res0, res1, res2, res3, res4, res5, res6, res7;
    v4i32 src0_r, src1_r, src2_r, src3_r, src0_l, src1_l, src2_l, src3_l;
    v4i32 scale0_r, scale1_r, scale2_r, scale3_r, scale0_l, scale1_l, scale2_l;
    v4i32 scale3_l, weigh0_r, weigh1_r, weigh2_r, weigh3_r, weigh0_l, weigh1_l;
    v4i32 weigh2_l, weigh3_l, rndfactv;
    v4i32 tmp0_r, tmp1_r, tmp2_r, tmp3_r, tmp4_r, tmp5_r, tmp6_r, tmp7_r;
    v4i32 tmp0_l, tmp1_l, tmp2_l, tmp3_l, tmp4_l, tmp5_l, tmp6_l, tmp7_l;
    v4i32 res0_r, res1_r, res2_r, res3_r, res4_r, res5_r, res6_r, res7_r;
    v4i32 res0_l, res1_l, res2_l, res3_l, res4_l, res5_l, res6_l, res7_l;
    WORD32 rnd_fact = (qp_div < 6) ? (1 << (5 - qp_div)) : 0;
    v4i32 vec0_r, vec1_r, vec2_r, vec3_r, vec0_l, vec1_l, vec2_l, vec3_l;
    v4i32 qp_div_v = __msa_fill_w(qp_div);

    rndfactv = __msa_fill_w(rnd_fact);

    LD_SH4(src, 8, src0, src1, src2, src3);
    LD_SH4(quant_scale, 8, scale0, scale1, scale2, scale3);
    LD_SH4(wgt_scale, 8, weigh0, weigh1, weigh2, weigh3);
    UNPCK_SH_SW(src0, src0_r, src0_l);
    UNPCK_SH_SW(src1, src1_r, src1_l);
    UNPCK_SH_SW(src2, src2_r, src2_l);
    UNPCK_SH_SW(src3, src3_r, src3_l);
    UNPCK_SH_SW(scale0, scale0_r, scale0_l);
    UNPCK_SH_SW(scale1, scale1_r, scale1_l);
    UNPCK_SH_SW(scale2, scale2_r, scale2_l);
    UNPCK_SH_SW(scale3, scale3_r, scale3_l);
    UNPCK_SH_SW(weigh0, weigh0_r, weigh0_l);
    UNPCK_SH_SW(weigh1, weigh1_r, weigh1_l);
    UNPCK_SH_SW(weigh2, weigh2_r, weigh2_l);
    UNPCK_SH_SW(weigh3, weigh3_r, weigh3_l);
    MUL4(src0_r, scale0_r, src1_r, scale1_r, src2_r, scale2_r, src3_r, scale3_r,
         src0_r, src1_r, src2_r, src3_r);
    MUL4(src0_l, scale0_l, src1_l, scale1_l, src2_l, scale2_l, src3_l, scale3_l,
         src0_l, src1_l, src2_l, src3_l);
    MUL4(src0_r, weigh0_r, src1_r, weigh1_r, src2_r, weigh2_r, src3_r, weigh3_r,
         src0_r, src1_r, src2_r, src3_r);
    MUL4(src0_l, weigh0_l, src1_l, weigh1_l, src2_l, weigh2_l, src3_l, weigh3_l,
         src0_l, src1_l, src2_l, src3_l);
    ADD4(src0_r, rndfactv, src1_r, rndfactv, src2_r, rndfactv, src3_r, rndfactv,
         src0_r, src1_r, src2_r, src3_r);
    ADD4(src0_l, rndfactv, src1_l, rndfactv, src2_l, rndfactv, src3_l, rndfactv,
         src0_l, src1_l, src2_l, src3_l);
    SLL_4V(src0_r, src1_r, src2_r, src3_r, qp_div_v);
    SLL_4V(src0_l, src1_l, src2_l, src3_l, qp_div_v);
    SRAI_W4_SW(src0_r, src1_r, src2_r, src3_r, 6);
    SRAI_W4_SW(src0_l, src1_l, src2_l, src3_l, 6);
    PCKEV_H4_SH(src0_l, src0_r, src1_l, src1_r, src2_l, src2_r, src3_l, src3_r,
                src0, src1, src2, src3);
    LD_SH4(src + 32, 8, src4, src5, src6, src7);
    LD_SH4(quant_scale + 32, 8, scale0, scale1, scale2, scale3);
    LD_SH4(wgt_scale + 32, 8, weigh0, weigh1, weigh2, weigh3);
    UNPCK_SH_SW(src4, src0_r, src0_l);
    UNPCK_SH_SW(src5, src1_r, src1_l);
    UNPCK_SH_SW(src6, src2_r, src2_l);
    UNPCK_SH_SW(src7, src3_r, src3_l);
    UNPCK_SH_SW(scale0, scale0_r, scale0_l);
    UNPCK_SH_SW(scale1, scale1_r, scale1_l);
    UNPCK_SH_SW(scale2, scale2_r, scale2_l);
    UNPCK_SH_SW(scale3, scale3_r, scale3_l);
    UNPCK_SH_SW(weigh0, weigh0_r, weigh0_l);
    UNPCK_SH_SW(weigh1, weigh1_r, weigh1_l);
    UNPCK_SH_SW(weigh2, weigh2_r, weigh2_l);
    UNPCK_SH_SW(weigh3, weigh3_r, weigh3_l);
    MUL4(src0_r, scale0_r, src1_r, scale1_r, src2_r, scale2_r, src3_r, scale3_r,
         src0_r, src1_r, src2_r, src3_r);
    MUL4(src0_l, scale0_l, src1_l, scale1_l, src2_l, scale2_l, src3_l, scale3_l,
         src0_l, src1_l, src2_l, src3_l);
    MUL4(src0_r, weigh0_r, src1_r, weigh1_r, src2_r, weigh2_r, src3_r, weigh3_r,
         src0_r, src1_r, src2_r, src3_r);
    MUL4(src0_l, weigh0_l, src1_l, weigh1_l, src2_l, weigh2_l, src3_l, weigh3_l,
         src0_l, src1_l, src2_l, src3_l);
    ADD4(src0_r, rndfactv, src1_r, rndfactv, src2_r, rndfactv, src3_r, rndfactv,
         src0_r, src1_r, src2_r, src3_r);
    ADD4(src0_l, rndfactv, src1_l, rndfactv, src2_l, rndfactv, src3_l, rndfactv,
         src0_l, src1_l, src2_l, src3_l);
    SLL_4V(src0_r, src1_r, src2_r, src3_r, qp_div_v);
    SLL_4V(src0_l, src1_l, src2_l, src3_l, qp_div_v);
    SRAI_W4_SW(src0_r, src1_r, src2_r, src3_r, 6);
    SRAI_W4_SW(src0_l, src1_l, src2_l, src3_l, 6);
    PCKEV_H4_SH(src0_l, src0_r, src1_l, src1_r, src2_l, src2_r, src3_l, src3_r,
                src4, src5, src6, src7);
    TRANSPOSE8x8_SH_SH(src0, src1, src2, src3, src4, src5, src6, src7,
                       src0, src1, src2, src3, src4, src5, src6, src7);

    vec0 = src0 + src4;
    vec1 = src0 - src4;
    vec2 = SRAI_H(src2, 1);
    vec2 = vec2 - src6;
    vec3 = SRAI_H(src6, 1);
    vec3 = src2 + vec3;

    BUTTERFLY_4(vec0, vec1, vec2, vec3, tmp0, tmp1, tmp2, tmp3);

    vec0 = SRAI_H(src7, 1);
    vec0 = src5 - vec0 - src3 - src7;
    vec1 = SRAI_H(src3, 1);
    vec1 = src1 - vec1 + src7 - src3;
    vec2 = SRAI_H(src5, 1);
    vec2 = vec2 - src1 + src7 + src5;
    vec3 = SRAI_H(src1, 1);
    vec3 = vec3 + src3 + src5 + src1;
    tmp4 = SRAI_H(vec3, 2);
    tmp4 += vec0;
    tmp5 = SRAI_H(vec2, 2);
    tmp5 += vec1;
    tmp6 = SRAI_H(vec1, 2);
    tmp6 -= vec2;
    tmp7 = SRAI_H(vec0, 2);
    tmp7 = vec3 - tmp7;

    BUTTERFLY_8(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7,
                res0, res1, res2, res3, res4, res5, res6, res7);
    TRANSPOSE8x8_SH_SH(res0, res1, res2, res3, res4, res5, res6, res7,
                       res0, res1, res2, res3, res4, res5, res6, res7);
    UNPCK_SH_SW(res0, tmp0_r, tmp0_l);
    UNPCK_SH_SW(res1, tmp1_r, tmp1_l);
    UNPCK_SH_SW(res2, tmp2_r, tmp2_l);
    UNPCK_SH_SW(res3, tmp3_r, tmp3_l);
    UNPCK_SH_SW(res4, tmp4_r, tmp4_l);
    UNPCK_SH_SW(res5, tmp5_r, tmp5_l);
    UNPCK_SH_SW(res6, tmp6_r, tmp6_l);
    UNPCK_SH_SW(res7, tmp7_r, tmp7_l);
    BUTTERFLY_4(tmp0_r, tmp0_l, tmp4_l, tmp4_r, vec0_r, vec0_l, vec1_l, vec1_r);

    vec2_r = SRAI_W(tmp2_r, 1);
    vec2_l = SRAI_W(tmp2_l, 1);
    vec2_r -= tmp6_r;
    vec2_l -= tmp6_l;
    vec3_r = SRAI_W(tmp6_r, 1);
    vec3_l = SRAI_W(tmp6_l, 1);
    vec3_r += tmp2_r;
    vec3_l += tmp2_l;

    BUTTERFLY_4(vec0_r, vec1_r, vec2_r, vec3_r, tmp0_r, tmp2_r, tmp4_r, tmp6_r);
    BUTTERFLY_4(vec0_l, vec1_l, vec2_l, vec3_l, tmp0_l, tmp2_l, tmp4_l, tmp6_l);

    vec0_r = SRAI_W(tmp7_r, 1);
    vec0_l = SRAI_W(tmp7_l, 1);
    vec0_r = tmp5_r - vec0_r - tmp3_r - tmp7_r;
    vec0_l = tmp5_l - vec0_l - tmp3_l - tmp7_l;
    vec1_r = SRAI_W(tmp3_r, 1);
    vec1_l = SRAI_W(tmp3_l, 1);
    vec1_r = tmp1_r - vec1_r + tmp7_r - tmp3_r;
    vec1_l = tmp1_l - vec1_l + tmp7_l - tmp3_l;
    vec2_r = SRAI_W(tmp5_r, 1);
    vec2_l = SRAI_W(tmp5_l, 1);
    vec2_r = vec2_r - tmp1_r + tmp7_r + tmp5_r;
    vec2_l = vec2_l - tmp1_l + tmp7_l + tmp5_l;
    vec3_r = SRAI_W(tmp1_r, 1);
    vec3_l = SRAI_W(tmp1_l, 1);
    vec3_r = vec3_r + tmp3_r + tmp5_r + tmp1_r;
    vec3_l = vec3_l + tmp3_l + tmp5_l + tmp1_l;
    tmp1_r = SRAI_W(vec3_r, 2);
    tmp1_l = SRAI_W(vec3_l, 2);
    tmp1_r += vec0_r;
    tmp1_l += vec0_l;
    tmp3_r = SRAI_W(vec2_r, 2);
    tmp3_l = SRAI_W(vec2_l, 2);
    tmp3_r += vec1_r;
    tmp3_l += vec1_l;
    tmp5_r = SRAI_W(vec1_r, 2);
    tmp5_l = SRAI_W(vec1_l, 2);
    tmp5_r -= vec2_r;
    tmp5_l -= vec2_l;
    tmp7_r = SRAI_W(vec0_r, 2);
    tmp7_l = SRAI_W(vec0_l, 2);
    tmp7_r = vec3_r - tmp7_r;
    tmp7_l = vec3_l - tmp7_l;

    BUTTERFLY_4(tmp0_r, tmp0_l, tmp7_l, tmp7_r, res0_r, res0_l, res7_l, res7_r);
    BUTTERFLY_4(tmp2_r, tmp2_l, tmp5_l, tmp5_r, res1_r, res1_l, res6_l, res6_r);
    BUTTERFLY_4(tmp4_r, tmp4_l, tmp3_l, tmp3_r, res2_r, res2_l, res5_l, res5_r);
    BUTTERFLY_4(tmp6_r, tmp6_l, tmp1_l, tmp1_r, res3_r, res3_l, res4_l, res4_r);

    SRARI_W4_SW(res0_r, res0_l, res1_r, res1_l, 6);
    SRARI_W4_SW(res2_r, res2_l, res3_r, res3_l, 6);
    SRARI_W4_SW(res4_r, res4_l, res5_r, res5_l, 6);
    SRARI_W4_SW(res6_r, res6_l, res7_r, res7_l, 6);

    PCKEV_H4_SH(res0_l, res0_r, res1_l, res1_r, res2_l, res2_r, res3_l, res3_r,
                res0, res1, res2, res3);
    PCKEV_H4_SH(res4_l, res4_r, res5_l, res5_r, res6_l, res6_r, res7_l, res7_r,
                res4, res5, res6, res7);
    LD_SB8(pred, pred_stride, dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7);
    ILVR_B4_SH(zeros, dst0, zeros, dst1, zeros, dst2, zeros, dst3, tmp0, tmp1,
               tmp2, tmp3);
    ILVR_B4_SH(zeros, dst4, zeros, dst5, zeros, dst6, zeros, dst7, tmp4, tmp5,
               tmp6, tmp7);
    ADD4(res0, tmp0, res1, tmp1, res2, tmp2, res3, tmp3, res0, res1, res2,
         res3);
    ADD4(res4, tmp4, res5, tmp5, res6, tmp6, res7, tmp7, res4, res5, res6,
         res7);
    CLIP_SH4_0_255(res0, res1, res2, res3);
    CLIP_SH4_0_255(res4, res5, res6, res7);
    PCKEV_B4_SB(res1, res0, res3, res2, res5, res4, res7, res6, dst0, dst1,
                dst2, dst3);
    ST8x4_UB(dst0, dst1, dst, dst_stride);
    dst += (4 * dst_stride);
    ST8x4_UB(dst2, dst3, dst, dst_stride);
}

static void avc_interleaved_chroma_iquant_itrans_addblk_4x4_msa(
    WORD16 *src,
    UWORD8 *pred, WORD32 pred_stride,
    UWORD8 *dst, WORD32 dst_stride,
    const UWORD16 *quant_scale,
    const UWORD16 *wgt_scale,
    UWORD32 qp_div_6, WORD16 dc_val)
{
    WORD16 rnd_fact = (qp_div_6 < 4) ? 1 << (3 - qp_div_6) : 0;
    UWORD64 pred0, pred1, pred2, pred3;
    UWORD64 dst0, dst1, dst2, dst3;
    v8i16 src0, src1, scale0, scale1, weigh0, weigh1, inp0, inp1, out0, out1;
    v8i16 hres0, hres1, hres2, hres3, vres0, vres1, vres2, vres3;
    v4i32 hres0_w, hres1_w, hres2_w, hres3_w, src0_w, src1_w, src2_w, src3_w;
    v4i32 scale0_w, scale1_w, scale2_w, scale3_w;
    v4i32 weigh0_w, weigh1_w, weigh2_w, weigh3_w, rndfact;
    v16i8 pred_vec0 = { 0 };
    v16i8 pred_vec1 = { 0 };
    v16i8 dst_vec0 = { 0 };
    v16i8 dst_vec1 = { 0 };
    v16i8 zero = { 0 };
    v4i32 qp_div_6_v = __msa_fill_w(qp_div_6);

    LD_SH2(src, 8, src0, src1);
    LD_SH2(quant_scale, 8, scale0, scale1);
    LD_SH2(wgt_scale, 8, weigh0, weigh1);
    rndfact = __msa_fill_w(rnd_fact);

    UNPCK_SH_SW(src0, src0_w, src1_w);
    UNPCK_SH_SW(src1, src2_w, src3_w);
    UNPCK_SH_SW(scale0, scale0_w, scale1_w);
    UNPCK_SH_SW(scale1, scale2_w, scale3_w);
    UNPCK_SH_SW(weigh0, weigh0_w, weigh1_w);
    UNPCK_SH_SW(weigh1, weigh2_w, weigh3_w);

    MUL4(src0_w, scale0_w, src1_w, scale1_w, src2_w, scale2_w, src3_w, scale3_w,
         src0_w, src1_w, src2_w, src3_w);
    MUL4(src0_w, weigh0_w, src1_w, weigh1_w, src2_w, weigh2_w, src3_w, weigh3_w,
         src0_w, src1_w, src2_w, src3_w);
    ADD4(src0_w, rndfact, src1_w, rndfact, src2_w, rndfact, src3_w, rndfact,
         src0_w, src1_w, src2_w, src3_w);
    SLL_4V(src0_w, src1_w, src2_w, src3_w, qp_div_6_v);
    SRAI_W4_SW(src0_w, src1_w, src2_w, src3_w, 4);

    src0_w = __msa_insert_w(src0_w, 0, dc_val);

    TRANSPOSE4x4_SW_SW(src0_w, src1_w, src2_w, src3_w, src0_w, src1_w, src2_w,
                       src3_w);
    AVC_ITRANS_W(src0_w, src1_w, src2_w, src3_w, hres0_w, hres1_w, hres2_w,
                 hres3_w);

    PCKEV_H2_SH(hres1_w, hres0_w, hres3_w, hres2_w, hres0, hres2);

    hres1 = (v8i16)__msa_ilvl_d((v2i64)hres0, (v2i64)hres0);
    hres3 = (v8i16)__msa_ilvl_d((v2i64)hres2, (v2i64)hres2);

    TRANSPOSE4x4_SH_SH(hres0, hres1, hres2, hres3, hres0, hres1, hres2, hres3);
    AVC_ITRANS_H(hres0, hres1, hres2, hres3, vres0, vres1, vres2, vres3);
    SRARI_H4_SH(vres0, vres1, vres2, vres3, 6);
    ILVR_D2_SH(vres1, vres0, vres3, vres2, inp0, inp1);

    LD4(pred, pred_stride, pred0, pred1, pred2, pred3);
    INSERT_D2_SB(pred0, pred1, pred_vec0);
    INSERT_D2_SB(pred2, pred3, pred_vec1);
    out0 = (v8i16)__msa_ilvev_b(zero, pred_vec0);
    out1 = (v8i16)__msa_ilvev_b(zero, pred_vec1);

    LD4(dst, dst_stride, dst0, dst1, dst2, dst3);
    INSERT_D2_SB(dst0, dst1, dst_vec0);
    INSERT_D2_SB(dst2, dst3, dst_vec1);
    dst_vec0 = __msa_ilvod_b(dst_vec0, zero);
    dst_vec1 = __msa_ilvod_b(dst_vec1, zero);

    ADD2(out0, inp0, out1, inp1, out0, out1);
    CLIP_SH2_0_255(out0, out1);

    dst_vec0 = dst_vec0 + (v16i8)out0;
    dst_vec1 = dst_vec1 + (v16i8)out1;

    ST8x4_UB(dst_vec0, dst_vec1, dst, dst_stride);
}

static void avc_interleaved_chroma_addblk_dc_msa(WORD16 dc, UWORD8 *pred,
                                                 WORD32 pred_stride,
                                                 UWORD8 *dst,
                                                 WORD32 dst_stride)
{
    WORD16 dc_val = (dc + 32) >> 6;
    UWORD64 pred0, pred1, pred2, pred3, dst0, dst1, dst2, dst3;
    v16i8 pred_vec0 = { 0 };
    v16i8 pred_vec1 = { 0 };
    v16i8 dst_vec0 = { 0 };
    v16i8 dst_vec1 = { 0 };
    v16i8 zero = { 0 };
    v8i16 dc_vec, out0, out1;

    dc_val = (dc + 32) >> 6;
    dc_vec = __msa_fill_h(dc_val);
    LD4(pred, pred_stride, pred0, pred1, pred2, pred3);
    INSERT_D2_SB(pred0, pred1, pred_vec0);
    INSERT_D2_SB(pred2, pred3, pred_vec1);
    out0 = (v8i16)__msa_ilvev_b(zero, pred_vec0);
    out1 = (v8i16)__msa_ilvev_b(zero, pred_vec1);

    LD4(dst, dst_stride, dst0, dst1, dst2, dst3);
    INSERT_D2_SB(dst0, dst1, dst_vec0);
    INSERT_D2_SB(dst2, dst3, dst_vec1);
    dst_vec0 = __msa_ilvod_b(dst_vec0, zero);
    dst_vec1 = __msa_ilvod_b(dst_vec1, zero);

    ADD2(out0, dc_vec, out1, dc_vec, out0, out1);
    CLIP_SH2_0_255(out0, out1);

    dst_vec0 = dst_vec0 + (v16i8)out0;
    dst_vec1 = dst_vec1 + (v16i8)out1;

    ST8x4_UB(dst_vec0, dst_vec1, dst, dst_stride);
}

void ih264_iquant_itrans_recon_4x4_msa(WORD16 *src, UWORD8 *pred,
                                       UWORD8 *dst, WORD32 pred_stride,
                                       WORD32 dst_stride,
                                       const UWORD16 *quant_scale,
                                       const UWORD16 *wgt_scale,
                                       UWORD32 qp_div_6, WORD16 *tmp,
                                       WORD32 start_idx, WORD16 *dc_src)
{
    WORD16 dc_val = 0;
    UNUSED(tmp);

    if(1 == start_idx)
    {
        dc_val = dc_src[0];
    }

    avc_luma_iquant_itrans_addblk_4x4_msa(src, pred, pred_stride, dst,
                                          dst_stride, quant_scale, wgt_scale,
                                          qp_div_6, start_idx, dc_val);
}

void ih264_iquant_itrans_recon_4x4_dc_msa(WORD16 *src, UWORD8 *pred,
                                          UWORD8 *dst, WORD32 pred_stride,
                                          WORD32 dst_stride,
                                          const UWORD16 *quant_scale,
                                          const UWORD16 *wgt_scale,
                                          UWORD32 qp_div_6, WORD16 *tmp,
                                          WORD32 start_idx, WORD16 *dc_src)
{
    WORD32 dc_val;
    WORD16 rnd_fact = (qp_div_6 < 4) ? 1 << (3 - qp_div_6) : 0;
    UNUSED(tmp);

    if(0 == start_idx)
    {
        dc_val = src[0];
        INV_QUANT(dc_val, quant_scale[0], wgt_scale[0], qp_div_6, rnd_fact, 4);
    }
    else
    {
        dc_val = dc_src[0];
    }

    avc_idct4x4_addblk_dc_msa(dc_val, pred, pred_stride, dst, dst_stride);
}

void ih264_iquant_itrans_recon_8x8_msa(WORD16 *src, UWORD8 *pred,
                                       UWORD8 *dst, WORD32 pred_stride,
                                       WORD32 dst_stride,
                                       const UWORD16 *quant_scale,
                                       const UWORD16 *wgt_scale,
                                       UWORD32 qp_div_6, WORD16 *tmp,
                                       WORD32 start_idx,
                                       WORD16 *dc_src)
{
    UNUSED(tmp);
    UNUSED(start_idx);
    UNUSED(dc_src);

    avc_luma_iquant_itrans_addblk_8x8_msa(src, pred, pred_stride,
                                          dst, dst_stride, quant_scale,
                                          wgt_scale, qp_div_6);
}

void ih264_iquant_itrans_recon_8x8_dc_msa(WORD16 *src, UWORD8 *pred,
                                          UWORD8 *dst, WORD32 pred_stride,
                                          WORD32 dst_stride,
                                          const UWORD16 *quant_scale,
                                          const UWORD16 *wgt_scale,
                                          UWORD32 qp_div_6, WORD16 *tmp,
                                          WORD32 start_idx, WORD16 *dc_src)
{
    WORD32 dc_val;
    WORD32 rnd_fact = (qp_div_6 < 6) ? (1 << (5 - qp_div_6)) : 0;
    UNUSED(tmp);
    UNUSED(start_idx);
    UNUSED(dc_src);

    dc_val = src[0];
    INV_QUANT(dc_val, quant_scale[0], wgt_scale[0], qp_div_6, rnd_fact, 6);

    avc_idct8_dc_addblk_msa(dc_val, pred, pred_stride, dst, dst_stride);
}

void ih264_iquant_itrans_recon_chroma_4x4_msa(WORD16 *src, UWORD8 *pred,
                                              UWORD8 *dst,
                                              WORD32 pred_stride,
                                              WORD32 dst_stride,
                                              const UWORD16 *quant_scale,
                                              const UWORD16 *wgt_scale,
                                              UWORD32 qp_div_6,
                                              WORD16 *tmp, WORD16 *dc_src)
{
    UNUSED(tmp);

    avc_interleaved_chroma_iquant_itrans_addblk_4x4_msa(src, pred, pred_stride,
                                                        dst, dst_stride,
                                                        quant_scale,
                                                        wgt_scale, qp_div_6,
                                                        dc_src[0]);
}

void ih264_iquant_itrans_recon_chroma_4x4_dc_msa(WORD16 *src, UWORD8 *pred,
                                                 UWORD8 *dst,
                                                 WORD32 pred_stride,
                                                 WORD32 dst_stride,
                                                 const UWORD16 *quant_scale,
                                                 const UWORD16 *wgt_scale,
                                                 UWORD32 qp_div_6,
                                                 WORD16 *tmp, WORD16 *dc_src)
{
    UNUSED(src);
    UNUSED(quant_scale);
    UNUSED(wgt_scale);
    UNUSED(qp_div_6);
    UNUSED(tmp);

    avc_interleaved_chroma_addblk_dc_msa(dc_src[0], pred, pred_stride,
                                         dst, dst_stride);
}
