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

#define AVC_LPF_P0P1P2_OR_Q0Q1Q2(p3_or_q3_org_in, p0_or_q0_org_in,          \
                                 q3_or_p3_org_in, p1_or_q1_org_in,          \
                                 p2_or_q2_org_in, q1_or_p1_org_in,          \
                                 p0_or_q0_out, p1_or_q1_out, p2_or_q2_out)  \
{                                                                           \
    v8i16 threshold;                                                        \
    v8i16 const3 = __msa_ldi_h(3);                                          \
                                                                            \
    threshold = p0_or_q0_org_in + q3_or_p3_org_in;                          \
    threshold += p1_or_q1_org_in;                                           \
                                                                            \
    p0_or_q0_out = SRAI_H(threshold, 1);                                    \
    p0_or_q0_out += p2_or_q2_org_in;                                        \
    p0_or_q0_out += q1_or_p1_org_in;                                        \
    p0_or_q0_out = __msa_srari_h(p0_or_q0_out, 3);                          \
                                                                            \
    p1_or_q1_out = p2_or_q2_org_in + threshold;                             \
    p1_or_q1_out = __msa_srari_h(p1_or_q1_out, 2);                          \
                                                                            \
    p2_or_q2_out = p2_or_q2_org_in * const3;                                \
    p2_or_q2_out += p3_or_q3_org_in;                                        \
    p2_or_q2_out += p3_or_q3_org_in;                                        \
    p2_or_q2_out += threshold;                                              \
    p2_or_q2_out = __msa_srari_h(p2_or_q2_out, 3);                          \
}

#define AVC_LPF_P0_OR_Q0(p0_or_q0_org_in, q1_or_p1_org_in,  \
                         p1_or_q1_org_in, p0_or_q0_out)     \
{                                                           \
    p0_or_q0_out = p0_or_q0_org_in + q1_or_p1_org_in;       \
    p0_or_q0_out += p1_or_q1_org_in;                        \
    p0_or_q0_out += p1_or_q1_org_in;                        \
    p0_or_q0_out = __msa_srari_h(p0_or_q0_out, 2);          \
}

