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
#include "ih264_defs.h"
#include "ih264_macros_msa.h"
#include "ih264e_half_pel.h"

static void avc_horiz_filter_6taps_17x16_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride)
{
    UWORD32 loop_cnt;
    WORD32 filt_sum0, filt_sum1;
    v16i8 src0, src1, src2, src3;
    v16i8 mask0 = { 0, 5, 1, 6, 2, 7, 3, 8, 4, 9, 5, 10, 6, 11, 7, 12 };
    v16i8 mask1 = { 1, 4, 2, 5, 3, 6, 4, 7, 5, 8, 6, 9, 7, 10, 8, 11 };
    v16i8 mask2 = { 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10 };
    v16i8 filt = { 0, 0, 0, 0, 0, 0, 0, 0, 1, -5, 20, 20, -5, 1, 0, 0 };
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7, vec8, vec9, vec10;
    v16i8 vec11;
    v16i8 minus5b = __msa_ldi_b(-5);
    v16i8 plus20b = __msa_ldi_b(20);
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5;

    for(loop_cnt = 4; loop_cnt--;)
    {
        LD_SB2(src, 8, src0, src1);
        src += src_stride;
        LD_SB2(src, 8, src2, src3);
        src += src_stride;

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec1);
        VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec2, vec3);
        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec4, vec5);
        VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec6, vec7);
        VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec8, vec9);
        VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec10, vec11);
        HADD_SB4_SH(vec0, vec1, vec2, vec3, dst0, dst1, dst2, dst3);
        DPADD_SB4_SH(vec4, vec5, vec6, vec7, minus5b, minus5b, minus5b, minus5b,
                     dst0, dst1, dst2, dst3);
        DPADD_SB4_SH(vec8, vec9, vec10, vec11, plus20b, plus20b, plus20b,
                     plus20b, dst0, dst1, dst2, dst3);
        SRARI_H4_SH(dst0, dst1, dst2, dst3, 5);
        SAT_SH4_SH(dst0, dst1, dst2, dst3, 7);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, dst0, dst2);
        XORI_B2_128_SH(dst0, dst2);

        /* do the 17th column here */
        DOTP_SB2_SH(src1, src3, filt, filt, dst4, dst5);

        filt_sum0 = __msa_copy_s_h(dst4, 4);
        filt_sum1 = __msa_copy_s_h(dst5, 4);
        filt_sum0 += __msa_copy_s_h(dst4, 5);
        filt_sum1 += __msa_copy_s_h(dst5, 5);
        filt_sum0 += __msa_copy_s_h(dst4, 6);
        filt_sum1 += __msa_copy_s_h(dst5, 6);
        filt_sum0 = (filt_sum0 + 16) >> 5;
        filt_sum1 = (filt_sum1 + 16) >> 5;

        if(filt_sum0 > 127)
        {
            filt_sum0 = 127;
        }
        else if(filt_sum0 < -128)
        {
            filt_sum0 = -128;
        }

        if(filt_sum1 > 127)
        {
            filt_sum1 = 127;
        }
        else if(filt_sum1 < -128)
        {
            filt_sum1 = -128;
        }

        filt_sum0 = (UWORD8)filt_sum0 ^ 128;
        filt_sum1 = (UWORD8)filt_sum1 ^ 128;

        LD_SB2(src, 8, src0, src1);
        src += src_stride;
        LD_SB2(src, 8, src2, src3);
        src += src_stride;

        ST_UB((v16u8)dst0, dst);
        *(dst + 16) = (UWORD8)filt_sum0;
        dst += dst_stride;
        ST_UB((v16u8)dst2, dst);
        *(dst + 16) = (UWORD8)filt_sum1;
        dst += dst_stride;

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec1);
        VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec2, vec3);
        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec4, vec5);
        VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec6, vec7);
        VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec8, vec9);
        VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec10, vec11);
        HADD_SB4_SH(vec0, vec1, vec2, vec3, dst0, dst1, dst2, dst3);
        DPADD_SB4_SH(vec4, vec5, vec6, vec7, minus5b, minus5b, minus5b, minus5b,
                     dst0, dst1, dst2, dst3);
        DPADD_SB4_SH(vec8, vec9, vec10, vec11, plus20b, plus20b, plus20b,
                     plus20b, dst0, dst1, dst2, dst3);
        SRARI_H4_SH(dst0, dst1, dst2, dst3, 5);
        SAT_SH4_SH(dst0, dst1, dst2, dst3, 7);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, dst0, dst2);
        XORI_B2_128_SH(dst0, dst2);

        /* do the 17th column here */
        DOTP_SB2_SH(src1, src3, filt, filt, dst4, dst5);

        filt_sum0 = __msa_copy_s_h(dst4, 4);
        filt_sum1 = __msa_copy_s_h(dst5, 4);
        filt_sum0 += __msa_copy_s_h(dst4, 5);
        filt_sum1 += __msa_copy_s_h(dst5, 5);
        filt_sum0 += __msa_copy_s_h(dst4, 6);
        filt_sum1 += __msa_copy_s_h(dst5, 6);
        filt_sum0 = (filt_sum0 + 16) >> 5;
        filt_sum1 = (filt_sum1 + 16) >> 5;

        if(filt_sum0 > 127)
        {
            filt_sum0 = 127;
        }
        else if(filt_sum0 < -128)
        {
            filt_sum0 = -128;
        }

        if(filt_sum1 > 127)
        {
            filt_sum1 = 127;
        }
        else if(filt_sum1 < -128)
        {
            filt_sum1 = -128;
        }

        filt_sum0 = (UWORD8)filt_sum0 ^ 128;
        filt_sum1 = (UWORD8)filt_sum1 ^ 128;

        ST_UB((v16u8)dst0, dst);
        *(dst + 16) = (UWORD8)filt_sum0;
        dst += dst_stride;
        ST_UB((v16u8)dst2, dst);
        *(dst + 16) = (UWORD8)filt_sum1;
        dst += dst_stride;
    }
}

