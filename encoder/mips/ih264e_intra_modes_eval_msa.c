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
#include <limits.h>

#include "ih264e_config.h"
#include "ih264_typedefs.h"
#include "ih264e_defs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_defs.h"
#include "ih264_macros.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ime_distortion_metrics.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_intra_modes_eval.h"
#include "ih264_macros_msa.h"

static void avc_evaluate_intra16x16_modes_msa(UWORD8 *src, UWORD32 src_stride,
                                              UWORD8 *ngbr_pel,
                                              UWORD8 *dst, UWORD32 dst_stride,
                                              WORD32 is_left, WORD32 is_top,
                                              UWORD32 valid_intra_modes,
                                              UWORD32 *final_intra_mode,
                                              WORD32 *sad_min)
{
    UWORD8 *src_temp;
    UWORD8 inp0, inp1, inp2, inp3;
    WORD32 row, cnt, dcval = 0;
    WORD32 sad_vert, sad_horz, sad_dc, min_sad;
    WORD32 horz_flag, vert_flag, dc_flag;
    v16u8 out, src0, src1, src2, src3, ref0, ref1, ref2, ref3;
    v8u16 sad, sum_h;

    sad_vert = INT_MAX;
    sad_horz = INT_MAX;
    sad_dc = INT_MAX;
    min_sad = INT_MAX;

    horz_flag = is_left && ((valid_intra_modes & 02) != 0);
    vert_flag = is_top && ((valid_intra_modes & 01) != 0);
    dc_flag = (valid_intra_modes & 04) != 0;

    if(horz_flag)
    {
        src_temp = src;

        inp0 = ngbr_pel[15];
        inp1 = ngbr_pel[14];
        inp2 = ngbr_pel[13];
        inp3 = ngbr_pel[12];

        LD_UB4(src_temp, src_stride, ref0, ref1, ref2, ref3);
        src_temp += (4 * src_stride);

        src0 = (v16u8)__msa_fill_b(inp0);
        src1 = (v16u8)__msa_fill_b(inp1);
        src2 = (v16u8)__msa_fill_b(inp2);
        src3 = (v16u8)__msa_fill_b(inp3);

        sad = SAD_UB2_UH(src0, src1, ref0, ref1);
        sad += SAD_UB2_UH(src2, src3, ref2, ref3);

        cnt = 11;
        for(row = 3; row--;)
        {
            inp0 = ngbr_pel[cnt];
            inp1 = ngbr_pel[cnt - 1];
            inp2 = ngbr_pel[cnt - 2];
            inp3 = ngbr_pel[cnt - 3];
            cnt -= 4;

            LD_UB4(src_temp, src_stride, ref0, ref1, ref2, ref3);
            src_temp += (4 * src_stride);

            src0 = (v16u8)__msa_fill_b(inp0);
            src1 = (v16u8)__msa_fill_b(inp1);
            src2 = (v16u8)__msa_fill_b(inp2);
            src3 = (v16u8)__msa_fill_b(inp3);

            sad += SAD_UB2_UH(src0, src1, ref0, ref1);
            sad += SAD_UB2_UH(src2, src3, ref2, ref3);
        }

        sad_horz = HADD_UH_U32(sad);
    }

    if(vert_flag)
    {
        src_temp = src;
        src0 = LD_UB(ngbr_pel + 17);
        LD_UB4(src_temp, src_stride, ref0, ref1, ref2, ref3);
        src_temp += (4 * src_stride);

        sad = SAD_UB2_UH(src0, src0, ref0, ref1);
        sad += SAD_UB2_UH(src0, src0, ref2, ref3);

        for(row = 3; row--;)
        {
            LD_UB4(src_temp, src_stride, ref0, ref1, ref2, ref3);
            src_temp += (4 * src_stride);

            sad += SAD_UB2_UH(src0, src0, ref0, ref1);
            sad += SAD_UB2_UH(src0, src0, ref2, ref3);
        }

        sad_vert = HADD_UH_U32(sad);
    }

    if(is_left)
    {
        out = LD_UB(ngbr_pel);
        sum_h = __msa_hadd_u_h(out, out);
        dcval += HADD_UH_U32(sum_h);
        dcval += 8;
    }

    if(is_top)
    {
        out = LD_UB(ngbr_pel + 17);
        sum_h = __msa_hadd_u_h(out, out);
        dcval += HADD_UH_U32(sum_h);
        dcval += 8;
    }

    dcval = (dcval) >> (3 + is_left + is_top);
    dcval += ((is_left == 0) & (is_top == 0)) << 7;

    if(dc_flag)
    {
        src_temp = src;
        src0 = (v16u8)__msa_fill_b(dcval);
        LD_UB4(src_temp, src_stride, ref0, ref1, ref2, ref3);
        src_temp += (4 * src_stride);

        sad = SAD_UB2_UH(src0, src0, ref0, ref1);
        sad += SAD_UB2_UH(src0, src0, ref2, ref3);

        for(row = 3; row--;)
        {
            LD_UB4(src_temp, src_stride, ref0, ref1, ref2, ref3);
            src_temp += (4 * src_stride);

            sad += SAD_UB2_UH(src0, src0, ref0, ref1);
            sad += SAD_UB2_UH(src0, src0, ref2, ref3);
        }

        sad_dc = HADD_UH_U32(sad);
    }

    min_sad = MIN3(sad_horz, sad_dc, sad_vert);

    /* Finding Minimum sad and doing corresponding prediction */
    if(min_sad < *sad_min)
    {
        *sad_min = min_sad;
        if(min_sad == sad_vert)
        {
            *final_intra_mode = VERT_I16x16;

            out = LD_UB(ngbr_pel + 17);
            ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
            dst += (8 * dst_stride);
            ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
        }
        else if(min_sad == sad_horz)
        {
            *final_intra_mode = HORZ_I16x16;

            cnt = 15;
            for(row = 4; row--;)
            {
                inp0 = ngbr_pel[cnt];
                inp1 = ngbr_pel[cnt - 1];
                inp2 = ngbr_pel[cnt - 2];
                inp3 = ngbr_pel[cnt - 3];
                cnt -= 4;

                src0 = (v16u8)__msa_fill_b(inp0);
                src1 = (v16u8)__msa_fill_b(inp1);
                src2 = (v16u8)__msa_fill_b(inp2);
                src3 = (v16u8)__msa_fill_b(inp3);

                ST_UB4(src0, src1, src2, src3, dst, dst_stride);
                dst += (4 * dst_stride);
            }
        }
        else
        {
            *final_intra_mode = DC_I16x16;

            out = (v16u8)__msa_fill_b(dcval);
            ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
            dst += (8 * dst_stride);
            ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
        }
    }
}