#define AVC_LPF_P1_OR_Q1(p0_or_q0_org_in, q0_or_p0_org_in,   \
                         p1_or_q1_org_in, p2_or_q2_org_in,   \
                         negate_tc_in, tc_in, p1_or_q1_out)  \
{                                                            \
    v8i16 clip3, temp;                                       \
                                                             \
    clip3 = (v8i16)__msa_aver_u_h((v8u16)p0_or_q0_org_in,    \
                                  (v8u16)q0_or_p0_org_in);   \
    temp = SLLI_H(p1_or_q1_org_in, 1);                       \
    clip3 -= temp;                                           \
    clip3 = __msa_ave_s_h(p2_or_q2_org_in, clip3);           \
    clip3 = CLIP_SH(clip3, negate_tc_in, tc_in);             \
    p1_or_q1_out = p1_or_q1_org_in + clip3;                  \
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

static void avc_loopfilter_luma_intra_edge_hor_msa(UWORD8 *data,
                                                   UWORD8 alpha_in,
                                                   UWORD8 beta_in,
                                                   UWORD32 img_width)
{
    v16u8 p2_asub_p0, q2_asub_q0, p0_asub_q0;
    v16u8 alpha, beta;
    v16u8 is_less_than, is_less_than_beta, negate_is_less_than_beta;
    v16u8 p2, p1, p0, q0, q1, q2;
    v16u8 p3_org, p2_org, p1_org, p0_org, q0_org, q1_org, q2_org, q3_org;
    v8i16 p1_org_r, p0_org_r, q0_org_r, q1_org_r;
    v8i16 p1_org_l, p0_org_l, q0_org_l, q1_org_l;
    v8i16 p2_r = { 0 };
    v8i16 p1_r = { 0 };
    v8i16 p0_r = { 0 };
    v8i16 q0_r = { 0 };
    v8i16 q1_r = { 0 };
    v8i16 q2_r = { 0 };
    v8i16 p2_l = { 0 };
    v8i16 p1_l = { 0 };
    v8i16 p0_l = { 0 };
    v8i16 q0_l = { 0 };
    v8i16 q1_l = { 0 };
    v8i16 q2_l = { 0 };
    v16u8 tmp_flag;
    v16i8 zero = { 0 };

    alpha = (v16u8)__msa_fill_b(alpha_in);
    beta = (v16u8)__msa_fill_b(beta_in);

    LD_UB4(data - (img_width << 1), img_width, p1_org, p0_org, q0_org, q1_org);

    {
        v16u8 p1_asub_p0, q1_asub_q0, is_less_than_alpha;

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
        q2_org = LD_UB(data + (2 * img_width));
        p3_org = LD_UB(data - (img_width << 2));
        p2_org = LD_UB(data - (3 * img_width));

        UNPCK_UB_SH(p1_org, p1_org_r, p1_org_l);
        UNPCK_UB_SH(p0_org, p0_org_r, p0_org_l);
        UNPCK_UB_SH(q0_org, q0_org_r, q0_org_l);

        tmp_flag = (v16u8)SRLI_B(alpha, 2);
        tmp_flag = (v16u8)ADDVI_B(tmp_flag, 2);
        tmp_flag = (p0_asub_q0 < tmp_flag);

        p2_asub_p0 = __msa_asub_u_b(p2_org, p0_org);
        is_less_than_beta = (p2_asub_p0 < beta);
        is_less_than_beta = is_less_than_beta & tmp_flag;
        negate_is_less_than_beta = XORI_B(is_less_than_beta, 0xff);
        is_less_than_beta = is_less_than_beta & is_less_than;
        negate_is_less_than_beta = negate_is_less_than_beta & is_less_than;
        {
            v8u16 is_less_than_beta_l, is_less_than_beta_r;

            q1_org_r = (v8i16)__msa_ilvr_b(zero, (v16i8)q1_org);

            is_less_than_beta_r =
                (v8u16)__msa_sldi_b((v16i8)is_less_than_beta, zero, 8);
            if(!__msa_test_bz_v((v16u8)is_less_than_beta_r))
            {
                v8i16 p3_org_r;

                ILVR_B2_SH(zero, p3_org, zero, p2_org, p3_org_r, p2_r);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(p3_org_r, p0_org_r, q0_org_r, p1_org_r,
                                         p2_r, q1_org_r, p0_r, p1_r, p2_r);
            }

            q1_org_l = (v8i16)__msa_ilvl_b(zero, (v16i8)q1_org);

            is_less_than_beta_l =
                (v8u16)__msa_sldi_b(zero, (v16i8)is_less_than_beta, 8);

            if(!__msa_test_bz_v((v16u8)is_less_than_beta_l))
            {
                v8i16 p3_org_l;

                ILVL_B2_SH(zero, p3_org, zero, p2_org, p3_org_l, p2_l);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(p3_org_l, p0_org_l, q0_org_l, p1_org_l,
                                         p2_l, q1_org_l, p0_l, p1_l, p2_l);
            }
        }
        /* combine and store */
        if(!__msa_test_bz_v(is_less_than_beta))
        {
            PCKEV_B3_UB(p0_l, p0_r, p1_l, p1_r, p2_l, p2_r, p0, p1, p2);

            p0_org = __msa_bmnz_v(p0_org, p0, is_less_than_beta);
            p1_org = __msa_bmnz_v(p1_org, p1, is_less_than_beta);
            p2_org = __msa_bmnz_v(p2_org, p2, is_less_than_beta);

            ST_UB(p1_org, data - (2 * img_width));
            ST_UB(p2_org, data - (3 * img_width));
        }
        {
            v8u16 negate_is_less_than_beta_r, negate_is_less_than_beta_l;

            negate_is_less_than_beta_r =
                (v8u16)__msa_sldi_b((v16i8)negate_is_less_than_beta, zero, 8);
            if(!__msa_test_bz_v((v16u8)negate_is_less_than_beta_r))
            {
                AVC_LPF_P0_OR_Q0(p0_org_r, q1_org_r, p1_org_r, p0_r);
            }

            negate_is_less_than_beta_l =
                (v8u16)__msa_sldi_b(zero, (v16i8)negate_is_less_than_beta, 8);
            if(!__msa_test_bz_v((v16u8)negate_is_less_than_beta_l))
            {
                AVC_LPF_P0_OR_Q0(p0_org_l, q1_org_l, p1_org_l, p0_l);
            }
        }
        /* combine */
        if(!__msa_test_bz_v(negate_is_less_than_beta))
        {
            p0 = (v16u8)__msa_pckev_b((v16i8)p0_l, (v16i8)p0_r);
            p0_org = __msa_bmnz_v(p0_org, p0, negate_is_less_than_beta);
        }

        ST_UB(p0_org, data - img_width);

        /* if(tmpFlag && (unsigned)ABS(q2-q0) < thresholds->beta_in) */
        q3_org = LD_UB(data + (3 * img_width));
        q2_asub_q0 = __msa_asub_u_b(q2_org, q0_org);
        is_less_than_beta = (q2_asub_q0 < beta);
        is_less_than_beta = is_less_than_beta & tmp_flag;
        negate_is_less_than_beta = XORI_B(is_less_than_beta, 0xff);
        is_less_than_beta = is_less_than_beta & is_less_than;
        negate_is_less_than_beta = negate_is_less_than_beta & is_less_than;

        {
            v8u16 is_less_than_beta_l, is_less_than_beta_r;
            is_less_than_beta_r =
                (v8u16)__msa_sldi_b((v16i8)is_less_than_beta, zero, 8);
            if(!__msa_test_bz_v((v16u8)is_less_than_beta_r))
            {
                v8i16 q3_org_r;

                ILVR_B2_SH(zero, q3_org, zero, q2_org, q3_org_r, q2_r);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(q3_org_r, q0_org_r, p0_org_r, q1_org_r,
                                         q2_r, p1_org_r, q0_r, q1_r, q2_r);
            }
            is_less_than_beta_l =
                (v8u16)__msa_sldi_b(zero, (v16i8)is_less_than_beta, 8);
            if(!__msa_test_bz_v((v16u8)is_less_than_beta_l))
            {
                v8i16 q3_org_l;

                ILVL_B2_SH(zero, q3_org, zero, q2_org, q3_org_l, q2_l);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(q3_org_l, q0_org_l, p0_org_l, q1_org_l,
                                         q2_l, p1_org_l, q0_l, q1_l, q2_l);
            }
        }

        /* combine and store */
        if(!__msa_test_bz_v(is_less_than_beta))
        {
            PCKEV_B3_UB(q0_l, q0_r, q1_l, q1_r, q2_l, q2_r, q0, q1, q2);
            q0_org = __msa_bmnz_v(q0_org, q0, is_less_than_beta);
            q1_org = __msa_bmnz_v(q1_org, q1, is_less_than_beta);
            q2_org = __msa_bmnz_v(q2_org, q2, is_less_than_beta);

            ST_UB(q1_org, data + img_width);
            ST_UB(q2_org, data + 2 * img_width);
        }
        {
            v8u16 negate_is_less_than_beta_r, negate_is_less_than_beta_l;
            negate_is_less_than_beta_r =
                (v8u16)__msa_sldi_b((v16i8)negate_is_less_than_beta, zero, 8);
            if(!__msa_test_bz_v((v16u8)negate_is_less_than_beta_r))
            {
                AVC_LPF_P0_OR_Q0(q0_org_r, p1_org_r, q1_org_r, q0_r);
            }

            negate_is_less_than_beta_l =
                (v8u16)__msa_sldi_b(zero, (v16i8)negate_is_less_than_beta, 8);
            if(!__msa_test_bz_v((v16u8)negate_is_less_than_beta_l))
            {
                AVC_LPF_P0_OR_Q0(q0_org_l, p1_org_l, q1_org_l, q0_l);
            }
        }
        /* combine */
        if(!__msa_test_bz_v(negate_is_less_than_beta))
        {
            q0 = (v16u8)__msa_pckev_b((v16i8)q0_l, (v16i8)q0_r);
            q0_org = __msa_bmnz_v(q0_org, q0, negate_is_less_than_beta);
        }
        ST_UB(q0_org, data);
    }
}

static void avc_loopfilter_luma_intra_edge_ver_msa(UWORD8 *data,
                                                   UWORD8 alpha_in,
                                                   UWORD8 beta_in,
                                                   UWORD32 img_width)
{
    UWORD8 *src;
    v16u8 alpha, beta, p0_asub_q0;
    v16u8 is_less_than_alpha, is_less_than;
    v16u8 is_less_than_beta, negate_is_less_than_beta;
    v16u8 p3_org, p2_org, p1_org, p0_org, q0_org, q1_org, q2_org, q3_org;
    v8i16 p1_org_r, p0_org_r, q0_org_r, q1_org_r;
    v8i16 p1_org_l, p0_org_l, q0_org_l, q1_org_l;
    v8i16 p2_r = { 0 };
    v8i16 p1_r = { 0 };
    v8i16 p0_r = { 0 };
    v8i16 q0_r = { 0 };
    v8i16 q1_r = { 0 };
    v8i16 q2_r = { 0 };
    v8i16 p2_l = { 0 };
    v8i16 p1_l = { 0 };
    v8i16 p0_l = { 0 };
    v8i16 q0_l = { 0 };
    v8i16 q1_l = { 0 };
    v8i16 q2_l = { 0 };
    v16i8 zero = { 0 };
    v16u8 tmp_flag;

#ifdef CLANG_BUILD
    asm volatile (
    #if (__mips == 64)
        "daddiu  %[src],  %[data], -4  \n\t"
    #else
        "addiu   %[src],  %[data], -4  \n\t"
    #endif

        : [src] "=r" (src)
        : [data] "r" (data)
    );
#else
    src = data - 4;
#endif

    {
        v16u8 row0, row1, row2, row3, row4, row5, row6, row7;
        v16u8 row8, row9, row10, row11, row12, row13, row14, row15;

        LD_UB8(src, img_width, row0, row1, row2, row3, row4, row5, row6, row7);
        LD_UB8(src + (8 * img_width), img_width,
               row8, row9, row10, row11, row12, row13, row14, row15);

        TRANSPOSE16x8_UB_UB(row0, row1, row2, row3,
                            row4, row5, row6, row7,
                            row8, row9, row10, row11,
                            row12, row13, row14, row15,
                            p3_org, p2_org, p1_org, p0_org,
                            q0_org, q1_org, q2_org, q3_org);
    }
    UNPCK_UB_SH(p1_org, p1_org_r, p1_org_l);
    UNPCK_UB_SH(p0_org, p0_org_r, p0_org_l);
    UNPCK_UB_SH(q0_org, q0_org_r, q0_org_l);
    UNPCK_UB_SH(q1_org, q1_org_r, q1_org_l);

    /* if( ((unsigned)ABS(p0-q0) < thresholds->alpha_in) &&
           ((unsigned)ABS(p1-p0) < thresholds->beta_in)  &&
           ((unsigned)ABS(q1-q0) < thresholds->beta_in) )   */
    {
        v16u8 p1_asub_p0, q1_asub_q0;

        p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
        p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
        q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

        alpha = (v16u8)__msa_fill_b(alpha_in);
        beta = (v16u8)__msa_fill_b(beta_in);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less_than = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less_than = is_less_than_beta & is_less_than;
    }

    if(!__msa_test_bz_v(is_less_than))
    {
        tmp_flag = (v16u8)SRLI_B(alpha, 2);
        tmp_flag = (v16u8)ADDVI_B(tmp_flag, 2);
        tmp_flag = (p0_asub_q0 < tmp_flag);

        {
            v16u8 p2_asub_p0;

            p2_asub_p0 = __msa_asub_u_b(p2_org, p0_org);
            is_less_than_beta = (p2_asub_p0 < beta);
        }
        is_less_than_beta = tmp_flag & is_less_than_beta;
        negate_is_less_than_beta = XORI_B(is_less_than_beta, 0xff);
        is_less_than_beta = is_less_than_beta & is_less_than;
        negate_is_less_than_beta = negate_is_less_than_beta & is_less_than;

        /* right */
        {
            v16u8 is_less_than_beta_r;

            is_less_than_beta_r =
                (v16u8)__msa_sldi_b((v16i8)is_less_than_beta, zero, 8);
            if(!__msa_test_bz_v(is_less_than_beta_r))
            {
                v8i16 p3_org_r;

                ILVR_B2_SH(zero, p3_org, zero, p2_org, p3_org_r, p2_r);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(p3_org_r, p0_org_r, q0_org_r, p1_org_r,
                                         p2_r, q1_org_r, p0_r, p1_r, p2_r);
            }
        }
        /* left */
        {
            v16u8 is_less_than_beta_l;

            is_less_than_beta_l =
                (v16u8)__msa_sldi_b(zero, (v16i8)is_less_than_beta, 8);
            if(!__msa_test_bz_v(is_less_than_beta_l))
            {
                v8i16 p3_org_l;

                ILVL_B2_SH(zero, p3_org, zero, p2_org, p3_org_l, p2_l);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(p3_org_l, p0_org_l, q0_org_l, p1_org_l,
                                         p2_l, q1_org_l, p0_l, p1_l, p2_l);
            }
        }
        /* combine and store */
        if(!__msa_test_bz_v(is_less_than_beta))
        {
            v16u8 p0, p2, p1;

            PCKEV_B3_UB(p0_l, p0_r, p1_l, p1_r, p2_l, p2_r, p0, p1, p2);
            p0_org = __msa_bmnz_v(p0_org, p0, is_less_than_beta);
            p1_org = __msa_bmnz_v(p1_org, p1, is_less_than_beta);
            p2_org = __msa_bmnz_v(p2_org, p2, is_less_than_beta);
        }
        /* right */
        {
            v16u8 negate_is_less_than_beta_r;

            negate_is_less_than_beta_r =
                (v16u8)__msa_sldi_b((v16i8)negate_is_less_than_beta, zero, 8);

            if(!__msa_test_bz_v(negate_is_less_than_beta_r))
            {
                AVC_LPF_P0_OR_Q0(p0_org_r, q1_org_r, p1_org_r, p0_r);
            }
        }
        /* left */
        {
            v16u8 negate_is_less_than_beta_l;

            negate_is_less_than_beta_l =
                (v16u8)__msa_sldi_b(zero, (v16i8)negate_is_less_than_beta, 8);
            if(!__msa_test_bz_v(negate_is_less_than_beta_l))
            {
                AVC_LPF_P0_OR_Q0(p0_org_l, q1_org_l, p1_org_l, p0_l);
            }
        }

        if(!__msa_test_bz_v(negate_is_less_than_beta))
        {
            v16u8 p0;

            p0 = (v16u8)__msa_pckev_b((v16i8)p0_l, (v16i8)p0_r);
            p0_org = __msa_bmnz_v(p0_org, p0, negate_is_less_than_beta);
        }

        {
            v16u8 q2_asub_q0;

            q2_asub_q0 = __msa_asub_u_b(q2_org, q0_org);
            is_less_than_beta = (q2_asub_q0 < beta);
        }

        is_less_than_beta = is_less_than_beta & tmp_flag;
        negate_is_less_than_beta = XORI_B(is_less_than_beta, 0xff);

        is_less_than_beta = is_less_than_beta & is_less_than;
        negate_is_less_than_beta = negate_is_less_than_beta & is_less_than;

        /* right */
        {
            v16u8 is_less_than_beta_r;

            is_less_than_beta_r =
                (v16u8)__msa_sldi_b((v16i8)is_less_than_beta, zero, 8);
            if(!__msa_test_bz_v(is_less_than_beta_r))
            {
                v8i16 q3_org_r;

                ILVR_B2_SH(zero, q3_org, zero, q2_org, q3_org_r, q2_r);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(q3_org_r, q0_org_r, p0_org_r, q1_org_r,
                                         q2_r, p1_org_r, q0_r, q1_r, q2_r);
            }
        }
        /* left */
        {
            v16u8 is_less_than_beta_l;

            is_less_than_beta_l =
                (v16u8)__msa_sldi_b(zero, (v16i8)is_less_than_beta, 8);
            if(!__msa_test_bz_v(is_less_than_beta_l))
            {
                v8i16 q3_org_l;

                ILVL_B2_SH(zero, q3_org, zero, q2_org, q3_org_l, q2_l);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(q3_org_l, q0_org_l, p0_org_l, q1_org_l,
                                         q2_l, p1_org_l, q0_l, q1_l, q2_l);
            }
        }
        /* combine and store */
        if(!__msa_test_bz_v(is_less_than_beta))
        {
            v16u8 q0, q1, q2;

            PCKEV_B3_UB(q0_l, q0_r, q1_l, q1_r, q2_l, q2_r, q0, q1, q2);
            q0_org = __msa_bmnz_v(q0_org, q0, is_less_than_beta);
            q1_org = __msa_bmnz_v(q1_org, q1, is_less_than_beta);
            q2_org = __msa_bmnz_v(q2_org, q2, is_less_than_beta);
        }

        /* right */
        {
            v16u8 negate_is_less_than_beta_r;

            negate_is_less_than_beta_r =
                (v16u8)__msa_sldi_b((v16i8)negate_is_less_than_beta, zero, 8);
            if(!__msa_test_bz_v(negate_is_less_than_beta_r))
            {
                AVC_LPF_P0_OR_Q0(q0_org_r, p1_org_r, q1_org_r, q0_r);
            }
        }
        /* left */
        {
            v16u8 negate_is_less_than_beta_l;

            negate_is_less_than_beta_l =
                (v16u8)__msa_sldi_b(zero, (v16i8)negate_is_less_than_beta, 8);
            if(!__msa_test_bz_v(negate_is_less_than_beta_l))
            {
                AVC_LPF_P0_OR_Q0(q0_org_l, p1_org_l, q1_org_l, q0_l);
            }
        }
        if(!__msa_test_bz_v(negate_is_less_than_beta))
        {
            v16u8 q0;

            q0 = (v16u8)__msa_pckev_b((v16i8)q0_l, (v16i8)q0_r);
            q0_org = __msa_bmnz_v(q0_org, q0, negate_is_less_than_beta);
        }
    }
    {
        v8i16 tp0, tp1, tp2, tp3, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

        ILVRL_B2_SH(p1_org, p2_org, tp0, tp2);
        ILVRL_B2_SH(q0_org, p0_org, tp1, tp3);
        ILVRL_B2_SH(q2_org, q1_org, tmp2, tmp5);

        ILVRL_H2_SH(tp1, tp0, tmp3, tmp4);
        ILVRL_H2_SH(tp3, tp2, tmp6, tmp7);

        src = data - 3;
        ST4x4_UB(tmp3, tmp3, 0, 1, 2, 3, src, img_width);
        ST2x4_UB(tmp2, 0, src + 4, img_width);
        src += 4 * img_width;
        ST4x4_UB(tmp4, tmp4, 0, 1, 2, 3, src, img_width);
        ST2x4_UB(tmp2, 4, src + 4, img_width);
        src += 4 * img_width;

        ST4x4_UB(tmp6, tmp6, 0, 1, 2, 3, src, img_width);
        ST2x4_UB(tmp5, 0, src + 4, img_width);
        src += 4 * img_width;
        ST4x4_UB(tmp7, tmp7, 0, 1, 2, 3, src, img_width);
        ST2x4_UB(tmp5, 4, src + 4, img_width);
    }
}

static void avc_h_loop_filter_luma_mbaff_intra_msa(UWORD8 *src, WORD32 stride,
                                                   WORD32 alpha_in,
                                                   WORD32 beta_in)
{
    UWORD64 load0, load1;
    UWORD32 out0, out2;
    UWORD16 out1, out3;
    v8u16 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v8u16 dst0_r, dst1_r, dst4_r, dst5_r;
    v8u16 dst2_x_r, dst2_y_r, dst3_x_r, dst3_y_r;
    v8u16 const2 = (v8u16)__msa_ldi_h(2);
    v8u16 const3 = (v8u16)__msa_ldi_h(3);
    v16u8 dst0, dst1, dst4, dst5, dst2_x, dst2_y, dst3_x, dst3_y;
    v8i16 tmp0, tmp1, tmp2, tmp3;
    v16u8 alpha, beta;
    v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0, p2_asub_p0, q2_asub_q0;
    v16u8 is_less_than, is_less_than_alpha, is_less_than_beta;
    v16u8 is_less_than_beta1, is_less_than_beta2;
    v16i8 src0 = { 0 };
    v16i8 src1 = { 0 };
    v16i8 src2 = { 0 };
    v16i8 src3 = { 0 };
    v16i8 src4 = { 0 };
    v16i8 src5 = { 0 };
    v16i8 src6 = { 0 };
    v16i8 src7 = { 0 };
    v16i8 zeros = { 0 };

    load0 = LD(src - 4);
    load1 = LD(src + stride - 4);
    src0 = (v16i8)__msa_insert_d((v2i64)src0, 0, load0);
    src1 = (v16i8)__msa_insert_d((v2i64)src1, 0, load1);

    load0 = LD(src + (2 * stride) - 4);
    load1 = LD(src + (3 * stride) - 4);
    src2 = (v16i8)__msa_insert_d((v2i64)src2, 0, load0);
    src3 = (v16i8)__msa_insert_d((v2i64)src3, 0, load1);

    load0 = LD(src + (4 * stride) - 4);
    load1 = LD(src + (5 * stride) - 4);
    src4 = (v16i8)__msa_insert_d((v2i64)src4, 0, load0);
    src5 = (v16i8)__msa_insert_d((v2i64)src5, 0, load1);

    load0 = LD(src + (6 * stride) - 4);
    load1 = LD(src + (7 * stride) - 4);
    src6 = (v16i8)__msa_insert_d((v2i64)src6, 0, load0);
    src7 = (v16i8)__msa_insert_d((v2i64)src7, 0, load1);

    ILVR_B4_SB(src1, src0, src3, src2, src5, src4, src7, src6,
               src0, src1, src2, src3);

    ILVR_H2_SH(src1, src0, src3, src2, tmp0, tmp2);
    ILVL_H2_SH(src1, src0, src3, src2, tmp1, tmp3);

    ILVR_W2_SB(tmp2, tmp0, tmp3, tmp1, src6, src3);
    ILVL_W2_SB(tmp2, tmp0, tmp3, tmp1, src1, src5);
    SLDI_B4_0_SB(src6, src1, src3, src5, src0, src2, src4, src7, 8);

    p0_asub_q0 = __msa_asub_u_b((v16u8)src2, (v16u8)src3);
    p1_asub_p0 = __msa_asub_u_b((v16u8)src1, (v16u8)src2);
    q1_asub_q0 = __msa_asub_u_b((v16u8)src4, (v16u8)src3);

    alpha = (v16u8)__msa_fill_b(alpha_in);
    beta = (v16u8)__msa_fill_b(beta_in);

    is_less_than_alpha = (p0_asub_q0 < alpha);
    is_less_than_beta = (p1_asub_p0 < beta);
    is_less_than = is_less_than_alpha & is_less_than_beta;
    is_less_than_beta = (q1_asub_q0 < beta);
    is_less_than = is_less_than & is_less_than_beta;

    alpha = (v16u8)SRLI_B(alpha, 2);
    alpha = (v16u8)ADDVI_B(alpha, 2);
    is_less_than_alpha = (p0_asub_q0 < alpha);

    p2_asub_p0 = __msa_asub_u_b((v16u8)src0, (v16u8)src2);
    is_less_than_beta1 = (p2_asub_p0 < beta);
    q2_asub_q0 = __msa_asub_u_b((v16u8)src5, (v16u8)src3);
    is_less_than_beta2 = (q2_asub_q0 < beta);

    ILVR_B4_UH(zeros, src0, zeros, src1, zeros, src2, zeros, src3,
               src0_r, src1_r, src2_r, src3_r);
    ILVR_B4_UH(zeros, src4, zeros, src5, zeros, src6, zeros, src7,
               src4_r, src5_r, src6_r, src7_r);

    dst2_x_r = src1_r + src2_r + src3_r;
    dst2_x_r = src0_r + (const2 * (dst2_x_r)) + src4_r;
    dst2_x_r = (v8u16)__msa_srari_h((v8i16)dst2_x_r, 3);
    dst1_r = src0_r + src1_r + src2_r + src3_r;
    dst1_r = (v8u16)__msa_srari_h((v8i16)dst1_r, 2);

    dst0_r = (const2 * src6_r) + (const3 * src0_r);
    dst0_r += src1_r + src2_r + src3_r;
    dst0_r = (v8u16)__msa_srari_h((v8i16)dst0_r, 3);
    dst2_y_r = (const2 * src1_r) + src2_r + src4_r;
    dst2_y_r = (v8u16)__msa_srari_h((v8i16)dst2_y_r, 2);

    PCKEV_B2_UB(dst2_x_r, dst2_x_r, dst2_y_r, dst2_y_r, dst2_x, dst2_y);
    dst2_x = __msa_bmnz_v(dst2_y, dst2_x, is_less_than_beta1);

    dst3_x_r = src2_r + src3_r + src4_r;
    dst3_x_r = src1_r + (const2 * dst3_x_r) + src5_r;
    dst3_x_r = (v8u16)__msa_srari_h((v8i16)dst3_x_r, 3);
    dst4_r = src2_r + src3_r + src4_r + src5_r;
    dst4_r = (v8u16)__msa_srari_h((v8i16)dst4_r, 2);

    dst5_r = (const2 * src7_r) + (const3 * src5_r);
    dst5_r += src4_r + src3_r + src2_r;
    dst5_r = (v8u16)__msa_srari_h((v8i16)dst5_r, 3);
    dst3_y_r = (const2 * src4_r) + src3_r + src1_r;
    dst3_y_r = (v8u16)__msa_srari_h((v8i16)dst3_y_r, 2);

    PCKEV_B2_UB(dst3_x_r, dst3_x_r, dst3_y_r, dst3_y_r, dst3_x, dst3_y);
    dst3_x = __msa_bmnz_v(dst3_y, dst3_x, is_less_than_beta2);

    dst2_y_r = (const2 * src1_r) + src2_r + src4_r;
    dst2_y_r = (v8u16)__msa_srari_h((v8i16)dst2_y_r, 2);
    dst3_y_r = (const2 * src4_r) + src3_r + src1_r;
    dst3_y_r = (v8u16)__msa_srari_h((v8i16)dst3_y_r, 2);

    PCKEV_B2_UB(dst2_y_r, dst2_y_r, dst3_y_r, dst3_y_r, dst2_y, dst3_y);

    dst2_x = __msa_bmnz_v(dst2_y, dst2_x, is_less_than_alpha);
    dst3_x = __msa_bmnz_v(dst3_y, dst3_x, is_less_than_alpha);
    dst2_x = __msa_bmnz_v((v16u8)src2, dst2_x, is_less_than);
    dst3_x = __msa_bmnz_v((v16u8)src3, dst3_x, is_less_than);

    is_less_than = is_less_than_alpha & is_less_than;
    dst1 = (v16u8)__msa_pckev_b((v16i8)dst1_r, (v16i8)dst1_r);
    is_less_than_beta1 = is_less_than_beta1 & is_less_than;
    dst1 = __msa_bmnz_v((v16u8)src1, dst1, is_less_than_beta1);

    dst0 = (v16u8)__msa_pckev_b((v16i8)dst0_r, (v16i8)dst0_r);
    dst0 = __msa_bmnz_v((v16u8)src0, dst0, is_less_than_beta1);
    dst4 = (v16u8)__msa_pckev_b((v16i8)dst4_r, (v16i8)dst4_r);
    is_less_than_beta2 = is_less_than_beta2 & is_less_than;
    dst4 = __msa_bmnz_v((v16u8)src4, dst4, is_less_than_beta2);
    dst5 = (v16u8)__msa_pckev_b((v16i8)dst5_r, (v16i8)dst5_r);
    dst5 = __msa_bmnz_v((v16u8)src5, dst5, is_less_than_beta2);

    ILVR_B2_UB(dst1, dst0, dst3_x, dst2_x, dst0, dst1);
    dst2_x = (v16u8)__msa_ilvr_b((v16i8)dst5, (v16i8)dst4);
    ILVRL_H2_SH(dst1, dst0, tmp0, tmp1);
    ILVRL_H2_SH(zeros, dst2_x, tmp2, tmp3);

    ILVR_W2_UB(tmp2, tmp0, tmp3, tmp1, dst0, dst4);
    SLDI_B2_0_UB(dst0, dst4, dst1, dst5, 8);
    dst2_x = (v16u8)__msa_ilvl_w((v4i32)tmp2, (v4i32)tmp0);
    dst2_y = (v16u8)__msa_ilvl_w((v4i32)tmp3, (v4i32)tmp1);
    SLDI_B2_0_UB(dst2_x, dst2_y, dst3_x, dst3_y, 8);

    src -= 3;

    out0 = __msa_copy_u_w((v4i32)dst0, 0);
    out1 = __msa_copy_u_h((v8i16)dst0, 2);
    out2 = __msa_copy_u_w((v4i32)dst1, 0);
    out3 = __msa_copy_u_h((v8i16)dst1, 2);

    SW(out0, src);
    SH(out1, (src + 4));
    src += stride;
    SW(out2, src);
    SH(out3, (src + 4));
    src += stride;

    out0 = __msa_copy_u_w((v4i32)dst2_x, 0);
    out1 = __msa_copy_u_h((v8i16)dst2_x, 2);
    out2 = __msa_copy_u_w((v4i32)dst3_x, 0);
    out3 = __msa_copy_u_h((v8i16)dst3_x, 2);

    SW(out0, src);
    SH(out1, (src + 4));
    src += stride;
    SW(out2, src);
    SH(out3, (src + 4));
    src += stride;

    out0 = __msa_copy_u_w((v4i32)dst4, 0);
    out1 = __msa_copy_u_h((v8i16)dst4, 2);
    out2 = __msa_copy_u_w((v4i32)dst5, 0);
    out3 = __msa_copy_u_h((v8i16)dst5, 2);

    SW(out0, src);
    SH(out1, (src + 4));
    src += stride;
    SW(out2, src);
    SH(out3, (src + 4));
    src += stride;

    out0 = __msa_copy_u_w((v4i32)dst2_y, 0);
    out1 = __msa_copy_u_h((v8i16)dst2_y, 2);
    out2 = __msa_copy_u_w((v4i32)dst3_y, 0);
    out3 = __msa_copy_u_h((v8i16)dst3_y, 2);

    SW(out0, src);
    SH(out1, (src + 4));
    src += stride;
    SW(out2, src);
    SH(out3, (src + 4));
}

static void avc_loopfilter_luma_inter_edge_ver_msa(UWORD8 *data,
                                                   UWORD8 bs0, UWORD8 bs1,
                                                   UWORD8 bs2, UWORD8 bs3,
                                                   UWORD8 tc0, UWORD8 tc1,
                                                   UWORD8 tc2, UWORD8 tc3,
                                                   UWORD8 alpha_in,
                                                   UWORD8 beta_in,
                                                   UWORD32 img_width)
{
    UWORD8 *src;
    v16u8 beta, tmp_vec, bs = { 0 };
    v16u8 tc = { 0 };
    v16u8 is_less_than, is_less_than_beta;
    v16u8 p1, p0, q0, q1;
    v8i16 p0_r, q0_r, p1_r = { 0 };
    v8i16 q1_r = { 0 };
    v8i16 p0_l, q0_l, p1_l = { 0 };
    v8i16 q1_l = { 0 };
    v16u8 p3_org, p2_org, p1_org, p0_org, q0_org, q1_org, q2_org, q3_org;
    v8i16 p2_org_r, p1_org_r, p0_org_r, q0_org_r, q1_org_r, q2_org_r;
    v8i16 p2_org_l, p1_org_l, p0_org_l, q0_org_l, q1_org_l, q2_org_l;
    v8i16 tc_r, tc_l;
    v16i8 zero = { 0 };
    v16u8 is_bs_grter_than0;

    tmp_vec = (v16u8)__msa_fill_b(bs0);
    bs = (v16u8)__msa_insve_w((v4i32)bs, 0, (v4i32)tmp_vec);
    tmp_vec = (v16u8)__msa_fill_b(bs1);
    bs = (v16u8)__msa_insve_w((v4i32)bs, 1, (v4i32)tmp_vec);
    tmp_vec = (v16u8)__msa_fill_b(bs2);
    bs = (v16u8)__msa_insve_w((v4i32)bs, 2, (v4i32)tmp_vec);
    tmp_vec = (v16u8)__msa_fill_b(bs3);
    bs = (v16u8)__msa_insve_w((v4i32)bs, 3, (v4i32)tmp_vec);

    if(!__msa_test_bz_v(bs))
    {
        tmp_vec = (v16u8)__msa_fill_b(tc0);
        tc = (v16u8)__msa_insve_w((v4i32)tc, 0, (v4i32)tmp_vec);
        tmp_vec = (v16u8)__msa_fill_b(tc1);
        tc = (v16u8)__msa_insve_w((v4i32)tc, 1, (v4i32)tmp_vec);
        tmp_vec = (v16u8)__msa_fill_b(tc2);
        tc = (v16u8)__msa_insve_w((v4i32)tc, 2, (v4i32)tmp_vec);
        tmp_vec = (v16u8)__msa_fill_b(tc3);
        tc = (v16u8)__msa_insve_w((v4i32)tc, 3, (v4i32)tmp_vec);

        is_bs_grter_than0 = (zero < bs);

        {
            v16u8 row0, row1, row2, row3, row4, row5, row6, row7;
            v16u8 row8, row9, row10, row11, row12, row13, row14, row15;

#ifdef CLANG_BUILD
            asm volatile (
            #if (__mips == 64)
                "daddiu  %[src],  %[data], -4  \n\t"
            #else
                "addiu   %[src],  %[data], -4  \n\t"
            #endif

                : [src] "=r" (src)
                : [data] "r" (data)
            );
#else
            src = data - 4;
#endif

            LD_UB8(src, img_width,
                   row0, row1, row2, row3, row4, row5, row6, row7);
            src += (8 * img_width);
            LD_UB8(src, img_width,
                   row8, row9, row10, row11, row12, row13, row14, row15);

            TRANSPOSE16x8_UB_UB(row0, row1, row2, row3, row4, row5, row6, row7,
                                row8, row9, row10, row11,
                                row12, row13, row14, row15,
                                p3_org, p2_org, p1_org, p0_org,
                                q0_org, q1_org, q2_org, q3_org);
        }
        {
            v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0, alpha;
            v16u8 is_less_than_alpha;

            p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
            p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
            q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

            alpha = (v16u8)__msa_fill_b(alpha_in);
            beta = (v16u8)__msa_fill_b(beta_in);

            is_less_than_alpha = (p0_asub_q0 < alpha);
            is_less_than_beta = (p1_asub_p0 < beta);
            is_less_than = is_less_than_beta & is_less_than_alpha;
            is_less_than_beta = (q1_asub_q0 < beta);
            is_less_than = is_less_than_beta & is_less_than;
            is_less_than = is_less_than & is_bs_grter_than0;
        }
        if(!__msa_test_bz_v(is_less_than))
        {
            v16i8 negate_tc, sign_negate_tc;
            v8i16 negate_tc_r, i16_negatetc_l;

            negate_tc = zero - (v16i8)tc;
            sign_negate_tc = CLTI_S_B(negate_tc, 0);

            ILVRL_B2_SH(sign_negate_tc, negate_tc, negate_tc_r, i16_negatetc_l);

            UNPCK_UB_SH(tc, tc_r, tc_l);
            UNPCK_UB_SH(p1_org, p1_org_r, p1_org_l);
            UNPCK_UB_SH(p0_org, p0_org_r, p0_org_l);
            UNPCK_UB_SH(q0_org, q0_org_r, q0_org_l);

            {
                v16u8 p2_asub_p0;
                v16u8 is_less_than_beta_r, is_less_than_beta_l;

                p2_asub_p0 = __msa_asub_u_b(p2_org, p0_org);
                is_less_than_beta = (p2_asub_p0 < beta);
                is_less_than_beta = is_less_than_beta & is_less_than;

                is_less_than_beta_r =
                    (v16u8)__msa_sldi_b((v16i8)is_less_than_beta, zero, 8);
                if(!__msa_test_bz_v(is_less_than_beta_r))
                {
                    p2_org_r = (v8i16)__msa_ilvr_b(zero, (v16i8)p2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_r, q0_org_r, p1_org_r, p2_org_r,
                                     negate_tc_r, tc_r, p1_r);
                }

                is_less_than_beta_l =
                    (v16u8)__msa_sldi_b(zero, (v16i8)is_less_than_beta, 8);
                if(!__msa_test_bz_v(is_less_than_beta_l))
                {
                    p2_org_l = (v8i16)__msa_ilvl_b(zero, (v16i8)p2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_l, q0_org_l, p1_org_l, p2_org_l,
                                     i16_negatetc_l, tc_l, p1_l);
                }
            }

            if(!__msa_test_bz_v(is_less_than_beta))
            {
                p1 = (v16u8)__msa_pckev_b((v16i8)p1_l, (v16i8)p1_r);
                p1_org = __msa_bmnz_v(p1_org, p1, is_less_than_beta);

                is_less_than_beta = ANDI_B(is_less_than_beta, 1);
                tc = tc + is_less_than_beta;
            }

            {
                v16u8 u8_q2asub_q0;
                v16u8 is_less_than_beta_l, is_less_than_beta_r;

                u8_q2asub_q0 = __msa_asub_u_b(q2_org, q0_org);
                is_less_than_beta = (u8_q2asub_q0 < beta);
                is_less_than_beta = is_less_than_beta & is_less_than;

                q1_org_r = (v8i16)__msa_ilvr_b(zero, (v16i8)q1_org);

                is_less_than_beta_r =
                    (v16u8)__msa_sldi_b((v16i8)is_less_than_beta, zero, 8);
                if(!__msa_test_bz_v(is_less_than_beta_r))
                {
                    q2_org_r = (v8i16)__msa_ilvr_b(zero, (v16i8)q2_org);
                    AVC_LPF_P1_OR_Q1(p0_org_r, q0_org_r, q1_org_r, q2_org_r,
                                     negate_tc_r, tc_r, q1_r);
                }

                q1_org_l = (v8i16)__msa_ilvl_b(zero, (v16i8)q1_org);

                is_less_than_beta_l =
                    (v16u8)__msa_sldi_b(zero, (v16i8)is_less_than_beta, 8);
                if(!__msa_test_bz_v(is_less_than_beta_l))
                {
                    q2_org_l = (v8i16)__msa_ilvl_b(zero, (v16i8)q2_org);
                    AVC_LPF_P1_OR_Q1(p0_org_l, q0_org_l, q1_org_l, q2_org_l,
                                     i16_negatetc_l, tc_l, q1_l);
                }
            }

            if(!__msa_test_bz_v(is_less_than_beta))
            {
                q1 = (v16u8)__msa_pckev_b((v16i8)q1_l, (v16i8)q1_r);
                q1_org = __msa_bmnz_v(q1_org, q1, is_less_than_beta);

                is_less_than_beta = ANDI_B(is_less_than_beta, 1);
                tc = tc + is_less_than_beta;
            }

            {
                v8i16 threshold_r, negate_thresh_r;
                v8i16 threshold_l, negate_thresh_l;
                v16i8 negate_thresh, sign_negate_thresh;

                negate_thresh = zero - (v16i8)tc;
                sign_negate_thresh = CLTI_S_B(negate_thresh, 0);

                ILVR_B2_SH(zero, tc, sign_negate_thresh, negate_thresh,
                           threshold_r, negate_thresh_r);

                AVC_LPF_P0Q0(q0_org_r, p0_org_r, p1_org_r, q1_org_r,
                             negate_thresh_r, threshold_r, p0_r, q0_r);

                threshold_l = (v8i16)__msa_ilvl_b(zero, (v16i8)tc);
                negate_thresh_l = (v8i16)__msa_ilvl_b(sign_negate_thresh,
                                                      negate_thresh);

                AVC_LPF_P0Q0(q0_org_l, p0_org_l, p1_org_l, q1_org_l,
                             negate_thresh_l, threshold_l, p0_l, q0_l);
            }

            PCKEV_B2_UB(p0_l, p0_r, q0_l, q0_r, p0, q0);

            p0_org = __msa_bmnz_v(p0_org, p0, is_less_than);
            q0_org = __msa_bmnz_v(q0_org, q0, is_less_than);
        }
        {
            v16i8 tp0, tp1, tp2, tp3;
            v8i16 tmp2, tmp5;
            v4i32 tmp3, tmp4, tmp6, tmp7;
            UWORD32 out0, out2;
            UWORD16 out1, out3;

            src = data - 3;

            ILVRL_B2_SB(p1_org, p2_org, tp0, tp2);
            ILVRL_B2_SB(q0_org, p0_org, tp1, tp3);
            ILVRL_B2_SH(q2_org, q1_org, tmp2, tmp5);

            ILVRL_H2_SW(tp1, tp0, tmp3, tmp4);
            ILVRL_H2_SW(tp3, tp2, tmp6, tmp7);

            out0 = __msa_copy_u_w(tmp3, 0);
            out1 = __msa_copy_u_h(tmp2, 0);
            out2 = __msa_copy_u_w(tmp3, 1);
            out3 = __msa_copy_u_h(tmp2, 1);

            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp3, 2);
            out1 = __msa_copy_u_h(tmp2, 2);
            out2 = __msa_copy_u_w(tmp3, 3);
            out3 = __msa_copy_u_h(tmp2, 3);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp4, 0);
            out1 = __msa_copy_u_h(tmp2, 4);
            out2 = __msa_copy_u_w(tmp4, 1);
            out3 = __msa_copy_u_h(tmp2, 5);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp4, 2);
            out1 = __msa_copy_u_h(tmp2, 6);
            out2 = __msa_copy_u_w(tmp4, 3);
            out3 = __msa_copy_u_h(tmp2, 7);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp6, 0);
            out1 = __msa_copy_u_h(tmp5, 0);
            out2 = __msa_copy_u_w(tmp6, 1);
            out3 = __msa_copy_u_h(tmp5, 1);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp6, 2);
            out1 = __msa_copy_u_h(tmp5, 2);
            out2 = __msa_copy_u_w(tmp6, 3);
            out3 = __msa_copy_u_h(tmp5, 3);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp7, 0);
            out1 = __msa_copy_u_h(tmp5, 4);
            out2 = __msa_copy_u_w(tmp7, 1);
            out3 = __msa_copy_u_h(tmp5, 5);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp7, 2);
            out1 = __msa_copy_u_h(tmp5, 6);
            out2 = __msa_copy_u_w(tmp7, 3);
            out3 = __msa_copy_u_h(tmp5, 7);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));
        }
    }
}

