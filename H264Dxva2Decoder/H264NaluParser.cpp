//----------------------------------------------------------------------------------------------
// H264NaluParser.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

static const BYTE TrailingBits[9] = {0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

CH264NaluParser::CH264NaluParser() : m_dwWidth(0), m_dwHeight(0), m_iNaluLenghtSize(0){

	ZeroMemory(&m_Picture, sizeof(PICTURE_INFO));
}

HRESULT CH264NaluParser::ParseVideoConfigDescriptor(const BYTE* pData, const DWORD dwSize){

	HRESULT hr;
	DWORD dwNaluSize = 0;
	DWORD dwLeft = dwSize;

	IF_FAILED_RETURN(hr = (dwSize > 8 && pData != NULL ? S_OK : E_FAIL));

	if(m_iNaluLenghtSize == 4){

		dwNaluSize = MAKE_DWORD(pData);
	}
	else if(m_iNaluLenghtSize == 2){

		dwNaluSize = pData[0] << 8;
		dwNaluSize |= pData[1];
	}
	else{

		// todo : size 1
		assert(FALSE);
		return E_FAIL;
	}

	pData += m_iNaluLenghtSize;
	dwLeft -= m_iNaluLenghtSize;

	hr = E_FAIL;

	// uiForbiddenZeroBit
	BYTE uiForbiddenZeroBit = *pData >> 7;

	if(uiForbiddenZeroBit != 0){
		TRACE((L"ParseNalHeader : uiForbiddenZeroBit != 0"));
		return hr;
	}

	// uiNalRefIdc
	m_Picture.btNalRefIdc = *pData >> 5;
	// eNalUnitType
	m_Picture.NalUnitType = (NAL_UNIT_TYPE)(*pData & 0x1f);

	IF_FAILED_RETURN(hr = (m_Picture.NalUnitType != NAL_UNIT_SPS ? E_FAIL : S_OK));

	pData++;
	dwLeft--;

	m_cBitStream.Init(pData, dwLeft * 8);

	IF_FAILED_RETURN(hr = ParseSPS());

	pData += (dwNaluSize - 1);
	dwLeft -= (dwNaluSize - 1);

	if(m_iNaluLenghtSize == 4){

		dwNaluSize = MAKE_DWORD(pData);
	}
	else if(m_iNaluLenghtSize == 2){

		dwNaluSize = pData[0] << 8;
		dwNaluSize |= pData[1];
	}
	else{

		// todo : size 1
		assert(FALSE);
		return E_FAIL;
	}

	pData += m_iNaluLenghtSize;
	dwLeft -= m_iNaluLenghtSize;

	hr = E_FAIL;

	// uiForbiddenZeroBit
	uiForbiddenZeroBit = *pData >> 7;

	if(uiForbiddenZeroBit != 0){
		TRACE((L"ParseNalHeader : uiForbiddenZeroBit != 0"));
		return hr;
	}

	// uiNalRefIdc
	m_Picture.btNalRefIdc = *pData >> 5;
	// eNalUnitType
	m_Picture.NalUnitType = (NAL_UNIT_TYPE)(*pData & 0x1f);

	IF_FAILED_RETURN(hr = (m_Picture.NalUnitType != NAL_UNIT_PPS ? E_FAIL : S_OK));

	pData++;
	dwLeft--;

	m_cBitStream.Init(pData, dwLeft * 8);

	IF_FAILED_RETURN(hr = ParsePPS());

	return hr;
}

HRESULT CH264NaluParser::ParseNaluHeader(CMFBuffer& pVideoBuffer){

	HRESULT hr = S_FALSE;
	DWORD dwNaluSize = 0;
	DWORD dwSize = pVideoBuffer.GetBufferSize();

	if(dwSize <= 4){
		TRACE((L"ParseNalHeader : buffer size <= 4"));
		return hr;
	}

	if(m_iNaluLenghtSize == 4){

		dwNaluSize = MAKE_DWORD(pVideoBuffer.GetStartBuffer());
	}
	else if(m_iNaluLenghtSize == 2){

		dwNaluSize = pVideoBuffer.GetStartBuffer()[0] << 8;
		dwNaluSize |= pVideoBuffer.GetStartBuffer()[1];
	}
	else{

		// todo : size 1
		assert(FALSE);
		return E_FAIL;
	}

	assert(dwNaluSize <= pVideoBuffer.GetBufferSize());

	IF_FAILED_RETURN(hr = pVideoBuffer.SetStartPosition(m_iNaluLenghtSize));

	// Todo : remove the consecutive ZERO at the end of current NAL in the reverse order.--2011.6.1
	// RemoveEmulationPreventionByte : no problem for now

	BYTE* pData = pVideoBuffer.GetStartBuffer();

	hr = E_FAIL;

	// uiForbiddenZeroBit
	BYTE uiForbiddenZeroBit = *pData >> 7;

	if(uiForbiddenZeroBit != 0){
		TRACE((L"ParseNalHeader : uiForbiddenZeroBit != 0"));
		return hr;
	}

	// uiNalRefIdc
	m_Picture.btNalRefIdc = *pData >> 5;
	// eNalUnitType
	m_Picture.NalUnitType = (NAL_UNIT_TYPE)(*pData & 0x1f);

	pData++;
	dwSize -= m_iNaluLenghtSize;

	switch(m_Picture.NalUnitType){

		case NAL_UNIT_CODED_SLICE:
		case NAL_UNIT_CODED_SLICE_IDR:
			m_cBitStream.Init(pData, dwSize * 8);
			hr = ParseCodedSlice();
			break;

		default:
			hr = S_OK;
	}

	if(SUCCEEDED(hr))
		hr = pVideoBuffer.SetStartPosition(dwNaluSize);

	return hr;
}

HRESULT CH264NaluParser::ParseSPS(){

	HRESULT hr = S_OK;

	SPS_DATA* pSPS = &m_Picture.sps;

	pSPS->profile_idc = (PROFILE_IDC)m_cBitStream.GetBits(8);
	pSPS->constraint_set_flag[0] = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pSPS->constraint_set_flag[1] = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pSPS->constraint_set_flag[2] = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pSPS->constraint_set_flag[3] = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pSPS->constraint_set_flag[4] = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pSPS->constraint_set_flag[5] = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	m_cBitStream.CheckZeroStream(2);
	pSPS->level_idc = (LEVEL_IDC)m_cBitStream.GetBits(8);
	pSPS->seq_parameter_set_id = m_cBitStream.UGolomb();

	if(pSPS->seq_parameter_set_id >= MAX_SPS_COUNT){

		IF_FAILED_RETURN(hr = E_FAIL);
	}

	// Todo : not sure for all of these values
	if(pSPS->profile_idc == 44 ||
		pSPS->profile_idc == PROFILE_SCALABLE_BASELINE ||
		pSPS->profile_idc == PROFILE_SCALABLE_HIGH ||
		pSPS->profile_idc == PROFILE_HIGH ||
		pSPS->profile_idc == PROFILE_HIGH10 ||
		pSPS->profile_idc == PROFILE_MULTI_VIEW_HIGH ||
		pSPS->profile_idc == PROFILE_HIGH422 ||
		pSPS->profile_idc == PROFILE_STEREO_HIGH ||
		pSPS->profile_idc == PROFILE_MULTI_DEPTH_VIEW_HIGH ||
		pSPS->profile_idc == PROFILE_HIGH444 ||
		pSPS->profile_idc == PROFILE_CAVLC444)
	{
		pSPS->chroma_format_idc = m_cBitStream.UGolomb();

		if(pSPS->chroma_format_idc == 3){
			pSPS->residual_colour_transform_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
		}

		pSPS->bit_depth_luma_minus8 = m_cBitStream.UGolomb();
		pSPS->bit_depth_chroma_minus8 = m_cBitStream.UGolomb();
		pSPS->qpprime_y_zero_transform_bypass_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
		pSPS->seq_scaling_matrix_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

		if(pSPS->seq_scaling_matrix_present_flag){

			// todo
			IF_FAILED_RETURN(hr = E_FAIL);
		}
	}
	else{

		pSPS->chroma_format_idc = 1;
	}

	// log2_max_frame_num_minus4 0 - 12
	pSPS->log2_max_frame_num_minus4 = m_cBitStream.UGolomb();
	pSPS->pic_order_cnt_type = m_cBitStream.UGolomb();

	pSPS->MaxFrameNum = 1 << (pSPS->log2_max_frame_num_minus4 + 4);

	if(pSPS->pic_order_cnt_type == 0){

		pSPS->log2_max_pic_order_cnt_lsb_minus4 = m_cBitStream.UGolomb();

		IF_FAILED_RETURN(hr = (pSPS->log2_max_pic_order_cnt_lsb_minus4 > 12) ? E_FAIL : S_OK);
		pSPS->MaxPicOrderCntLsb = 1 << (pSPS->log2_max_pic_order_cnt_lsb_minus4 + 4);
	}
	else if(pSPS->pic_order_cnt_type == 1){

		// Todo : get delta_pic_order_always_zero_flag/etc...
		IF_FAILED_RETURN(hr = E_FAIL);
	}
	else if(pSPS->pic_order_cnt_type == 2){

		//pSPS->MaxFrameNum = 1 << (pSPS->log2_max_pic_order_cnt_lsb_minus4 + 4);
	}
	else{

		IF_FAILED_RETURN(hr = E_FAIL);
	}

	pSPS->num_ref_frames = m_cBitStream.UGolomb();
	pSPS->gaps_in_frame_num_value_allowed_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pSPS->pic_width_in_mbs_minus1 = m_cBitStream.UGolomb();
	pSPS->pic_height_in_map_units_minus1 = m_cBitStream.UGolomb();
	pSPS->frame_mbs_only_flag = (BYTE)m_cBitStream.GetBits(1);

	if(!pSPS->frame_mbs_only_flag){

		// Todo : mb_adaptive_frame_field_flag
		IF_FAILED_RETURN(hr = E_FAIL);
	}

	pSPS->direct_8x8_inference_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pSPS->frame_cropping_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	m_dwWidth = (m_Picture.sps.pic_width_in_mbs_minus1 + 1) * 16;
	m_dwHeight = (2 - m_Picture.sps.frame_mbs_only_flag) * (m_Picture.sps.pic_height_in_map_units_minus1 + 1) * 16;

	if(pSPS->frame_cropping_flag){

		pSPS->frame_crop_left_offset = m_cBitStream.UGolomb();
		pSPS->frame_crop_right_offset = m_cBitStream.UGolomb();
		pSPS->frame_crop_top_offset = m_cBitStream.UGolomb();
		pSPS->frame_crop_bottom_offset = m_cBitStream.UGolomb();

		// DXVA2 Decoding Surface should be size without cropping... but seems to be OK
		m_dwWidth = m_dwWidth - (pSPS->frame_crop_left_offset * 2) - (pSPS->frame_crop_right_offset * 2);
		m_dwHeight = m_dwHeight - (pSPS->frame_crop_top_offset * 2) - (pSPS->frame_crop_bottom_offset * 2);
	}

	pSPS->vui_parameters_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pSPS->vui_parameters_present_flag)
		IF_FAILED_RETURN(hr = ParseVuiParameters(pSPS));

	// rbsp_trailing_bits();

	return hr;
}

