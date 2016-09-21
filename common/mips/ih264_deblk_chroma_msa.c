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
#include "ih264_platform_macros.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_macros.h"
#include "ih264_macros_msa.h"

#define AVC_LPF_P0_OR_Q0(p0_or_q0_org_in, q1_or_p1_org_in,  \
                         p1_or_q1_org_in, p0_or_q0_out)     \
{                                                           \
    p0_or_q0_out = p0_or_q0_org_in + q1_or_p1_org_in;       \
    p0_or_q0_out += p1_or_q1_org_in;                        \
    p0_or_q0_out += p1_or_q1_org_in;                        \
    p0_or_q0_out = __msa_srari_h(p0_or_q0_out, 2);          \
}

#define AVC_LPF_P0Q0(q0_or_p0_org_in, p0_or_q0_org_in,          \
                     p1_or_q1_org_in, q1_or_p1_org_in,          \
                     negate_threshold_in, threshold_in,         \
                     p0_or_q0_out, q0_or_p0_out)                \
{                                                               \
    v8i16 q0_sub_p0, p1_sub_q1, delta;                          \
                                                                \
    q0_sub_p0 = q0_or_p0_org_in - p0_or_q0_org_in;              \
    p1_sub_q1 = p1_or_q1_org_in - q1_or_p1_org_in;              \
    q0_sub_p0 = SLLI_H(q0_sub_p0, 2);                           \
    p1_sub_q1 = ADDVI_H(p1_sub_q1, 4);                          \
    delta = q0_sub_p0 + p1_sub_q1;                              \
    delta = SRAI_H(delta, 3);                                   \
                                                                \
    delta = CLIP_SH(delta, negate_threshold_in, threshold_in);  \
                                                                \
    p0_or_q0_out = p0_or_q0_org_in + delta;                     \
    q0_or_p0_out = q0_or_p0_org_in - delta;                     \
                                                                \
    CLIP_SH2_0_255(p0_or_q0_out, q0_or_p0_out);                 \
}

static void avc_lpf_chroma_interleaved_intra_hz_msa(UWORD8 *src,
                                                    WORD32 src_stride,
                                                    UWORD8 alpha_cb,
                                                    UWORD8 beta_cb,
                                                    UWORD8 alpha_cr,
                                                    UWORD8 beta_cr)
{
    UWORD16 alpha_val, beta_val;
    v16u8 alpha, beta, is_less_than;
    v16u8 p0, q0, p1_org, p0_org, q0_org, q1_org;
    v8i16 p0_r = { 0 }, q0_r = { 0 }, p0_l = { 0 }, q0_l = { 0 };

    alpha_val = ((alpha_cr << 8) & 0xFF00) | (alpha_cb & 0xFF);
    beta_val = ((beta_cr << 8) & 0xFF00) | (beta_cb & 0xFF);

    alpha = (v16u8)__msa_fill_h(alpha_val);
    beta = (v16u8)__msa_fill_h(beta_val);

    LD_UB4(src - (src_stride << 1), src_stride, p1_org, p0_org, q0_org, q1_org);

    {
        v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0;
        v16u8 is_less_than_alpha, is_less_than_beta;

        p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
        p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
        q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less_than = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less_than = is_less_than_beta & is_less_than;
    }

    if(!__msa_test_bz_v(is_less_than))
    {
        v16i8 zero = { 0 };
        v16u8 is_less_than_r, is_less_than_l;

        is_less_than_r = (v16u8)__msa_sldi_b((v16i8)is_less_than, zero, 8);
        if(!__msa_test_bz_v(is_less_than_r))
        {
            v8i16 p1_org_r, p0_org_r, q0_org_r, q1_org_r;

            ILVR_B4_SH(zero, p1_org, zero, p0_org, zero, q0_org,
                       zero, q1_org, p1_org_r, p0_org_r, q0_org_r,
                       q1_org_r);
            AVC_LPF_P0_OR_Q0(p0_org_r, q1_org_r, p1_org_r, p0_r);
            AVC_LPF_P0_OR_Q0(q0_org_r, p1_org_r, q1_org_r, q0_r);
        }

        is_less_than_l = (v16u8)__msa_sldi_b(zero, (v16i8)is_less_than, 8);
        if(!__msa_test_bz_v(is_less_than_l))
        {
            v8i16 p1_org_l, p0_org_l, q0_org_l, q1_org_l;

            ILVL_B4_SH(zero, p1_org, zero, p0_org, zero, q0_org,
                       zero, q1_org, p1_org_l, p0_org_l, q0_org_l,
                       q1_org_l);
            AVC_LPF_P0_OR_Q0(p0_org_l, q1_org_l, p1_org_l, p0_l);
            AVC_LPF_P0_OR_Q0(q0_org_l, p1_org_l, q1_org_l, q0_l);
        }

        PCKEV_B2_UB(p0_l, p0_r, q0_l, q0_r, p0, q0);

        p0_org = __msa_bmnz_v(p0_org, p0, is_less_than);
        q0_org = __msa_bmnz_v(q0_org, q0, is_less_than);

        ST_UB(p0_org, (src - src_stride));
        ST_UB(q0_org, src);
    }
}

