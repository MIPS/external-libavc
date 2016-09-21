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
#include "ih264_padding.h"
#include "ih264_macros_msa.h"

static void pad_tb_msa(UWORD8 *src_ptr, UWORD8 *dst_ptr, WORD32 stride,
                       WORD32 width, WORD32 pad_size)
{
    UWORD8 *dst;
    WORD32 col;
    v16u8 out;

    if(40 == pad_size)
    {
        for(col = (width >> 4); col--;)
        {
            out = LD_UB(src_ptr);
            dst = dst_ptr;

            ST_UB8(out, out, out, out, out, out, out, out, dst, stride);
            dst += (8 * stride);
            ST_UB8(out, out, out, out, out, out, out, out, dst, stride);
            dst += (8 * stride);
            ST_UB8(out, out, out, out, out, out, out, out, dst, stride);
            dst += (8 * stride);
            ST_UB8(out, out, out, out, out, out, out, out, dst, stride);
            dst += (8 * stride);
            ST_UB8(out, out, out, out, out, out, out, out, dst, stride);

            src_ptr += 16;
            dst_ptr += 16;
        }
    }
    else if(20 == pad_size)
    {
        for(col = (width >> 4); col--;)
        {
            out = LD_UB(src_ptr);
            dst = dst_ptr;

            ST_UB8(out, out, out, out, out, out, out, out, dst, stride);
            dst += (8 * stride);
            ST_UB8(out, out, out, out, out, out, out, out, dst, stride);
            dst += (8 * stride);
            ST_UB4(out, out, out, out, dst, stride);

            src_ptr += 16;
            dst_ptr += 16;
        }
    }
    else if(16 == pad_size)
    {
        for(col = (width >> 4); col--;)
        {
            out = LD_UB(src_ptr);
            dst = dst_ptr;

            ST_UB8(out, out, out, out, out, out, out, out, dst, stride);
            dst += (8 * stride);
            ST_UB8(out, out, out, out, out, out, out, out, dst, stride);

            src_ptr += 16;
            dst_ptr += 16;
        }
    }
    else if(8 == pad_size)
    {
        for(col = (width >> 4); col--;)
        {
            out = LD_UB(src_ptr);

            ST_UB8(out, out, out, out, out, out, out, out, dst_ptr, stride);

            src_ptr += 16;
            dst_ptr += 16;
        }
    }
}

static void pad_lr_msa(UWORD8 *src_ptr, UWORD8 *dst_ptr, WORD32 stride,
                       WORD32 height, WORD32 pad_size)
{
    WORD32 row;
    UWORD8 in0, in1, in2, in3;
    v16u8 src0, src1, src2, src3;

    if(32 == pad_size)
    {
        for(row = (height >> 2); row--;)
        {
            in0 = *src_ptr;
            src_ptr += stride;
            in1 = *src_ptr;
            src_ptr += stride;
            in2 = *src_ptr;
            src_ptr += stride;
            in3 = *src_ptr;
            src_ptr += stride;

            src0 = (v16u8)__msa_fill_b(in0);
            src1 = (v16u8)__msa_fill_b(in1);
            src2 = (v16u8)__msa_fill_b(in2);
            src3 = (v16u8)__msa_fill_b(in3);

            ST_UB2(src0, src0, dst_ptr, 16);
            dst_ptr += stride;
            ST_UB2(src1, src1, dst_ptr, 16);
            dst_ptr += stride;
            ST_UB2(src2, src2, dst_ptr, 16);
            dst_ptr += stride;
            ST_UB2(src3, src3, dst_ptr, 16);
            dst_ptr += stride;
        }
    }
    else if(16 == pad_size)
    {
        for(row = (height >> 2); row--;)
        {
            in0 = *src_ptr;
            src_ptr += stride;
            in1 = *src_ptr;
            src_ptr += stride;
            in2 = *src_ptr;
            src_ptr += stride;
            in3 = *src_ptr;
            src_ptr += stride;

            src0 = (v16u8)__msa_fill_b(in0);
            src1 = (v16u8)__msa_fill_b(in1);
            src2 = (v16u8)__msa_fill_b(in2);
            src3 = (v16u8)__msa_fill_b(in3);

            ST_UB4(src0, src1, src2, src3, dst_ptr, stride);
            dst_ptr += (4 * stride);
        }
    }
}

