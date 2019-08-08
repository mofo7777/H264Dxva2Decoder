//----------------------------------------------------------------------------------------------
// Dxva2Decoder.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

CDxva2Decoder::CDxva2Decoder(){

	memset(&m_pSurface9, 0, sizeof(m_pSurface9));
	memset(&m_ExecuteParams, 0, sizeof(DXVA2_DecodeExecuteParams));
	memset(m_BufferDesc, 0, 4 * sizeof(DXVA2_DecodeBufferDesc));
	memset(&m_H264PictureParams, 0, sizeof(DXVA_PicParams_H264));
	memset(&m_H264QuantaMatrix, 0, sizeof(DXVA_Qmatrix_H264));
	memset(m_H264SliceShort, 0, MAX_SUB_SLICE * sizeof(DXVA_Slice_H264_Short));

	m_pVideoDecoder = NULL;
	m_pDirect3DDeviceManager9 = NULL;
	m_hD3d9Device = NULL;
	m_pConfigs = NULL;
	m_gH264Vld = DXVA2_ModeH264_E;
	m_iPrevTopFieldOrderCount = 0;
	m_eNalUnitType = NAL_UNIT_UNSPEC_0;
	m_btNalRefIdc = 0x00;
	m_dwStatusReportFeedbackNumber = 1;
	memset(g_Dxva2SurfaceIndexV2, 0, sizeof(g_Dxva2SurfaceIndexV2));
}

HRESULT CDxva2Decoder::InitVideoDecoder(IDirect3DDeviceManager9* pDirect3DDeviceManager9, const DXVA2_VideoDesc* pDxva2Desc, const SPS_DATA& sps){

	HRESULT hr;
	IDirectXVideoDecoderService* pDecoderService = NULL;
	DXVA2_ConfigPictureDecode* pConfigs = NULL;
	UINT uiCount;
	UINT uiIndex = 0;

	IF_FAILED_RETURN(pDirect3DDeviceManager9 && pDxva2Desc ? S_OK : E_POINTER);
	IF_FAILED_RETURN(m_pDirect3DDeviceManager9 == NULL && m_hD3d9Device == NULL ? S_OK : E_UNEXPECTED);
	IF_FAILED_RETURN(m_pVideoDecoder == NULL && m_pSurface9[0] == NULL ? S_OK : E_UNEXPECTED);

	m_pDirect3DDeviceManager9 = pDirect3DDeviceManager9;
	m_pDirect3DDeviceManager9->AddRef();

	try{

		IF_FAILED_THROW(m_pDirect3DDeviceManager9->OpenDeviceHandle(&m_hD3d9Device));

		IF_FAILED_THROW(m_pDirect3DDeviceManager9->GetVideoService(m_hD3d9Device, IID_IDirectXVideoDecoderService, reinterpret_cast<void**>(&pDecoderService)));

		IF_FAILED_THROW(pDecoderService->GetDecoderConfigurations(m_gH264Vld, pDxva2Desc, NULL, &uiCount, &pConfigs));

		IF_FAILED_THROW((pConfigs == NULL || uiCount == 0) ? E_FAIL : S_OK);

		for(; uiIndex < uiCount; uiIndex++){

			if(pConfigs[uiIndex].ConfigBitstreamRaw == 2){

				// if ConfigBitstreamRaw == 2, we can use DXVA_Slice_H264_Short
				m_pConfigs = pConfigs;
				break;
			}
		}

		IF_FAILED_THROW(m_pConfigs == NULL ? E_FAIL : S_OK);

#ifdef USE_D3D_SURFACE_ALIGMENT
		IF_FAILED_THROW(pDecoderService->CreateSurface(D3DALIGN(pDxva2Desc->SampleWidth, 16), D3DALIGN(pDxva2Desc->SampleHeight, 16), NUM_DXVA2_SURFACE - 1,
			static_cast<D3DFORMAT>(D3DFMT_NV12), D3DPOOL_DEFAULT, 0, DXVA2_VideoDecoderRenderTarget, m_pSurface9, NULL));
#else
		IF_FAILED_THROW(pDecoderService->CreateSurface(pDxva2Desc->SampleWidth, pDxva2Desc->SampleHeight, NUM_DXVA2_SURFACE - 1,
			static_cast<D3DFORMAT>(D3DFMT_NV12), D3DPOOL_DEFAULT, 0, DXVA2_VideoDecoderRenderTarget, m_pSurface9, NULL));
#endif

		IF_FAILED_THROW(pDecoderService->CreateVideoDecoder(m_gH264Vld, pDxva2Desc, &m_pConfigs[uiIndex], m_pSurface9, NUM_DXVA2_SURFACE, &m_pVideoDecoder));

		InitDxva2Struct(sps);
	}
	catch(HRESULT){}

	SAFE_RELEASE(pDecoderService);

	return hr;
}

