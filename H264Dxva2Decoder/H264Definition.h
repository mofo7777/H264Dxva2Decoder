//----------------------------------------------------------------------------------------------
// H264Definition.h
//----------------------------------------------------------------------------------------------
#ifndef H264DEFINITION_H
#define H264DEFINITION_H

#define MAX_SPS_COUNT		32
#define MAX_PPS_COUNT		256
#define MAX_SLICEGROUP_IDS	8
#define MAX_REF_PIC_COUNT	16
#define MAX_SUB_SLICE		32

enum NAL_UNIT_TYPE{

	NAL_UNIT_UNSPEC_0 = 0,
	NAL_UNIT_CODED_SLICE = 1,
	NAL_UNIT_CODED_SLICE_DPA = 2,
	NAL_UNIT_CODED_SLICE_DPB = 3,
	NAL_UNIT_CODED_SLICE_DPC = 4,
	NAL_UNIT_CODED_SLICE_IDR = 5,
	NAL_UNIT_SEI = 6,
	NAL_UNIT_SPS = 7,
	NAL_UNIT_PPS = 8,
	NAL_UNIT_AU_DELIMITER = 9,
	NAL_UNIT_END_OF_SEQ = 10,
	NAL_UNIT_END_OF_STR = 11,
	NAL_UNIT_FILLER_DATA = 12,
	NAL_UNIT_SPS_EXT = 13,
	NAL_UNIT_PREFIX = 14,
	NAL_UNIT_SUBSET_SPS = 15,
	NAL_UNIT_RESV_16 = 16,
	NAL_UNIT_RESV_17 = 17,
	NAL_UNIT_RESV_18 = 18,
	NAL_UNIT_AUX_CODED_SLICE = 19,
	NAL_UNIT_CODED_SLICE_EXT = 20,
	NAL_UNIT_RESV_21 = 21,
	NAL_UNIT_RESV_22 = 22,
	NAL_UNIT_RESV_23 = 23,
	NAL_UNIT_UNSPEC_24 = 24,
	NAL_UNIT_UNSPEC_25 = 25,
	NAL_UNIT_UNSPEC_26 = 26,
	NAL_UNIT_UNSPEC_27 = 27,
	NAL_UNIT_UNSPEC_28 = 28,
	NAL_UNIT_UNSPEC_29 = 29,
	NAL_UNIT_UNSPEC_30 = 30,
	NAL_UNIT_UNSPEC_31 = 31
};

enum PROFILE_IDC{

	PROFILE_UNKNOWN = 0,
	PROFILE_CAVLC444 = 44,
	PROFILE_BASELINE = 66,
	PROFILE_MAIN = 77,
	PROFILE_SCALABLE_BASELINE = 83,
	PROFILE_SCALABLE_HIGH = 86,
	PROFILE_EXTENDED = 88,
	PROFILE_HIGH = 100,
	PROFILE_HIGH10 = 110,
	PROFILE_MULTI_VIEW_HIGH = 118,
	PROFILE_HIGH422 = 122,
	PROFILE_STEREO_HIGH = 128,
	PROFILE_134 = 134,
	PROFILE_135 = 135,
	PROFILE_MULTI_DEPTH_VIEW_HIGH = 138,
	PROFILE_139 = 139,
	PROFILE_HIGH444PP = 244
};

enum LEVEL_IDC{

	LEVEL_IDC_UNKNOWN = 0,
	LEVEL_IDC_9 = 9,
	LEVEL_IDC_10 = 10,
	LEVEL_IDC_11 = 11,
	LEVEL_IDC_12 = 12,
	LEVEL_IDC_13 = 13,
	LEVEL_IDC_20 = 20,
	LEVEL_IDC_21 = 21,
	LEVEL_IDC_22 = 22,
	LEVEL_IDC_30 = 30,
	LEVEL_IDC_31 = 31,
	LEVEL_IDC_32 = 32,
	LEVEL_IDC_40 = 40,
	LEVEL_IDC_41 = 41,
	LEVEL_IDC_42 = 42,
	LEVEL_IDC_50 = 50,
	LEVEL_IDC_51 = 51,
	LEVEL_IDC_52 = 52
};

enum SLICE_TYPE{