HRESULT CH264NaluParser::ParsePPS(){

	HRESULT hr = S_OK;

	PPS_DATA* pPPS = &m_Picture.pps;

	pPPS->pic_parameter_set_id = m_cBitStream.UGolomb();

	if(pPPS->pic_parameter_set_id >= MAX_PPS_COUNT){
		IF_FAILED_RETURN(hr = E_FAIL);
	}

	pPPS->seq_parameter_set_id = m_cBitStream.UGolomb();

	if(pPPS->seq_parameter_set_id >= MAX_SPS_COUNT){
		IF_FAILED_RETURN(hr = E_FAIL);
	}

	pPPS->entropy_coding_mode_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pPPS->pic_order_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	/*if(pPPS->pic_order_present_flag != FALSE){
		// todo
		IF_FAILED_RETURN(hr = E_FAIL);
	}*/

	pPPS->num_slice_groups_minus1 = m_cBitStream.UGolomb();

	if((pPPS->num_slice_groups_minus1 + 1) > MAX_SLICEGROUP_IDS){
		IF_FAILED_RETURN(hr = E_FAIL);
	}

	if(pPPS->num_slice_groups_minus1 > 0){

		// Todo : slice_group_map_type/etc...
		IF_FAILED_RETURN(hr = E_FAIL);
	}

	pPPS->num_ref_idx_l0_active_minus1 = m_cBitStream.UGolomb();
	pPPS->num_ref_idx_l1_active_minus1 = m_cBitStream.UGolomb();

	if(pPPS->num_ref_idx_l0_active_minus1 > MAX_REF_PIC_COUNT || pPPS->num_ref_idx_l1_active_minus1 > MAX_REF_PIC_COUNT){
		IF_FAILED_RETURN(hr = E_FAIL);
	}

	pPPS->weighted_pred_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pPPS->weighted_bipred_idc = (BYTE)m_cBitStream.GetBits(2);
	pPPS->pic_init_qp_minus26 = m_cBitStream.SGolomb();
	pPPS->pic_init_qs_minus26 = m_cBitStream.SGolomb();
	// chroma_qp_index_offset -12 to 12
	pPPS->chroma_qp_index_offset[0] = pPPS->chroma_qp_index_offset[1] = m_cBitStream.SGolomb();
	pPPS->deblocking_filter_control_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pPPS->constrained_intra_pred_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pPPS->redundant_pic_cnt_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pPPS->redundant_pic_cnt_present_flag != FALSE){
		// todo
		IF_FAILED_RETURN(hr = E_FAIL);
	}

	int bits = m_cBitStream.BitsRemain();

	if(bits == 0){
		return hr;
	}

	if(bits <= 8){

		BYTE trail_check = (BYTE)m_cBitStream.PeekBits(bits);

		if(trail_check == TrailingBits[bits]){
			return hr;
		}
	}

	pPPS->transform_8x8_mode_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pPPS->pic_scaling_matrix_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pPPS->pic_scaling_matrix_present_flag){

		IF_FAILED_RETURN(hr = E_FAIL);

		/*uint max_count = 6 + (2 * transform_8x8_mode_flag);

		for(uint ix = 0; ix < max_count; ix++){

			temp = m_cBitStream.GetBits(1);

			if(temp){
				scaling_list(ix, ix < 6 ? 16 : 64, bs);
		}
		*/
	}

	pPPS->second_chroma_qp_index_offset = m_cBitStream.SGolomb();

	// rbsp_trailing_bits();

	return hr;
}

