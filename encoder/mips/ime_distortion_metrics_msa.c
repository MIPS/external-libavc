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

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

#include "ih264_macros_msa.h"

static UWORD32 sad_16width_msa(UWORD8 *src, WORD32 src_stride, UWORD8 *ref,
                               WORD32 ref_stride, WORD32 height)
{
    WORD32 ht_cnt;
    v16u8 src0, src1, ref0, ref1;
    v8u16 sad = { 0 };

    for(ht_cnt = (height >> 2); ht_cnt--;)
    {
        LD_UB2(src, src_stride, src0, src1);
        src += (2 * src_stride);
        LD_UB2(ref, ref_stride, ref0, ref1);
        ref += (2 * ref_stride);
        sad += SAD_UB2_UH(src0, src1, ref0, ref1);

        LD_UB2(src, src_stride, src0, src1);
        src += (2 * src_stride);
        LD_UB2(ref, ref_stride, ref0, ref1);
        ref += (2 * ref_stride);
        sad += SAD_UB2_UH(src0, src1, ref0, ref1);
    }

    return HADD_UH_U32(sad);
}

static void sad_16width_x3d_msa(UWORD8 *src_ptr, WORD32 src_stride,
                                UWORD8 *ref0_ptr, UWORD8 *ref1_ptr,
                                UWORD8 *ref2_ptr, WORD32 ref_stride,
                                WORD32 height, WORD32 *sad_array)
{
    WORD32 ht_cnt;
    v16u8 src, ref, diff;
    v8u16 sad0 = { 0 };
    v8u16 sad1 = { 0 };
    v8u16 sad2 = { 0 };

    for(ht_cnt = (height >> 1); ht_cnt--;)
    {
        src = LD_UB(src_ptr);
        src_ptr += src_stride;

        ref = LD_UB(ref0_ptr);
        ref0_ptr += ref_stride;
        diff = __msa_asub_u_b(src, ref);
        sad0 += __msa_hadd_u_h(diff, diff);

        ref = LD_UB(ref1_ptr);
        ref1_ptr += ref_stride;
        diff = __msa_asub_u_b(src, ref);
        sad1 += __msa_hadd_u_h(diff, diff);

        ref = LD_UB(ref2_ptr);
        ref2_ptr += ref_stride;
        diff = __msa_asub_u_b(src, ref);
        sad2 += __msa_hadd_u_h(diff, diff);

        src = LD_UB(src_ptr);
        src_ptr += src_stride;

        ref = LD_UB(ref0_ptr);
        ref0_ptr += ref_stride;
        diff = __msa_asub_u_b(src, ref);
        sad0 += __msa_hadd_u_h(diff, diff);

        ref = LD_UB(ref1_ptr);
        ref1_ptr += ref_stride;
        diff = __msa_asub_u_b(src, ref);
        sad1 += __msa_hadd_u_h(diff, diff);

        ref = LD_UB(ref2_ptr);
        ref2_ptr += ref_stride;
        diff = __msa_asub_u_b(src, ref);
        sad2 += __msa_hadd_u_h(diff, diff);
    }

    sad_array[0] = HADD_UH_U32(sad0);
    sad_array[1] = HADD_UH_U32(sad1);
    sad_array[2] = HADD_UH_U32(sad2);
}