static void avc_evaluate_intra4x4_modes_msa(UWORD8 *src_ptr, UWORD32 src_stride,
                                            UWORD8 *ngbr_pel,
                                            UWORD8 *dst, UWORD32 dst_stride,
                                            WORD32 is_left, WORD32 is_top,
                                            UWORD32 valid_intra_modes,
                                            UWORD32 lambda, UWORD32 predictd_mode,
                                            UWORD32 *final_intra_mode,
                                            WORD32 *sad_min)
{
    WORD32 dcval = 0;
    WORD32 cost[9] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX,
                       INT_MAX, INT_MAX, INT_MAX, INT_MAX };
    WORD32 sad_val, min_cost, src0_w, src1_w, src2_w, src3_w;
    v16u8 pred0 = { 0 };
    v16u8 pred1 = { 0 };
    v16u8 pred2 = { 0 };
    v16u8 pred3 = { 0 };
    v16u8 pred4 = { 0 };
    v16u8 pred5 = { 0 };
    v16u8 pred6 = { 0 };
    v16u8 pred7 = { 0 };
    v16u8 pred8 = { 0 };
    v16u8 src = { 0 };
    v16u8 out = { 0 };
    v16u8 ngbr, diff;
    v16i8 aver01, filt_121, zero = { 0 };
    v8u16 sad;

    LW4(src_ptr, src_stride, src0_w, src1_w, src2_w, src3_w);
    INSERT_W4_UB(src0_w, src1_w, src2_w, src3_w, src);
    ngbr = LD_UB(ngbr_pel);

    if(valid_intra_modes & 1)
    {
        v16u8 pred;

        pred = (v16u8)__msa_sldi_b(zero, (v16i8)ngbr, 5);
        pred0 = (v16u8)__msa_splati_w((v4i32)pred, 0);

        diff = __msa_asub_u_b(src, pred0);
        sad = __msa_hadd_u_h(diff, diff);
        sad_val = HADD_UH_U32(sad);
        cost[0] = sad_val + ((0 == predictd_mode) ? lambda : 4 * lambda);
    }

    if(valid_intra_modes & 2)
    {
        v16u8 pred = { 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0 };

        pred1 = (v16u8)__msa_vshf_b((v16i8)pred, zero, (v16i8)ngbr);
        diff = __msa_asub_u_b(src, pred1);
        sad = __msa_hadd_u_h(diff, diff);
        sad_val = HADD_UH_U32(sad);
        cost[1] = sad_val + ((1 == predictd_mode) ? lambda : 4 * lambda);
    }

    if(valid_intra_modes & 4)
    {
        if(is_left)
        {
            dcval = ngbr_pel[0] + ngbr_pel[1] + ngbr_pel[2] + ngbr_pel[3] + 2;
        }

        if(is_top)
        {
            dcval += ngbr_pel[5] + ngbr_pel[6] + ngbr_pel[7] + ngbr_pel[8] + 2;
        }

        dcval = (dcval) ? (dcval >> (1 + is_left + is_top)) : 128;

        pred2 = (v16u8)__msa_fill_b(dcval);
        diff = __msa_asub_u_b(src, pred2);
        sad = __msa_hadd_u_h(diff, diff);
        sad_val = HADD_UH_U32(sad);
        cost[2] = sad_val + ((2 == predictd_mode) ? lambda : 4 * lambda);
    }

    min_cost = MIN3(cost[0], cost[1], cost[2]);

    if(valid_intra_modes > 7)
    {
        WORD32 val121;
        v16u8 src0, src1, src2;
        v8u16 src0_r, src1_r, src2_r, src0_l, src1_l, src2_l, vec0, vec1;

        ngbr_pel[13] = ngbr_pel[14] = ngbr_pel[12];

        src0 = LD_UB(ngbr_pel);
        src1 = (v16u8)__msa_sldi_b((v16i8)src0, (v16i8)src0, 1);
        src2 = (v16u8)__msa_sldi_b((v16i8)src0, (v16i8)src0, 2);

        aver01 = (v16i8)__msa_aver_u_b(src0, src1);
        aver01 = __msa_insve_b(aver01, 15, (v16i8)src0);

        ILVRL_B2_UH(zero, src0, src0_r, src0_l);
        ILVRL_B2_UH(zero, src1, src1_r, src1_l);
        ILVRL_B2_UH(zero, src2, src2_r, src2_l);

        vec0 = src0_r + SLLI_H(src1_r, 1) + src2_r;
        vec1 = src0_l + SLLI_H(src1_l, 1) + src2_l;

        SRARI_H2_UH(vec0, vec1, 2);

        filt_121 = __msa_pckev_b((v16i8)vec1, (v16i8)vec0);

        val121 = (ngbr_pel[0] + (ngbr_pel[0] << 1) + ngbr_pel[1] + 2) >> 2;
        filt_121 = __msa_insert_b(filt_121, 15, val121);

        if(valid_intra_modes & 8)
        {
            v16u8 pred = { 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10, 8, 9, 10, 11 };

            pred3 = (v16u8)__msa_vshf_b((v16i8)pred, zero, filt_121);
            diff = __msa_asub_u_b(src, pred3);
            sad = __msa_hadd_u_h(diff, diff);
            sad_val = HADD_UH_U32(sad);
            cost[3] = sad_val + ((3 == predictd_mode) ? lambda : 4 * lambda);
        }

        if(valid_intra_modes & 16)
        {
            v16u8 pred = { 3, 4, 5, 6, 2, 3, 4, 5, 1, 2, 3, 4, 0, 1, 2, 3 };

            pred4 = (v16u8)__msa_vshf_b((v16i8)pred, zero, filt_121);
            diff = __msa_asub_u_b(src, pred4);
            sad = __msa_hadd_u_h(diff, diff);
            sad_val = HADD_UH_U32(sad);
            cost[4] = sad_val + ((4 == predictd_mode) ? lambda : 4 * lambda);
        }

        if(valid_intra_modes & 32)
        {
            v16u8 pred = { 4, 5, 6, 7, 19, 20, 21, 22, 18, 4, 5, 6, 17, 19, 20,
                           21 };

            pred5 = (v16u8)__msa_vshf_b((v16i8)pred, filt_121, aver01);
            diff = __msa_asub_u_b(src, pred5);
            sad = __msa_hadd_u_h(diff, diff);
            sad_val = HADD_UH_U32(sad);

            cost[5] = sad_val + ((5 == predictd_mode) ? lambda : 4 * lambda);
        }

        if(valid_intra_modes & 64)
        {
            v16u8 pred = { 3, 19, 20, 21, 2, 18, 3, 19, 1, 17, 2, 18, 0, 16, 1,
                           17 };

            pred6 = (v16u8)__msa_vshf_b((v16i8)pred, filt_121, aver01);
            diff = __msa_asub_u_b(src, pred6);
            sad = __msa_hadd_u_h(diff, diff);
            sad_val = HADD_UH_U32(sad);

            cost[6] = sad_val + ((6 == predictd_mode) ? lambda : 4 * lambda);
        }

        if(valid_intra_modes & 128)
        {
            v16u8 pred = { 5, 6, 7, 8, 21, 22, 23, 24, 6, 7, 8, 9, 22, 23, 24,
                           25 };

            pred7 = (v16u8)__msa_vshf_b((v16i8)pred, filt_121, aver01);
            diff = __msa_asub_u_b(src, pred7);
            sad = __msa_hadd_u_h(diff, diff);
            sad_val = HADD_UH_U32(sad);

            cost[7] = sad_val + ((7 == predictd_mode) ? lambda : 4 * lambda);
        }

        if(valid_intra_modes & 256)
        {
            v16u8 pred = { 2, 17, 1, 16, 1, 16, 0, 31, 0, 31, 15, 15, 15, 15, 15,
                           15 };

            pred8 = (v16u8)__msa_vshf_b((v16i8)pred, filt_121, aver01);
            diff = __msa_asub_u_b(src, pred8);
            sad = __msa_hadd_u_h(diff, diff);
            sad_val = HADD_UH_U32(sad);

            cost[8] = sad_val + ((8 == predictd_mode) ? lambda : 4 * lambda);
        }

        min_cost = MIN3(min_cost, MIN3(cost[3], cost[4], cost[5]),
                        MIN3(cost[6], cost[7], cost[8]));
    }

    *sad_min = min_cost;

    if(min_cost == cost[0])
    {
        *final_intra_mode = VERT_I4x4;
        out = pred0;
    }
    else if(min_cost == cost[1])
    {
        *final_intra_mode = HORZ_I4x4;
        out = pred1;
    }
    else if(min_cost == cost[2])
    {
        *final_intra_mode = DC_I4x4;
        out = pred2;
    }
    else if(min_cost == cost[3])
    {
        *final_intra_mode = DIAG_DL_I4x4;
        out = pred3;
    }
    else if(min_cost == cost[4])
    {
        *final_intra_mode = DIAG_DR_I4x4;
        out = pred4;
    }
    else if(min_cost == cost[5])
    {
        *final_intra_mode = VERT_R_I4x4;
        out = pred5;
    }
    else if(min_cost == cost[6])
    {
        *final_intra_mode = HORZ_D_I4x4;
        out = pred6;
    }
    else if(min_cost == cost[7])
    {
        *final_intra_mode = VERT_L_I4x4;
        out = pred7;
    }
    else if(min_cost == cost[8])
    {
        *final_intra_mode = HORZ_U_I4x4;
        out = pred8;
    }

    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
}