void CDxva2Decoder::OnRelease(){

	memset(&m_ExecuteParams, 0, sizeof(DXVA2_DecodeExecuteParams));
	memset(m_BufferDesc, 0, 4 * sizeof(DXVA2_DecodeBufferDesc));
	memset(&m_H264PictureParams, 0, sizeof(DXVA_PicParams_H264));
	memset(&m_H264QuantaMatrix, 0, sizeof(DXVA_Qmatrix_H264));
	memset(m_H264SliceShort, 0, MAX_SUB_SLICE * sizeof(DXVA_Slice_H264_Short));

	m_iPrevTopFieldOrderCount = 0;
	m_eNalUnitType = NAL_UNIT_UNSPEC_0;
	m_btNalRefIdc = 0x00;

	if(m_pDirect3DDeviceManager9 && m_hD3d9Device){

		LOG_HRESULT(m_pDirect3DDeviceManager9->CloseDeviceHandle(m_hD3d9Device));
		m_hD3d9Device = NULL;
	}

	for(int i = 0; i < NUM_DXVA2_SURFACE; i++){
		SAFE_RELEASE(m_pSurface9[i]);
	}

	if(m_pConfigs){
		CoTaskMemFree(m_pConfigs);
		m_pConfigs = NULL;
	}

	SAFE_RELEASE(m_pDirect3DDeviceManager9);
	SAFE_RELEASE(m_pVideoDecoder);
	m_dwStatusReportFeedbackNumber = 1;

	m_dqPoc.clear();
	m_dqPicturePresentation.clear();

	ResetAllSurfaceIndex();
}

void CDxva2Decoder::Reset(){

	m_iPrevTopFieldOrderCount = 0;
	m_eNalUnitType = NAL_UNIT_UNSPEC_0;
	m_btNalRefIdc = 0x00;
	m_dwStatusReportFeedbackNumber = 1;

	m_dqPoc.clear();
	m_dqPicturePresentation.clear();

	ResetAllSurfaceIndex();
}