static void sad_16width_x4d_msa(UWORD8 *src_ptr, WORD32 src_stride,
                                UWORD8 *aref_ptr[], WORD32 ref_stride,
                                WORD32 height, WORD32 *sad_array)
{
    WORD32 ht_cnt;
    UWORD8 *ref0_ptr, *ref1_ptr, *ref2_ptr, *ref3_ptr;
    v16u8 src, ref0, ref1, ref2, ref3, diff;
    v8u16 sad0 = { 0 };
    v8u16 sad1 = { 0 };
    v8u16 sad2 = { 0 };
    v8u16 sad3 = { 0 };

    ref0_ptr = aref_ptr[0];
    ref1_ptr = aref_ptr[1];
    ref2_ptr = aref_ptr[2];
    ref3_ptr = aref_ptr[3];

    for(ht_cnt = (height >> 1); ht_cnt--;)
    {
        src = LD_UB(src_ptr);
        src_ptr += src_stride;
        ref0 = LD_UB(ref0_ptr);
        ref0_ptr += ref_stride;
        ref1 = LD_UB(ref1_ptr);
        ref1_ptr += ref_stride;
        ref2 = LD_UB(ref2_ptr);
        ref2_ptr += ref_stride;
        ref3 = LD_UB(ref3_ptr);
        ref3_ptr += ref_stride;

        diff = __msa_asub_u_b(src, ref0);
        sad0 += __msa_hadd_u_h(diff, diff);
        diff = __msa_asub_u_b(src, ref1);
        sad1 += __msa_hadd_u_h(diff, diff);
        diff = __msa_asub_u_b(src, ref2);
        sad2 += __msa_hadd_u_h(diff, diff);
        diff = __msa_asub_u_b(src, ref3);
        sad3 += __msa_hadd_u_h(diff, diff);

        src = LD_UB(src_ptr);
        src_ptr += src_stride;
        ref0 = LD_UB(ref0_ptr);
        ref0_ptr += ref_stride;
        ref1 = LD_UB(ref1_ptr);
        ref1_ptr += ref_stride;
        ref2 = LD_UB(ref2_ptr);
        ref2_ptr += ref_stride;
        ref3 = LD_UB(ref3_ptr);
        ref3_ptr += ref_stride;

        diff = __msa_asub_u_b(src, ref0);
        sad0 += __msa_hadd_u_h(diff, diff);
        diff = __msa_asub_u_b(src, ref1);
        sad1 += __msa_hadd_u_h(diff, diff);
        diff = __msa_asub_u_b(src, ref2);
        sad2 += __msa_hadd_u_h(diff, diff);
        diff = __msa_asub_u_b(src, ref3);
        sad3 += __msa_hadd_u_h(diff, diff);
    }

    sad_array[0] = HADD_UH_U32(sad0);
    sad_array[1] = HADD_UH_U32(sad1);
    sad_array[2] = HADD_UH_U32(sad2);
    sad_array[3] = HADD_UH_U32(sad3);
}

