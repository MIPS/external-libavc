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
#include "ih264_defs.h"
#include "ih264_typedefs.h"
#include "ih264_macros.h"
#include "ih264_platform_macros.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_macros_msa.h"

static void intra_predict_vert_4x4_msa(UWORD8 *src, UWORD8 *dst,
                                       WORD32 dst_stride)
{
    UWORD32 out = LW(src);

    SW4(out, out, out, out, dst, dst_stride);
}

static void intra_predict_vert_8x8_msa(UWORD8 *src, UWORD8 *dst,
                                       WORD32 dst_stride)
{
    UWORD64 out = LD(src);

    SD4(out, out, out, out, dst, dst_stride);
    dst += (4 * dst_stride);
    SD4(out, out, out, out, dst, dst_stride);
}

static void intra_predict_vert_16x16_msa(UWORD8 *src, UWORD8 *dst,
                                         WORD32 dst_stride)
{
    v16u8 out = LD_UB(src);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
    dst += (8 * dst_stride);
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

static void intra_predict_horiz_4x4_msa(UWORD8 *src, UWORD8 *dst,
                                        WORD32 dst_stride)
{
    UWORD32 out0, out1, out2, out3;

    out0 = src[0] * 0x01010101;
    out1 = src[1] * 0x01010101;
    out2 = src[2] * 0x01010101;
    out3 = src[3] * 0x01010101;

    SW4(out0, out1, out2, out3, dst, dst_stride);
}

static void intra_predict_horiz_8x8_msa(UWORD8 *src, UWORD8 *dst,
                                        WORD32 dst_stride)
{
    UWORD64 out0, out1, out2, out3, out4, out5, out6, out7;

    out0 = src[0] * 0x0101010101010101ull;
    out1 = src[1] * 0x0101010101010101ull;
    out2 = src[2] * 0x0101010101010101ull;
    out3 = src[3] * 0x0101010101010101ull;
    out4 = src[4] * 0x0101010101010101ull;
    out5 = src[5] * 0x0101010101010101ull;
    out6 = src[6] * 0x0101010101010101ull;
    out7 = src[7] * 0x0101010101010101ull;

    SD4(out0, out1, out2, out3, dst, dst_stride);
    dst += (4 * dst_stride);
    SD4(out4, out5, out6, out7, dst, dst_stride);
}

static void intra_predict_horiz_16x16_msa(UWORD8 *src, UWORD8 *dst,
                                          WORD32 dst_stride)
{
    UWORD32 row;
    UWORD8 inp0, inp1, inp2, inp3;
    v16u8 src0, src1, src2, src3;

    for(row = 4; row--;)
    {
        inp0 = src[0];
        inp1 = src[1];
        inp2 = src[2];
        inp3 = src[3];
        src += 4;

        src0 = (v16u8)__msa_fill_b(inp0);
        src1 = (v16u8)__msa_fill_b(inp1);
        src2 = (v16u8)__msa_fill_b(inp2);
        src3 = (v16u8)__msa_fill_b(inp3);

        ST_UB4(src0, src1, src2, src3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void intra_predict_dc_4x4_msa(UWORD8 *src_top, UWORD8 *src_left,
                                     UWORD8 *dst, WORD32 dst_stride)
{
    UWORD32 val0, val1;
    v16i8 store, src = { 0 };
    v8u16 sum_h;
    v4u32 sum_w;
    v2u64 sum_d;

    val0 = LW(src_top);
    val1 = LW(src_left);
    INSERT_W2_SB(val0, val1, src);
    sum_h = __msa_hadd_u_h((v16u8)src, (v16u8)src);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32)__msa_srari_w((v4i32)sum_d, 3);
    store = __msa_splati_b((v16i8)sum_w, 0);
    val0 = __msa_copy_u_w((v4i32)store, 0);

    SW4(val0, val0, val0, val0, dst, dst_stride);
}

static void intra_predict_dc_tl_4x4_msa(UWORD8 *src, UWORD8 *dst,
                                        WORD32 dst_stride)
{
    UWORD32 val0;
    v16i8 store, data = { 0 };
    v8u16 sum_h;
    v4u32 sum_w;

    val0 = LW(src);
    data = (v16i8)__msa_insert_w((v4i32)data, 0, val0);
    sum_h = __msa_hadd_u_h((v16u8)data, (v16u8)data);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_w = (v4u32)__msa_srari_w((v4i32)sum_w, 2);
    store = __msa_splati_b((v16i8)sum_w, 0);
    val0 = __msa_copy_u_w((v4i32)store, 0);

    SW4(val0, val0, val0, val0, dst, dst_stride);
}

static void intra_predict_dc_8x8_msa(UWORD8 *src_top, UWORD8 *src_left,
                                     UWORD8 *dst, WORD32 dst_stride)
{
    UWORD64 val0, val1;
    v16i8 store;
    v16u8 src = { 0 };
    v8u16 sum_h;
    v4u32 sum_w;
    v2u64 sum_d;

    val0 = LD(src_top);
    val1 = LD(src_left);
    INSERT_D2_UB(val0, val1, src);
    sum_h = __msa_hadd_u_h(src, src);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32)__msa_pckev_w((v4i32)sum_d, (v4i32)sum_d);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32)__msa_srari_w((v4i32)sum_d, 4);
    store = __msa_splati_b((v16i8)sum_w, 0);
    val0 = __msa_copy_u_d((v2i64)store, 0);

    SD4(val0, val0, val0, val0, dst, dst_stride);
    dst += (4 * dst_stride);
    SD4(val0, val0, val0, val0, dst, dst_stride);
}

static void intra_predict_dc_tl_8x8_msa(UWORD8 *src, UWORD8 *dst,
                                        WORD32 dst_stride)
{
    UWORD64 val0;
    v16i8 store;
    v16u8 data = { 0 };
    v8u16 sum_h;
    v4u32 sum_w;
    v2u64 sum_d;

    val0 = LD(src);
    data = (v16u8)__msa_insert_d((v2i64)data, 0, val0);
    sum_h = __msa_hadd_u_h(data, data);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32)__msa_srari_w((v4i32)sum_d, 3);
    store = __msa_splati_b((v16i8)sum_w, 0);
    val0 = __msa_copy_u_d((v2i64)store, 0);

    SD4(val0, val0, val0, val0, dst, dst_stride);
    dst += (4 * dst_stride);
    SD4(val0, val0, val0, val0, dst, dst_stride);
}