	P_SLICE_TYPE = 0,
	B_SLICE_TYPE = 1,
	I_SLICE_TYPE = 2,
	SP_SLICE_TYPE = 3,
	SI_SLICE_TYPE = 4,
	P_SLICE_TYPE2 = 5,
	B_SLICE_TYPE2 = 6,
	I_SLICE_TYPE2 = 7,
	SP_SLICE_TYPE2 = 8,
	SI_SLICE_TYPE2 = 9
};

enum SAMPLE_ASPECT_RATIO{

	SAR_UNKNOWN = 0,
	SAR_1_1 = 1,
	SAR_12_11 = 2,
	SAR_10_11 = 3,
	SAR_16_11 = 4,
	SAR_40_33 = 5,
	SAR_24_11 = 6,
	SAR_20_11 = 7,
	SAR_32_11 = 8,
	SAR_80_33 = 9,
	SAR_18_11 = 10,
	SAR_15_11 = 11,
	SAR_64_33 = 12,
	SAR_160_99 = 13,
	SAR_EXTENDED = 255
};

enum VUI_VIDEO_FORMAT{

	VUI_VF_COMPONENT = 0,
	VUI_VF_PAL = 1,
	VUI_VF_SECAM = 2,
	VUI_VF_MAC = 3,
	VUI_VF_UNKNOWN = 4,
	VUI_VF_RESERVED1 = 5,
	VUI_VF_RESERVED2 = 6
};

enum PRIMARIES_COLOUR{

	PRIMARIES_COLOUR_RESERVED0 = 0,
	PRIMARIES_COLOUR_BT709 = 1,
	PRIMARIES_COLOUR_UNSPECIFIED = 2,
	PRIMARIES_COLOUR_RESERVED3 = 3,
	PRIMARIES_COLOUR_BT470M = 4,
	PRIMARIES_COLOUR_BT470BG = 5,
	PRIMARIES_COLOUR_SMPTE170M = 6,
	PRIMARIES_COLOUR_SMPTE240M = 7,
	PRIMARIES_COLOUR_FILM = 8/*,
	PM_BT2020 = 9,
	PM_SMPTE428 = 10,
	PM_SMPTE428_1 = PM_SMPTE428,
	PM_SMPTE431 = 11,
	PM_SMPTE432 = 12,
	PM_JEDEC_P22 = 22*/
};

enum TRANSFER_CHARACTERISTICS{

	TRANSFER_CHARACTERISTICS_RESERVED0 = 0,
	TRANSFER_CHARACTERISTICS_BT709 = 1,
	TRANSFER_CHARACTERISTICS_UNSPECIFIED = 2,
	TRANSFER_CHARACTERISTICS_RESERVED3 = 3,
	TRANSFER_CHARACTERISTICS_BT470M = 4,
	TRANSFER_CHARACTERISTICS_BT470BG = 5,
	TRANSFER_CHARACTERISTICS_SMPTE170M = 6,
	TRANSFER_CHARACTERISTICS_SMPTE240M = 7,
	TRANSFER_CHARACTERISTICS_LINEAR = 8,
	TRANSFER_CHARACTERISTICS_LOG = 9,
	TRANSFER_CHARACTERISTICS_LOG_SQRT = 10
};

enum MATRIX_COEFFICIENTS{

	MATRIX_COEFFICIENTS_RGB = 0,
	MATRIX_COEFFICIENTS_BT709 = 1,
	MATRIX_COEFFICIENTS_UNSPECIFIED = 2,
	MATRIX_COEFFICIENTS_RESERVED3 = 3,
	MATRIX_COEFFICIENTS_FCC = 4,
	MATRIX_COEFFICIENTS_BT470BG = 5,
	MATRIX_COEFFICIENTS_SMPTE170M = 6,
	MATRIX_COEFFICIENTS_SMPTE240M = 7,
	MATRIX_COEFFICIENTS_YCGCO = 8
};

enum SEI_MESSAGE_TYPE{

	SEI_BUFFERING_PERIOD = 0,
	SEI_PIC_TIMING = 1,
	SEI_USER_DATA_ITU_T_T35 = 4,
	SEI_USER_DATA_UNREGISTERED = 5,
	SEI_RECOVERY_POINT = 6,
	SEI_FRAME_PACKING = 45,
	SEI_DISPLAY_ORIENTATION = 47
};