static void avc_lpf_chroma_interleaved_intra_vt_msa(UWORD8 *src,
                                                    WORD32 src_stride,
                                                    UWORD8 alpha_cb,
                                                    UWORD8 beta_cb,
                                                    UWORD8 alpha_cr,
                                                    UWORD8 beta_cr)
{
    UWORD8 *data;
    v16u8 alpha, beta, alpha_c, beta_c;
    v16u8 p0, q0, p1_org, p0_org, q0_org, q1_org;
    v16u8 p1_u, p0_u, q0_u, q1_u, p1_v, p0_v, q0_v, q1_v;
    v16u8 tmp0, tmp1, tmp2, tmp3, vec0, vec1;
    v16u8 row0, row1, row2, row3, row4, row5, row6, row7;
    v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0;
    v16u8 is_less_than_beta, is_less_than_alpha;
    v16u8 is_less_than, is_less_than_r, is_less_than_l;
    v16i8 zero = { 0 };
    v8i16 p0_r = { 0 }, q0_r = { 0 }, p0_l = { 0 }, q0_l = { 0 };

#ifdef CLANG_BUILD
        asm volatile (
        #if(__mips == 64)
            "daddiu  %[data],  %[src], -4  \n\t"
        #else
            "addiu   %[data],  %[src], -4  \n\t"
        #endif

            : [data] "=r" (data)
            : [src] "r" (src)
        );
#else
        data = src - 4;
#endif

    LD_UB8(data, src_stride,
           row0, row1, row2, row3, row4, row5, row6, row7);

    TRANSPOSE8x8_UB_UB(row0, row1, row2, row3, row4, row5, row6, row7,
                       p1_u, p1_v, p0_u, p0_v, q0_u, q0_v, q1_u, q1_v);

    ILVR_D4_UB(p1_v, p1_u, p0_v, p0_u, q0_v, q0_u, q1_v, q1_u, p1_org, p0_org,
               q0_org, q1_org);

    p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
    p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
    q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

    alpha = (v16u8)__msa_fill_b(alpha_cb);
    beta = (v16u8)__msa_fill_b(beta_cb);
    alpha_c = (v16u8)__msa_fill_b(alpha_cr);
    beta_c = (v16u8)__msa_fill_b(beta_cr);

    ILVR_D2_UB(alpha_c, alpha, beta_c, beta, alpha, beta);

    is_less_than_alpha = (p0_asub_q0 < alpha);
    is_less_than_beta = (p1_asub_p0 < beta);
    is_less_than = is_less_than_beta & is_less_than_alpha;
    is_less_than_beta = (q1_asub_q0 < beta);
    is_less_than = is_less_than_beta & is_less_than;

    if(!__msa_test_bz_v(is_less_than))
    {
        is_less_than_r = (v16u8)__msa_sldi_b((v16i8)is_less_than, zero, 8);
        if(!__msa_test_bz_v(is_less_than_r))
        {
            v8i16 p1_org_r, p0_org_r, q0_org_r, q1_org_r;

            ILVR_B4_SH(zero, p1_org, zero, p0_org, zero, q0_org,
                       zero, q1_org, p1_org_r, p0_org_r, q0_org_r, q1_org_r);
            AVC_LPF_P0_OR_Q0(p0_org_r, q1_org_r, p1_org_r, p0_r);
            AVC_LPF_P0_OR_Q0(q0_org_r, p1_org_r, q1_org_r, q0_r);
        }

        is_less_than_l = (v16u8)__msa_sldi_b(zero, (v16i8)is_less_than, 8);
        if(!__msa_test_bz_v(is_less_than_l))
        {
            v8i16 p1_org_l, p0_org_l, q0_org_l, q1_org_l;

            ILVL_B4_SH(zero, p1_org, zero, p0_org, zero, q0_org,
                       zero, q1_org, p1_org_l, p0_org_l, q0_org_l, q1_org_l);
            AVC_LPF_P0_OR_Q0(p0_org_l, q1_org_l, p1_org_l, p0_l);
            AVC_LPF_P0_OR_Q0(q0_org_l, p1_org_l, q1_org_l, q0_l);
        }

        PCKEV_B2_UB(p0_l, p0_r, q0_l, q0_r, p0, q0);

        p0_org = __msa_bmnz_v(p0_org, p0, is_less_than);
        q0_org = __msa_bmnz_v(q0_org, q0, is_less_than);

        SLDI_B2_0_UB(p0_org, q0_org, p0_v, q0_v, 8);
        ILVR_D2_UB(p0_v, p0_org, q0_v, q0_org, tmp0, tmp1);
        ILVRL_B2_UB(tmp1, tmp0, tmp2, tmp3);
        ILVRL_B2_UB(tmp3, tmp2, vec0, vec1);
        ST4x8_UB(vec0, vec1, (src - 2), src_stride);
    }
}