static void intra_predict_dc_16x16_msa(UWORD8 *src_top, UWORD8 *src_left,
                                       UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 top, left, out;
    v8u16 sum_h, sum_top, sum_left;
    v4u32 sum_w;
    v2u64 sum_d;

    top = LD_UB(src_top);
    left = LD_UB(src_left);
    HADD_UB2_UH(top, left, sum_top, sum_left);
    sum_h = sum_top + sum_left;
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32)__msa_pckev_w((v4i32)sum_d, (v4i32)sum_d);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32)__msa_srari_w((v4i32)sum_d, 5);
    out = (v16u8)__msa_splati_b((v16i8)sum_w, 0);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
    dst += (8 * dst_stride);
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

static void intra_predict_dc_tl_16x16_msa(UWORD8 *src, UWORD8 *dst,
                                          WORD32 dst_stride)
{
    v16u8 data, out;
    v8u16 sum_h;
    v4u32 sum_w;
    v2u64 sum_d;

    data = LD_UB(src);
    sum_h = __msa_hadd_u_h(data, data);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32)__msa_pckev_w((v4i32)sum_d, (v4i32)sum_d);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32)__msa_srari_w((v4i32)sum_d, 4);
    out = (v16u8)__msa_splati_b((v16i8)sum_w, 0);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
    dst += (8 * dst_stride);
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

static void intra_predict_128dc_4x4_msa(UWORD8 *dst, WORD32 dst_stride)
{
    v16i8 store = __msa_ldi_b(128);
    UWORD32 out = __msa_copy_u_w((v4i32)store, 0);

    SW4(out, out, out, out, dst, dst_stride);
}