HRESULT CDxva2Decoder::DecodeFrame(CMFBuffer& cMFNaluBuffer, const PICTURE_INFO& Picture, const LONGLONG& llTime, const int iSubSliceCount){

	HRESULT hr = S_OK;
	IDirect3DDevice9* pDevice = NULL;
	void* pBuffer = NULL;
	UINT uiSize = 0;
	static BOOL bFirst = TRUE;

	assert(m_pVideoDecoder != NULL);

	DWORD dwCurPictureId = GetFreeSurfaceIndex();
	IF_FAILED_RETURN(dwCurPictureId == (DWORD)-1 ? E_FAIL : S_OK);

	try{

		IF_FAILED_THROW(m_pDirect3DDeviceManager9->TestDevice(m_hD3d9Device));
		IF_FAILED_THROW(m_pDirect3DDeviceManager9->LockDevice(m_hD3d9Device, &pDevice, TRUE));

		do{

			hr = m_pVideoDecoder->BeginFrame(m_pSurface9[dwCurPictureId], NULL);
			Sleep(1);
		}
		while(hr == E_PENDING);

		IF_FAILED_THROW(hr);

		// Picture
		InitPictureParams(dwCurPictureId, Picture);
		HandlePOC(dwCurPictureId, Picture, llTime);

		IF_FAILED_THROW(m_pVideoDecoder->GetBuffer(DXVA2_PictureParametersBufferType, &pBuffer, &uiSize));
		assert(sizeof(DXVA_PicParams_H264) <= uiSize);
		memcpy(pBuffer, &m_H264PictureParams, sizeof(DXVA_PicParams_H264));
		IF_FAILED_THROW(m_pVideoDecoder->ReleaseBuffer(DXVA2_PictureParametersBufferType));

		// QuantaMatrix
		IF_FAILED_THROW(m_pVideoDecoder->GetBuffer(DXVA2_InverseQuantizationMatrixBufferType, &pBuffer, &uiSize));
		assert(sizeof(DXVA_Qmatrix_H264) <= uiSize);
		memcpy(pBuffer, &m_H264QuantaMatrix, sizeof(DXVA_Qmatrix_H264));
		IF_FAILED_THROW(m_pVideoDecoder->ReleaseBuffer(DXVA2_InverseQuantizationMatrixBufferType));

		// BitStream
		IF_FAILED_THROW(m_pVideoDecoder->GetBuffer(DXVA2_BitStreamDateBufferType, &pBuffer, &uiSize));
		// todo : if iSubSliceCount > 1, perhaps we need to do AddNalUnitBufferPadding for each sub-slices
		IF_FAILED_THROW(AddNalUnitBufferPadding(cMFNaluBuffer, uiSize));
		assert(cMFNaluBuffer.GetBufferSize() <= uiSize);
		memcpy(pBuffer, cMFNaluBuffer.GetStartBuffer(), cMFNaluBuffer.GetBufferSize());
		IF_FAILED_THROW(m_pVideoDecoder->ReleaseBuffer(DXVA2_BitStreamDateBufferType));

		// Slices
		IF_FAILED_THROW(m_pVideoDecoder->GetBuffer(DXVA2_SliceControlBufferType, &pBuffer, &uiSize));
		assert(iSubSliceCount * sizeof(DXVA_Slice_H264_Short) <= uiSize);
		memcpy(pBuffer, m_H264SliceShort, iSubSliceCount * sizeof(DXVA_Slice_H264_Short));
		IF_FAILED_THROW(m_pVideoDecoder->ReleaseBuffer(DXVA2_SliceControlBufferType));

		// CompBuffers
		// Allready set
		//--> m_BufferDesc[0].DataSize = sizeof(DXVA_PicParams_H264);
		//--> m_BufferDesc[1].DataSize = sizeof(DXVA_Qmatrix_H264);
		m_BufferDesc[2].DataSize = cMFNaluBuffer.GetBufferSize();
		m_BufferDesc[2].NumMBsInBuffer = (Picture.sps.pic_width_in_mbs_minus1 + 1) * (Picture.sps.pic_height_in_map_units_minus1 + 1);
		m_BufferDesc[3].DataSize = iSubSliceCount * sizeof(DXVA_Slice_H264_Short);
		m_BufferDesc[3].NumMBsInBuffer = m_BufferDesc[2].NumMBsInBuffer;

		IF_FAILED_THROW(m_pVideoDecoder->Execute(&m_ExecuteParams));

		IF_FAILED_THROW(m_pVideoDecoder->EndFrame(NULL));

		IF_FAILED_THROW(m_pDirect3DDeviceManager9->UnlockDevice(m_hD3d9Device, FALSE));
	}
	catch(HRESULT){}

	SAFE_RELEASE(pDevice);

	return hr;
}

BOOL CDxva2Decoder::CheckFrame(SAMPLE_PRESENTATION& SamplPresentation){

	BOOL bHasPicture = FALSE;

	for(deque<PICTURE_PRESENTATION>::const_iterator it = m_dqPicturePresentation.begin(); it != m_dqPicturePresentation.end(); ++it){

		if(it->TopFieldOrderCnt == 0 || it->TopFieldOrderCnt == (m_iPrevTopFieldOrderCount + 2) || it->TopFieldOrderCnt == (m_iPrevTopFieldOrderCount + 1)){

			SamplPresentation.dwDXVA2Index = it->dwDXVA2Index;
			m_iPrevTopFieldOrderCount = it->TopFieldOrderCnt;
			SamplPresentation.llTime = it->llTime;
			m_dqPicturePresentation.erase(it);
			bHasPicture = TRUE;
			break;
		}
	}

	// Use assert to check m_dqPicturePresentation size
	//assert(m_dqPicturePresentation.size() < NUM_DXVA2_SURFACE);

	// Check past frames, sometimes needed...
	if(bHasPicture)
		ErasePastFrames(SamplPresentation.llTime);

	return bHasPicture;
}