HRESULT CH264NaluParser::ParseCodedSlice(){

	HRESULT hr = S_OK;

	ZeroMemory(m_Picture.slice.vReorderedList, sizeof(m_Picture.slice.vReorderedList));
	ZeroMemory(&m_Picture.slice.PicMarking, sizeof(m_Picture.slice.PicMarking));

	m_Picture.slice.first_mb_in_slice = (USHORT)m_cBitStream.UGolomb();

	// todo : handle multiple NALU in one slice frame
	IF_FAILED_RETURN(hr = (m_Picture.slice.first_mb_in_slice ? E_FAIL : S_OK));

	m_Picture.slice.slice_type = (USHORT)m_cBitStream.UGolomb();
	m_Picture.slice.pic_parameter_set_id = (USHORT)m_cBitStream.UGolomb();
	m_Picture.slice.frame_num = (USHORT)m_cBitStream.GetBits(m_Picture.sps.log2_max_frame_num_minus4 + 4);

	// if !m_Picture.sps.frame_mbs_only_flag : field_pic_flag ... if(field_pic_flag) bottom_field_flag

	if(m_Picture.NalUnitType == NAL_UNIT_CODED_SLICE_IDR){

		// idr_pic_id
		m_cBitStream.UGolomb();
		m_Picture.slice.FrameNum = m_Picture.slice.frame_num;
		m_Picture.slice.FrameNumWrap = m_Picture.slice.frame_num;
	}

	if(m_Picture.sps.pic_order_cnt_type == 0){

		m_Picture.slice.pic_order_cnt_lsb = (USHORT)m_cBitStream.GetBits(m_Picture.sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
		// assert(pic_order_cnt_lsb < MaxPicOrderCntLsb)
		// if(m_Picture.pps.bBottomFieldPicOrderFlag && !field_pic_flag)

		if(m_Picture.pps.pic_order_present_flag)
			/*DWORD delta_pic_order_cnt_bottom =*/ m_cBitStream.SGolomb();
	}
	else{

		// dwPicOrderCountType == 2 : Display Order == Decoding Order
		// if used for reference
		m_Picture.slice.pic_order_cnt_lsb = 2 * m_Picture.slice.frame_num;
		// else
		// m_Picture.slice.pic_order_cnt_lsb = (2 * m_Picture.slice.frame_num) - 1;
	}

	// if m_Picture.pps.redundant_pic_cnt_present_flag : redundant_pic_cnt

	if(m_Picture.slice.slice_type == B_SLICE_TYPE || m_Picture.slice.slice_type == B_SLICE_TYPE2){

		// direct_spatial_mv_pred_flag (TRUE/FALSE)
		m_cBitStream.GetBits(1);
	}

	m_Picture.slice.num_ref_idx_l0_active_minus1 = m_Picture.pps.num_ref_idx_l0_active_minus1;
	m_Picture.slice.num_ref_idx_l1_active_minus1 = m_Picture.pps.num_ref_idx_l1_active_minus1;

	if(m_Picture.slice.slice_type == P_SLICE_TYPE || m_Picture.slice.slice_type == P_SLICE_TYPE2 ||
		m_Picture.slice.slice_type == SP_SLICE_TYPE || m_Picture.slice.slice_type == SP_SLICE_TYPE2 ||
		m_Picture.slice.slice_type == B_SLICE_TYPE || m_Picture.slice.slice_type == B_SLICE_TYPE2){

		BOOL num_ref_idx_active_override_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

		if(num_ref_idx_active_override_flag){

			m_Picture.slice.num_ref_idx_l0_active_minus1 = m_cBitStream.UGolomb();

			if(m_Picture.slice.slice_type == B_SLICE_TYPE || m_Picture.slice.slice_type == B_SLICE_TYPE2){

				m_Picture.slice.num_ref_idx_l1_active_minus1 = m_cBitStream.UGolomb();
			}
		}
	}

	m_Picture.slice.dwListCountL0 = 0;
	m_Picture.slice.dwListCountL1 = 0;

	if(m_Picture.slice.slice_type != I_SLICE_TYPE && m_Picture.slice.slice_type != I_SLICE_TYPE2 &&
		m_Picture.slice.slice_type != SI_SLICE_TYPE && m_Picture.slice.slice_type != SI_SLICE_TYPE2){

		BOOL ref_pic_list_reordering_flag_l0 = m_cBitStream.GetBits(1) ? TRUE : FALSE;

		if(ref_pic_list_reordering_flag_l0){

			int iIndex = -1;

			do{

				iIndex++;
				m_Picture.slice.dwListCountL0++;

				m_Picture.slice.vReorderedList[0][iIndex].reordering_of_pic_nums_idc = (USHORT)m_cBitStream.UGolomb();

				if(m_Picture.slice.vReorderedList[0][iIndex].reordering_of_pic_nums_idc == 0 || m_Picture.slice.vReorderedList[0][iIndex].reordering_of_pic_nums_idc == 1)
					m_Picture.slice.vReorderedList[0][iIndex].abs_diff_pic_num_minus1 = (USHORT)m_cBitStream.UGolomb();
				else if(m_Picture.slice.vReorderedList[0][iIndex].reordering_of_pic_nums_idc == 2)
					m_Picture.slice.vReorderedList[0][iIndex].long_term_pic_num = (USHORT)m_cBitStream.UGolomb();
				else if(m_Picture.slice.vReorderedList[0][iIndex].reordering_of_pic_nums_idc != 3)
					IF_FAILED_RETURN(hr = E_FAIL);
			}
			while(m_Picture.slice.vReorderedList[0][iIndex].reordering_of_pic_nums_idc != 3);
		}
	}

	if(m_Picture.slice.slice_type == B_SLICE_TYPE || m_Picture.slice.slice_type == B_SLICE_TYPE2){

		BOOL ref_pic_list_reordering_flag_l1 = m_cBitStream.GetBits(1) ? TRUE : FALSE;

		if(ref_pic_list_reordering_flag_l1){

			int iIndex = -1;

			do{

				iIndex++;
				m_Picture.slice.dwListCountL1++;

				m_Picture.slice.vReorderedList[1][iIndex].reordering_of_pic_nums_idc = (USHORT)m_cBitStream.UGolomb();

				if(m_Picture.slice.vReorderedList[1][iIndex].reordering_of_pic_nums_idc == 0 || m_Picture.slice.vReorderedList[1][iIndex].reordering_of_pic_nums_idc == 1)
					m_Picture.slice.vReorderedList[1][iIndex].abs_diff_pic_num_minus1 = (USHORT)m_cBitStream.UGolomb();
				else if(m_Picture.slice.vReorderedList[1][iIndex].reordering_of_pic_nums_idc == 2)
					m_Picture.slice.vReorderedList[1][iIndex].long_term_pic_num = (USHORT)m_cBitStream.UGolomb();
				else if(m_Picture.slice.vReorderedList[1][iIndex].reordering_of_pic_nums_idc != 3)
					IF_FAILED_RETURN(hr = E_FAIL);
			}
			while(m_Picture.slice.vReorderedList[1][iIndex].reordering_of_pic_nums_idc != 3);
		}
	}

	if((m_Picture.pps.weighted_pred_flag &&
		(m_Picture.slice.slice_type == P_SLICE_TYPE || m_Picture.slice.slice_type == P_SLICE_TYPE2 || m_Picture.slice.slice_type == SP_SLICE_TYPE || m_Picture.slice.slice_type == SP_SLICE_TYPE2)) ||
		(m_Picture.pps.weighted_bipred_idc == 1 && (m_Picture.slice.slice_type == B_SLICE_TYPE || m_Picture.slice.slice_type == B_SLICE_TYPE2)))
		ParsePredWeightTable();

	if(m_Picture.btNalRefIdc != 0)
		ParseDecRefPicMarking();

	// todo : memory_management_control_operation (All pic_order_cnt_type i think)
	/*if(memory_management_control_operation == 5){
	  prevFrameNumOffset = 0;
	  tempPicOrderCnt = PicOrderCnt(CurrPic);
	  TopFieldOrderCnt = TopFieldOrderCnt – tempPicOrderCnt
	}
	else
	*/

	HandlePOC();

	return hr;
}

void CH264NaluParser::HandlePOC(){

	if(m_Picture.sps.pic_order_cnt_type == 0){

		if(m_Picture.NalUnitType == NAL_UNIT_CODED_SLICE_IDR){

			m_Picture.slice.prevPicOrderCntMsb = 0;
			m_Picture.slice.prevPicOrderCntLsb = 0;
			m_Picture.slice.TopFieldOrderCnt = 0;
		}
		else{

			/*if(memory_management_control_operation == 5){

			  if(TopFiled){
				prevPicOrderCntMsb = 0;
				prevPicOrderCntLsb = TopFieldOrderCnt;
			  }
			  else{
				prevPicOrderCntMsb = 0;
				prevPicOrderCntLsb = 0;
			  }
			}
			else{*/

			if((m_Picture.slice.pic_order_cnt_lsb < m_Picture.slice.prevPicOrderCntLsb) && ((m_Picture.slice.prevPicOrderCntLsb - m_Picture.slice.pic_order_cnt_lsb) >= (m_Picture.sps.MaxPicOrderCntLsb / 2)))
				m_Picture.slice.PicOrderCntMsb = m_Picture.slice.prevPicOrderCntMsb + m_Picture.sps.MaxPicOrderCntLsb;
			else if((m_Picture.slice.pic_order_cnt_lsb > m_Picture.slice.prevPicOrderCntLsb) && ((m_Picture.slice.pic_order_cnt_lsb - m_Picture.slice.prevPicOrderCntLsb) > (m_Picture.sps.MaxPicOrderCntLsb / 2)))
				m_Picture.slice.PicOrderCntMsb = m_Picture.slice.prevPicOrderCntMsb - m_Picture.sps.MaxPicOrderCntLsb;
			else
				m_Picture.slice.PicOrderCntMsb = m_Picture.slice.prevPicOrderCntMsb;


			m_Picture.slice.TopFieldOrderCnt = m_Picture.slice.PicOrderCntMsb + m_Picture.slice.pic_order_cnt_lsb;
			m_Picture.slice.prevPicOrderCntMsb = m_Picture.slice.PicOrderCntMsb;
			m_Picture.slice.prevPicOrderCntLsb = m_Picture.slice.pic_order_cnt_lsb;
		}
	}
	else if(m_Picture.sps.pic_order_cnt_type == 1){

		// todo
	}
	else if(m_Picture.sps.pic_order_cnt_type == 2){

		if(m_Picture.NalUnitType == NAL_UNIT_CODED_SLICE_IDR)
			m_Picture.slice.FrameNumOffset = 0;
		else if(m_Picture.slice.prevFrameNum > m_Picture.slice.frame_num)
			m_Picture.slice.FrameNumOffset = m_Picture.slice.prevFrameNumOffset + m_Picture.sps.MaxFrameNum;
		else
			m_Picture.slice.FrameNumOffset = m_Picture.slice.prevFrameNumOffset;

		if(m_Picture.NalUnitType == NAL_UNIT_CODED_SLICE_IDR)
			m_Picture.slice.tempPicOrderCnt = 0;
		else if(m_Picture.btNalRefIdc == 0)
			m_Picture.slice.tempPicOrderCnt = 2 * (m_Picture.slice.FrameNumOffset + m_Picture.slice.frame_num) - 1;
		else
			m_Picture.slice.tempPicOrderCnt = 2 * (m_Picture.slice.FrameNumOffset + m_Picture.slice.frame_num);

		m_Picture.slice.TopFieldOrderCnt = m_Picture.slice.tempPicOrderCnt;
		m_Picture.slice.prevFrameNum = m_Picture.slice.frame_num;
		m_Picture.slice.prevFrameNumOffset = m_Picture.slice.FrameNumOffset;
	}

	if(m_Picture.NalUnitType != NAL_UNIT_CODED_SLICE_IDR){

		if(m_Picture.slice.FrameNum > m_Picture.slice.frame_num)
			m_Picture.slice.FrameNumWrap = m_Picture.slice.FrameNum - m_Picture.sps.MaxFrameNum;
		else
			m_Picture.slice.FrameNumWrap = m_Picture.slice.FrameNum;

		m_Picture.slice.PicNum = m_Picture.slice.FrameNumWrap;
		m_Picture.slice.LongTermPicNum = m_Picture.slice.LongTermFrameIdx;
	}
}

HRESULT CH264NaluParser::ParseVuiParameters(SPS_DATA* pSPS){

	HRESULT hr = S_OK;

	pSPS->sVuiParameters.aspect_ratio_info_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pSPS->sVuiParameters.aspect_ratio_info_present_flag){

		pSPS->sVuiParameters.aspect_ratio_idc = (SAMPLE_ASPECT_RATIO)m_cBitStream.GetBits(8);

		if(pSPS->sVuiParameters.aspect_ratio_idc == SAR_EXTENDED)
		{
			pSPS->sVuiParameters.sar_width = m_cBitStream.GetBits(16);
			pSPS->sVuiParameters.sar_height = m_cBitStream.GetBits(16);
		}
	}

	pSPS->sVuiParameters.overscan_info_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pSPS->sVuiParameters.overscan_info_present_flag)
		pSPS->sVuiParameters.overscan_appropriate_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	pSPS->sVuiParameters.video_signal_type_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pSPS->sVuiParameters.video_signal_type_present_flag){

		pSPS->sVuiParameters.video_format = (VUI_VIDEO_FORMAT)m_cBitStream.GetBits(3);
		pSPS->sVuiParameters.video_full_range_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
		pSPS->sVuiParameters.colour_description_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

		if(pSPS->sVuiParameters.colour_description_present_flag){

			pSPS->sVuiParameters.colour_primaries = (PRIMARIES_COLOUR)m_cBitStream.GetBits(8);
			pSPS->sVuiParameters.transfer_characteristics = (TRANSFER_CHARACTERISTICS)m_cBitStream.GetBits(8);
			pSPS->sVuiParameters.matrix_coefficients = (MATRIX_COEFFICIENTS)m_cBitStream.GetBits(8);
		}
	}

	pSPS->sVuiParameters.chroma_loc_info_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pSPS->sVuiParameters.chroma_loc_info_present_flag){

		pSPS->sVuiParameters.chroma_sample_loc_type_top_field = m_cBitStream.UGolomb();
		pSPS->sVuiParameters.chroma_sample_loc_type_bottom_field = m_cBitStream.UGolomb();
	}

	pSPS->sVuiParameters.timing_info_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pSPS->sVuiParameters.timing_info_present_flag){

		pSPS->sVuiParameters.num_units_in_tick = m_cBitStream.GetBits(32);
		pSPS->sVuiParameters.time_scale = m_cBitStream.GetBits(32);
		pSPS->sVuiParameters.fixed_frame_rate_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	}

	pSPS->sVuiParameters.nal_hrd_parameters_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pSPS->sVuiParameters.nal_hrd_parameters_present_flag){

		hrd_parameters();
	}

	pSPS->sVuiParameters.vcl_hrd_parameters_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pSPS->sVuiParameters.vcl_hrd_parameters_present_flag){

		hrd_parameters();
	}

	if(pSPS->sVuiParameters.nal_hrd_parameters_present_flag || pSPS->sVuiParameters.vcl_hrd_parameters_present_flag)
		pSPS->sVuiParameters.low_delay_hrd_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	pSPS->sVuiParameters.pic_struct_present_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
	pSPS->sVuiParameters.bitstream_restriction_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

	if(pSPS->sVuiParameters.bitstream_restriction_flag){

		pSPS->sVuiParameters.motion_vectors_over_pic_boundaries_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
		pSPS->sVuiParameters.max_bytes_per_pic_denom = m_cBitStream.UGolomb();
		pSPS->sVuiParameters.max_bits_per_mb_denom = m_cBitStream.UGolomb();
		pSPS->sVuiParameters.log2_max_mv_length_horizontal = m_cBitStream.UGolomb();
		pSPS->sVuiParameters.log2_max_mv_length_vertical = m_cBitStream.UGolomb();
		pSPS->sVuiParameters.num_reorder_frames = m_cBitStream.UGolomb();
		pSPS->sVuiParameters.max_dec_frame_buffering = m_cBitStream.UGolomb();
	}

	return hr;
}