static void avc_vert_int_filter_6taps_24x17_msa(UWORD8 *src,
                                                WORD32 src_stride,
                                                WORD16 *int_buf,
                                                WORD32 buf_stride,
                                                UWORD8 *dst_v,
                                                WORD32 dst_stride)
{
    WORD32 loop_cnt;
    WORD16 filt_const0 = 0xfb01;
    WORD16 filt_const1 = 0x1414;
    WORD16 filt_const2 = 0x1fb;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src11, src12, src13;
    v16i8 src10_r, src32_r, src54_r, src21_r, src43_r, src65_r, src10_l;
    v16i8 src32_l, src54_l, src21_l, src43_l, src65_l, src87_r, src98_r;
    v16i8 src109_r, src1110_r, src1211_r, src1312_r, filt0, filt1, filt2;
    v8i16 out0_r, out1_r, out2_r, out3_r, out0_l, out1_l;
    v16u8 res0, res1, res2, res3;

    filt0 = (v16i8)__msa_fill_h(filt_const0);
    filt1 = (v16i8)__msa_fill_h(filt_const1);
    filt2 = (v16i8)__msa_fill_h(filt_const2);

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    LD_SB5(src + 16, src_stride, src7, src8, src9, src10, src11);
    src += (5 * src_stride);

    XORI_B5_128_SB(src0, src1, src2, src3, src4);
    ILVR_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3, src10_r, src21_r,
               src32_r, src43_r);
    ILVL_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3, src10_l, src21_l,
               src32_l, src43_l);
    ILVR_B4_SB(src8, src7, src9, src8, src10, src9, src11, src10, src87_r,
               src98_r, src109_r, src1110_r);

    XORI_B4_128_SB(src87_r, src98_r, src109_r, src1110_r);

    for(loop_cnt = 8; loop_cnt--;)
    {
        LD_SB2(src, src_stride, src5, src6);
        LD_SB2(src + 16, src_stride, src12, src13);
        src += (2 * src_stride);

        XORI_B2_128_SB(src5, src6);

        ILVR_B2_SB(src12, src11, src13, src12, src1211_r, src1312_r);
        ILVR_B2_SB(src5, src4, src6, src5, src54_r, src65_r);
        ILVL_B2_SB(src5, src4, src6, src5, src54_l, src65_l);

        XORI_B2_128_SB(src1211_r, src1312_r);

        out0_r = DPADD_SH3_SH(src10_r, src32_r, src54_r, filt0, filt1, filt2);
        out1_r = DPADD_SH3_SH(src21_r, src43_r, src65_r, filt0, filt1, filt2);
        out2_r = DPADD_SH3_SH(src87_r, src109_r, src1211_r, filt0, filt1, filt2);
        out3_r = DPADD_SH3_SH(src98_r, src1110_r, src1312_r, filt0, filt1, filt2);
        out0_l = DPADD_SH3_SH(src10_l, src32_l, src54_l, filt0, filt1, filt2);
        out1_l = DPADD_SH3_SH(src21_l, src43_l, src65_l, filt0, filt1, filt2);

        ST_SH2(out0_r, out0_l, int_buf, 8);
        ST_SH(out2_r, int_buf + 16);
        int_buf += buf_stride;
        ST_SH2(out1_r, out1_l, int_buf, 8);
        ST_SH(out3_r, int_buf + 16);
        int_buf += buf_stride;

        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 5);
        SRARI_H2_SH(out0_l, out1_l, 5);

        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        SAT_SH2_SH(out0_l, out1_l, 7);

        PCKEV_B4_UB(out0_l, out0_r, out1_l, out1_r, out2_r, out2_r, out3_r, out3_r,
                    res0, res1, res2, res3);

        XORI_B4_128_UB(res0, res1, res2, res3);
        SLDI_B2_UB(res2, res3, res0, res1, res0, res1, 2);

        ST_UB(res0, dst_v);
        dst_v[16] = res2[2];
        dst_v += dst_stride;

        ST_UB(res1, dst_v);
        dst_v[16] = res3[2];
        dst_v += dst_stride;

        src10_r = src32_r;
        src32_r = src54_r;
        src21_r = src43_r;
        src43_r = src65_r;
        src10_l = src32_l;
        src32_l = src54_l;
        src21_l = src43_l;
        src43_l = src65_l;
        src4 = src6;

        src87_r = src109_r;
        src109_r = src1211_r;
        src98_r = src1110_r;
        src1110_r = src1312_r;
        src11 = src13;
    }

    LD_SB2(src, 16, src5, src12);

    src1211_r = __msa_ilvr_b(src12, src11);

    XORI_B2_128_SB(src5, src1211_r);

    ILVRL_B2_SB(src5, src4, src54_r, src54_l);

    out0_r = DPADD_SH3_SH(src10_r, src32_r, src54_r, filt0, filt1, filt2);
    out0_l = DPADD_SH3_SH(src10_l, src32_l, src54_l, filt0, filt1, filt2);
    out2_r = DPADD_SH3_SH(src87_r, src109_r, src1211_r, filt0, filt1, filt2);
    ST_SH2(out0_r, out0_l, int_buf, 8);
    ST_SH(out2_r, int_buf + 16);
    SRARI_H2_SH(out0_r, out0_l, 5);
    out2_r = __msa_srari_h(out2_r, 5);
    SAT_SH3_SH(out0_r, out0_l, out2_r, 7);

    res0 = PCKEV_XORI128_UB(out0_r, out0_l);
    res1 = PCKEV_XORI128_UB(out2_r, out2_r);

    res0 = (v16u8)__msa_sldi_b((v16i8)res1, (v16i8)res0, 2);

    ST_UB(res0, dst_v);
    dst_v[16] = res1[2];
}