static void intra_predict_128dc_8x8_msa(UWORD8 *dst, WORD32 dst_stride)
{
    v16i8 store = __msa_ldi_b(128);
    UWORD64 out = __msa_copy_u_d((v2i64)store, 0);

    SD4(out, out, out, out, dst, dst_stride);
    dst += (4 * dst_stride);
    SD4(out, out, out, out, dst, dst_stride);
}

static void intra_predict_128dc_16x16_msa(UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 out = (v16u8)__msa_ldi_b(128);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
    dst += (8 * dst_stride);
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

static void intra_predict_ddl_4x4_msa(UWORD8 *src_ptr, UWORD8 *dst,
                                      WORD32 dst_stride)
{
    UWORD8 src_val = src_ptr[7];
    UWORD64 data;
    UWORD32 out0, out1, out2, out3;
    v16u8 vec4, vec5, res0, src = { 0 };
    v8u16 vec0, vec1, vec2, vec3;
    v4i32 res1, res2, res3;

    data = LD(src_ptr);

    src = (v16u8)__msa_insert_d((v2i64)src, 0, data);
    vec4 = (v16u8)__msa_sldi_b((v16i8)src, (v16i8)src, 1);
    vec5 = (v16u8)__msa_sldi_b((v16i8)src, (v16i8)src, 2);
    vec5 = (v16u8)__msa_insert_b((v16i8)vec5, 6, src_val);
    ILVR_B2_UH(vec5, src, vec4, vec4, vec0, vec1);
    ILVL_B2_UH(vec5, src, vec4, vec4, vec2, vec3);
    HADD_UB4_UH(vec0, vec1, vec2, vec3, vec0, vec1, vec2, vec3);
    ADD2(vec0, vec1, vec2, vec3, vec0, vec2);
    SRARI_H2_UH(vec0, vec2, 2);

    res0 = (v16u8)__msa_pckev_b((v16i8)vec2, (v16i8)vec0);
    res1 = (v4i32)__msa_sldi_b((v16i8)res0, (v16i8)res0, 1);
    res2 = (v4i32)__msa_sldi_b((v16i8)res0, (v16i8)res0, 2);
    res3 = (v4i32)__msa_sldi_b((v16i8)res0, (v16i8)res0, 3);
    out0 = __msa_copy_u_w((v4i32)res0, 0);
    out1 = __msa_copy_u_w(res1, 0);
    out2 = __msa_copy_u_w(res2, 0);
    out3 = __msa_copy_u_w(res3, 0);
    SW4(out0, out1, out2, out3, dst, dst_stride);
}

static void intra_predict_ddl_8x8_msa(UWORD8 *src_ptr, UWORD8 *dst,
                                      WORD32 dst_stride)
{
    UWORD8 src_val = src_ptr[15];
    UWORD64 out0, out1, out2, out3;
    v16u8 src, vec4, vec5, res0;
    v8u16 vec0, vec1, vec2, vec3;
    v2i64 res1, res2, res3;

    src = LD_UB(src_ptr);

    vec4 = (v16u8)__msa_sldi_b((v16i8)src, (v16i8)src, 1);
    vec5 = (v16u8)__msa_sldi_b((v16i8)src, (v16i8)src, 2);
    vec5 = (v16u8)__msa_insert_b((v16i8)vec5, 14, src_val);
    ILVR_B2_UH(vec5, src, vec4, vec4, vec0, vec1);
    ILVL_B2_UH(vec5, src, vec4, vec4, vec2, vec3);
    HADD_UB4_UH(vec0, vec1, vec2, vec3, vec0, vec1, vec2, vec3);
    ADD2(vec0, vec1, vec2, vec3, vec0, vec2);
    SRARI_H2_UH(vec0, vec2, 2);

    res0 = (v16u8)__msa_pckev_b((v16i8)vec2, (v16i8)vec0);
    res1 = (v2i64)__msa_sldi_b((v16i8)res0, (v16i8)res0, 1);
    res2 = (v2i64)__msa_sldi_b((v16i8)res0, (v16i8)res0, 2);
    res3 = (v2i64)__msa_sldi_b((v16i8)res0, (v16i8)res0, 3);
    out0 = __msa_copy_u_d((v2i64)res0, 0);
    out1 = __msa_copy_u_d(res1, 0);
    out2 = __msa_copy_u_d(res2, 0);
    out3 = __msa_copy_u_d(res3, 0);
    SD4(out0, out1, out2, out3, dst, dst_stride);
    dst += (4 * dst_stride);

    res0 = (v16u8)__msa_sldi_b((v16i8)res0, (v16i8)res0, 4);
    res1 = (v2i64)__msa_sldi_b((v16i8)res0, (v16i8)res0, 1);
    res2 = (v2i64)__msa_sldi_b((v16i8)res0, (v16i8)res0, 2);
    res3 = (v2i64)__msa_sldi_b((v16i8)res0, (v16i8)res0, 3);
    out0 = __msa_copy_u_d((v2i64)res0, 0);
    out1 = __msa_copy_u_d(res1, 0);
    out2 = __msa_copy_u_d(res2, 0);
    out3 = __msa_copy_u_d(res3, 0);

    SD4(out0, out1, out2, out3, dst, dst_stride);
}

void ih264_intra_pred_luma_4x4_mode_vert_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                             WORD32 src_strd, WORD32 dst_strd,
                                             WORD32 ngbr_avail)
{
    UNUSED(src_strd);
    UNUSED(ngbr_avail);

    intra_predict_vert_4x4_msa(pu1_src + BLK_SIZE + 1, pu1_dst, dst_strd);
}