static void avc_compute_satqd_16x16_lumainter_msa(UWORD8 *src,
                                                  WORD32 src_stride,
                                                  UWORD8 *pred,
                                                  WORD32 pred_stride,
                                                  WORD32 *mb_distortion,
                                                  UWORD32 *is_non_zero,
                                                  UWORD16 *thrsh)
{
    UWORD32 itr, flag = 0;
    WORD16 s1, s2, s3, s4, sad_1, sad_2, sad_3;
    WORD16 ls1, ls2, ls3, ls4, ls5, ls6, ls7, ls8;
    v8i16 zero = { 0 };
    v16u8 src0, src1, src2, src3, pred0, pred1, pred2, pred3;
    v8i16 diff0, diff1, diff2, diff3, diff4, diff5, diff6, diff7, sum0;
    v4i32 temp0, temp1, sum_w;
    v2i64 sum_d;

    *mb_distortion = 0;
    for(itr = 4; itr--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        LD_UB4(pred, pred_stride, pred0, pred1, pred2, pred3);

        ILVR_B4_SH(src0, pred0, src1, pred1, src2, pred2, src3, pred3,
                   diff0, diff1, diff2, diff3);
        ILVL_B4_SH(src0, pred0, src1, pred1, src2, pred2, src3, pred3,
                   diff4, diff5, diff6, diff7);

        HSUB_UB4_SH(diff0, diff1, diff2, diff3, diff0, diff1, diff2, diff3);

        diff0 = __msa_add_a_h(diff0, zero);
        diff1 = __msa_add_a_h(diff1, zero);
        diff2 = __msa_add_a_h(diff2, zero);
        diff3 = __msa_add_a_h(diff3, zero);

        diff0 += diff3;
        diff1 += diff2;

        diff0 = __msa_shf_h(diff0, 156);
        diff1 = __msa_shf_h(diff1, 156);

        sum0 = diff0 + diff1;
        sum_w = __msa_hadd_s_w(sum0, sum0);
        sum_d = __msa_hadd_s_d(sum_w, sum_w);

        sad_1 = (WORD32)__msa_copy_s_d(sum_d, 0);
        *mb_distortion += sad_1;
        sad_3 = (WORD32)__msa_copy_s_d(sum_d, 1);
        *mb_distortion += sad_3;

        if(0 == flag)
        {
            temp0 = __msa_hadd_s_w(diff0, diff0);
            temp1 = __msa_hadd_s_w(diff1, diff1);

            s1 = __msa_copy_s_w(temp0, 0);
            s4 = __msa_copy_s_w(temp0, 1);
            s2 = __msa_copy_s_w(temp1, 0);
            s3 = __msa_copy_s_w(temp1, 1);

            sad_2 = sad_1 << 1;

            ls1 = sad_2 - (s2 + s3);
            ls2 = sad_2 - (s1 + s4);
            ls3 = sad_2 - (s3 + s4);
            ls4 = sad_2 - (s3 - (s1 << 1));
            ls5 = sad_2 - (s4 - (s2 << 1));
            ls6 = sad_2 - (s1 + s2);
            ls7 = sad_2 - (s2 - (s4 << 1));
            ls8 = sad_2 - (s1 - (s3 << 1));

            if(thrsh[8] <= sad_1 || thrsh[0] <= ls2 || thrsh[1] <= ls1 ||
               thrsh[2] <= ls8 || thrsh[3] <= ls5 || thrsh[4] <= ls6 ||
               thrsh[5] <= ls3 || thrsh[6] <= ls7 || thrsh[7] <= ls4)
            {
                flag = 1;
            }
            else
            {
                s1 = __msa_copy_s_w(temp0, 2);
                s4 = __msa_copy_s_w(temp0, 3);
                s2 = __msa_copy_s_w(temp1, 2);
                s3 = __msa_copy_s_w(temp1, 3);

                sad_2 = sad_3 << 1;

                ls1 = sad_2 - (s2 + s3);
                ls2 = sad_2 - (s1 + s4);
                ls3 = sad_2 - (s3 + s4);
                ls4 = sad_2 - (s3 - (s1 << 1));
                ls5 = sad_2 - (s4 - (s2 << 1));
                ls6 = sad_2 - (s1 + s2);
                ls7 = sad_2 - (s2 - (s4 << 1));
                ls8 = sad_2 - (s1 - (s3 << 1));

                if(thrsh[8] <= sad_3 || thrsh[0] <= ls2 || thrsh[1] <= ls1 ||
                   thrsh[2] <= ls8 || thrsh[3] <= ls5 || thrsh[4] <= ls6 ||
                   thrsh[5] <= ls3 || thrsh[6] <= ls7 || thrsh[7] <= ls4)
                {
                    flag = 1;
                }
            }
        }

        HSUB_UB4_SH(diff4, diff5, diff6, diff7, diff0, diff1, diff2, diff3);

        diff0 = __msa_add_a_h(diff0, zero);
        diff1 = __msa_add_a_h(diff1, zero);
        diff2 = __msa_add_a_h(diff2, zero);
        diff3 = __msa_add_a_h(diff3, zero);

        diff0 += diff3;
        diff1 += diff2;

        diff0 = __msa_shf_h(diff0, 156);
        diff1 = __msa_shf_h(diff1, 156);

        sum0 = diff0 + diff1;
        sum_w = __msa_hadd_s_w(sum0, sum0);
        sum_d = __msa_hadd_s_d(sum_w, sum_w);

        sad_1 = (WORD32)__msa_copy_s_d(sum_d, 0);
        *mb_distortion += sad_1;
        sad_3 = (WORD32)__msa_copy_s_d(sum_d, 1);
        *mb_distortion += sad_3;

        if(0 == flag)
        {
            temp0 = __msa_hadd_s_w(diff0, diff0);
            temp1 = __msa_hadd_s_w(diff1, diff1);

            s1 = __msa_copy_s_w(temp0, 0);
            s4 = __msa_copy_s_w(temp0, 1);
            s2 = __msa_copy_s_w(temp1, 0);
            s3 = __msa_copy_s_w(temp1, 1);

            sad_2 = sad_1 << 1;

            ls1 = sad_2 - (s2 + s3);
            ls2 = sad_2 - (s1 + s4);
            ls3 = sad_2 - (s3 + s4);
            ls4 = sad_2 - (s3 - (s1 << 1));
            ls5 = sad_2 - (s4 - (s2 << 1));
            ls6 = sad_2 - (s1 + s2);
            ls7 = sad_2 - (s2 - (s4 << 1));
            ls8 = sad_2 - (s1 - (s3 << 1));

            if(thrsh[8] <= sad_1 || thrsh[0] <= ls2 || thrsh[1] <= ls1 ||
               thrsh[2] <= ls8 || thrsh[3] <= ls5 || thrsh[4] <= ls6 ||
               thrsh[5] <= ls3 || thrsh[6] <= ls7 || thrsh[7] <= ls4)
            {
                flag = 1;
            }
            else
            {
                s1 = __msa_copy_s_w(temp0, 2);
                s4 = __msa_copy_s_w(temp0, 3);
                s2 = __msa_copy_s_w(temp1, 2);
                s3 = __msa_copy_s_w(temp1, 3);

                sad_2 = sad_3 << 1;

                ls1 = sad_2 - (s2 + s3);
                ls2 = sad_2 - (s1 + s4);
                ls3 = sad_2 - (s3 + s4);
                ls4 = sad_2 - (s3 - (s1 << 1));
                ls5 = sad_2 - (s4 - (s2 << 1));
                ls6 = sad_2 - (s1 + s2);
                ls7 = sad_2 - (s2 - (s4 << 1));
                ls8 = sad_2 - (s1 - (s3 << 1));
                if(thrsh[8] <= sad_3 || thrsh[0] <= ls2 || thrsh[1] <= ls1 ||
                   thrsh[2] <= ls8 || thrsh[3] <= ls5 || thrsh[4] <= ls6 ||
                   thrsh[5] <= ls3 || thrsh[6] <= ls7 || thrsh[7] <= ls4)
                {
                    flag = 1;
                }
            }
        }

        src += (4 * src_stride);
        pred += (4 * pred_stride);
    }

    *is_non_zero = flag;
}