HRESULT CH264NaluParser::ParsePredWeightTable(){

	HRESULT hr = S_OK;

	/*DWORD luma_log2_weight_denom =*/ m_cBitStream.UGolomb();

	if(m_Picture.sps.chroma_format_idc)
		/*DWORD chroma_log2_weight_denom =*/ m_cBitStream.UGolomb();

	DWORD dwNumRefIdx = m_Picture.slice.num_ref_idx_l0_active_minus1 + 1;

	for(UINT ui = 0; ui < dwNumRefIdx; ui++){

		BOOL luma_weight_l0_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

		if(luma_weight_l0_flag){

			/*DWORD luma_weight_l0_i =*/ m_cBitStream.SGolomb();
			/*DWORD luma_offset_l0_i =*/ m_cBitStream.SGolomb();
		}

		if(m_Picture.sps.chroma_format_idc){

			BOOL chroma_weight_l0_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

			if(chroma_weight_l0_flag){

				for(int j = 0; j < 2; j++){

					/*DWORD luma_weight_l0_i_j =*/ m_cBitStream.SGolomb();
					/*DWORD luma_offset_l0_i_j =*/ m_cBitStream.SGolomb();
				}
			}
		}
	}

	if(m_Picture.slice.slice_type == B_SLICE_TYPE || m_Picture.slice.slice_type == B_SLICE_TYPE2){

		dwNumRefIdx = m_Picture.slice.num_ref_idx_l1_active_minus1 + 1;

		for(UINT ui = 0; ui < dwNumRefIdx; ui++){

			BOOL luma_weight_l1_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

			if(luma_weight_l1_flag){

				/*DWORD luma_weight_l1_i =*/ m_cBitStream.SGolomb();
				/*DWORD luma_offset_l1_i =*/ m_cBitStream.SGolomb();
			}

			if(m_Picture.sps.chroma_format_idc){

				BOOL chroma_weight_l1_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

				if(chroma_weight_l1_flag){

					for(int j = 0; j < 2; j++){

						/*DWORD luma_weight_l1_i_j =*/ m_cBitStream.SGolomb();
						/*DWORD luma_offset_l1_i_j =*/ m_cBitStream.SGolomb();
					}
				}
			}
		}
	}

	return hr;
}