static void avc_loopfilter_luma_inter_edge_hor_msa(UWORD8 *data,
                                                   UWORD8 bs0, UWORD8 bs1,
                                                   UWORD8 bs2, UWORD8 bs3,
                                                   UWORD8 tc0, UWORD8 tc1,
                                                   UWORD8 tc2, UWORD8 tc3,
                                                   UWORD8 alpha_in,
                                                   UWORD8 beta_in,
                                                   UWORD32 image_width)
{
    v16u8 p2_asub_p0, u8_q2asub_q0;
    v16u8 alpha, beta, is_less_than, is_less_than_beta;
    v16u8 p1, p0, q0, q1;
    v8i16 p1_r = { 0 };
    v8i16 p0_r, q0_r, q1_r = { 0 };
    v8i16 p1_l = { 0 };
    v8i16 p0_l, q0_l, q1_l = { 0 };
    v16u8 p2_org, p1_org, p0_org, q0_org, q1_org, q2_org;
    v8i16 p2_org_r, p1_org_r, p0_org_r, q0_org_r, q1_org_r, q2_org_r;
    v8i16 p2_org_l, p1_org_l, p0_org_l, q0_org_l, q1_org_l, q2_org_l;
    v16i8 zero = { 0 };
    v16u8 tmp_vec;
    v16u8 bs = { 0 };
    v16i8 tc = { 0 };

    tmp_vec = (v16u8)__msa_fill_b(bs0);
    bs = (v16u8)__msa_insve_w((v4i32)bs, 0, (v4i32)tmp_vec);
    tmp_vec = (v16u8)__msa_fill_b(bs1);
    bs = (v16u8)__msa_insve_w((v4i32)bs, 1, (v4i32)tmp_vec);
    tmp_vec = (v16u8)__msa_fill_b(bs2);
    bs = (v16u8)__msa_insve_w((v4i32)bs, 2, (v4i32)tmp_vec);
    tmp_vec = (v16u8)__msa_fill_b(bs3);
    bs = (v16u8)__msa_insve_w((v4i32)bs, 3, (v4i32)tmp_vec);

    if(!__msa_test_bz_v(bs))
    {
        tmp_vec = (v16u8)__msa_fill_b(tc0);
        tc = (v16i8)__msa_insve_w((v4i32)tc, 0, (v4i32)tmp_vec);
        tmp_vec = (v16u8)__msa_fill_b(tc1);
        tc = (v16i8)__msa_insve_w((v4i32)tc, 1, (v4i32)tmp_vec);
        tmp_vec = (v16u8)__msa_fill_b(tc2);
        tc = (v16i8)__msa_insve_w((v4i32)tc, 2, (v4i32)tmp_vec);
        tmp_vec = (v16u8)__msa_fill_b(tc3);
        tc = (v16i8)__msa_insve_w((v4i32)tc, 3, (v4i32)tmp_vec);

        alpha = (v16u8)__msa_fill_b(alpha_in);
        beta = (v16u8)__msa_fill_b(beta_in);

        LD_UB5(data - (3 * image_width), image_width,
               p2_org, p1_org, p0_org, q0_org, q1_org);

        {
            v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0;
            v16u8 is_less_than_alpha, is_bs_grter_than0;

            is_bs_grter_than0 = ((v16u8)zero < bs);
            p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
            p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
            q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

            is_less_than_alpha = (p0_asub_q0 < alpha);
            is_less_than_beta = (p1_asub_p0 < beta);
            is_less_than = is_less_than_beta & is_less_than_alpha;
            is_less_than_beta = (q1_asub_q0 < beta);
            is_less_than = is_less_than_beta & is_less_than;
            is_less_than = is_less_than & is_bs_grter_than0;
        }

        if(!__msa_test_bz_v(is_less_than))
        {
            v16i8 sign_negate_tc, negate_tc;
            v8i16 negate_tc_r, i16_negatetc_l, tc_l, tc_r;

            q2_org = LD_UB(data + (2 * image_width));
            negate_tc = zero - tc;
            sign_negate_tc = CLTI_S_B(negate_tc, 0);

            ILVRL_B2_SH(sign_negate_tc, negate_tc, negate_tc_r, i16_negatetc_l);

            UNPCK_UB_SH(tc, tc_r, tc_l);
            UNPCK_UB_SH(p1_org, p1_org_r, p1_org_l);
            UNPCK_UB_SH(p0_org, p0_org_r, p0_org_l);
            UNPCK_UB_SH(q0_org, q0_org_r, q0_org_l);

            p2_asub_p0 = __msa_asub_u_b(p2_org, p0_org);
            is_less_than_beta = (p2_asub_p0 < beta);
            is_less_than_beta = is_less_than_beta & is_less_than;
            {
                v8u16 is_less_than_beta_r, is_less_than_beta_l;

                is_less_than_beta_r =
                    (v8u16)__msa_sldi_b((v16i8)is_less_than_beta, zero, 8);
                if(!__msa_test_bz_v((v16u8)is_less_than_beta_r))
                {
                    p2_org_r = (v8i16)__msa_ilvr_b(zero, (v16i8)p2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_r, q0_org_r, p1_org_r, p2_org_r,
                                     negate_tc_r, tc_r, p1_r);
                }

                is_less_than_beta_l =
                    (v8u16)__msa_sldi_b(zero, (v16i8)is_less_than_beta, 8);
                if(!__msa_test_bz_v((v16u8)is_less_than_beta_l))
                {
                    p2_org_l = (v8i16)__msa_ilvl_b(zero, (v16i8)p2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_l, q0_org_l, p1_org_l, p2_org_l,
                                     i16_negatetc_l, tc_l, p1_l);
                }
            }
            if(!__msa_test_bz_v(is_less_than_beta))
            {
                p1 = (v16u8)__msa_pckev_b((v16i8)p1_l, (v16i8)p1_r);
                p1_org = __msa_bmnz_v(p1_org, p1, is_less_than_beta);
                ST_UB(p1_org, data - (2 * image_width));

                is_less_than_beta = ANDI_B(is_less_than_beta, 1);
                tc = tc + (v16i8)is_less_than_beta;
            }

            u8_q2asub_q0 = __msa_asub_u_b(q2_org, q0_org);
            is_less_than_beta = (u8_q2asub_q0 < beta);
            is_less_than_beta = is_less_than_beta & is_less_than;

            {
                v8u16 is_less_than_beta_r, is_less_than_beta_l;
                is_less_than_beta_r =
                    (v8u16)__msa_sldi_b((v16i8)is_less_than_beta, zero, 8);

                q1_org_r = (v8i16)__msa_ilvr_b(zero, (v16i8)q1_org);
                if(!__msa_test_bz_v((v16u8)is_less_than_beta_r))
                {
                    q2_org_r = (v8i16)__msa_ilvr_b(zero, (v16i8)q2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_r, q0_org_r, q1_org_r, q2_org_r,
                                     negate_tc_r, tc_r, q1_r);
                }
                is_less_than_beta_l =
                    (v8u16)__msa_sldi_b(zero, (v16i8)is_less_than_beta, 8);

                q1_org_l = (v8i16)__msa_ilvl_b(zero, (v16i8)q1_org);
                if(!__msa_test_bz_v((v16u8)is_less_than_beta_l))
                {
                    q2_org_l = (v8i16)__msa_ilvl_b(zero, (v16i8)q2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_l, q0_org_l, q1_org_l, q2_org_l,
                                     i16_negatetc_l, tc_l, q1_l);
                }
            }
            if(!__msa_test_bz_v(is_less_than_beta))
            {
                q1 = (v16u8)__msa_pckev_b((v16i8)q1_l, (v16i8)q1_r);
                q1_org = __msa_bmnz_v(q1_org, q1, is_less_than_beta);
                ST_UB(q1_org, data + image_width);

                is_less_than_beta = ANDI_B(is_less_than_beta, 1);
                tc = tc + (v16i8)is_less_than_beta;
            }
            {
                v16i8 negate_thresh, sign_negate_thresh;
                v8i16 threshold_r, threshold_l;
                v8i16 negate_thresh_l, negate_thresh_r;

                negate_thresh = zero - tc;
                sign_negate_thresh = CLTI_S_B(negate_thresh, 0);

                ILVR_B2_SH(zero, tc, sign_negate_thresh, negate_thresh,
                           threshold_r, negate_thresh_r);
                AVC_LPF_P0Q0(q0_org_r, p0_org_r, p1_org_r, q1_org_r,
                             negate_thresh_r, threshold_r, p0_r, q0_r);

                threshold_l = (v8i16)__msa_ilvl_b(zero, tc);
                negate_thresh_l = (v8i16)__msa_ilvl_b(sign_negate_thresh,
                                                      negate_thresh);
                AVC_LPF_P0Q0(q0_org_l, p0_org_l, p1_org_l, q1_org_l,
                             negate_thresh_l, threshold_l, p0_l, q0_l);
            }

            PCKEV_B2_UB(p0_l, p0_r, q0_l, q0_r, p0, q0);

            p0_org = __msa_bmnz_v(p0_org, p0, is_less_than);
            q0_org = __msa_bmnz_v(q0_org, q0, is_less_than);

            ST_UB(p0_org, (data - image_width));
            ST_UB(q0_org, data);
        }
    }
}