void ih264_intra_pred_luma_4x4_mode_horz_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                             WORD32 src_strd, WORD32 dst_strd,
                                             WORD32 ngbr_avail)
{
    UNUSED(src_strd);
    UNUSED(ngbr_avail);

    intra_predict_horiz_4x4_msa(pu1_src, pu1_dst + 3 * dst_strd, -dst_strd);
}

void ih264_intra_pred_luma_4x4_mode_dc_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                           WORD32 src_strd, WORD32 dst_strd,
                                           WORD32 ngbr_avail)
{
    WORD8 useleft; /* availability of left predictors (only for DC) */
    UWORD8 usetop; /* availability of top predictors (only for DC) */
    UNUSED(src_strd);

    useleft = BOOLEAN(ngbr_avail & LEFT_MB_AVAILABLE_MASK);
    usetop = BOOLEAN(ngbr_avail & TOP_MB_AVAILABLE_MASK);

    if(useleft & usetop)
    {
        intra_predict_dc_4x4_msa(pu1_src + BLK_SIZE + 1, pu1_src,
                                 pu1_dst, dst_strd);
    }
    else if(useleft)
    {
        intra_predict_dc_tl_4x4_msa(pu1_src, pu1_dst, dst_strd);
    }
    else if(usetop)
    {
        intra_predict_dc_tl_4x4_msa(pu1_src + BLK_SIZE + 1, pu1_dst, dst_strd);
    }
    else
    {
        intra_predict_128dc_4x4_msa(pu1_dst, dst_strd);
    }
}

void ih264_intra_pred_luma_4x4_mode_diag_dl_msa(UWORD8 *pu1_src,
                                                UWORD8 *pu1_dst,
                                                WORD32 src_strd,
                                                WORD32 dst_strd,
                                                WORD32 ngbr_avail)
{
    UNUSED(src_strd);
    UNUSED(ngbr_avail);

    intra_predict_ddl_4x4_msa(pu1_src + BLK_SIZE + 1, pu1_dst, dst_strd);
}

void ih264_intra_pred_luma_8x8_mode_vert_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                             WORD32 src_strd, WORD32 dst_strd,
                                             WORD32 ngbr_avail)
{
    UNUSED(src_strd);
    UNUSED(ngbr_avail);

    intra_predict_vert_8x8_msa(pu1_src + BLK8x8SIZE + 1, pu1_dst, dst_strd);
}

