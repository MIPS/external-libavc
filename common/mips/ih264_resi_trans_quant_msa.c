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

/* System include files */
#include <stddef.h>

/* User include files */
#include "ih264_typedefs.h"
#include "ih264_defs.h"
#include "ih264_size_defs.h"
#include "ih264_macros.h"
#include "ih264_trans_macros.h"
#include "ih264_trans_data.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"

#include "ih264_macros_msa.h"

#define FORWARD_QUANT_8(out, thresh, diff, round, right_shift, nnz_coeff)      \
{                                                                              \
    WORD8 itr_m;                                                               \
    v8i16 sign_m, mask_m, abs_out_m, minus_out_m;                              \
    v8i16 zero = { 0 };                                                          \
    v4i32 scale0_r_m, scale0_l_m, temp0_r_m, temp0_l_m;                        \
                                                                               \
    sign_m = CLTI_S_H(out, 0);                                                 \
    abs_out_m = __msa_add_a_h(out, zero);                                      \
    mask_m = abs_out_m < thresh;                                               \
    ILVRL_H2_SW(zero, diff, scale0_r_m, scale0_l_m);                           \
    ILVRL_H2_SW(zero, abs_out_m, temp0_r_m, temp0_l_m);                        \
    MUL2(temp0_r_m, scale0_r_m, temp0_l_m, scale0_l_m, temp0_r_m, temp0_l_m);  \
    ADD2(temp0_r_m, round, temp0_l_m, round, temp0_r_m, temp0_l_m);            \
                                                                               \
    temp0_r_m >>= right_shift;                                                 \
    temp0_l_m >>= right_shift;                                                 \
    out = __msa_pckev_h((v8i16)temp0_l_m, (v8i16)temp0_r_m);                   \
                                                                               \
    minus_out_m = zero - out;                                                  \
    out = (v8i16)__msa_bmnz_v((v16u8)out, (v16u8)minus_out_m, (v16u8)sign_m);  \
    out = (v8i16)__msa_bmnz_v((v16u8)out, (v16u8)zero, (v16u8)mask_m);         \
    for(itr_m = 0; itr_m < 8; itr_m++)                                         \
    {                                                                          \
        if(out[itr_m])                                                         \
        {                                                                      \
            nnz_coeff++;                                                       \
        }                                                                      \
    }                                                                          \
}

#define FORWARD_QUANT_4(out, thresh, diff, round, right_shift, nnz_coeff)      \
{                                                                              \
    WORD8 itr_m;                                                               \
    v8i16 sign_m, mask_m, abs_out_m, minus_out_m;                              \
    v8i16 zero = { 0 };                                                          \
    v4i32 scale0_r_m, temp0_r_m;                                               \
                                                                               \
    sign_m = CLTI_S_H(out, 0);                                                 \
    abs_out_m = __msa_add_a_h(out, zero);                                      \
    mask_m = abs_out_m < thresh;                                               \
    scale0_r_m = (v4i32)__msa_ilvr_h(zero, (v8i16)diff);                       \
    temp0_r_m = (v4i32)__msa_ilvr_h(zero, abs_out_m);                          \
    temp0_r_m *= scale0_r_m;                                                   \
    temp0_r_m += round;                                                        \
                                                                               \
    temp0_r_m >>= right_shift;                                                 \
    out = __msa_pckev_h((v8i16)zero, (v8i16)temp0_r_m);                        \
                                                                               \
    minus_out_m = zero - out;                                                  \
    out = (v8i16)__msa_bmnz_v((v16u8)out, (v16u8)minus_out_m, (v16u8)sign_m);  \
    out = (v8i16)__msa_bmnz_v((v16u8)out, (v16u8)zero, (v16u8)mask_m);         \
                                                                               \
    for(itr_m = 0; itr_m < 4; itr_m++)                                         \
    {                                                                          \
        if(out[itr_m])                                                         \
        {                                                                      \
            nnz_coeff++;                                                       \
        }                                                                      \
    }                                                                          \
}