static void avc_lpf_chroma_interleaved_intra_vt_mbaff_msa(UWORD8 *src,
                                                          WORD32 src_stride,
                                                          UWORD8 alpha_cb,
                                                          UWORD8 beta_cb,
                                                          UWORD8 alpha_cr,
                                                          UWORD8 beta_cr)
{
    UWORD8 *data;
    v16u8 alpha, beta, alpha_c, beta_c;
    v16u8 p0_up, q0_up, p1, p0, q0, q1;
    v16u8 p1_u, p0_u, q0_u, q1_u, p1_v, p0_v, q0_v, q1_v;
    v16u8 row0, row1, row2, row3, p0_asub_q0, p1_asub_p0, q1_asub_q0;
    v16u8 is_less_than_beta, is_less_than_alpha, is_less_than, is_less_than_r;
    v16i8 tmp0, tmp1, tmp2, tmp3, vec0, zero = { 0 };
    v8i16 p1_r, p0_r, q0_r, q1_r, p0_up_r = { 0 }, q0_up_r = { 0 };

#ifdef CLANG_BUILD
        asm volatile (
        #if(__mips == 64)
            "daddiu  %[data],  %[src], -4  \n\t"
        #else
            "addiu   %[data],  %[src], -4  \n\t"
        #endif

            : [data] "=r" (data)
            : [src] "r" (src)
        );
#else
        data = src - 4;
#endif

    LD_UB4(data, src_stride, row0, row1, row2, row3);

    TRANSPOSE4x8_UB_UB(row0, row1, row2, row3,
                       p1_u, p1_v, p0_u, p0_v, q0_u, q0_v, q1_u, q1_v);

    ILVR_W4_UB(p1_v, p1_u, p0_v, p0_u, q0_v, q0_u, q1_v, q1_u, p1, p0, q0, q1);

    p0_asub_q0 = __msa_asub_u_b(p0, q0);
    p1_asub_p0 = __msa_asub_u_b(p1, p0);
    q1_asub_q0 = __msa_asub_u_b(q1, q0);

    alpha = (v16u8)__msa_fill_b(alpha_cb);
    beta = (v16u8)__msa_fill_b(beta_cb);
    alpha_c = (v16u8)__msa_fill_b(alpha_cr);
    beta_c = (v16u8)__msa_fill_b(beta_cr);

    ILVR_W2_UB(alpha_c, alpha, beta_c, beta, alpha, beta);

    is_less_than_alpha = (p0_asub_q0 < alpha);
    is_less_than_beta = (p1_asub_p0 < beta);
    is_less_than = is_less_than_beta & is_less_than_alpha;
    is_less_than_beta = (q1_asub_q0 < beta);
    is_less_than = is_less_than_beta & is_less_than;

    if(!__msa_test_bz_v(is_less_than))
    {
        is_less_than_r = (v16u8)__msa_sldi_b((v16i8)is_less_than, zero, 8);
        if(!__msa_test_bz_v(is_less_than_r))
        {
            ILVR_B4_SH(zero, p1, zero, p0, zero, q0, zero, q1, p1_r, p0_r,
                       q0_r, q1_r);
            AVC_LPF_P0_OR_Q0(p0_r, q1_r, p1_r, p0_up_r);
            AVC_LPF_P0_OR_Q0(q0_r, p1_r, q1_r, q0_up_r);
        }

        PCKEV_B2_UB(p0_up_r, p0_up_r, q0_up_r, q0_up_r, p0_up, q0_up);

        p0 = __msa_bmnz_v(p0, p0_up, is_less_than);
        q0 = __msa_bmnz_v(q0, q0_up, is_less_than);

        SLDI_B2_0_UB(p0, q0, p0_v, q0_v, 4);
        ILVR_W2_SB(p0_v, p0, q0_v, q0, tmp0, tmp1);
        ILVRL_B2_SB(tmp1, tmp0, tmp2, tmp3);
        vec0 = __msa_ilvr_b(tmp3, tmp2);
        ST4x4_UB(vec0, vec0, 0, 1, 2, 3, (src - 2), src_stride);
    }
}