static void avc_evaluate_intrachroma_modes_msa(UWORD8 *src, UWORD32 src_stride,
                                               UWORD8 *ngbr_pel,
                                               UWORD8 *dst, UWORD32 dst_stride,
                                               WORD32 is_left, WORD32 is_top,
                                               UWORD32 valid_intra_modes,
                                               UWORD32 *final_intra_mode,
                                               WORD32 *sad_min)
{
    WORD32 val0, val1, val2, val3, val4, val5, val6, val7;
    WORD32 dcval0, dcval1, dcval2, dcval3, dcval4, dcval5, dcval6, dcval7;
    WORD32 sad_vert, sad_horz, sad_dc, min_sad;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16u8 pred2, pred3, pred4, pred5, pred6, pred7;
    v16u8 pred0 = { 0 };
    v16u8 pred1 = { 0 };
    v16i8 ngbr;
    v8i16 ngbr_invert = { 0 };
    v8u16 sad;

    sad_vert = INT_MAX;
    sad_horz = INT_MAX;
    sad_dc = INT_MAX;
    min_sad = INT_MAX;

    LD_UB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);

    if(is_left)
    {
        if(valid_intra_modes & 02)
        {
            v16i8 invert_shu = { 14, 15, 12, 13, 10, 11, 8, 9,
                                 6, 7, 4, 5, 2, 3, 0, 1 };

            ngbr = LD_SB(ngbr_pel);

            ngbr_invert = (v8i16)__msa_vshf_b(invert_shu, ngbr, ngbr);

            SPLATI_H4_UB(ngbr_invert, 0, 1, 2, 3, pred0, pred1, pred2, pred3);
            SPLATI_H4_UB(ngbr_invert, 4, 5, 6, 7, pred4, pred5, pred6, pred7);

            sad = SAD_UB2_UH(src0, src1, pred0, pred1);
            sad += SAD_UB2_UH(src2, src3, pred2, pred3);
            sad += SAD_UB2_UH(src4, src5, pred4, pred5);
            sad += SAD_UB2_UH(src6, src7, pred6, pred7);

            sad_horz = HADD_UH_U32(sad);
        }
    }

    if(is_top)
    {
        if(valid_intra_modes & 04)
        {
            pred0 = LD_UB(ngbr_pel + 18);

            sad = SAD_UB2_UH(src0, src1, pred0, pred0);
            sad += SAD_UB2_UH(src2, src3, pred0, pred0);
            sad += SAD_UB2_UH(src4, src5, pred0, pred0);
            sad += SAD_UB2_UH(src6, src7, pred0, pred0);

            sad_vert = HADD_UH_U32(sad);
        }
    }

    if(valid_intra_modes & 01)
    {
        if(is_left && is_top)
        {
            val0 = ngbr_pel[14] + ngbr_pel[12] + ngbr_pel[10] + ngbr_pel[8];
            val1 = ngbr_pel[18] + ngbr_pel[20] + ngbr_pel[22] + ngbr_pel[24];
            val2 = ngbr_pel[6] + ngbr_pel[4] + ngbr_pel[2] + ngbr_pel[0];
            val3 = ngbr_pel[26] + ngbr_pel[28] + ngbr_pel[30] + ngbr_pel[32];
            val4 = ngbr_pel[15] + ngbr_pel[13] + ngbr_pel[11] + ngbr_pel[9];
            val5 = ngbr_pel[19] + ngbr_pel[21] + ngbr_pel[23] + ngbr_pel[25];
            val6 = ngbr_pel[7] + ngbr_pel[5] + ngbr_pel[3] + ngbr_pel[1];
            val7 = ngbr_pel[27] + ngbr_pel[29] + ngbr_pel[31] + ngbr_pel[33];

            dcval0 = (val0 + val1 + 4) >> 3;
            dcval1 = (val4 + val5 + 4) >> 3;
            dcval6 = (val2 + val3 + 4) >> 3;
            dcval7 = (val6 + val7 + 4) >> 3;
            dcval2 = (val3 + 2) >> 2;
            dcval3 = (val7 + 2) >> 2;
            dcval4 = (val2 + 2) >> 2;
            dcval5 = (val6 + 2) >> 2;

            dcval0 = ((dcval1 << 8) & 0x0000FF00) | (dcval0 & 0x000000FF);
            dcval2 = ((dcval3 << 8) & 0x0000FF00) | (dcval2 & 0x000000FF);
            dcval4 = ((dcval5 << 8) & 0x0000FF00) | (dcval4 & 0x000000FF);
            dcval6 = ((dcval7 << 8) & 0x0000FF00) | (dcval6 & 0x000000FF);

            pred0 = (v16u8)__msa_fill_h(dcval0);
            pred2 = (v16u8)__msa_fill_h(dcval2);
            pred1 = (v16u8)__msa_fill_h(dcval4);
            pred3 = (v16u8)__msa_fill_h(dcval6);

            ILVR_D2_UB(pred2, pred0, pred3, pred1, pred0, pred1);
        }
        else if(is_left)
        {
            val0 = ngbr_pel[14] + ngbr_pel[12] + ngbr_pel[10] + ngbr_pel[8];
            val4 = ngbr_pel[15] + ngbr_pel[13] + ngbr_pel[11] + ngbr_pel[9];
            val2 = ngbr_pel[6] + ngbr_pel[4] + ngbr_pel[2] + ngbr_pel[0];
            val6 = ngbr_pel[7] + ngbr_pel[5] + ngbr_pel[3] + ngbr_pel[1];

            dcval0 = (val0 + 2) >> 2;
            dcval1 = (val4 + 2) >> 2;
            dcval4 = (val2 + 2) >> 2;
            dcval5 = (val6 + 2) >> 2;

            dcval0 = ((dcval1 << 8) & 0x0000FF00) | (dcval0 & 0x000000FF);
            dcval4 = ((dcval5 << 8) & 0x0000FF00) | (dcval4 & 0x000000FF);

            pred0 = (v16u8)__msa_fill_h(dcval0);
            pred1 = (v16u8)__msa_fill_h(dcval4);
        }
        else if(is_top)
        {
            val1 = ngbr_pel[18] + ngbr_pel[20] + ngbr_pel[22] + ngbr_pel[24];
            val5 = ngbr_pel[19] + ngbr_pel[21] + ngbr_pel[23] + ngbr_pel[25];
            val3 = ngbr_pel[26] + ngbr_pel[28] + ngbr_pel[30] + ngbr_pel[32];
            val7 = ngbr_pel[27] + ngbr_pel[29] + ngbr_pel[31] + ngbr_pel[33];

            dcval0 = (val1 + 2) >> 2;
            dcval1 = (val5 + 2) >> 2;
            dcval2 = (val3 + 2) >> 2;
            dcval3 = (val7 + 2) >> 2;

            dcval0 = ((dcval1 << 8) & 0x0000FF00) | (dcval0 & 0x000000FF);
            dcval2 = ((dcval3 << 8) & 0x0000FF00) | (dcval2 & 0x000000FF);

            pred0 = (v16u8)__msa_fill_h(dcval0);
            pred1 = (v16u8)__msa_fill_h(dcval2);

            pred0 = (v16u8)__msa_ilvr_d((v2i64)pred1, (v2i64)pred0);
            pred1 = pred0;
        }
        else
        {
            pred0 = (v16u8)__msa_fill_b(128);
            pred1 = (v16u8)__msa_fill_b(128);
        }

        sad = SAD_UB2_UH(src0, src1, pred0, pred0);
        sad += SAD_UB2_UH(src2, src3, pred0, pred0);
        sad += SAD_UB2_UH(src4, src5, pred1, pred1);
        sad += SAD_UB2_UH(src6, src7, pred1, pred1);

        sad_dc = HADD_UH_U32(sad);
    }

    min_sad = MIN3(sad_horz, sad_dc, sad_vert);

    /* Finding minimum sad and doing corresponding prediction */
    if(min_sad < *sad_min)
    {
        *sad_min = min_sad;

        if(min_sad == sad_dc)
        {
            *final_intra_mode = DC_CH_I8x8;
            ST_UB8(pred0, pred0, pred0, pred0, pred1, pred1, pred1, pred1,
                   dst, dst_stride);
        }
        else if(min_sad == sad_horz)
        {
            *final_intra_mode = HORZ_CH_I8x8;

            SPLATI_H4_UB(ngbr_invert, 0, 1, 2, 3, pred0, pred1, pred2, pred3);
            SPLATI_H4_UB(ngbr_invert, 4, 5, 6, 7, pred4, pred5, pred6, pred7);

            ST_UB8(pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7,
                   dst, dst_stride);
        }
        else
        {
            *final_intra_mode = VERT_CH_I8x8;

            pred0 = LD_UB(ngbr_pel + 18);
            ST_UB8(pred0, pred0, pred0, pred0, pred0, pred0, pred0, pred0,
                   dst, dst_stride);
        }
    }
}