static void avc_h_loop_filter_luma_mbaff_msa(UWORD8 *in, WORD32 stride,
                                             WORD32 alpha_in, WORD32 beta_in,
                                             WORD8 *tc0)
{
    UWORD8 *data = in;
    UWORD32 out0, out1, out2, out3;
    UWORD64 load;
    UWORD32 tc_val;
    v16u8 alpha, beta;
    v16i8 inp0 = { 0 };
    v16i8 inp1 = { 0 };
    v16i8 inp2 = { 0 };
    v16i8 inp3 = { 0 };
    v16i8 inp4 = { 0 };
    v16i8 inp5 = { 0 };
    v16i8 inp6 = { 0 };
    v16i8 inp7 = { 0 };
    v16i8 src0, src1, src2, src3;
    v8i16 src4, src5, src6, src7;
    v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0, p2_asub_p0, q2_asub_q0;
    v16u8 is_less_than, is_less_than_alpha, is_less_than_beta;
    v16u8 is_less_than_beta1, is_less_than_beta2;
    v8i16 tc, tc_orig_r, tc_plus1;
    v16u8 is_tc_orig1, is_tc_orig2, tc_orig = { 0 };
    v8i16 p0_ilvr_q0, p0_add_q0, q0_sub_p0, p1_sub_q1;
    v8u16 src2_r, src3_r;
    v8i16 p2_r, p1_r, q2_r, q1_r;
    v16u8 p2, q2, p0, q0;
    v4i32 dst0, dst1;
    v16i8 zeros = { 0 };

    alpha = (v16u8)__msa_fill_b(alpha_in);
    beta = (v16u8)__msa_fill_b(beta_in);

    if(tc0[0] < 0)
    {
        data += (2 * stride);
    }
    else
    {
        load = LD(data - 3);
        inp0 = (v16i8)__msa_insert_d((v2i64)inp0, 0, load);
        load = LD(data - 3 + stride);
        inp1 = (v16i8)__msa_insert_d((v2i64)inp1, 0, load);
        data += (2 * stride);
    }

    if(tc0[1] < 0)
    {
        data += (2 * stride);
    }
    else
    {
        load = LD(data - 3);
        inp2 = (v16i8)__msa_insert_d((v2i64)inp2, 0, load);
        load = LD(data - 3 + stride);
        inp3 = (v16i8)__msa_insert_d((v2i64)inp3, 0, load);
        data += (2 * stride);
    }

    if(tc0[2] < 0)
    {
        data += (2 * stride);
    }
    else
    {
        load = LD(data - 3);
        inp4 = (v16i8)__msa_insert_d((v2i64)inp4, 0, load);
        load = LD(data - 3 + stride);
        inp5 = (v16i8)__msa_insert_d((v2i64)inp5, 0, load);
        data += (2 * stride);
    }

    if(tc0[3] < 0)
    {
        data += (2 * stride);
    }
    else
    {
        load = LD(data - 3);
        inp6 = (v16i8)__msa_insert_d((v2i64)inp6, 0, load);
        load = LD(data - 3 + stride);
        inp7 = (v16i8)__msa_insert_d((v2i64)inp7, 0, load);
        data += (2 * stride);
    }

    ILVR_B4_SB(inp1, inp0, inp3, inp2, inp5, inp4, inp7, inp6,
               src0, src1, src2, src3);

    ILVR_H2_SH(src1, src0, src3, src2, src4, src6);
    ILVL_H2_SH(src1, src0, src3, src2, src5, src7);

    src0 = (v16i8)__msa_ilvr_w((v4i32)src6, (v4i32)src4);
    src1 = __msa_sldi_b(zeros, (v16i8)src0, 8);
    src2 = (v16i8)__msa_ilvl_w((v4i32)src6, (v4i32)src4);
    src3 = __msa_sldi_b(zeros, (v16i8)src2, 8);
    src4 = (v8i16)__msa_ilvr_w((v4i32)src7, (v4i32)src5);
    src5 = (v8i16)__msa_sldi_b(zeros, (v16i8)src4, 8);

    p0_asub_q0 = __msa_asub_u_b((v16u8)src2, (v16u8)src3);
    p1_asub_p0 = __msa_asub_u_b((v16u8)src1, (v16u8)src2);
    q1_asub_q0 = __msa_asub_u_b((v16u8)src4, (v16u8)src3);
    p2_asub_p0 = __msa_asub_u_b((v16u8)src0, (v16u8)src2);
    q2_asub_q0 = __msa_asub_u_b((v16u8)src5, (v16u8)src3);

    is_less_than_alpha = (p0_asub_q0 < alpha);
    is_less_than_beta = (p1_asub_p0 < beta);
    is_less_than = is_less_than_alpha & is_less_than_beta;
    is_less_than_beta = (q1_asub_q0 < beta);
    is_less_than = is_less_than_beta & is_less_than;

    is_less_than_beta1 = (p2_asub_p0 < beta);
    is_less_than_beta2 = (q2_asub_q0 < beta);

    p0_ilvr_q0 = (v8i16)__msa_ilvr_b((v16i8)src3, (v16i8)src2);
    p0_add_q0 = (v8i16)__msa_hadd_u_h((v16u8)p0_ilvr_q0, (v16u8)p0_ilvr_q0);
    p0_add_q0 = __msa_srari_h(p0_add_q0, 1);

    ILVR_B2_SH(zeros, src0, zeros, src1, p2_r, p1_r);
    p2_r += p0_add_q0;
    p2_r = SRAI_H(p2_r, 1);
    p2_r -= p1_r;
    ILVR_B2_SH(zeros, src5, zeros, src4, q2_r, q1_r);
    q2_r += p0_add_q0;
    q2_r = SRAI_H(q2_r, 1);
    q2_r -= q1_r;

    tc_val = LW(tc0);
    tc_orig = (v16u8)__msa_insert_w((v4i32)tc_orig, 0, tc_val);
    tc_orig = (v16u8)__msa_ilvr_b((v16i8)tc_orig, (v16i8)tc_orig);
    is_tc_orig1 = tc_orig;
    is_tc_orig2 = tc_orig;
    tc_orig_r = (v8i16)__msa_ilvr_b(zeros, (v16i8)tc_orig);
    tc = tc_orig_r;

    p2_r = CLIP_SH(p2_r, -tc_orig_r, tc_orig_r);
    q2_r = CLIP_SH(q2_r, -tc_orig_r, tc_orig_r);

    p2_r += p1_r;
    q2_r += q1_r;

    PCKEV_B2_UB(p2_r, p2_r, q2_r, q2_r, p2, q2);

    is_tc_orig1 = (zeros < is_tc_orig1);
    is_tc_orig2 = is_tc_orig1;
    is_tc_orig1 = is_less_than_beta1 & is_tc_orig1;
    is_tc_orig2 = is_less_than_beta2 & is_tc_orig2;
    is_tc_orig1 = is_less_than & is_tc_orig1;
    is_tc_orig2 = is_less_than & is_tc_orig2;

    p2 = __msa_bmnz_v((v16u8)src1, p2, is_tc_orig1);
    q2 = __msa_bmnz_v((v16u8)src4, q2, is_tc_orig2);

    q0_sub_p0 = __msa_hsub_u_h((v16u8)p0_ilvr_q0, (v16u8)p0_ilvr_q0);
    q0_sub_p0 = SLLI_H(q0_sub_p0, 2);
    p1_sub_q1 = p1_r - q1_r;
    q0_sub_p0 += p1_sub_q1;
    q0_sub_p0 = __msa_srari_h(q0_sub_p0, 3);

    tc_plus1 = ADDVI_H(tc, 1);
    is_less_than_beta1 = (v16u8)__msa_ilvr_b((v16i8)is_less_than_beta1,
                                             (v16i8)is_less_than_beta1);
    tc = (v8i16)__msa_bmnz_v((v16u8)tc, (v16u8)tc_plus1, is_less_than_beta1);
    tc_plus1 = ADDVI_H(tc, 1);
    is_less_than_beta2 = (v16u8)__msa_ilvr_b((v16i8)is_less_than_beta2,
                                             (v16i8)is_less_than_beta2);
    tc = (v8i16)__msa_bmnz_v((v16u8)tc, (v16u8)tc_plus1, is_less_than_beta2);

    q0_sub_p0 = CLIP_SH(q0_sub_p0, -tc, tc);

    ILVR_B2_UH(zeros, src2, zeros, src3, src2_r, src3_r);
    src2_r += (v8u16)q0_sub_p0;
    src3_r -= (v8u16)q0_sub_p0;

    src2_r = (v8u16)CLIP_SH_0_255(src2_r);
    src3_r = (v8u16)CLIP_SH_0_255(src3_r);

    PCKEV_B2_UB(src2_r, src2_r, src3_r, src3_r, p0, q0);

    p0 = __msa_bmnz_v((v16u8)src2, p0, is_less_than);
    q0 = __msa_bmnz_v((v16u8)src3, q0, is_less_than);

    ILVR_B2_UB(p0, p2, q2, q0, p2, q2);

    ILVRL_H2_SW(q2, p2, dst0, dst1);

    data = in;

    out0 = __msa_copy_u_w(dst0, 0);
    out1 = __msa_copy_u_w(dst0, 1);
    out2 = __msa_copy_u_w(dst0, 2);
    out3 = __msa_copy_u_w(dst0, 3);

    if(tc0[0] < 0)
    {
        data += (2 * stride);
    }
    else
    {
        SW(out0, (data - 2));
        data += stride;
        SW(out1, (data - 2));
        data += stride;
    }

    if(tc0[1] < 0)
    {
        data += (2 * stride);
    }
    else
    {
        SW(out2, (data - 2));
        data += stride;
        SW(out3, (data - 2));
        data += stride;
    }

    out0 = __msa_copy_u_w(dst1, 0);
    out1 = __msa_copy_u_w(dst1, 1);
    out2 = __msa_copy_u_w(dst1, 2);
    out3 = __msa_copy_u_w(dst1, 3);

    if(tc0[2] < 0)
    {
        data += (2 * stride);
    }
    else
    {
        SW(out0, (data - 2));
        data += stride;
        SW(out1, (data - 2));
        data += stride;
    }

    if(tc0[3] >= 0)
    {
        SW(out2, (data - 2));
        data += stride;
        SW(out3, (data - 2));
    }
}

