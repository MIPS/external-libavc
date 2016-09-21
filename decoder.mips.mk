libavcd_inc_dir_mips    +=  $(LOCAL_PATH)/common/mips

libavcd_srcs_c_mips     +=  decoder/mips/ih264d_function_selector.c

ifeq ($(ARCH_MIPS_HAS_MSA),true)
libavcd_cflags_mips     += -mfp64 -mmsa
libavcd_cflags_mips     += -DDEFAULT_ARCH=D_ARCH_MIPS_MSA

libavcd_srcs_c_mips     +=  decoder/mips/ih264d_function_selector_msa.c

libavcd_srcs_c_mips     +=  common/mips/ih264_inter_pred_luma_msa.c
libavcd_srcs_c_mips     +=  common/mips/ih264_inter_pred_chroma_msa.c
libavcd_srcs_c_mips     +=  common/mips/ih264_deblk_luma_msa.c
libavcd_srcs_c_mips     +=  common/mips/ih264_deblk_chroma_msa.c
libavcd_srcs_c_mips     +=  common/mips/ih264_luma_intra_pred_msa.c
libavcd_srcs_c_mips     +=  common/mips/ih264_chroma_intra_pred_msa.c
libavcd_srcs_c_mips     +=  common/mips/ih264_ihadamard_scaling_msa.c
libavcd_srcs_c_mips     +=  common/mips/ih264_iquant_itrans_recon_msa.c
libavcd_srcs_c_mips     +=  common/mips/ih264_weighted_pred_msa.c
libavcd_srcs_c_mips     +=  common/mips/ih264_padding_msa.c
else
libavcd_cflags_mips     += -DDISABLE_MSA -DDEFAULT_ARCH=D_ARCH_MIPS_NOMSA
endif

LOCAL_C_INCLUDES_mips   += $(libavcd_inc_dir_mips)
LOCAL_SRC_FILES_mips    += $(libavcd_srcs_c_mips)
LOCAL_CFLAGS_mips       += $(libavcd_cflags_mips)