HRESULT CDxva2Decoder::AddSliceShortInfo(const int iSubSliceCount, const DWORD dwSize, const BOOL b4BytesStartCode){

	HRESULT hr;
	IF_FAILED_RETURN(iSubSliceCount > 0 && iSubSliceCount < MAX_SUB_SLICE ? S_OK : E_INVALIDARG);

	const int iIndex = iSubSliceCount - 1;

	m_H264SliceShort[iIndex].SliceBytesInBuffer = dwSize;
	m_H264SliceShort[iIndex].BSNALunitDataLocation = 0;
	m_H264SliceShort[iIndex].wBadSliceChopping = 0;

	if(b4BytesStartCode){

		m_H264SliceShort[iIndex].SliceBytesInBuffer--;
		m_H264SliceShort[iIndex].BSNALunitDataLocation++;
	}

	for(int i = 1; i < iSubSliceCount; i++)
		m_H264SliceShort[iIndex].BSNALunitDataLocation += (m_H264SliceShort[i - 1].SliceBytesInBuffer + (b4BytesStartCode ? 1 : 0));

	return hr;
}

HRESULT CDxva2Decoder::InitDecoderService(IDirectXVideoDecoderService* pDecoderService){

	HRESULT hr;
	GUID* pGuids = NULL;
	D3DFORMAT* pFormats = NULL;
	UINT uiCount;
	BOOL bDecoderH264 = FALSE;

	IF_FAILED_RETURN(pDecoderService ? S_OK : E_POINTER);

	try{

		IF_FAILED_THROW(pDecoderService->GetDecoderDeviceGuids(&uiCount, &pGuids));

		for(UINT ui = 0; ui < uiCount; ui++){

			if(pGuids[ui] == m_gH264Vld){
				bDecoderH264 = TRUE;
				break;
			}
		}

		IF_FAILED_THROW(bDecoderH264 ? S_OK : E_FAIL);
		bDecoderH264 = FALSE;

		IF_FAILED_THROW(pDecoderService->GetDecoderRenderTargets(m_gH264Vld, &uiCount, &pFormats));

		for(UINT ui = 0; ui < uiCount; ui++){

			if(pFormats[ui] == (D3DFORMAT)D3DFMT_NV12){
				bDecoderH264 = TRUE;
				break;
			}
		}

		IF_FAILED_THROW(bDecoderH264 ? S_OK : E_FAIL);
	}
	catch(HRESULT){}

	CoTaskMemFree(pFormats);
	CoTaskMemFree(pGuids);

	return hr;
}

void CDxva2Decoder::InitDxva2Struct(const SPS_DATA& sps){

	memset(&m_ExecuteParams, 0, sizeof(DXVA2_DecodeExecuteParams));
	memset(m_BufferDesc, 0, 4 * sizeof(DXVA2_DecodeBufferDesc));
	memset(&m_H264PictureParams, 0, sizeof(DXVA_PicParams_H264));
	InitQuantaMatrixParams(sps);
	memset(m_H264SliceShort, 0, MAX_SUB_SLICE * sizeof(DXVA_Slice_H264_Short));

	m_BufferDesc[0].CompressedBufferType = DXVA2_PictureParametersBufferType;
	m_BufferDesc[0].DataSize = sizeof(DXVA_PicParams_H264);

	m_BufferDesc[1].CompressedBufferType = DXVA2_InverseQuantizationMatrixBufferType;
	m_BufferDesc[1].DataSize = sizeof(DXVA_Qmatrix_H264);

	m_BufferDesc[2].CompressedBufferType = DXVA2_BitStreamDateBufferType;

	m_BufferDesc[3].CompressedBufferType = DXVA2_SliceControlBufferType;

	m_ExecuteParams.NumCompBuffers = 4;
	m_ExecuteParams.pCompressedBuffers = m_BufferDesc;
}

