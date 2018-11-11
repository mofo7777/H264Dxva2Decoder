//----------------------------------------------------------------------------------------------
// H264Definition.h
//----------------------------------------------------------------------------------------------
#ifndef H264DEFINITION_H
#define H264DEFINITION_H

#define ATOM_MIN_SIZE_HEADER		8
#define ATOM_MIN_READ_SIZE_HEADER	16

#define ATOM_TYPE_FTYP  0x66747970
#define ATOM_TYPE_FREE  0x66726565
#define ATOM_TYPE_MOOV  0x6D6F6F76
#define ATOM_TYPE_MDAT  0x6d646174
#define ATOM_TYPE_WIDE  0x77696465
#define ATOM_TYPE_UUID  0x75756964

#define ATOM_TYPE_MVHD  0x6d766864
#define ATOM_TYPE_TRAK  0x7472616B
#define ATOM_TYPE_UDTA  0x75647461
#define ATOM_TYPE_IODS  0x696f6473

#define ATOM_TYPE_TKHD  0x746b6864
#define ATOM_TYPE_EDTS  0x65647473
#define ATOM_TYPE_ELST  0x656c7374
#define ATOM_TYPE_MDIA  0x6d646961
#define ATOM_TYPE_TREF  0x74726566

#define ATOM_TYPE_MDHD  0x6d646864
#define ATOM_TYPE_HDLR  0x68646c72
#define ATOM_TYPE_MINF  0x6d696e66
#define ATOM_TYPE_STBL  0x7374626c
#define ATOM_TYPE_VMHD  0x766d6864
#define ATOM_TYPE_SMHD  0x736d6864
#define ATOM_TYPE_GMHD  0x676d6864
#define ATOM_TYPE_DINF  0x64696e66
#define ATOM_TYPE_NMHD  0x6e6d6864
#define ATOM_TYPE_HMHD  0x686d6864

#define ATOM_TYPE_STSD  0x73747364
#define ATOM_TYPE_STTS  0x73747473
#define ATOM_TYPE_CTTS  0x63747473
#define ATOM_TYPE_STSS  0x73747373
#define ATOM_TYPE_STSC  0x73747363
#define ATOM_TYPE_STSZ  0x7374737a
#define ATOM_TYPE_STCO  0x7374636f
#define ATOM_TYPE_CO64  0x636f3634
#define ATOM_TYPE_SDTP  0x73647470
#define ATOM_TYPE_SBGP  0x73626770
#define ATOM_TYPE_SGPD  0x73677064

#define HANDLER_TYPE_VIDEO  0x76696465
#define HANDLER_TYPE_SOUND  0x736F756E
#define HANDLER_TYPE_TEXT   0x74657874
#define HANDLER_TYPE_ODSM   0x6f64736d
#define HANDLER_TYPE_SDSM   0x7364736d
#define HANDLER_TYPE_HINT   0x68696e74

#define ATOM_TYPE_AVC1  0x61766331
#define ATOM_TYPE_MP4V  0x6d703476
#define ATOM_TYPE_MP4A  0x6d703461
#define ATOM_TYPE_TEXT  0x74657874
#define ATOM_TYPE_MP4S  0x6d703473
#define ATOM_TYPE_AC_3  0x61632d33
#define ATOM_TYPE_RTP_  0x72747020

#define ATOM_TYPE_AVCC  0x61766343

#define ATOM_TYPE_PASP  0x70617370
#define ATOM_TYPE_ESDS  0x65736473

#define MAX_SPS_COUNT      32
#define MAX_PPS_COUNT      256
#define MAX_SLICEGROUP_IDS 8
#define MAX_REF_PIC_COUNT  16

#define MPG4_ES_DESCRIPTOR					0x03
#define MPG4_DECODER_CONFIG_DESCRIPTOR		0X04
#define MPG4_DECODER_SPECIFIC_DESCRIPTOR	0X05

// This is for mpeg2, check for h264
#define MAX_SLICE	175 // (1 to 175 : 0x00000001 to 0x000001AF)

inline DWORD MAKE_DWORD_HOSTORDER(const BYTE* pData){

	return ((DWORD*)pData)[0];
}

inline UINT64 MAKE_DWORD_HOSTORDER64(const BYTE* pData){

	return ((UINT64*)pData)[0];
}

inline DWORD MAKE_DWORD(const BYTE* pData){

	return _byteswap_ulong(MAKE_DWORD_HOSTORDER(pData));
}

inline UINT64 MAKE_DWORD64(const BYTE* pData){

	return _byteswap_uint64(MAKE_DWORD_HOSTORDER64(pData));
}

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
	PROFILE_MULTI_DEPTH_VIEW_HIGH = 138,
	PROFILE_HIGH444 = 144,
	PROFILE_CAVLC444 = 244,
	PROFILE_CONSTRAINED_BASE = 256,
	PROFILE_CONSTRAINED_HIGH = 257,
	PROFILE_CONSTRAINED_SCALABLE_BASE = 258,
	PROFILE_CONSTRAINED_SCALABLE_HIGH = 259,
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
	BOOL residual_colour_transform_flag;
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
};

struct PPS_DATA{

	DWORD pic_parameter_set_id;
	DWORD seq_parameter_set_id;
	BOOL entropy_coding_mode_flag;
	BOOL pic_order_present_flag;
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

struct PICTURE_REFERENCE_INFO{

	INT TopFieldOrderCnt;
};

#endif