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
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* User Include files */
#include "ih264_typedefs.h"
#include "iv.h"
#include "ivd.h"
#include "ih264_defs.h"
#include "ih264_size_defs.h"
#include "ih264_error.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264d_structs.h"
#include "ih264d_function_selector.h"

void ih264d_init_function_ptr_msa(dec_struct_t *ps_codec)
{
    /* luma intra 16x16 functions */
    ps_codec->apf_intra_pred_luma_16x16[0] = ih264_intra_pred_luma_16x16_mode_vert_msa;
    ps_codec->apf_intra_pred_luma_16x16[1] = ih264_intra_pred_luma_16x16_mode_horz_msa;
    ps_codec->apf_intra_pred_luma_16x16[2] = ih264_intra_pred_luma_16x16_mode_dc_msa;

    /* luma intra 4x4 functions */
    ps_codec->apf_intra_pred_luma_4x4[0] = ih264_intra_pred_luma_4x4_mode_vert_msa;
    ps_codec->apf_intra_pred_luma_4x4[1] = ih264_intra_pred_luma_4x4_mode_horz_msa;
    ps_codec->apf_intra_pred_luma_4x4[2] = ih264_intra_pred_luma_4x4_mode_dc_msa;
    ps_codec->apf_intra_pred_luma_4x4[3] = ih264_intra_pred_luma_4x4_mode_diag_dl_msa;

    /* luma intra 8x8 functions */
    ps_codec->apf_intra_pred_luma_8x8[0] = ih264_intra_pred_luma_8x8_mode_vert_msa;
    ps_codec->apf_intra_pred_luma_8x8[1] = ih264_intra_pred_luma_8x8_mode_horz_msa;
    ps_codec->apf_intra_pred_luma_8x8[2] = ih264_intra_pred_luma_8x8_mode_dc_msa;
    ps_codec->apf_intra_pred_luma_8x8[3] = ih264_intra_pred_luma_8x8_mode_diag_dl_msa;

    /* chroma intra 8x8 functions */
    ps_codec->apf_intra_pred_chroma[0] = ih264_intra_pred_chroma_8x8_mode_vert_msa;
    ps_codec->apf_intra_pred_chroma[1] = ih264_intra_pred_chroma_8x8_mode_horz_msa;

    ps_codec->pf_default_weighted_pred_luma = ih264_default_weighted_pred_luma_msa;
    ps_codec->pf_default_weighted_pred_chroma = ih264_default_weighted_pred_chroma_msa;
    ps_codec->pf_weighted_pred_luma = ih264_weighted_pred_luma_msa;
    ps_codec->pf_weighted_pred_chroma = ih264_weighted_pred_chroma_msa;
    ps_codec->pf_weighted_bi_pred_luma = ih264_weighted_bi_pred_luma_msa;
    ps_codec->pf_weighted_bi_pred_chroma = ih264_weighted_bi_pred_chroma_msa;

    /* Padding Functions */
    ps_codec->pf_pad_top = ih264_pad_top_msa;
    ps_codec->pf_pad_bottom = ih264_pad_bottom_msa;
    ps_codec->pf_pad_left_luma = ih264_pad_left_luma_msa;
    ps_codec->pf_pad_left_chroma = ih264_pad_left_chroma_msa;
    ps_codec->pf_pad_right_luma = ih264_pad_right_luma_msa;
    ps_codec->pf_pad_right_chroma = ih264_pad_right_chroma_msa;

    /* inverse transform functions */
    ps_codec->pf_iquant_itrans_recon_luma_4x4 = ih264_iquant_itrans_recon_4x4_msa;
    ps_codec->pf_iquant_itrans_recon_luma_4x4_dc = ih264_iquant_itrans_recon_4x4_dc_msa;
    ps_codec->pf_iquant_itrans_recon_luma_8x8 = ih264_iquant_itrans_recon_8x8_msa;
    ps_codec->pf_iquant_itrans_recon_luma_8x8_dc = ih264_iquant_itrans_recon_8x8_dc_msa;
    ps_codec->pf_iquant_itrans_recon_chroma_4x4 = ih264_iquant_itrans_recon_chroma_4x4_msa;
    ps_codec->pf_iquant_itrans_recon_chroma_4x4_dc = ih264_iquant_itrans_recon_chroma_4x4_dc_msa;
    ps_codec->pf_ihadamard_scaling_4x4 = ih264_ihadamard_scaling_4x4_msa;

    /* luma deblocking functions */
    ps_codec->pf_deblk_luma_vert_bs4 = ih264_deblk_luma_vert_bs4_msa;
    ps_codec->pf_deblk_luma_vert_bslt4 = ih264_deblk_luma_vert_bslt4_msa;
    ps_codec->pf_deblk_luma_vert_bs4_mbaff = ih264_deblk_luma_vert_bs4_mbaff_msa;
    ps_codec->pf_deblk_luma_vert_bslt4_mbaff = ih264_deblk_luma_vert_bslt4_mbaff_msa;
    ps_codec->pf_deblk_luma_horz_bs4 = ih264_deblk_luma_horz_bs4_msa;
    ps_codec->pf_deblk_luma_horz_bslt4 = ih264_deblk_luma_horz_bslt4_msa;

    /* chroma deblocking functions */
    ps_codec->pf_deblk_chroma_vert_bs4 = ih264_deblk_chroma_vert_bs4_msa;
    ps_codec->pf_deblk_chroma_vert_bslt4 = ih264_deblk_chroma_vert_bslt4_msa;
    ps_codec->pf_deblk_chroma_vert_bs4_mbaff = ih264_deblk_chroma_vert_bs4_mbaff_msa;
    ps_codec->pf_deblk_chroma_vert_bslt4_mbaff = ih264_deblk_chroma_vert_bslt4_mbaff_msa;
    ps_codec->pf_deblk_chroma_horz_bs4 = ih264_deblk_chroma_horz_bs4_msa;
    ps_codec->pf_deblk_chroma_horz_bslt4 = ih264_deblk_chroma_horz_bslt4_msa;

    /* luma inter pred functions */
    ps_codec->apf_inter_pred_luma[0] = ih264_inter_pred_luma_copy_msa;
    ps_codec->apf_inter_pred_luma[1] = ih264_inter_pred_luma_horz_qpel_msa;
    ps_codec->apf_inter_pred_luma[2] = ih264_inter_pred_luma_horz_msa;
    ps_codec->apf_inter_pred_luma[3] = ih264_inter_pred_luma_horz_qpel_msa;
    ps_codec->apf_inter_pred_luma[4] = ih264_inter_pred_luma_vert_qpel_msa;
    ps_codec->apf_inter_pred_luma[5] = ih264_inter_pred_luma_horz_qpel_vert_qpel_msa;
    ps_codec->apf_inter_pred_luma[6] = ih264_inter_pred_luma_horz_hpel_vert_qpel_msa;
    ps_codec->apf_inter_pred_luma[7] = ih264_inter_pred_luma_horz_qpel_vert_qpel_msa;
    ps_codec->apf_inter_pred_luma[8] = ih264_inter_pred_luma_vert_msa;
    ps_codec->apf_inter_pred_luma[9] = ih264_inter_pred_luma_horz_qpel_vert_hpel_msa;
    ps_codec->apf_inter_pred_luma[10] = ih264_inter_pred_luma_horz_hpel_vert_hpel_msa;
    ps_codec->apf_inter_pred_luma[11] = ih264_inter_pred_luma_horz_qpel_vert_hpel_msa;
    ps_codec->apf_inter_pred_luma[12] = ih264_inter_pred_luma_vert_qpel_msa;
    ps_codec->apf_inter_pred_luma[13] = ih264_inter_pred_luma_horz_qpel_vert_qpel_msa;
    ps_codec->apf_inter_pred_luma[14] = ih264_inter_pred_luma_horz_hpel_vert_qpel_msa;
    ps_codec->apf_inter_pred_luma[15] = ih264_inter_pred_luma_horz_qpel_vert_qpel_msa;

    /* chroma inter pred functions */
    ps_codec->pf_inter_pred_chroma = ih264_inter_pred_chroma_msa;

    return;
}
