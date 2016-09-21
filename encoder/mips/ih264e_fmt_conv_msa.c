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

/* User Include files */
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_defs.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "ih264_error.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ih264e_defs.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_fmt_conv.h"
#include "ih264_macros_msa.h"

static void plane_copy_interleave_msa(UWORD8 *src_ptr0, WORD32 src0_stride,
                                      UWORD8 *src_ptr1, WORD32 src1_stride,
                                      UWORD8 *dst, WORD32 dst_stride,
                                      WORD32 width, WORD32 height)
{
    WORD32 loop_width, loop_height, w_mul8, h;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16u8 vec_ilv_r0, vec_ilv_r1, vec_ilv_r2, vec_ilv_r3;
    v16u8 vec_ilv_l0, vec_ilv_l1, vec_ilv_l2, vec_ilv_l3;

    w_mul8 = width - width % 8;
    h = height - height % 4;

    for(loop_height = (h >> 2); loop_height--;)
    {
        for(loop_width = (width >> 4); loop_width--;)
        {
            LD_UB4(src_ptr0, src0_stride, src0, src1, src2, src3);
            LD_UB4(src_ptr1, src1_stride, src4, src5, src6, src7);
            ILVR_B4_UB(src4, src0, src5, src1, src6, src2, src7, src3,
                       vec_ilv_r0, vec_ilv_r1, vec_ilv_r2, vec_ilv_r3);
            ILVL_B4_UB(src4, src0, src5, src1, src6, src2, src7, src3,
                       vec_ilv_l0, vec_ilv_l1, vec_ilv_l2, vec_ilv_l3);
            ST_UB4(vec_ilv_r0, vec_ilv_r1, vec_ilv_r2, vec_ilv_r3,
                   dst, dst_stride);
            ST_UB4(vec_ilv_l0, vec_ilv_l1, vec_ilv_l2, vec_ilv_l3,
                   (dst + 16), dst_stride);
            src_ptr0 += 16;
            src_ptr1 += 16;
            dst += 32;
        }

        for(loop_width = (width % 16) >> 3; loop_width--;)
        {
            LD_UB4(src_ptr0, src0_stride, src0, src1, src2, src3);
            LD_UB4(src_ptr1, src1_stride, src4, src5, src6, src7);
            ILVR_B4_UB(src4, src0, src5, src1, src6, src2, src7, src3,
                       vec_ilv_r0, vec_ilv_r1, vec_ilv_r2, vec_ilv_r3);
            ST_UB4(vec_ilv_r0, vec_ilv_r1, vec_ilv_r2, vec_ilv_r3,
                   dst, dst_stride);
            src_ptr0 += 8;
            src_ptr1 += 8;
            dst += 16;
        }

        for(loop_width = w_mul8; loop_width < width; loop_width++)
        {
            dst[0] = src_ptr0[0];
            dst[1] = src_ptr1[0];
            dst[dst_stride] = src_ptr0[src0_stride];
            dst[dst_stride + 1] = src_ptr1[src1_stride];
            dst[2 * dst_stride] = src_ptr0[2 * src0_stride];
            dst[2 * dst_stride + 1] = src_ptr1[2 * src1_stride];
            dst[3 * dst_stride] = src_ptr0[3 * src0_stride];
            dst[3 * dst_stride + 1] = src_ptr1[3 * src1_stride];
            src_ptr0 += 1;
            src_ptr1 += 1;
            dst += 2;
        }

        src_ptr0 += ((4 * src0_stride) - width);
        src_ptr1 += ((4 * src1_stride) - width);
        dst += ((4 * dst_stride) - (width * 2));
    }

    for(loop_height = h; loop_height < height; loop_height++)
    {
        for(loop_width = (width >> 4); loop_width--;)
        {
            src0 = LD_UB(src_ptr0);
            src4 = LD_UB(src_ptr1);
            ILVRL_B2_UB(src4, src0, vec_ilv_r0, vec_ilv_l0);
            ST_UB2(vec_ilv_r0, vec_ilv_l0, dst, 16);
            src_ptr0 += 16;
            src_ptr1 += 16;
            dst += 32;
        }

        for(loop_width = (width % 16) >> 3; loop_width--;)
        {
            src0 = LD_UB(src_ptr0);
            src4 = LD_UB(src_ptr1);
            vec_ilv_r0 = (v16u8)__msa_ilvr_b((v16i8)src4, (v16i8)src0);
            ST_UB(vec_ilv_r0, dst);
            src_ptr0 += 8;
            src_ptr1 += 8;
            dst += 16;
        }

        for(loop_width = w_mul8; loop_width < width; loop_width++)
        {
            dst[0] = src_ptr0[0];
            dst[1] = src_ptr1[0];
            src_ptr0 += 1;
            src_ptr1 += 1;
            dst += 2;
        }

        src_ptr0 += (src0_stride - width);
        src_ptr1 += (src1_stride - width);
        dst += (dst_stride - (width * 2));
    }
}