struct VUI_PARAMETERS{

	BOOL aspect_ratio_info_present_flag;
	SAMPLE_ASPECT_RATIO aspect_ratio_idc;
	DWORD sar_width;
	DWORD sar_height;
	BOOL overscan_info_present_flag;
	BOOL overscan_appropriate_flag;
	BOOL video_signal_type_present_flag;
	VUI_VIDEO_FORMAT video_format;
	BOOL video_full_range_flag;
	BOOL colour_description_present_flag;
	PRIMARIES_COLOUR colour_primaries;
	TRANSFER_CHARACTERISTICS transfer_characteristics;
	MATRIX_COEFFICIENTS matrix_coefficients;
	BOOL chroma_loc_info_present_flag;
	DWORD chroma_sample_loc_type_top_field;
	DWORD chroma_sample_loc_type_bottom_field;
	BOOL timing_info_present_flag;
	DWORD num_units_in_tick;
	DWORD time_scale;
	BOOL fixed_frame_rate_flag;
	BOOL nal_hrd_parameters_present_flag;
	BOOL vcl_hrd_parameters_present_flag;
	BOOL low_delay_hrd_flag;
	BOOL pic_struct_present_flag;
	BOOL bitstream_restriction_flag;
	BOOL motion_vectors_over_pic_boundaries_flag;
	DWORD max_bytes_per_pic_denom;
	DWORD max_bits_per_mb_denom;
	DWORD log2_max_mv_length_horizontal;
	DWORD log2_max_mv_length_vertical;
	DWORD num_reorder_frames;
	DWORD max_dec_frame_buffering;
};

struct SPS_DATA{

	PROFILE_IDC profile_idc;
	BOOL constraint_set_flag[6];
	LEVEL_IDC level_idc;
	DWORD seq_parameter_set_id;
	DWORD chroma_format_idc;
	BOOL separate_colour_plane_flag;
	DWORD bit_depth_luma_minus8;
	DWORD bit_depth_chroma_minus8;
	BOOL qpprime_y_zero_transform_bypass_flag;
	BOOL seq_scaling_matrix_present_flag;
	DWORD log2_max_frame_num_minus4;
	DWORD pic_order_cnt_type;
	DWORD log2_max_pic_order_cnt_lsb_minus4;
	DWORD num_ref_frames;
	BOOL gaps_in_frame_num_value_allowed_flag;
	DWORD pic_width_in_mbs_minus1;
	DWORD pic_height_in_map_units_minus1;
	BYTE frame_mbs_only_flag;
	BOOL direct_8x8_inference_flag;
	BOOL frame_cropping_flag;
	DWORD frame_crop_left_offset;
	DWORD frame_crop_right_offset;
	DWORD frame_crop_top_offset;
	DWORD frame_crop_bottom_offset;
	BOOL vui_parameters_present_flag;
	VUI_PARAMETERS sVuiParameters;
	// pic_order_cnt_type == 0
	DWORD MaxPicOrderCntLsb;
	// pic_order_cnt_type == 2
	DWORD MaxFrameNum;
	BOOL bHasCustomScalingList;
	UCHAR ScalingList4x4[6][16];
	UCHAR ScalingList8x8[6][64];
};

struct PPS_DATA{

	DWORD pic_parameter_set_id;
	DWORD seq_parameter_set_id;
	BOOL entropy_coding_mode_flag;
	BOOL bottom_field_pic_order_in_frame_present_flag;
	DWORD num_slice_groups_minus1;
	DWORD num_ref_idx_l0_active_minus1;
	DWORD num_ref_idx_l1_active_minus1;
	BOOL weighted_pred_flag;
	BYTE weighted_bipred_idc;
	int pic_init_qp_minus26;
	int pic_init_qs_minus26;
	int chroma_qp_index_offset[2];
	BOOL deblocking_filter_control_present_flag;
	BOOL constrained_intra_pred_flag;
	BOOL redundant_pic_cnt_present_flag;
	BOOL transform_8x8_mode_flag;
	BOOL pic_scaling_matrix_present_flag;
	DWORD second_chroma_qp_index_offset;
	BOOL bHasCustomScalingList;
	UCHAR ScalingList4x4[6][16];
	UCHAR ScalingList8x8[6][64];
};