void CDxva2Decoder::InitPictureParams(const DWORD dwIndex, const PICTURE_INFO& Picture){

	int iIndex = 0;

	if(m_eNalUnitType == NAL_UNIT_CODED_SLICE_IDR){

		for(int i = 0; i < NUM_DXVA2_SURFACE; i++){
			g_Dxva2SurfaceIndexV2[i].bUsedByDecoder = FALSE;
			g_Dxva2SurfaceIndexV2[i].bNalRef = FALSE;
		}

		m_dqPoc.clear();
	}

	m_H264PictureParams.wFrameWidthInMbsMinus1 = (USHORT)Picture.sps.pic_width_in_mbs_minus1;
	m_H264PictureParams.wFrameHeightInMbsMinus1 = (USHORT)Picture.sps.pic_height_in_map_units_minus1;
	m_H264PictureParams.CurrPic.Index7Bits = dwIndex;
	m_H264PictureParams.CurrPic.AssociatedFlag = Picture.pps.bottom_field_pic_order_in_frame_present_flag;
	m_H264PictureParams.num_ref_frames = (UCHAR)Picture.sps.num_ref_frames;

	//m_H264PictureParams.wBitFields = 0;
	//m_H264PictureParams.field_pic_flag = 0;
	//m_H264PictureParams.MbaffFrameFlag = 0;
	m_H264PictureParams.residual_colour_transform_flag = Picture.sps.separate_colour_plane_flag;
	//m_H264PictureParams.sp_for_switch_flag = 0;
	m_H264PictureParams.chroma_format_idc = Picture.sps.chroma_format_idc;
	m_H264PictureParams.RefPicFlag = m_btNalRefIdc != 0;
	m_H264PictureParams.constrained_intra_pred_flag = Picture.pps.constrained_intra_pred_flag;
	m_H264PictureParams.weighted_pred_flag = Picture.pps.weighted_pred_flag;
	m_H264PictureParams.weighted_bipred_idc = Picture.pps.weighted_bipred_idc;
	m_H264PictureParams.MbsConsecutiveFlag = 1;
	m_H264PictureParams.frame_mbs_only_flag = Picture.sps.frame_mbs_only_flag;
	m_H264PictureParams.transform_8x8_mode_flag = Picture.pps.transform_8x8_mode_flag;
	m_H264PictureParams.MinLumaBipredSize8x8Flag = Picture.sps.level_idc >= 31;
	m_H264PictureParams.IntraPicFlag = (Picture.slice.slice_type == I_SLICE_TYPE || Picture.slice.slice_type == I_SLICE_TYPE2);

	m_H264PictureParams.bit_depth_luma_minus8 = (UCHAR)Picture.sps.bit_depth_luma_minus8;
	m_H264PictureParams.bit_depth_chroma_minus8 = (UCHAR)Picture.sps.bit_depth_chroma_minus8;
	m_H264PictureParams.Reserved16Bits = 3;
	m_H264PictureParams.StatusReportFeedbackNumber = m_dwStatusReportFeedbackNumber++;

	for(int i = 0; i < 16; i++){

		m_H264PictureParams.RefFrameList[i].bPicEntry = 0xff;
		m_H264PictureParams.FieldOrderCntList[i][0] = m_H264PictureParams.FieldOrderCntList[i][1] = 0;
		m_H264PictureParams.FrameNumList[i] = 0;
	}

	m_H264PictureParams.CurrFieldOrderCnt[1] = m_H264PictureParams.CurrFieldOrderCnt[0] = Picture.slice.TopFieldOrderCnt;
	m_H264PictureParams.UsedForReferenceFlags = 0;

	for(auto it = m_dqPoc.begin(); it != m_dqPoc.end() && iIndex < 16; iIndex++, ++it){

		m_H264PictureParams.FrameNumList[iIndex] = it->usFrameList;

		m_H264PictureParams.RefFrameList[iIndex].Index7Bits = it->bRefFrameList;
		m_H264PictureParams.RefFrameList[iIndex].AssociatedFlag = 0;
		m_H264PictureParams.FieldOrderCntList[iIndex][0] = m_H264PictureParams.FieldOrderCntList[iIndex][1] = it->TopFieldOrderCnt;
		m_H264PictureParams.UsedForReferenceFlags |= 1 << (2 * iIndex);
		m_H264PictureParams.UsedForReferenceFlags |= 1 << (2 * iIndex + 1);
	}

	m_H264PictureParams.pic_init_qs_minus26 = (CHAR)Picture.pps.pic_init_qs_minus26;
	m_H264PictureParams.chroma_qp_index_offset = (CHAR)Picture.pps.chroma_qp_index_offset[0];
	m_H264PictureParams.second_chroma_qp_index_offset = (CHAR)Picture.pps.chroma_qp_index_offset[1];
	m_H264PictureParams.ContinuationFlag = 1;
	m_H264PictureParams.pic_init_qp_minus26 = (CHAR)Picture.pps.pic_init_qp_minus26;
	m_H264PictureParams.num_ref_idx_l0_active_minus1 = (UCHAR)Picture.pps.num_ref_idx_l0_active_minus1;
	m_H264PictureParams.num_ref_idx_l1_active_minus1 = (UCHAR)Picture.pps.num_ref_idx_l1_active_minus1;
	//m_H264PictureParams.Reserved8BitsA = 0;
	//m_H264PictureParams.NonExistingFrameFlags = 0;
	m_H264PictureParams.frame_num = Picture.slice.frame_num;
	m_H264PictureParams.log2_max_frame_num_minus4 = (UCHAR)Picture.sps.log2_max_frame_num_minus4;
	m_H264PictureParams.pic_order_cnt_type = (UCHAR)Picture.sps.pic_order_cnt_type;
	m_H264PictureParams.log2_max_pic_order_cnt_lsb_minus4 = (UCHAR)Picture.sps.log2_max_pic_order_cnt_lsb_minus4;
	//m_H264PictureParams.delta_pic_order_always_zero_flag = 0;
	m_H264PictureParams.direct_8x8_inference_flag = (UCHAR)Picture.sps.direct_8x8_inference_flag;
	m_H264PictureParams.entropy_coding_mode_flag = (UCHAR)Picture.pps.entropy_coding_mode_flag;
	m_H264PictureParams.pic_order_present_flag = (UCHAR)Picture.pps.bottom_field_pic_order_in_frame_present_flag;
	m_H264PictureParams.num_slice_groups_minus1 = (UCHAR)Picture.pps.num_slice_groups_minus1;
	//m_H264PictureParams.slice_group_map_type = 0;
	m_H264PictureParams.deblocking_filter_control_present_flag = (UCHAR)Picture.pps.deblocking_filter_control_present_flag;
	m_H264PictureParams.redundant_pic_cnt_present_flag = (UCHAR)Picture.pps.redundant_pic_cnt_present_flag;
	//m_H264PictureParams.Reserved8BitsB = 0;
	m_H264PictureParams.slice_group_change_rate_minus1 = (USHORT)Picture.pps.num_slice_groups_minus1;

	if(m_dwStatusReportFeedbackNumber == ULONG_MAX){
		m_dwStatusReportFeedbackNumber = 1;
	}
}