static void avc_lpf_chroma_interleaved_inter_hz_msa(UWORD8 *src,
                                                    WORD32 src_stride,
                                                    UWORD8 alpha_cb,
                                                    UWORD8 beta_cb,
                                                    UWORD8 alpha_cr,
                                                    UWORD8 beta_cr,
                                                    UWORD8 bs0, UWORD8 bs1,
                                                    UWORD8 bs2, UWORD8 bs3,
                                                    const UWORD8 *cliptab_cb,
                                                    const UWORD8 *cliptab_cr)
{
    UWORD16 alpha_val, beta_val, tc0, tc1, tc2, tc3;
    v16u8 alpha, beta, is_less, p0_up, q0_up;
    v16u8 p1, p0, q0, q1, p0_asub_q0, p1_asub_p0, q1_asub_q0;
    v16u8 is_less_than_beta, is_less_than_alpha, is_bs_grter_than0;
    v16i8 negate_tc, sign_negate_tc, zero = { 0 };
    v8i16 is_less_r, is_less_l, negate_tc_r, negate_tc_l, tc_r, tc_l;
    v8i16 p1_r, p0_r, q0_r, q1_r, p1_l, p0_l, q0_l, q1_l;
    v8i16 p0_up_r = { 0 }, q0_up_r = { 0 }, p0_up_l = { 0 }, q0_up_l = { 0 };
    v4i32 tmp_vec, bs = { 0 };
    v4i32 tc = { 0 };

    tmp_vec = (v4i32)__msa_fill_b(bs0);
    bs = __msa_insve_w(bs, 0, tmp_vec);
    tmp_vec = (v4i32)__msa_fill_b(bs1);
    bs = __msa_insve_w(bs, 1, tmp_vec);
    tmp_vec = (v4i32)__msa_fill_b(bs2);
    bs = __msa_insve_w(bs, 2, tmp_vec);
    tmp_vec = (v4i32)__msa_fill_b(bs3);
    bs = __msa_insve_w(bs, 3, tmp_vec);

    if(!__msa_test_bz_v((v16u8)bs))
    {
        tc0 = (((cliptab_cr[bs0] + 1) << 8) & 0xFF00) |
              ((cliptab_cb[bs0] + 1) & 0xFF);
        tc1 = (((cliptab_cr[bs1] + 1) << 8) & 0xFF00) |
              ((cliptab_cb[bs1] + 1) & 0xFF);
        tc2 = (((cliptab_cr[bs2] + 1) << 8) & 0xFF00) |
              ((cliptab_cb[bs2] + 1) & 0xFF);
        tc3 = (((cliptab_cr[bs3] + 1) << 8) & 0xFF00) |
              ((cliptab_cb[bs3] + 1) & 0xFF);

        tmp_vec = (v4i32)__msa_fill_h(tc0);
        tc = __msa_insve_w(tc, 0, tmp_vec);
        tmp_vec = (v4i32)__msa_fill_h(tc1);
        tc = __msa_insve_w(tc, 1, tmp_vec);
        tmp_vec = (v4i32)__msa_fill_h(tc2);
        tc = __msa_insve_w(tc, 2, tmp_vec);
        tmp_vec = (v4i32)__msa_fill_h(tc3);
        tc = __msa_insve_w(tc, 3, tmp_vec);

        is_bs_grter_than0 = (v16u8)(zero < (v16i8)bs);

        alpha_val = ((alpha_cr << 8) & 0xFF00) | (alpha_cb & 0xFF);
        beta_val = ((beta_cr << 8) & 0xFF00) | (beta_cb & 0xFF);

        alpha = (v16u8)__msa_fill_h(alpha_val);
        beta = (v16u8)__msa_fill_h(beta_val);

        LD_UB4(src - (src_stride << 1), src_stride, p1, p0, q0, q1);

        p0_asub_q0 = __msa_asub_u_b(p0, q0);
        p1_asub_p0 = __msa_asub_u_b(p1, p0);
        q1_asub_q0 = __msa_asub_u_b(q1, q0);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less = is_less_than_beta & is_less;

        is_less = is_less & is_bs_grter_than0;

        if(!__msa_test_bz_v(is_less))
        {
            negate_tc = zero - (v16i8)tc;
            sign_negate_tc = CLTI_S_B(negate_tc, 0);

            ILVRL_B2_SH(sign_negate_tc, negate_tc, negate_tc_r, negate_tc_l);

            UNPCK_UB_SH(tc, tc_r, tc_l);
            UNPCK_UB_SH(p1, p1_r, p1_l);
            UNPCK_UB_SH(p0, p0_r, p0_l);
            UNPCK_UB_SH(q0, q0_r, q0_l);
            UNPCK_UB_SH(q1, q1_r, q1_l);

            is_less_r = (v8i16)__msa_sldi_b((v16i8)is_less, zero, 8);
            if(!__msa_test_bz_v((v16u8)is_less_r))
            {
                AVC_LPF_P0Q0(q0_r, p0_r, p1_r, q1_r, negate_tc_r, tc_r,
                             p0_up_r, q0_up_r);
            }

            is_less_l = (v8i16)__msa_sldi_b(zero, (v16i8)is_less, 8);
            if(!__msa_test_bz_v((v16u8)is_less_l))
            {
                AVC_LPF_P0Q0(q0_l, p0_l, p1_l, q1_l, negate_tc_l, tc_l,
                             p0_up_l, q0_up_l);
            }

            PCKEV_B2_UB(p0_up_l, p0_up_r, q0_up_l, q0_up_r, p0_up, q0_up);

            p0 = __msa_bmnz_v(p0, p0_up, is_less);
            q0 = __msa_bmnz_v(q0, q0_up, is_less);

            ST_UB(p0, src - src_stride);
            ST_UB(q0, src);
        }
    }
}