static void avc_mid_filter_s16in_17x17_msa(WORD16 *src, WORD32 src_stride,
                                           UWORD8 *dst, WORD32 dst_stride)
{
    WORD32 loop_cnt;
    WORD32 filt_sum0;
    v8i16 src0, src1, src2, src3, src4, src5;
    v16i8 mask0 = { 0, 1, 10, 11, 2, 3, 12, 13, 4, 5, 14, 15, 6, 7, 16, 17 };
    v16i8 mask1 = { 2, 3, 8, 9, 4, 5, 10, 11, 6, 7, 12, 13, 8, 9, 14, 15 };
    v16i8 mask2 = { 4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13 };
    v16i8 mask3, mask4, mask5;
    v8i16 filt = { 1, -5, 20, 20, -5, 1, 0, 0 };
    v4i32 dst0, dst1, dst2, dst3;
    v8i16 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7, vec8, vec9, vec10, vec11;
    v8i16 minus5h = __msa_ldi_h(-5);
    v8i16 plus20h = __msa_ldi_h(20);
    v8i16 out_hv0, out_hv1;
    v16u8 res_hv0;

    LD_SH2(src, 8, src0, src1);
    src2 = LD_SH(src + 16);
    src += src_stride;

    for(loop_cnt = 8; loop_cnt--;)
    {
        mask3 = ADDVI_B(mask0, 8);
        mask4 = ADDVI_B(mask1, 8);
        mask5 = ADDVI_B(mask2, 8);

        VSHF_B2_SH(src0, src1, src0, src1, mask0, mask3, vec0, vec1);
        VSHF_B2_SH(src1, src2, src1, src2, mask0, mask3, vec2, vec3);
        VSHF_B2_SH(src0, src1, src0, src1, mask1, mask4, vec4, vec5);
        VSHF_B2_SH(src1, src2, src1, src2, mask1, mask4, vec6, vec7);
        VSHF_B2_SH(src0, src1, src0, src1, mask2, mask5, vec8, vec9);
        VSHF_B2_SH(src1, src2, src1, src2, mask2, mask5, vec10, vec11);

        dst0 = __msa_hadd_s_w(vec0, vec0);
        dst1 = __msa_hadd_s_w(vec1, vec1);
        dst2 = __msa_hadd_s_w(vec2, vec2);
        dst3 = __msa_hadd_s_w(vec3, vec3);

        dst0 = __msa_dpadd_s_w(dst0, vec4, minus5h);
        dst1 = __msa_dpadd_s_w(dst1, vec5, minus5h);
        dst2 = __msa_dpadd_s_w(dst2, vec6, minus5h);
        dst3 = __msa_dpadd_s_w(dst3, vec7, minus5h);

        dst0 = __msa_dpadd_s_w(dst0, vec8, plus20h);
        dst1 = __msa_dpadd_s_w(dst1, vec9, plus20h);
        dst2 = __msa_dpadd_s_w(dst2, vec10, plus20h);
        dst3 = __msa_dpadd_s_w(dst3, vec11, plus20h);

        SRARI_W4_SW(dst0, dst1, dst2, dst3, 10);
        SAT_SW4_SW(dst0, dst1, dst2, dst3, 7);

        out_hv0 = __msa_pckev_h((v8i16)dst1, (v8i16)dst0);
        out_hv1 = __msa_pckev_h((v8i16)dst3, (v8i16)dst2);

        res_hv0 = PCKEV_XORI128_UB(out_hv0, out_hv1);

        LD_SH2(src, 8, src3, src4);
        src5 = LD_SH(src + 16);
        src += src_stride;

        dst0 = __msa_dotp_s_w((v8i16)src2, (v8i16)filt);
        filt_sum0 = __msa_copy_s_w(dst0, 0);
        filt_sum0 += __msa_copy_s_w(dst0, 1);
        filt_sum0 += __msa_copy_s_w(dst0, 2);
        filt_sum0 = (filt_sum0 + 512) >> 10;

        if(filt_sum0 > 127)
        {
            filt_sum0 = 127;
        }
        else if(filt_sum0 < -128)
        {
            filt_sum0 = -128;
        }

        mask3 = ADDVI_B(mask0, 8);
        mask4 = ADDVI_B(mask1, 8);
        mask5 = ADDVI_B(mask2, 8);

        VSHF_B2_SH(src3, src4, src3, src4, mask0, mask3, vec0, vec1);
        VSHF_B2_SH(src4, src5, src4, src5, mask0, mask3, vec2, vec3);
        VSHF_B2_SH(src3, src4, src3, src4, mask1, mask4, vec4, vec5);
        VSHF_B2_SH(src4, src5, src4, src5, mask1, mask4, vec6, vec7);
        VSHF_B2_SH(src3, src4, src3, src4, mask2, mask5, vec8, vec9);
        VSHF_B2_SH(src4, src5, src4, src5, mask2, mask5, vec10, vec11);

        filt_sum0 = (UWORD8)filt_sum0 ^ 128;

        ST_UB(res_hv0, dst);
        *(dst + 16) = (UWORD8)filt_sum0;
        dst += dst_stride;

        dst0 = __msa_hadd_s_w(vec0, vec0);
        dst1 = __msa_hadd_s_w(vec1, vec1);
        dst2 = __msa_hadd_s_w(vec2, vec2);
        dst3 = __msa_hadd_s_w(vec3, vec3);

        dst0 = __msa_dpadd_s_w(dst0, vec4, minus5h);
        dst1 = __msa_dpadd_s_w(dst1, vec5, minus5h);
        dst2 = __msa_dpadd_s_w(dst2, vec6, minus5h);
        dst3 = __msa_dpadd_s_w(dst3, vec7, minus5h);

        dst0 = __msa_dpadd_s_w(dst0, vec8, plus20h);
        dst1 = __msa_dpadd_s_w(dst1, vec9, plus20h);
        dst2 = __msa_dpadd_s_w(dst2, vec10, plus20h);
        dst3 = __msa_dpadd_s_w(dst3, vec11, plus20h);

        SRARI_W4_SW(dst0, dst1, dst2, dst3, 10);
        SAT_SW4_SW(dst0, dst1, dst2, dst3, 7);

        out_hv0 = __msa_pckev_h((v8i16)dst1, (v8i16)dst0);
        out_hv1 = __msa_pckev_h((v8i16)dst3, (v8i16)dst2);

        res_hv0 = PCKEV_XORI128_UB(out_hv0, out_hv1);

        dst0 = __msa_dotp_s_w((v8i16)src5, (v8i16)filt);
        filt_sum0 = __msa_copy_s_w(dst0, 0);
        filt_sum0 += __msa_copy_s_w(dst0, 1);
        filt_sum0 += __msa_copy_s_w(dst0, 2);
        filt_sum0 = (filt_sum0 + 512) >> 10;

        LD_SH2(src, 8, src0, src1);
        src2 = LD_SH(src + 16);
        src += src_stride;

        if(filt_sum0 > 127)
        {
            filt_sum0 = 127;
        }
        else if(filt_sum0 < -128)
        {
            filt_sum0 = -128;
        }

        filt_sum0 = (UWORD8)filt_sum0 ^ 128;

        ST_UB(res_hv0, dst);
        *(dst + 16) = (UWORD8)filt_sum0;
        dst += dst_stride;

    }

    mask3 = ADDVI_B(mask0, 8);
    mask4 = ADDVI_B(mask1, 8);
    mask5 = ADDVI_B(mask2, 8);

    VSHF_B2_SH(src0, src1, src0, src1, mask0, mask3, vec0, vec1);
    VSHF_B2_SH(src1, src2, src1, src2, mask0, mask3, vec2, vec3);
    VSHF_B2_SH(src0, src1, src0, src1, mask1, mask4, vec4, vec5);
    VSHF_B2_SH(src1, src2, src1, src2, mask1, mask4, vec6, vec7);
    VSHF_B2_SH(src0, src1, src0, src1, mask2, mask5, vec8, vec9);
    VSHF_B2_SH(src1, src2, src1, src2, mask2, mask5, vec10, vec11);

    dst0 = __msa_hadd_s_w(vec0, vec0);
    dst1 = __msa_hadd_s_w(vec1, vec1);
    dst2 = __msa_hadd_s_w(vec2, vec2);
    dst3 = __msa_hadd_s_w(vec3, vec3);

    dst0 = __msa_dpadd_s_w(dst0, vec4, minus5h);
    dst1 = __msa_dpadd_s_w(dst1, vec5, minus5h);
    dst2 = __msa_dpadd_s_w(dst2, vec6, minus5h);
    dst3 = __msa_dpadd_s_w(dst3, vec7, minus5h);

    dst0 = __msa_dpadd_s_w(dst0, vec8, plus20h);
    dst1 = __msa_dpadd_s_w(dst1, vec9, plus20h);
    dst2 = __msa_dpadd_s_w(dst2, vec10, plus20h);
    dst3 = __msa_dpadd_s_w(dst3, vec11, plus20h);

    SRARI_W4_SW(dst0, dst1, dst2, dst3, 10);
    SAT_SW4_SW(dst0, dst1, dst2, dst3, 7);

    out_hv0 = __msa_pckev_h((v8i16)dst1, (v8i16)dst0);
    out_hv1 = __msa_pckev_h((v8i16)dst3, (v8i16)dst2);

    res_hv0 = PCKEV_XORI128_UB(out_hv0, out_hv1);

    dst0 = __msa_dotp_s_w((v8i16)src2, (v8i16)filt);
    filt_sum0 = __msa_copy_s_w(dst0, 0);
    filt_sum0 += __msa_copy_s_w(dst0, 1);
    filt_sum0 += __msa_copy_s_w(dst0, 2);
    filt_sum0 = (filt_sum0 + 512) >> 10;

    if(filt_sum0 > 127)
    {
        filt_sum0 = 127;
    }
    else if(filt_sum0 < -128)
    {
        filt_sum0 = -128;
    }

    filt_sum0 = (UWORD8)filt_sum0 ^ 128;

    ST_UB(res_hv0, dst);
    *(dst + 16) = (UWORD8)filt_sum0;
}

void ih264e_sixtapfilter_horz_msa(UWORD8 *src, UWORD8 *dst,
                                  WORD32 src_stride, WORD32 dst_stride)
{
    avc_horiz_filter_6taps_17x16_msa(src - 2, src_stride, dst, dst_stride);
}

void ih264e_sixtap_filter_2dvh_vert_msa(UWORD8 *src, UWORD8 *dst1, UWORD8 *dst2,
                                        WORD32 src_stride, WORD32 dst_stride,
                                        WORD32 *pred, WORD32 pred_stride)
{
    WORD16 *pred1 = (WORD16 *)pred;

    avc_vert_int_filter_6taps_24x17_msa(src - 2 - 2 * src_stride, src_stride,
                                        pred1, pred_stride, dst1, dst_stride);
    avc_mid_filter_s16in_17x17_msa(pred1, pred_stride, dst2, dst_stride);
}