static void avc_resi_trans_quant_8x8_msa(UWORD8 *src, WORD32 src_stride,
                                         UWORD8 *pred, WORD32 pred_stride,
                                         WORD16 *dst, UWORD8 *nnz,
                                         const UWORD16 *scale_matrix,
                                         const UWORD16 *threshold_matrix,
                                         UWORD32 qbits, UWORD32 round_factor)
{
    UWORD32 nonzero_coeff = 0;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16u8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8u16 diff0, diff1, diff2, diff3, diff4, diff5, diff6, diff7;
    v8u16 thr0, thr1, thr2, thr3, thr4, thr5, thr6, thr7;
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    v8i16 a0, a1, a2, a3, a4, a5, a6, a7;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v4i32 round = __msa_fill_w(round_factor);
    v4i32 qbits_v = __msa_fill_w(qbits);

    LD_UB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
    LD_UB8(pred, pred_stride,
           pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7);
    ILVR_B8_UH(src0, pred0, src1, pred1, src2, pred2, src3, pred3,
               src4, pred4, src5, pred5, src6, pred6, src7, pred7,
               diff0, diff1, diff2, diff3, diff4, diff5, diff6, diff7);

    HSUB_UB4_SH(diff0, diff1, diff2, diff3, temp0, temp1, temp2, temp3);
    HSUB_UB4_SH(diff4, diff5, diff6, diff7, temp4, temp5, temp6, temp7);
    TRANSPOSE8x8_SH_SH(temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7,
                       temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7);
    ADD4(temp0, temp7, temp1, temp6, temp2, temp5, temp3, temp4,
         a0, a1, a2, a3);
    BUTTERFLY_4(a0, a1, a2, a3, a4, a5, a7, a6);

    out0 = a4 + a5;
    out2 = a6 + SRAI_H(a7, 1);
    out4 = a4 - a5;
    out6 = SRAI_H(a6, 1) - a7;

    SUB4(temp0, temp7, temp1, temp6, temp2, temp5, temp3, temp4,
         a0, a1, a2, a3);

    a4 = a1 + a2 + (SRAI_H(a0, 1) + a0);
    a5 = a0 - a3 - (SRAI_H(a2, 1) + a2);
    a6 = a0 + a3 - (SRAI_H(a1, 1) + a1);
    a7 = a1 - a2 + (SRAI_H(a3, 1) + a3);

    out1 = a4 + SRAI_H(a7, 2);
    out3 = a5 + SRAI_H(a6, 2);
    out5 = a6 - SRAI_H(a5, 2);
    out7 = SRAI_H(a4, 2) - a7;

    TRANSPOSE8x8_SH_SH(out0, out1, out2, out3, out4, out5, out6, out7,
                       out0, out1, out2, out3, out4, out5, out6, out7);
    ADD4(out0, out7, out1, out6, out2, out5, out3, out4, a0, a1, a2, a3);
    BUTTERFLY_4(a0, a1, a2, a3, a4, a5, a7, a6);
    SUB4(out0, out7, out1, out6, out2, out5, out3, out4, a0, a1, a2, a3);

    out0 = a4 + a5;
    out2 = a6 + SRAI_H(a7, 1);
    out4 = a4 - a5;
    out6 = SRAI_H(a6, 1) - a7;

    a4 = a1 + a2 + (SRAI_H(a0, 1) + a0);
    a5 = a0 - a3 - (SRAI_H(a2, 1) + a2);
    a6 = a0 + a3 - (SRAI_H(a1, 1) + a1);
    a7 = a1 - a2 + (SRAI_H(a3, 1) + a3);

    out1 = a4 + SRAI_H(a7, 2);
    out3 = a5 + SRAI_H(a6, 2);
    out5 = a6 - SRAI_H(a5, 2);
    out7 = SRAI_H(a4, 2) - a7;

    LD_UH8(scale_matrix, 8,
           diff0, diff1, diff2, diff3, diff4, diff5, diff6, diff7);
    LD_UH8(threshold_matrix, 8,
           thr0, thr1, thr2, thr3, thr4, thr5, thr6, thr7);
    FORWARD_QUANT_8(out0, thr0, diff0, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_8(out1, thr1, diff1, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_8(out2, thr2, diff2, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_8(out3, thr3, diff3, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_8(out4, thr4, diff4, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_8(out5, thr5, diff5, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_8(out6, thr6, diff6, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_8(out7, thr7, diff7, round, qbits_v, nonzero_coeff);

    *nnz = nonzero_coeff;
    ST_SH8(out0, out1, out2, out3, out4, out5, out6, out7, dst, 8);
}

static void avc_resi_trans_quant_4x4_msa(UWORD8 *src, WORD32 src_stride,
                                         UWORD8 *pred, WORD32 pred_stride,
                                         WORD16 *dst, UWORD8 *nnz,
                                         WORD16 *alt_dc_addr,
                                         const UWORD16 *scale_matrix,
                                         const UWORD16 *threshold_matrix,
                                         UWORD32 qbits, UWORD32 round_factor)
{
    UWORD32 nonzero_coeff = 0;
    WORD32 src0_w, src1_w, src2_w, src3_w, pred0_w, pred1_w, pred2_w, pred3_w;
    UWORD64 dst0, dst1, dst2, dst3;
    v16u8 src0 = { 0 };
    v16u8 src1 = { 0 };
    v16u8 src2 = { 0 };
    v16u8 src3 = { 0 };
    v16u8 pred0 = { 0 };
    v16u8 pred1 = { 0 };
    v16u8 pred2 = { 0 };
    v16u8 pred3 = { 0 };
    v8i16 diff0, diff1, diff2, diff3, thr0, thr1, thr2, thr3;
    v8i16 temp0, temp1, temp2, temp3, a0, a1, a2, a3, out0, out1, out2, out3;
    v4i32 round = __msa_fill_w(round_factor);
    v4i32 qbits_v = __msa_fill_w(qbits);

    LW4(src, src_stride, src0_w, src1_w, src2_w, src3_w);
    LW4(pred, pred_stride, pred0_w, pred1_w, pred2_w, pred3_w);

    src0 = (v16u8)__msa_insert_w((v4i32)src0, 0, src0_w);
    src1 = (v16u8)__msa_insert_w((v4i32)src1, 0, src1_w);
    src2 = (v16u8)__msa_insert_w((v4i32)src2, 0, src2_w);
    src3 = (v16u8)__msa_insert_w((v4i32)src3, 0, src3_w);
    pred0 = (v16u8)__msa_insert_w((v4i32)pred0, 0, pred0_w);
    pred1 = (v16u8)__msa_insert_w((v4i32)pred1, 0, pred1_w);
    pred2 = (v16u8)__msa_insert_w((v4i32)pred2, 0, pred2_w);
    pred3 = (v16u8)__msa_insert_w((v4i32)pred3, 0, pred3_w);

    ILVR_B4_SH(src0, pred0, src1, pred1, src2, pred2, src3, pred3,
               diff0, diff1, diff2, diff3);
    HSUB_UB4_SH(diff0, diff1, diff2, diff3, temp0, temp1, temp2, temp3);
    TRANSPOSE4x4_SH_SH(temp0, temp1, temp2, temp3, temp0, temp1, temp2, temp3);
    BUTTERFLY_4(temp0, temp1, temp2, temp3, a0, a1, a2, a3);

    out0 = a0 + a1;
    out1 = SLLI_H(a3, 1) + a2;
    out2 = a0 - a1;
    out3 = a3 - SLLI_H(a2, 1);
    TRANSPOSE4x4_SH_SH(out0, out1, out2, out3, out0, out1, out2, out3);
    BUTTERFLY_4(out0, out1, out2, out3, a0, a1, a2, a3);

    out0 = a0 + a1;
    out1 = SLLI_H(a3, 1) + a2;
    out2 = a0 - a1;
    out3 = a3 - SLLI_H(a2, 1);

    (*alt_dc_addr) = __msa_copy_s_h(out0, 0);
    LD4x4_SH(scale_matrix, diff0, diff1, diff2, diff3);
    LD4x4_SH(threshold_matrix, thr0, thr1, thr2, thr3);
    FORWARD_QUANT_4(out0, thr0, diff0, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_4(out1, thr1, diff1, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_4(out2, thr2, diff2, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_4(out3, thr3, diff3, round, qbits_v, nonzero_coeff);

    *nnz = nonzero_coeff;
    dst0 = __msa_copy_u_d((v2i64)out0, 0);
    dst1 = __msa_copy_u_d((v2i64)out1, 0);
    dst2 = __msa_copy_u_d((v2i64)out2, 0);
    dst3 = __msa_copy_u_d((v2i64)out3, 0);

    SD4(dst0, dst1, dst2, dst3, dst, 4);
}

static void avc_resi_trans_quant_chroma_4x4_msa(UWORD8 *src, WORD32 src_stride,
                                                UWORD8 *pred,
                                                WORD32 pred_stride,
                                                WORD16 *dst, UWORD8 *nnz,
                                                WORD16 *dc_alt_addr,
                                                const UWORD16 *scale_matrix,
                                                const UWORD16 *threshold_matrix,
                                                UWORD32 qbits,
                                                UWORD32 round_factor)
{
    UWORD32 nonzero_coeff = 0;
    UWORD64 dst0, dst1, dst2, dst3;
    v16u8 src0 = { 0 };
    v16u8 src1 = { 0 };
    v16u8 src2 = { 0 };
    v16u8 src3 = { 0 };
    v16u8 pred0 = { 0 };
    v16u8 pred1 = { 0 };
    v16u8 pred2 = { 0 };
    v16u8 pred3 = { 0 };
    v8i16 diff0, diff1, diff2, diff3, thr0, thr1, thr2, thr3;
    v8i16 temp0, temp1, temp2, temp3, a0, a1, a2, a3, out0, out1, out2, out3;
    v4i32 round = __msa_fill_w(round_factor);
    v4i32 qbits_v = __msa_fill_w(qbits);

    LD_UB4(src, src_stride, src0, src1, src2, src3);
    LD_UB4(pred, pred_stride, pred0, pred1, pred2, pred3);

    ILVEV_B2_SH(pred0, src0, pred1, src1, diff0, diff1);
    ILVEV_B2_SH(pred2, src2, pred3, src3, diff2, diff3);
    HSUB_UB4_SH(diff0, diff1, diff2, diff3, temp0, temp1, temp2, temp3);
    TRANSPOSE4x4_SH_SH(temp0, temp1, temp2, temp3, temp0, temp1, temp2, temp3);
    BUTTERFLY_4(temp0, temp1, temp2, temp3, a0, a1, a2, a3);

    out0 = a0 + a1;
    out1 = SLLI_H(a3, 1) + a2;
    out2 = a0 - a1;
    out3 = a3 - SLLI_H(a2, 1);

    TRANSPOSE4x4_SH_SH(out0, out1, out2, out3, out0, out1, out2, out3);
    BUTTERFLY_4(out0, out1, out2, out3, a0, a1, a2, a3);

    out0 = a0 + a1;
    out1 = SLLI_H(a3, 1) + a2;
    out2 = a0 - a1;
    out3 = a3 - SLLI_H(a2, 1);

    (*dc_alt_addr) = __msa_copy_s_h(out0, 0);
    LD4x4_SH(scale_matrix, diff0, diff1, diff2, diff3);
    LD4x4_SH(threshold_matrix, thr0, thr1, thr2, thr3);
    FORWARD_QUANT_4(out0, thr0, diff0, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_4(out1, thr1, diff1, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_4(out2, thr2, diff2, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_4(out3, thr3, diff3, round, qbits_v, nonzero_coeff);

    *nnz = nonzero_coeff;
    dst0 = __msa_copy_u_d((v2i64)out0, 0);
    dst1 = __msa_copy_u_d((v2i64)out1, 0);
    dst2 = __msa_copy_u_d((v2i64)out2, 0);
    dst3 = __msa_copy_u_d((v2i64)out3, 0);

    SD4(dst0, dst1, dst2, dst3, dst, 4);
}

static void avc_hadamard_quant_4x4_msa(WORD16 *src, WORD16 *dst, UWORD8 *nnz,
                                       const UWORD16 *scale_matrix,
                                       const UWORD16 *threshold_matrix,
                                       UWORD32 qbits, UWORD32 round_factor)
{
    UWORD32 nonzero_coeff = 0;
    UWORD64 dst0, dst1, dst2, dst3;
    v8i16 src0, src1, src2, src3, temp0, temp1, temp2, temp3, thr, scale;
    v4i32 round = __msa_fill_w(round_factor);
    v4i32 qbits_v = __msa_fill_w(qbits);

    *nnz = 0;
    thr = __msa_fill_h(threshold_matrix[0]);
    scale = __msa_fill_h(scale_matrix[0]);

    LD4x4_SH(src, src0, src1, src2, src3);
    TRANSPOSE4x4_SH_SH(src0, src1, src2, src3, src0, src1, src2, src3);
    BUTTERFLY_4(src0, src1, src2, src3, temp0, temp1, temp2, temp3);
    BUTTERFLY_4(temp0, temp3, temp2, temp1, src0, src1, src3, src2);

    TRANSPOSE4x4_SH_SH(src0, src1, src2, src3, src0, src1, src2, src3);
    BUTTERFLY_4(src0, src1, src2, src3, temp0, temp1, temp2, temp3);
    BUTTERFLY_4(temp0, temp3, temp2, temp1, src0, src1, src3, src2);
    SRAI_H4_SH(src0, src1, src2, src3, 1);

    FORWARD_QUANT_4(src0, thr, scale, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_4(src1, thr, scale, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_4(src2, thr, scale, round, qbits_v, nonzero_coeff);
    FORWARD_QUANT_4(src3, thr, scale, round, qbits_v, nonzero_coeff);

    *nnz = nonzero_coeff;
    dst0 = __msa_copy_u_d((v2i64)src0, 0);
    dst1 = __msa_copy_u_d((v2i64)src1, 0);
    dst2 = __msa_copy_u_d((v2i64)src2, 0);
    dst3 = __msa_copy_u_d((v2i64)src3, 0);

    SD4(dst0, dst1, dst2, dst3, dst, 4);
}

void ih264_resi_trans_quant_4x4_msa(UWORD8 *pu1_src,
                                    UWORD8 *pu1_pred,
                                    WORD16 *pi2_out,
                                    WORD32 src_strd,
                                    WORD32 pred_strd,
                                    const UWORD16 *pu2_scale_matrix,
                                    const UWORD16 *pu2_threshold_matrix,
                                    UWORD32 u4_qbits,
                                    UWORD32 u4_round_factor,
                                    UWORD8 *pu1_nnz,
                                    WORD16 *pi2_alt_dc_addr)
{
    avc_resi_trans_quant_4x4_msa(pu1_src, src_strd, pu1_pred, pred_strd,
                                 pi2_out, pu1_nnz, pi2_alt_dc_addr,
                                 pu2_scale_matrix, pu2_threshold_matrix,
                                 u4_qbits, u4_round_factor);
}

void ih264_resi_trans_quant_chroma_4x4_msa(UWORD8 *pu1_src,
                                           UWORD8 *pu1_pred,
                                           WORD16 *pi2_out,
                                           WORD32 src_strd,
                                           WORD32 pred_strd,
                                           const UWORD16 *pu2_scale_matrix,
                                           const UWORD16 *pu2_threshold_matrix,
                                           UWORD32 u4_qbits,
                                           UWORD32 u4_round_factor,
                                           UWORD8 *pu1_nnz,
                                           WORD16 *pu1_dc_alt_addr)
{
    avc_resi_trans_quant_chroma_4x4_msa(pu1_src, src_strd, pu1_pred, pred_strd,
                                        pi2_out, pu1_nnz, pu1_dc_alt_addr,
                                        pu2_scale_matrix, pu2_threshold_matrix,
                                        u4_qbits, u4_round_factor);
}

void ih264_hadamard_quant_4x4_msa(WORD16 *pi2_src,
                                  WORD16 *pi2_dst,
                                  const UWORD16 *pu2_scale_matrix,
                                  const UWORD16 *pu2_threshold_matrix,
                                  UWORD32 u4_qbits,
                                  UWORD32 u4_round_factor,
                                  UWORD8 *pu1_nnz)
{
    avc_hadamard_quant_4x4_msa(pi2_src, pi2_dst, pu1_nnz, pu2_scale_matrix,
                               pu2_threshold_matrix, u4_qbits, u4_round_factor);
}

void ih264_resi_trans_quant_8x8_msa(UWORD8 *pu1_src,
                                    UWORD8 *pu1_pred,
                                    WORD16 *pi2_out,
                                    WORD32 src_strd,
                                    WORD32 pred_strd,
                                    const UWORD16 *pu2_scale_matrix,
                                    const UWORD16 *pu2_threshold_matrix,
                                    UWORD32 u4_qbits,
                                    UWORD32 u4_round_factor,
                                    UWORD8 *pu1_nnz,
                                    WORD16 *pu1_dc_alt_addr)
{
    UNUSED(pu1_dc_alt_addr);

    avc_resi_trans_quant_8x8_msa(pu1_src, src_strd, pu1_pred, pred_strd,
                                 pi2_out, pu1_nnz, pu2_scale_matrix,
                                 pu2_threshold_matrix, u4_qbits,
                                 u4_round_factor);
}