struct REORDERED_LIST{

	USHORT reordering_of_pic_nums_idc;
	USHORT abs_diff_pic_num_minus1;
	USHORT long_term_pic_num;
};

enum MMCO_OP{

	MMCO_OP_END = 0,
	MMCO_OP_SHORT2UNUSED = 1,
	MMCO_OP_LONG2UNUSED = 2,
	MMCO_OP_SHORT2LONG = 3,
	MMCO_OP_SET_MAX_LONG = 4,
	MMCO_OP_RESET = 5,
	MMCO_OP_LONG = 6,
	MMCO_OP_MAX = 66
};

struct MMCO_OP_MODE{

	MMCO_OP mmcoOP;
	DWORD difference_of_pic_nums_minus1;
	DWORD Long_term_pic_num;
	DWORD Long_term_frame_idx;
	DWORD Max_long_term_frame_idx_plus1;
};

struct PICTURE_MARKING{

	BOOL no_output_of_prior_pics_flag;
	BOOL long_term_reference_flag;
	BOOL adaptive_ref_pic_marking_mode_flag;
	INT iNumOPMode;
	MMCO_OP_MODE mmcoOPMode[MMCO_OP_MAX];
};

struct SLICE_DATA{

	USHORT first_mb_in_slice;
	USHORT slice_type;
	USHORT pic_parameter_set_id;
	USHORT frame_num;
	USHORT pic_order_cnt_lsb;
	DWORD num_ref_idx_l0_active_minus1;
	DWORD num_ref_idx_l1_active_minus1;
	// Reordered List
	DWORD dwListCountL0;
	DWORD dwListCountL1;
	REORDERED_LIST vReorderedList[2][16];
	PICTURE_MARKING PicMarking;
	// All pic_order_cnt_type
	DWORD TopFieldOrderCnt;
	// pic_order_cnt_type == 0
	DWORD PicOrderCntMsb;
	DWORD PicOrderCntLsb;
	DWORD prevPicOrderCntMsb;
	DWORD prevPicOrderCntLsb;
	// pic_order_cnt_type == 2
	DWORD prevFrameNum;
	DWORD prevFrameNumOffset;
	DWORD FrameNumOffset;
	DWORD tempPicOrderCnt;
	// Reference Picture List
	DWORD FrameNum;
	DWORD FrameNumWrap;
	DWORD PicNum;
	DWORD LongTermFrameIdx;
	DWORD LongTermPicNum;
	// MMCO
	DWORD MaxLongTermFrameIdx;
};

struct PICTURE_INFO{

	SPS_DATA sps;
	PPS_DATA pps;
	SLICE_DATA slice;
	NAL_UNIT_TYPE NalUnitType;
	BYTE btNalRefIdc;
};

const UCHAR Default_4x4_Intra[16] = {

	6, 13, 13, 20,
	20, 20, 28, 28,
	28, 28, 32, 32,
	32, 37, 37, 42
};

const UCHAR Default_4x4_Inter[16] = {

	10, 14, 14, 20,
	20, 20, 24, 24,
	24, 24, 27, 27,
	27, 30, 30, 34
};

const UCHAR Default_8x8_Intra[64] = {

	6, 10, 10, 13, 11, 13, 16, 16,
	16, 16, 18, 18, 18, 18, 18, 23,
	23, 23, 23, 23, 23, 25, 25, 25,
	25, 25, 25, 25, 27, 27, 27, 27,
	27, 27, 27, 27, 29, 29, 29, 29,
	29, 29, 29, 31, 31, 31, 31, 31,
	31, 33, 33, 33, 33, 33, 36, 36,
	36, 36, 38, 38, 38, 40, 40, 42
};

const UCHAR Default_8x8_Inter[64] = {

	9, 13, 13, 15, 13, 15, 17, 17,
	17, 17, 19, 19, 19, 19, 19, 21,
	21, 21, 21, 21, 21, 22, 22, 22,
	22, 22, 22, 22, 24, 24, 24, 24,
	24, 24, 24, 24, 25, 25, 25, 25,
	25, 25, 25, 27, 27, 27, 27, 27,
	27, 28, 28, 28, 28, 28, 30, 30,
	30, 30, 32, 32, 32, 33, 33, 35
};

#endif