static void avc_lpf_chroma_interleaved_inter_vt_msa(UWORD8 *src,
                                                    WORD32 src_stride,
                                                    UWORD8 alpha_cb,
                                                    UWORD8 beta_cb,
                                                    UWORD8 alpha_cr,
                                                    UWORD8 beta_cr,
                                                    UWORD8 bs0, UWORD8 bs1,
                                                    UWORD8 bs2, UWORD8 bs3,
                                                    const UWORD8 *cliptab_cb,
                                                    const UWORD8 *cliptab_cr)
{
    UWORD8 *data;
    v16u8 alpha, beta, alpha_c, beta_c;
    v16u8 p0_up, q0_up, p0_asub_q0, p1_asub_p0, q1_asub_q0, p1, p0, q0, q1;
    v16u8 is_less, is_less_than_beta, is_less_than_alpha, is_bs_grter_than0;
    v16u8 p1_u, p0_u, q0_u, q1_u, p1_v, p0_v, q0_v, q1_v;
    v16u8 row0, row1, row2, row3, row4, row5, row6, row7;
    v16i8 tmp0, tmp1, tmp2, tmp3, vec0, vec1;
    v16i8 neg_tc, sign_neg_tc, zero = { 0 };
    v8i16 is_less_r, is_less_l, tc_r, tc_l, neg_tc_r, neg_tc_l;
    v8i16 p0_up_r = { 0 }, q0_up_r = { 0 }, p0_up_l = { 0 }, q0_up_l = { 0 };
    v8i16 p1_r, p0_r, q0_r, q1_r, p1_l, p0_l, q0_l, q1_l;
    v8i16 tmp_vec, tmp_clipcb, tmp_clipcr, bs = { 0 };
    v8i16 tc = { 0 };
    v16u8 const4 = (v16u8)__msa_ldi_b(4);

    tmp_vec = (v8i16)__msa_fill_b(bs0);
    bs = __msa_insve_h(bs, 0, tmp_vec);
    bs = __msa_insve_h(bs, 4, tmp_vec);

    tmp_vec = (v8i16)__msa_fill_b(bs1);
    bs = __msa_insve_h(bs, 1, tmp_vec);
    bs = __msa_insve_h(bs, 5, tmp_vec);

    tmp_vec = (v8i16)__msa_fill_b(bs2);
    bs = __msa_insve_h(bs, 2, tmp_vec);
    bs = __msa_insve_h(bs, 6, tmp_vec);

    tmp_vec = (v8i16)__msa_fill_b(bs3);
    bs = __msa_insve_h(bs, 3, tmp_vec);
    bs = __msa_insve_h(bs, 7, tmp_vec);

    if(!__msa_test_bz_v((v16u8)bs))
    {
        tmp_clipcb = (v8i16)__msa_fill_b((cliptab_cb[bs0] + 1));
        tmp_clipcr = (v8i16)__msa_fill_b((cliptab_cr[bs0] + 1));
        tc = __msa_insve_h(tc, 0, tmp_clipcb);
        tc = __msa_insve_h(tc, 4, tmp_clipcr);

        tmp_clipcb = (v8i16)__msa_fill_b((cliptab_cb[bs1] + 1));
        tmp_clipcr = (v8i16)__msa_fill_b((cliptab_cr[bs1] + 1));
        tc = __msa_insve_h(tc, 1, tmp_clipcb);
        tc = __msa_insve_h(tc, 5, tmp_clipcr);

        tmp_clipcb = (v8i16)__msa_fill_b((cliptab_cb[bs2] + 1));
        tmp_clipcr = (v8i16)__msa_fill_b((cliptab_cr[bs2] + 1));
        tc = __msa_insve_h(tc, 2, tmp_clipcb);
        tc = __msa_insve_h(tc, 6, tmp_clipcr);

        tmp_clipcb = (v8i16)__msa_fill_b((cliptab_cb[bs3] + 1));
        tmp_clipcr = (v8i16)__msa_fill_b((cliptab_cr[bs3] + 1));
        tc = __msa_insve_h(tc, 3, tmp_clipcb);
        tc = __msa_insve_h(tc, 7, tmp_clipcr);

        is_bs_grter_than0 = (v16u8)(zero < (v16i8)bs);

#ifdef CLANG_BUILD
        asm volatile (
        #if(__mips == 64)
            "daddiu  %[data],  %[src], -4  \n\t"
        #else
            "addiu   %[data],  %[src], -4  \n\t"
        #endif

            : [data] "=r" (data)
            : [src] "r" (src)
        );
#else
        data = src - 4;
#endif

        LD_UB8(data, src_stride,
               row0, row1, row2, row3, row4, row5, row6, row7);

        TRANSPOSE8x8_UB_UB(row0, row1, row2, row3, row4, row5, row6, row7,
                           p1_u, p1_v, p0_u, p0_v, q0_u, q0_v, q1_u, q1_v);

        ILVR_D4_UB(p1_v, p1_u, p0_v, p0_u, q0_v, q0_u, q1_v, q1_u, p1, p0, q0,
                   q1);

        p0_asub_q0 = __msa_asub_u_b(p0, q0);
        p1_asub_p0 = __msa_asub_u_b(p1, p0);
        q1_asub_q0 = __msa_asub_u_b(q1, q0);

        alpha = (v16u8)__msa_fill_b(alpha_cb);
        beta = (v16u8)__msa_fill_b(beta_cb);
        alpha_c = (v16u8)__msa_fill_b(alpha_cr);
        beta_c = (v16u8)__msa_fill_b(beta_cr);

        ILVR_D2_UB(alpha_c, alpha, beta_c, beta, alpha, beta);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less = is_less_than_beta & is_less;
        is_less = is_bs_grter_than0 & is_less;

        if(!__msa_test_bz_v(is_less))
        {
            UNPCK_UB_SH(p1, p1_r, p1_l);
            UNPCK_UB_SH(p0, p0_r, p0_l);
            UNPCK_UB_SH(q0, q0_r, q0_l);
            UNPCK_UB_SH(q1, q1_r, q1_l);

            is_less = is_less & ((v16u8)bs < const4);

            if(!__msa_test_bz_v((v16u8)is_less))
            {
                neg_tc = zero - (v16i8)tc;
                sign_neg_tc = CLTI_S_B(neg_tc, 0);

                ILVRL_B2_SH(sign_neg_tc, neg_tc, neg_tc_r, neg_tc_l);

                UNPCK_UB_SH(tc, tc_r, tc_l);

                is_less_r = (v8i16)__msa_sldi_b((v16i8)is_less, zero, 8);
                if(!__msa_test_bz_v((v16u8)is_less_r))
                {
                    AVC_LPF_P0Q0(q0_r, p0_r, p1_r, q1_r, neg_tc_r, tc_r,
                                 p0_up_r, q0_up_r);
                }

                is_less_l = (v8i16)__msa_sldi_b(zero, (v16i8)is_less, 8);
                if(!__msa_test_bz_v((v16u8)is_less_l))
                {
                    AVC_LPF_P0Q0(q0_l, p0_l, p1_l, q1_l, neg_tc_l, tc_l,
                                 p0_up_l, q0_up_l);
                }

                PCKEV_B2_UB(p0_up_l, p0_up_r, q0_up_l, q0_up_r, p0_up, q0_up);

                p0 = __msa_bmnz_v(p0, p0_up, is_less);
                q0 = __msa_bmnz_v(q0, q0_up, is_less);
            }

            SLDI_B2_0_UB(p0, q0, p0_v, q0_v, 8);
            ILVR_D2_SB(p0_v, p0, q0_v, q0, tmp0, tmp1);
            ILVRL_B2_SB(tmp1, tmp0, tmp2, tmp3);
            ILVRL_B2_SB(tmp3, tmp2, vec0, vec1);
            ST4x8_UB(vec0, vec1, (src - 2), src_stride);
        }
    }
}