static void pad_interleaved_lr_msa(UWORD8 *src_ptr, UWORD8 *dst_ptr,
                                   WORD32 stride, WORD32 height,
                                   WORD32 pad_size)
{
    WORD32 row;
    UWORD16 in0, in1, in2, in3;
    v16u8 src0, src1, src2, src3;

    if(32 == pad_size)
    {
        for(row = (height >> 2); row--;)
        {
            in0 = LH(src_ptr);
            src_ptr += stride;
            in1 = LH(src_ptr);
            src_ptr += stride;
            in2 = LH(src_ptr);
            src_ptr += stride;
            in3 = LH(src_ptr);
            src_ptr += stride;

            src0 = (v16u8)__msa_fill_h(in0);
            src1 = (v16u8)__msa_fill_h(in1);
            src2 = (v16u8)__msa_fill_h(in2);
            src3 = (v16u8)__msa_fill_h(in3);

            ST_UB2(src0, src0, dst_ptr, 16);
            dst_ptr += stride;
            ST_UB2(src1, src1, dst_ptr, 16);
            dst_ptr += stride;
            ST_UB2(src2, src2, dst_ptr, 16);
            dst_ptr += stride;
            ST_UB2(src3, src3, dst_ptr, 16);
            dst_ptr += stride;
        }
    }
    else if(16 == pad_size)
    {
        for(row = (height >> 2); row--;)
        {
            in0 = LH(src_ptr);
            src_ptr += stride;
            in1 = LH(src_ptr);
            src_ptr += stride;
            in2 = LH(src_ptr);
            src_ptr += stride;
            in3 = LH(src_ptr);
            src_ptr += stride;

            src0 = (v16u8)__msa_fill_h(in0);
            src1 = (v16u8)__msa_fill_h(in1);
            src2 = (v16u8)__msa_fill_h(in2);
            src3 = (v16u8)__msa_fill_h(in3);

            ST_UB4(src0, src1, src2, src3, dst_ptr, stride);
            dst_ptr += (4 * stride);
        }
    }
}

void ih264_pad_top_msa(UWORD8 *src, WORD32 src_stride, WORD32 width,
                       WORD32 pad_size)
{
    pad_tb_msa(src, src - pad_size * src_stride, src_stride, width, pad_size);
}

void ih264_pad_bottom_msa(UWORD8 *src, WORD32 src_stride, WORD32 width,
                          WORD32 pad_size)
{
    pad_tb_msa(src - src_stride, src, src_stride, width, pad_size);
}

void ih264_pad_left_luma_msa(UWORD8 *src, WORD32 src_stride, WORD32 height,
                             WORD32 pad_size)
{
    pad_lr_msa(src, src - pad_size, src_stride, height, pad_size);
}

void ih264_pad_left_chroma_msa(UWORD8 *src, WORD32 src_stride, WORD32 height,
                               WORD32 pad_size)
{
    pad_interleaved_lr_msa(src, src - pad_size, src_stride, height, pad_size);
}

void ih264_pad_right_luma_msa(UWORD8 *src, WORD32 src_stride, WORD32 height,
                              WORD32 pad_size)
{
    pad_lr_msa(src - 1, src, src_stride, height, pad_size);
}

void ih264_pad_right_chroma_msa(UWORD8 *src, WORD32 src_stride, WORD32 height,
                                WORD32 pad_size)
{
    pad_interleaved_lr_msa(src - 2, src, src_stride, height, pad_size);
}