void ih264_deblk_luma_vert_bs4_msa(UWORD8 *pu1_src, WORD32 src_strd,
                                   WORD32 alpha, WORD32 beta)
{
    avc_loopfilter_luma_intra_edge_ver_msa(pu1_src, alpha, beta, src_strd);
}

void ih264_deblk_luma_horz_bs4_msa(UWORD8 *pu1_src, WORD32 src_strd,
                                   WORD32 alpha, WORD32 beta)
{
    avc_loopfilter_luma_intra_edge_hor_msa(pu1_src, alpha, beta, src_strd);
}

void ih264_deblk_luma_vert_bs4_mbaff_msa(UWORD8 *pu1_src, WORD32 src_strd,
                                         WORD32 alpha, WORD32 beta)
{
    avc_h_loop_filter_luma_mbaff_intra_msa(pu1_src, src_strd, alpha, beta);
}

void ih264_deblk_luma_vert_bslt4_msa(UWORD8 *pu1_src, WORD32 src_strd,
                                     WORD32 alpha, WORD32 beta, UWORD32 u4_bs,
                                     const UWORD8 *pu1_cliptab)
{
    UWORD8 tc0, tc1, tc2, tc3;
    UWORD8 bs0 = (u4_bs >> 24) & 0x0ff;
    UWORD8 bs1 = (u4_bs >> 16) & 0x0ff;
    UWORD8 bs2 = (u4_bs >> 8) & 0x0ff;
    UWORD8 bs3 = u4_bs & 0x0ff;

    tc0 = pu1_cliptab[bs0];
    tc1 = pu1_cliptab[bs1];
    tc2 = pu1_cliptab[bs2];
    tc3 = pu1_cliptab[bs3];

    avc_loopfilter_luma_inter_edge_ver_msa(pu1_src, bs0, bs1, bs2, bs3,
                                           tc0, tc1, tc2, tc3, alpha, beta,
                                           src_strd);
}