static void avc_lpf_chroma_interleaved_inter_vt_mbaff_msa(UWORD8 *src,
                                                          WORD32 src_stride,
                                                          UWORD8 alpha_cb,
                                                          UWORD8 beta_cb,
                                                          UWORD8 alpha_cr,
                                                          UWORD8 beta_cr,
                                                          UWORD8 bs0, UWORD8 bs1,
                                                          UWORD8 bs2, UWORD8 bs3,
                                                          const UWORD8 *cliptab_cb,
                                                          const UWORD8 *cliptab_cr)
{
    UWORD8 *data;
    v16u8 alpha, beta, alpha_c, beta_c;
    v16u8 p0_up, q0_up, p0_asub_q0, p1_asub_p0, q1_asub_q0, p1, p0, q0, q1;
    v16u8 is_less, is_less_than_beta, is_less_than_alpha, is_bs_grter_than0;
    v16u8 p1_u, p0_u, q0_u, q1_u, p1_v, p0_v, q0_v, q1_v;
    v16u8 row0, row1, row2, row3;
    v16i8 tmp0, tmp1, tmp2, tmp3, vec0, neg_tc, sign_neg_tc, zero = { 0 };
    v8i16 is_less_r, tc_r, neg_tc_r, p1_r, p0_r, q0_r, q1_r;
    v8i16 p0_up_r = { 0 }, q0_up_r = { 0 };
    v16i8 tmp_vec, tmp_clipcb, tmp_clipcr, bs = { 0 };
    v16i8 tc = { 0 };
    v16u8 const4 = (v16u8)__msa_ldi_b(4);

    tmp_vec = __msa_fill_b(bs0);
    bs = __msa_insve_b(bs, 0, tmp_vec);
    bs = __msa_insve_b(bs, 4, tmp_vec);

    tmp_vec = __msa_fill_b(bs1);
    bs = __msa_insve_b(bs, 1, tmp_vec);
    bs = __msa_insve_b(bs, 5, tmp_vec);

    tmp_vec = __msa_fill_b(bs2);
    bs = __msa_insve_b(bs, 2, tmp_vec);
    bs = __msa_insve_b(bs, 6, tmp_vec);

    tmp_vec = __msa_fill_b(bs3);
    bs = __msa_insve_b(bs, 3, tmp_vec);
    bs = __msa_insve_b(bs, 7, tmp_vec);

    if(!__msa_test_bz_v((v16u8)bs))
    {
        tmp_clipcb = __msa_fill_b((cliptab_cb[bs0] + 1));
        tmp_clipcr = __msa_fill_b((cliptab_cr[bs0] + 1));
        tc = __msa_insve_b(tc, 0, tmp_clipcb);
        tc = __msa_insve_b(tc, 4, tmp_clipcr);

        tmp_clipcb = __msa_fill_b((cliptab_cb[bs1] + 1));
        tmp_clipcr = __msa_fill_b((cliptab_cr[bs1] + 1));
        tc = __msa_insve_b(tc, 1, tmp_clipcb);
        tc = __msa_insve_b(tc, 5, tmp_clipcr);

        tmp_clipcb = __msa_fill_b((cliptab_cb[bs2] + 1));
        tmp_clipcr = __msa_fill_b((cliptab_cr[bs2] + 1));
        tc = __msa_insve_b(tc, 2, tmp_clipcb);
        tc = __msa_insve_b(tc, 6, tmp_clipcr);

        tmp_clipcb = __msa_fill_b((cliptab_cb[bs3] + 1));
        tmp_clipcr = __msa_fill_b((cliptab_cr[bs3] + 1));
        tc = __msa_insve_b(tc, 3, tmp_clipcb);
        tc = __msa_insve_b(tc, 7, tmp_clipcr);

        is_bs_grter_than0 = (v16u8)(zero < bs);

#ifdef CLANG_BUILD
        asm volatile (
        #if(__mips == 64)
            "daddiu  %[data],  %[src], -4  \n\t"
        #else
            "addiu   %[data],  %[src], -4  \n\t"
        #endif

            : [data] "=r" (data)
            : [src] "r" (src)
        );
#else
        data = src - 4;
#endif

        LD_UB4(data, src_stride, row0, row1, row2, row3);

        TRANSPOSE4x8_UB_UB(row0, row1, row2, row3,
                           p1_u, p1_v, p0_u, p0_v, q0_u, q0_v, q1_u, q1_v);

        ILVR_W4_UB(p1_v, p1_u, p0_v, p0_u, q0_v, q0_u, q1_v, q1_u, p1, p0, q0,
                   q1);

        p0_asub_q0 = __msa_asub_u_b(p0, q0);
        p1_asub_p0 = __msa_asub_u_b(p1, p0);
        q1_asub_q0 = __msa_asub_u_b(q1, q0);

        alpha = (v16u8)__msa_fill_b(alpha_cb);
        beta = (v16u8)__msa_fill_b(beta_cb);
        alpha_c = (v16u8)__msa_fill_b(alpha_cr);
        beta_c = (v16u8)__msa_fill_b(beta_cr);

        ILVR_W2_UB(alpha_c, alpha, beta_c, beta, alpha, beta);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less = is_less_than_beta & is_less;
        is_less = is_bs_grter_than0 & is_less;

        if(!__msa_test_bz_v(is_less))
        {
            ILVR_B4_SH(zero, p1, zero, p0, zero, q0, zero, q1, p1_r, p0_r,
                       q0_r, q1_r);

            is_less = is_less & ((v16u8)bs < const4);

            if(!__msa_test_bz_v((v16u8)is_less))
            {
                neg_tc = zero - tc;
                sign_neg_tc = CLTI_S_B(neg_tc, 0);

                ILVR_B2_SH(sign_neg_tc, neg_tc, zero, tc, neg_tc_r, tc_r);

                is_less_r = (v8i16)__msa_sldi_b((v16i8)is_less, zero, 8);
                if(!__msa_test_bz_v((v16u8)is_less_r))
                {
                    AVC_LPF_P0Q0(q0_r, p0_r, p1_r, q1_r, neg_tc_r, tc_r,
                                 p0_up_r, q0_up_r);
                }

                PCKEV_B2_UB(p0_up_r, p0_up_r, q0_up_r, q0_up_r, p0_up, q0_up);

                p0 = __msa_bmnz_v(p0, p0_up, is_less);
                q0 = __msa_bmnz_v(q0, q0_up, is_less);
            }

            SLDI_B2_0_UB(p0, q0, p0_v, q0_v, 4);
            ILVR_W2_SB(p0_v, p0, q0_v, q0, tmp0, tmp1);
            ILVRL_B2_SB(tmp1, tmp0, tmp2, tmp3);
            vec0 = __msa_ilvr_b(tmp3, tmp2);
            ST4x4_UB(vec0, vec0, 0, 1, 2, 3, (src - 2), src_stride);
        }
    }
}