static UWORD32 avc_sad_16width_msa(UWORD8 *src, WORD32 src_stride,
                                   UWORD8 *ref, WORD32 ref_stride,
                                   WORD32 height, WORD32 max_sad)
{
    WORD32 ht_cnt;
    WORD32 sad_ret = 0;
    v16u8 src0, src1, src2, src3, ref0, ref1, ref2, ref3;
    v8u16 sad0 = { 0 };
    v8u16 sad1 = { 0 };
    v8u16 sad2 = { 0 };
    v8u16 sad3 = { 0 };
    v16u8 diff0, diff1, diff2, diff3;

    for(ht_cnt = (height >> 2); ht_cnt--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);
        LD_UB4(ref, ref_stride, ref0, ref1, ref2, ref3);
        ref += (4 * ref_stride);

        diff0 = __msa_asub_u_b(src0, ref0);
        sad0 = __msa_hadd_u_h(diff0, diff0);
        sad_ret += HADD_UH_U32(sad0);

        if(max_sad < sad_ret)
        {
            break;
        }

        diff1 = __msa_asub_u_b(src1, ref1);
        sad1 = __msa_hadd_u_h(diff1, diff1);
        sad_ret += HADD_UH_U32(sad1);

        if(max_sad < sad_ret)
        {
            break;
        }

        diff2 = __msa_asub_u_b(src2, ref2);
        sad2 = __msa_hadd_u_h(diff2, diff2);
        sad_ret += HADD_UH_U32(sad2);

        if(max_sad < sad_ret)
        {
            break;
        }

        diff3 = __msa_asub_u_b(src3, ref3);
        sad3 = __msa_hadd_u_h(diff3, diff3);
        sad_ret += HADD_UH_U32(sad3);

        if(max_sad < sad_ret)
        {
            break;
        }
    }

    return sad_ret;
}