HRESULT CH264NaluParser::ParseDecRefPicMarking(){

	HRESULT hr = S_OK;

	PICTURE_MARKING* pPicMarking = &m_Picture.slice.PicMarking;

	if(m_Picture.NalUnitType == NAL_UNIT_CODED_SLICE_IDR){

		pPicMarking->no_output_of_prior_pics_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;
		pPicMarking->long_term_reference_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

		if(pPicMarking->long_term_reference_flag){

			pPicMarking->iNumOPMode = 1;
			pPicMarking->mmcoOPMode[0].mmcoOP = MMCO_OP_LONG;
			// todo : is it Long_term_pic_num or other
			//pPicMarking->mmcoOPMode[0].Long_term_pic_num = 0;
			//assert(FALSE);
		}

		pPicMarking->adaptive_ref_pic_marking_mode_flag = TRUE;
	}
	else{

		pPicMarking->adaptive_ref_pic_marking_mode_flag = m_cBitStream.GetBits(1) ? TRUE : FALSE;

		if(pPicMarking->adaptive_ref_pic_marking_mode_flag){

			MMCO_OP memory_management_control_operation;

			for(int i = 0; i < MMCO_OP_MAX; i++){

				pPicMarking->iNumOPMode++;

				memory_management_control_operation = (MMCO_OP)m_cBitStream.UGolomb();

				if(memory_management_control_operation == MMCO_OP_END){
					break;
				}

				if(memory_management_control_operation == MMCO_OP_SHORT2UNUSED){

					pPicMarking->mmcoOPMode[i].mmcoOP = MMCO_OP_SHORT2UNUSED;
					pPicMarking->mmcoOPMode[i].difference_of_pic_nums_minus1 = m_cBitStream.UGolomb();
				}
				else if(memory_management_control_operation == MMCO_OP_LONG2UNUSED){

					pPicMarking->mmcoOPMode[i].mmcoOP = MMCO_OP_LONG2UNUSED;
					pPicMarking->mmcoOPMode[i].Long_term_pic_num = m_cBitStream.UGolomb();
				}
				else if(memory_management_control_operation == MMCO_OP_SHORT2LONG){

					pPicMarking->mmcoOPMode[i].mmcoOP = MMCO_OP_SHORT2LONG;
					pPicMarking->mmcoOPMode[i].difference_of_pic_nums_minus1 = m_cBitStream.UGolomb();
					pPicMarking->mmcoOPMode[i].Long_term_frame_idx = m_cBitStream.UGolomb();
				}
				else if(memory_management_control_operation == MMCO_OP_SET_MAX_LONG){

					pPicMarking->mmcoOPMode[i].mmcoOP = MMCO_OP_SET_MAX_LONG;
					pPicMarking->mmcoOPMode[i].Max_long_term_frame_idx_plus1 = m_cBitStream.UGolomb();
				}
				else if(memory_management_control_operation == MMCO_OP_LONG){

					pPicMarking->mmcoOPMode[i].mmcoOP = MMCO_OP_LONG;
					pPicMarking->mmcoOPMode[i].Long_term_frame_idx = m_cBitStream.UGolomb();
				}
			}
		}
	}

	return hr;
}

void CH264NaluParser::hrd_parameters(){

	DWORD cpb_cnt_minus1 = m_cBitStream.UGolomb();
	/*DWORD bit_rate_scale =*/ m_cBitStream.GetBits(4);
	/*DWORD cpb_size_scale =*/ m_cBitStream.GetBits(4);

	for(DWORD SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; SchedSelIdx++){

		/*DWORD bit_rate_value_minus1 =*/ m_cBitStream.UGolomb();
		/*DWORD cpb_size_value_minus1 =*/ m_cBitStream.UGolomb();
		/*DWORD cbr_flag =*/ m_cBitStream.GetBits(1);
	}

	/*DWORD initial_cpb_removal_delay_length_minus1 =*/ m_cBitStream.GetBits(5);
	/*DWORD cpb_removal_delay_length_minus1 =*/ m_cBitStream.GetBits(5);
	/*DWORD dpb_output_delay_length_minus1 =*/ m_cBitStream.GetBits(5);
	/*DWORD time_offset_length =*/ m_cBitStream.GetBits(5);
}