void CDxva2Decoder::InitQuantaMatrixParams(const SPS_DATA& sps){

	if(sps.bHasCustomScalingList == FALSE){

		memset(&m_H264QuantaMatrix, 16, sizeof(DXVA_Qmatrix_H264));
	}
	else{

		memcpy(m_H264QuantaMatrix.bScalingLists4x4, sps.ScalingList4x4, sizeof(m_H264QuantaMatrix.bScalingLists4x4));
		memcpy(m_H264QuantaMatrix.bScalingLists8x8, sps.ScalingList8x8, sizeof(m_H264QuantaMatrix.bScalingLists8x8));
	}
}

HRESULT CDxva2Decoder::AddNalUnitBufferPadding(CMFBuffer& cMFNaluBuffer, const UINT uiSize){

	HRESULT hr;

	UINT uiPadding = MIN(128 - (cMFNaluBuffer.GetBufferSize() & 127), uiSize);

	IF_FAILED_RETURN(cMFNaluBuffer.Reserve(uiPadding));
	memset(cMFNaluBuffer.GetReadStartBuffer(), 0, uiPadding);
	IF_FAILED_RETURN(cMFNaluBuffer.SetEndPosition(uiPadding));

	return hr;
}

void CDxva2Decoder::HandlePOC(const DWORD dwIndex, const PICTURE_INFO& Picture, const LONGLONG& llTime){

	if(m_btNalRefIdc && Picture.slice.PicMarking.adaptive_ref_pic_marking_mode_flag == FALSE && m_dqPoc.size() >= Picture.sps.num_ref_frames){

		if(m_dqPoc.size() != 0){

			UCHAR uc = m_dqPoc.back().bRefFrameList;
			g_Dxva2SurfaceIndexV2[uc].bUsedByDecoder = FALSE;
			g_Dxva2SurfaceIndexV2[uc].bNalRef = FALSE;
		}

		m_dqPoc.pop_back();
	}

	if(m_btNalRefIdc){

		POC NewPoc;

		NewPoc.usFrameList = Picture.slice.frame_num;
		NewPoc.bRefFrameList = (UCHAR)dwIndex;
		NewPoc.TopFieldOrderCnt = Picture.slice.TopFieldOrderCnt;
		NewPoc.usSliceType = Picture.slice.slice_type;
		NewPoc.bLongRef = FALSE;

		m_dqPoc.push_front(NewPoc);

		for(int i = 0; i < NUM_DXVA2_SURFACE; i++){

			if(g_Dxva2SurfaceIndexV2[i].bUsedByDecoder && g_Dxva2SurfaceIndexV2[i].bNalRef == FALSE)
				g_Dxva2SurfaceIndexV2[i].bUsedByDecoder = FALSE;
		}
	}

	g_Dxva2SurfaceIndexV2[dwIndex].bUsedByDecoder = TRUE;
	g_Dxva2SurfaceIndexV2[dwIndex].bNalRef = m_btNalRefIdc != 0x00;

	PICTURE_PRESENTATION pp;
	pp.TopFieldOrderCnt = Picture.slice.TopFieldOrderCnt;
	pp.dwDXVA2Index = dwIndex;
	pp.SliceType = (SLICE_TYPE)Picture.slice.slice_type;
	pp.llTime = llTime;

	if(Picture.slice.TopFieldOrderCnt != 0)
		m_dqPicturePresentation.push_front(pp);
	else
		m_dqPicturePresentation.push_back(pp);

	if(!m_btNalRefIdc)
		return;

	UINT uiCurIndex;
	MMCO_OP op;

	//todo 8.2.4 list reordering
	for(int iIndex = 0; iIndex < Picture.slice.PicMarking.iNumOPMode; iIndex++){

		op = Picture.slice.PicMarking.mmcoOPMode[iIndex].mmcoOP;

		if(op == MMCO_OP_SHORT2UNUSED){

			uiCurIndex = Picture.slice.PicMarking.mmcoOPMode[iIndex].difference_of_pic_nums_minus1 + 1;

			if(uiCurIndex >= m_dqPoc.size())
				uiCurIndex = (UINT)(m_dqPoc.size() - 1);

			if(uiCurIndex < m_dqPoc.size()){

				auto it = m_dqPoc.begin() + uiCurIndex;

				g_Dxva2SurfaceIndexV2[it->bRefFrameList].bUsedByDecoder = FALSE;
				g_Dxva2SurfaceIndexV2[it->bRefFrameList].bNalRef = FALSE;

				m_dqPoc.erase(it);
			}
		}
		else if(op == MMCO_OP_SHORT2LONG){

			for(auto it = m_dqPoc.begin(); it != m_dqPoc.end(); ++it){

				if(it->TopFieldOrderCnt == Picture.slice.PicMarking.mmcoOPMode[iIndex].Long_term_frame_idx){

					it->bLongRef = TRUE;
					break;
				}
			}
		}
	}
}