void ih264_deblk_luma_horz_bslt4_msa(UWORD8 *pu1_src, WORD32 src_strd,
                                     WORD32 alpha, WORD32 beta, UWORD32 u4_bs,
                                     const UWORD8 *pu1_cliptab)
{
    UWORD8 tc0, tc1, tc2, tc3;
    UWORD8 bs0 = (u4_bs >> 24) & 0x0ff;
    UWORD8 bs1 = (u4_bs >> 16) & 0x0ff;
    UWORD8 bs2 = (u4_bs >> 8) & 0x0ff;
    UWORD8 bs3 = u4_bs & 0x0ff;

    tc0 = pu1_cliptab[bs0];
    tc1 = pu1_cliptab[bs1];
    tc2 = pu1_cliptab[bs2];
    tc3 = pu1_cliptab[bs3];

    avc_loopfilter_luma_inter_edge_hor_msa(pu1_src, bs0, bs1, bs2, bs3,
                                           tc0, tc1, tc2, tc3, alpha, beta,
                                           src_strd);
}

void ih264_deblk_luma_vert_bslt4_mbaff_msa(UWORD8 *pu1_src, WORD32 src_strd,
                                           WORD32 alpha, WORD32 beta,
                                           UWORD32 u4_bs,
                                           const UWORD8 *pu1_cliptab)
{
    WORD8 tc[4];
    UWORD8 bs0 = (u4_bs >> 24) & 0x0ff;
    UWORD8 bs1 = (u4_bs >> 16) & 0x0ff;
    UWORD8 bs2 = (u4_bs >> 8) & 0x0ff;
    UWORD8 bs3 = u4_bs & 0x0ff;

    tc[0] = pu1_cliptab[bs0];
    tc[1] = pu1_cliptab[bs1];
    tc[2] = pu1_cliptab[bs2];
    tc[3] = pu1_cliptab[bs3];

    avc_h_loop_filter_luma_mbaff_msa(pu1_src, src_strd, alpha, beta, tc);
}