static void format_conv_422i_to_420sp_msa(UWORD8 *src_ptr, WORD32 src_stride,
                                          UWORD8 *y_buf_ptr, WORD32 y_stride,
                                          UWORD8 *u_buf_ptr, WORD32 u_stride,
                                          UWORD8 *v_buf_ptr, WORD32 v_stride,
                                          WORD32 width, WORD32 height)
{
    UWORD8 *src, *y_buf, *u_buf, *v_buf;
    UWORD8 cb_even0, cr_even0, cb_even1, cr_even1;
    WORD32 row, col, w_mod, val;
    UWORD64 dst2, dst3, dst4, dst5;
    v16u8 mask, u_out0, v_out0, u_out1, v_out1;
    v16u8 src0, src1, src2, src3, temp0, temp1, temp2, temp3;

    mask = (v16u8)__msa_fill_h(255);
    w_mod = width - (width % 16);

    for(row = (height >> 2); row--;)
    {
        src = src_ptr;
        y_buf = y_buf_ptr;
        u_buf = u_buf_ptr;
        v_buf = v_buf_ptr;

        for(col = 0; col < (width << 1); col = col + 16)
        {
            LD_UB4(src, (2 * src_stride), src0, src1, src2, src3);

            temp0 = (v16u8)__msa_pckev_b((v16i8)src0, (v16i8)src0);
            temp1 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
            temp2 = (v16u8)__msa_pckev_b((v16i8)src2, (v16i8)src2);
            temp3 = (v16u8)__msa_pckod_b((v16i8)src3, (v16i8)src2);

            LD_UB2(u_buf, u_stride, u_out0, u_out1);
            LD_UB2(v_buf, v_stride, v_out0, v_out1);

            ST8x4_UB(temp1, temp3, y_buf, y_stride);

            u_out0 = __msa_bmnz_v(u_out0, temp0, mask);
            v_out0 =(v16u8)__msa_ilvod_b((v16i8)v_out0, (v16i8)temp0);

            u_out1 = __msa_bmnz_v(u_out1, temp2, mask);
            v_out1 =(v16u8)__msa_ilvod_b((v16i8)v_out1, (v16i8)temp2);

            dst2 =__msa_copy_u_d((v2i64)u_out0, 0);
            dst3 =__msa_copy_u_d((v2i64)v_out0, 0);
            dst4 =__msa_copy_u_d((v2i64)u_out1, 0);
            dst5 =__msa_copy_u_d((v2i64)v_out1, 0);

            SD(dst2, u_buf);
            SD(dst3, v_buf);
            SD(dst4, u_buf + u_stride);
            SD(dst5, v_buf + v_stride);

            src += 16;
            y_buf += 8;
            u_buf += 8;
            v_buf += 8;
        }

        for(col = 2 * w_mod; col < (width << 1); col = col + 4)
        {
           cb_even0 = src[col];
           cr_even0 = src[col + 2];
           cb_even1 = src[col + (2 * src_stride)];
           cr_even1 = src[col + 2 + (2 * src_stride)];

           val = col >> 1;

           u_buf[val] = cb_even0;
           v_buf[val] = cr_even0;
           u_buf[val + u_stride] = cb_even1;
           v_buf[val + v_stride] = cr_even1;

           y_buf[val] = src[col + 1];
           y_buf[val + 1] = src[col + 3];
           y_buf[val + (2 * y_stride)] = src[col + 1 + (4 * src_stride)];
           y_buf[val + 1 + (2 * y_stride)] = src[col + 3 + (4 * src_stride)];

           y_buf[val + y_stride] = src[col + 1 + (2 * src_stride)];
           y_buf[val + 1 + y_stride] = src[col + 3 + (2 * src_stride)];
           y_buf[val + 3 * y_stride] = src[col + 1 + (6 * src_stride)];
           y_buf[val + 1 + 3 * y_stride] = src[col + 3 + (6 * src_stride)];

            src += 4;
            y_buf += 2;
            u_buf += 2;
            v_buf += 2;
        }

        src_ptr += (8 * src_stride);
        y_buf_ptr += (4 * y_stride);
        u_buf_ptr += (2 * u_stride);
        v_buf_ptr += (2 * v_stride);
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

void ih264e_fmt_conv_420p_to_420sp_msa(UWORD8 *src_y, UWORD8 *src_u,
                                       UWORD8 *src_v, UWORD8 *dst_y,
                                       UWORD8 *dst_uv, UWORD16 height,
                                       UWORD16 width, UWORD16 stridey,
                                       UWORD16 strideu, UWORD16 stridev,
                                       UWORD16 dst_stride_y,
                                       UWORD16 dst_stride_uv,
                                       UWORD32 convert_uv_only)

{
    UWORD32 width_uv;

    if(0 == convert_uv_only)
    {
        copy_16multx8mult_msa(src_y, stridey, dst_y, dst_stride_y,
                              height, width);
    }

    height = (height + 1) >> 1;
    width_uv = (width + 1) >> 1;
    plane_copy_interleave_msa(src_u, strideu, src_v, stridev,
                              dst_uv, dst_stride_uv, width_uv,
                              height);
}

void ih264e_fmt_conv_422i_to_420sp_msa(UWORD8 *y_buf, UWORD8 *u_buf,
                                       UWORD8 *v_buf, UWORD8 *p422i_buf,
                                       WORD32 y_width, WORD32 y_height,
                                       WORD32 y_stride, WORD32 u_stride,
                                       WORD32 v_stride, WORD32 i422i_stride)
{
    format_conv_422i_to_420sp_msa(p422i_buf, i422i_stride, y_buf, y_stride,
                                  u_buf, u_stride, v_buf, v_stride,
                                  y_width, y_height);
}