void CDxva2Decoder::ErasePastFrames(const LONGLONG llTime){

	for(deque<PICTURE_PRESENTATION>::const_iterator it = m_dqPicturePresentation.begin(); it != m_dqPicturePresentation.end()){

		if(it->llTime <= llTime)
			it = m_dqPicturePresentation.erase(it);
		else
			++it;
	}
}

void CDxva2Decoder::FreeSurfaceIndexRenderer(const DWORD dwSurfaceIndex){

	assert(dwSurfaceIndex >= 0 && dwSurfaceIndex < NUM_DXVA2_SURFACE);

	AutoLock lock(m_CriticSection);

	g_Dxva2SurfaceIndexV2[dwSurfaceIndex].bUsedByRenderer = FALSE;
}

DWORD CDxva2Decoder::GetFreeSurfaceIndex(){

	DWORD dwSurfaceIndex = (DWORD)-1;

	AutoLock lock(m_CriticSection);

	for(DWORD dw = 0; dw < NUM_DXVA2_SURFACE; dw++){

		if(g_Dxva2SurfaceIndexV2[dw].bUsedByDecoder == FALSE && g_Dxva2SurfaceIndexV2[dw].bUsedByRenderer == FALSE){

			//g_Dxva2SurfaceIndexV2[dw].bUsedByDecoder = TRUE;
			g_Dxva2SurfaceIndexV2[dw].bUsedByRenderer = TRUE;
			dwSurfaceIndex = dw;
			break;
		}
	}

	return dwSurfaceIndex;
}

void CDxva2Decoder::ResetAllSurfaceIndex(){

	AutoLock lock(m_CriticSection);
	memset(g_Dxva2SurfaceIndexV2, 0, sizeof(g_Dxva2SurfaceIndexV2));
}