void ih264_deblk_chroma_vert_bs4_msa(UWORD8 *src, WORD32 src_stride,
                                     WORD32 alpha_cb, WORD32 beta_cb,
                                     WORD32 alpha_cr, WORD32 beta_cr)
{
    avc_lpf_chroma_interleaved_intra_vt_msa(src, src_stride, alpha_cb,
                                            beta_cb, alpha_cr, beta_cr);
}

void ih264_deblk_chroma_horz_bs4_msa(UWORD8 *src, WORD32 src_stride,
                                     WORD32 alpha_cb, WORD32 beta_cb,
                                     WORD32 alpha_cr, WORD32 beta_cr)
{
    avc_lpf_chroma_interleaved_intra_hz_msa(src, src_stride, alpha_cb,
                                            beta_cb, alpha_cr, beta_cr);
}

void ih264_deblk_chroma_vert_bs4_mbaff_msa(UWORD8 *src, WORD32 src_stride,
                                           WORD32 alpha_cb, WORD32 beta_cb,
                                           WORD32 alpha_cr, WORD32 beta_cr)
{
    avc_lpf_chroma_interleaved_intra_vt_mbaff_msa(src, src_stride, alpha_cb,
                                                  beta_cb, alpha_cr, beta_cr);
}

void ih264_deblk_chroma_vert_bslt4_msa(UWORD8 *src, WORD32 src_stride,
                                       WORD32 alpha_cb, WORD32 beta_cb,
                                       WORD32 alpha_cr, WORD32 beta_cr,
                                       UWORD32 bs, const UWORD8 *cliptab_cb,
                                       const UWORD8 *cliptab_cr)
{
    UWORD8 bs0 = (bs >> 24) & 0x0ff;
    UWORD8 bs1 = (bs >> 16) & 0x0ff;
    UWORD8 bs2 = (bs >> 8) & 0x0ff;
    UWORD8 bs3 = bs & 0x0ff;

    avc_lpf_chroma_interleaved_inter_vt_msa(src, src_stride, alpha_cb,
                                            beta_cb, alpha_cr, beta_cr,
                                            bs0, bs1, bs2, bs3, cliptab_cb,
                                            cliptab_cr);
}