void ih264e_evaluate_intra16x16_modes_msa(UWORD8 *src, UWORD8 *ngbr_pels,
                                          UWORD8 *dst, UWORD32 src_stride,
                                          UWORD32 dst_stride, WORD32 n_avblty,
                                          UWORD32 *intra_mode, WORD32 *sad_min,
                                          UWORD32 valid_intra_modes)
{
    avc_evaluate_intra16x16_modes_msa(src, src_stride, ngbr_pels,
                                      dst, dst_stride,
                                      (n_avblty & LEFT_MB_AVAILABLE_MASK),
                                      (n_avblty & TOP_MB_AVAILABLE_MASK) >> 2,
                                      valid_intra_modes, intra_mode, sad_min);
}

void ih264e_evaluate_intra_4x4_modes_msa(UWORD8 *src, UWORD8 *ngbr_pels,
                                         UWORD8 *dst, UWORD32 src_stride,
                                         UWORD32 dst_stride, WORD32 n_avblty,
                                         UWORD32 *intra_mode, WORD32 *sad_min,
                                         UWORD32 valid_intra_modes,
                                         UWORD32 lambda, UWORD32 predictd_mode)
{
    avc_evaluate_intra4x4_modes_msa(src, src_stride, ngbr_pels,
                                    dst, dst_stride,
                                    (n_avblty & LEFT_MB_AVAILABLE_MASK),
                                    (n_avblty & TOP_MB_AVAILABLE_MASK) >> 2,
                                    valid_intra_modes, lambda, predictd_mode,
                                    intra_mode, sad_min);
}

void ih264e_evaluate_intra_chroma_modes_msa(UWORD8 *src, UWORD8 *ngbr_pels,
                                            UWORD8 *dst, UWORD32 src_stride,
                                            UWORD32 dst_stride, WORD32 n_avblty,
                                            UWORD32 *intra_mode,
                                            WORD32 *sad_min,
                                            UWORD32 valid_intra_modes)
{
    avc_evaluate_intrachroma_modes_msa(src, src_stride, ngbr_pels,
                                       dst, dst_stride,
                                       (n_avblty & LEFT_MB_AVAILABLE_MASK),
                                       (n_avblty & TOP_MB_AVAILABLE_MASK) >> 2,
                                       valid_intra_modes, intra_mode, sad_min);
}
