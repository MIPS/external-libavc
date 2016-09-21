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
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
*/
/**
*******************************************************************************
* @file
*  imp2d_function_selector.c
*
* @brief
*  Contains functions to initialize function pointers used in hevc
*
* @author
*  Naveen
*
* @par List of Functions:
* @remarks
*  None
*
*******************************************************************************
*/
/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

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

void ih264d_init_function_ptr(dec_struct_t *ps_codec)
{
    ih264d_init_function_ptr_generic(ps_codec);

#if !defined(DISABLE_MSA)
    if(ARCH_MIPS_MSA == ps_codec->e_processor_arch)
    {
        ih264d_init_function_ptr_msa(ps_codec);
    }
#endif
}

void ih264d_init_arch(dec_struct_t *ps_codec)
{
#ifdef DEFAULT_ARCH
#if DEFAULT_ARCH == D_ARCH_MIPS_NOMSA
    ps_codec->e_processor_arch = ARCH_MIPS_GENERIC;
#elif DEFAULT_ARCH == D_ARCH_MIPS_MSA
    ps_codec->e_processor_arch = ARCH_MIPS_MSA;
#endif
#else
    ps_codec->e_processor_arch = ARCH_MIPS_MSA;
#endif
}