void ih264_intra_pred_luma_8x8_mode_horz_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                             WORD32 src_strd, WORD32 dst_strd,
                                             WORD32 ngbr_avail)
{
    UNUSED(src_strd);
    UNUSED(ngbr_avail);

    intra_predict_horiz_8x8_msa(pu1_src, pu1_dst + 7 * dst_strd, -dst_strd);
}

void ih264_intra_pred_luma_8x8_mode_dc_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                           WORD32 src_strd, WORD32 dst_strd,
                                           WORD32 ngbr_avail)
{
    WORD8 useleft; /* availability of left predictors (only for DC) */
    UWORD8 usetop; /* availability of top predictors (only for DC) */
    UNUSED(src_strd);

    useleft = BOOLEAN(ngbr_avail & LEFT_MB_AVAILABLE_MASK);
    usetop = BOOLEAN(ngbr_avail & TOP_MB_AVAILABLE_MASK);

    if(useleft & usetop)
    {
        intra_predict_dc_8x8_msa(pu1_src + BLK8x8SIZE + 1, pu1_src,
                                 pu1_dst, dst_strd);
    }
    else if(useleft)
    {
        intra_predict_dc_tl_8x8_msa(pu1_src, pu1_dst, dst_strd);
    }
    else if(usetop)
    {
        intra_predict_dc_tl_8x8_msa(pu1_src + BLK8x8SIZE + 1, pu1_dst,
                                    dst_strd);
    }
    else
    {
        intra_predict_128dc_8x8_msa(pu1_dst, dst_strd);
    }
}

void ih264_intra_pred_luma_8x8_mode_diag_dl_msa(UWORD8 *pu1_src,
                                                UWORD8 *pu1_dst,
                                                WORD32 src_strd,
                                                WORD32 dst_strd,
                                                WORD32 ngbr_avail)
{
    UNUSED(src_strd);
    UNUSED(ngbr_avail);

    intra_predict_ddl_8x8_msa(pu1_src + BLK8x8SIZE + 1, pu1_dst, dst_strd);
}

void ih264_intra_pred_luma_16x16_mode_vert_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                               WORD32 src_strd, WORD32 dst_strd,
                                               WORD32 ngbr_avail)
{
    UNUSED(src_strd);
    UNUSED(ngbr_avail);

    intra_predict_vert_16x16_msa(pu1_src + MB_SIZE + 1, pu1_dst, dst_strd);
}

void ih264_intra_pred_luma_16x16_mode_horz_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                               WORD32 src_strd, WORD32 dst_strd,
                                               WORD32 ngbr_avail)
{
    UNUSED(src_strd);
    UNUSED(ngbr_avail);

    intra_predict_horiz_16x16_msa(pu1_src, pu1_dst + 15 * dst_strd, -dst_strd);
}

void ih264_intra_pred_luma_16x16_mode_dc_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                             WORD32 src_strd, WORD32 dst_strd,
                                             WORD32 ngbr_avail)
{
    WORD8 useleft; /* availability of left predictors (only for DC) */
    UWORD8 usetop; /* availability of top predictors (only for DC) */
    UNUSED(src_strd);

    useleft = BOOLEAN(ngbr_avail & LEFT_MB_AVAILABLE_MASK);
    usetop = BOOLEAN(ngbr_avail & TOP_MB_AVAILABLE_MASK);

    if(useleft & usetop)
    {
        intra_predict_dc_16x16_msa(pu1_src + MB_SIZE + 1, pu1_src,
                                   pu1_dst, dst_strd);
    }
    else if(useleft)
    {
        intra_predict_dc_tl_16x16_msa(pu1_src, pu1_dst, dst_strd);
    }
    else if(usetop)
    {
        intra_predict_dc_tl_16x16_msa(pu1_src + MB_SIZE + 1, pu1_dst, dst_strd);
    }
    else
    {
        intra_predict_128dc_16x16_msa(pu1_dst, dst_strd);
    }
}