void ime_compute_sad_16x16_msa(UWORD8 *src, UWORD8 *ref, WORD32 src_strd,
                               WORD32 ref_strd, WORD32 max_sad,
                               WORD32 *pmb_distortion)
{
    *pmb_distortion = (WORD32)avc_sad_16width_msa(src, src_strd, ref, ref_strd,
                                                  16, max_sad);
}

void ime_compute_sad_16x16_fast_msa(UWORD8 *src, UWORD8 *ref, WORD32 src_strd,
                                    WORD32 ref_strd, WORD32 max_sad,
                                    WORD32 *pmb_distortion)
{
    WORD32 sad;

    sad = (WORD32)avc_sad_16width_msa(src, src_strd, ref, ref_strd, 16,
                                      max_sad);

    *pmb_distortion = (sad << 1);

    return;
}

void ime_compute_sad_16x8_msa(UWORD8 *src, UWORD8 *ref, WORD32 src_strd,
                              WORD32 ref_strd, WORD32 max_sad,
                              WORD32 *pmb_distortion)
{
    *pmb_distortion = (WORD32)avc_sad_16width_msa(src, src_strd, ref, ref_strd,
                                                  8, max_sad);
}

void ime_calculate_sad4_prog_msa(UWORD8 *ref, UWORD8 *src, WORD32 ref_strd,
                                 WORD32 src_strd, WORD32 *psad)
{
    UWORD8 *aref_ptr[4];

    aref_ptr[0] = ref - 1;
    aref_ptr[1] = ref + 1;
    aref_ptr[2] = ref - ref_strd;
    aref_ptr[3] = ref + ref_strd;

    sad_16width_x4d_msa(src, src_strd, aref_ptr, ref_strd, 16, psad);
}

void ime_calculate_sad3_prog_msa(UWORD8 *ref1, UWORD8 *ref2, UWORD8 *ref3,
                                 UWORD8 *src, WORD32 ref_strd, WORD32 src_strd,
                                 WORD32 *psad)
{
    sad_16width_x3d_msa(src, src_strd, ref1, ref2, ref3, ref_strd, 16, psad);
}

void ime_calculate_sad2_prog_msa(UWORD8 *ref1, UWORD8 *ref2, UWORD8 *src,
                                 WORD32 ref_strd, WORD32 src_strd, WORD32 *psad)
{
    psad[0] = sad_16width_msa(src, src_strd, ref1, ref_strd, 16);
    psad[1] = sad_16width_msa(src, src_strd, ref2, ref_strd, 16);
}

void ime_sub_pel_compute_sad_16x16_msa(UWORD8 *src, UWORD8 *ref_half_x,
                                       UWORD8 *ref_half_y, UWORD8 *ref_half_xy,
                                       WORD32 src_strd, WORD32 ref_strd,
                                       WORD32 *psad)
{
    UWORD8 *aref_ptr[4];

    aref_ptr[0] = ref_half_x;
    aref_ptr[1] = ref_half_x - 1;
    aref_ptr[2] = ref_half_y;
    aref_ptr[3] = ref_half_y - ref_strd;

    sad_16width_x4d_msa(src, src_strd, aref_ptr, ref_strd, 16, psad);

    aref_ptr[0] = ref_half_xy;
    aref_ptr[1] = ref_half_xy - 1;
    aref_ptr[2] = ref_half_xy - ref_strd;
    aref_ptr[3] = ref_half_xy - ref_strd - 1;

    sad_16width_x4d_msa(src, src_strd, aref_ptr, ref_strd, 16, psad + 4);
}

void ime_compute_satqd_16x16_lumainter_msa(UWORD8 *src, UWORD8 *est,
                                           WORD32 src_strd, WORD32 est_strd,
                                           UWORD16 *thrsh,
                                           WORD32 *mb_distortion,
                                           UWORD32 *is_zero)
{
    avc_compute_satqd_16x16_lumainter_msa(src, src_strd, est, est_strd,
                                          mb_distortion, is_zero, thrsh);
}