void ih264_deblk_chroma_horz_bslt4_msa(UWORD8 *src, WORD32 src_stride,
                                       WORD32 alpha_cb, WORD32 beta_cb,
                                       WORD32 alpha_cr, WORD32 beta_cr,
                                       UWORD32 bs, const UWORD8 *cliptab_cb,
                                       const UWORD8 *cliptab_cr)
{
    UWORD8 bs0 = (bs >> 24) & 0x0ff;
    UWORD8 bs1 = (bs >> 16) & 0x0ff;
    UWORD8 bs2 = (bs >> 8) & 0x0ff;
    UWORD8 bs3 = bs & 0x0ff;

    avc_lpf_chroma_interleaved_inter_hz_msa(src, src_stride, alpha_cb,
                                            beta_cb, alpha_cr, beta_cr,
                                            bs0, bs1, bs2, bs3, cliptab_cb,
                                            cliptab_cr);
}

void ih264_deblk_chroma_vert_bslt4_mbaff_msa(UWORD8 *src, WORD32 src_stride,
                                             WORD32 alpha_cb, WORD32 beta_cb,
                                             WORD32 alpha_cr, WORD32 beta_cr,
                                             UWORD32 bs,
                                             const UWORD8 *cliptab_cb,
                                             const UWORD8 *cliptab_cr)
{
    UWORD8 bs0 = (bs >> 24) & 0x0ff;
    UWORD8 bs1 = (bs >> 16) & 0x0ff;
    UWORD8 bs2 = (bs >> 8) & 0x0ff;
    UWORD8 bs3 = bs & 0x0ff;

    avc_lpf_chroma_interleaved_inter_vt_mbaff_msa(src, src_stride, alpha_cb,
                                                  beta_cb, alpha_cr, beta_cr,
                                                  bs0, bs1, bs2, bs3,
                                                  cliptab_cb, cliptab_cr);
}
