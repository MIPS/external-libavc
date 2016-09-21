libavce_inc_dir_mips    +=  $(LOCAL_PATH)/common/mips
libavce_inc_dir_mips    +=  $(LOCAL_PATH)/encoder/mips

libavce_srcs_c_mips     +=  encoder/mips/ih264e_function_selector.c

ifeq ($(ARCH_MIPS_HAS_MSA),true)
libavce_cflags_mips     += -mfp64 -mmsa
libavce_cflags_mips     += -DDEFAULT_ARCH=D_ARCH_MIPS_MSA

libavce_srcs_c_mips     +=  encoder/mips/ih264e_function_selector_msa.c
libavce_srcs_c_mips     +=  encoder/mips/ih264e_half_pel_msa.c
libavce_srcs_c_mips     +=  encoder/mips/ime_distortion_metrics_msa.c
libavce_srcs_c_mips     +=  encoder/mips/ih264e_intra_modes_eval_msa.c
libavce_srcs_c_mips     +=  encoder/mips/ih264e_fmt_conv_msa.c

libavce_srcs_c_mips     +=  common/mips/ih264_inter_pred_luma_msa.c
libavce_srcs_c_mips     +=  common/mips/ih264_inter_pred_chroma_msa.c
libavce_srcs_c_mips     +=  common/mips/ih264_deblk_luma_msa.c
libavce_srcs_c_mips     +=  common/mips/ih264_deblk_chroma_msa.c
libavce_srcs_c_mips     +=  common/mips/ih264_luma_intra_pred_msa.c
libavce_srcs_c_mips     +=  common/mips/ih264_chroma_intra_pred_msa.c
libavce_srcs_c_mips     +=  common/mips/ih264_ihadamard_scaling_msa.c
libavce_srcs_c_mips     +=  common/mips/ih264_iquant_itrans_recon_msa.c
libavce_srcs_c_mips     +=  common/mips/ih264_resi_trans_quant_msa.c
libavce_srcs_c_mips     +=  common/mips/ih264_padding_msa.c
libavce_srcs_c_mips     +=  common/mips/ih264_bilinear_pred_luma_msa.c
else
libavce_cflags_mips     += -DDISABLE_MSA -DDEFAULT_ARCH=D_ARCH_MIPS_NOMSA
endif

LOCAL_C_INCLUDES_mips   += $(libavce_inc_dir_mips)
LOCAL_SRC_FILES_mips    += $(libavce_srcs_c_mips)
LOCAL_CFLAGS_mips       += $(libavce_cflags_mips)
