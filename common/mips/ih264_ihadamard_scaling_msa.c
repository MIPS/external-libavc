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
#include "ih264_macros.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_macros_msa.h"

static void avc_inv_hadamard_scaling_4x4_msa(WORD16 *src, WORD16 *dst,
                                             WORD16 quant_scale,
                                             WORD16 weight_scale,
                                             WORD32 qp_div_6)
{
    WORD32 rnd_fact = (qp_div_6 < 6) ? (1 << (5 - qp_div_6)) : 0;
    v8i16 src0, src1, diff0, diff1, diff2, diff3, temp0, temp1, temp2, temp3;
    v4i32 src0_w, src1_w, src2_w, src3_w, scale_w, weigh_w, rndfact_w;
    v4i32 qp_div_6_v = __msa_fill_w(qp_div_6);

    LD4x4_SH(src, diff0, diff1, diff2, diff3);
    TRANSPOSE4x4_SH_SH(diff0, diff1, diff2, diff3, diff0, diff1, diff2, diff3);
    BUTTERFLY_4(diff0, diff1, diff2, diff3, temp0, temp1, temp2, temp3);
    BUTTERFLY_4(temp0, temp3, temp2, temp1, diff0, diff1, diff3, diff2);
    TRANSPOSE4x4_SH_SH(diff0, diff1, diff2, diff3, diff0, diff1, diff2, diff3);
    BUTTERFLY_4(diff0, diff1, diff2, diff3, temp0, temp1, temp2, temp3);
    BUTTERFLY_4(temp0, temp3, temp2, temp1, diff0, diff1, diff3, diff2);
    PCKEV_D2_SH(diff1, diff0, diff3, diff2, src0, src1);

    scale_w = __msa_fill_w(quant_scale);
    weigh_w = __msa_fill_w(weight_scale);
    rndfact_w = __msa_fill_w(rnd_fact);

    UNPCK_SH_SW(src0, src0_w, src1_w);
    UNPCK_SH_SW(src1, src2_w, src3_w);
    MUL4(src0_w, scale_w, src1_w, scale_w, src2_w, scale_w, src3_w, scale_w,
         src0_w, src1_w, src2_w, src3_w);
    MUL4(src0_w, weigh_w, src1_w, weigh_w, src2_w, weigh_w, src3_w, weigh_w,
         src0_w, src1_w, src2_w, src3_w);
    ADD4(src0_w, rndfact_w, src1_w, rndfact_w, src2_w, rndfact_w, src3_w,
         rndfact_w, src0_w, src1_w, src2_w, src3_w);
    SLL_4V(src0_w, src1_w, src2_w, src3_w, qp_div_6_v);
    SRAI_W4_SW(src0_w, src1_w, src2_w, src3_w, 6);
    PCKEV_H2_SH(src1_w, src0_w, src3_w, src2_w, src0, src1);
    ST_SH2(src0, src1, dst, 8);
}

void ih264_ihadamard_scaling_4x4_msa(WORD16 *src, WORD16 *dst,
                                     const UWORD16 *quant_scale,
                                     const UWORD16 *weight_scale,
                                     UWORD32 qp_div_6, WORD32 *tmp)
{
    UNUSED(tmp);

    avc_inv_hadamard_scaling_4x4_msa(src, dst, quant_scale[0], weight_scale[0],
                                     qp_div_6);
}
