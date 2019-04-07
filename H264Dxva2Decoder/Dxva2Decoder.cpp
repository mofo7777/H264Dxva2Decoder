//----------------------------------------------------------------------------------------------
// Dxva2Decoder.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

CDxva2Decoder::CDxva2Decoder(){

	memset(&m_Dxva2Desc, 0, sizeof(DXVA2_VideoDesc));

	for(int i = 0; i < NUM_DXVA2_SURFACE; i++){
		m_pSurface9[i] = NULL;
	}

	memset(&m_ExecuteParams, 0, sizeof(DXVA2_DecodeExecuteParams));
	memset(m_BufferDesc, 0, 4 * sizeof(DXVA2_DecodeBufferDesc));
	memset(&m_H264PictureParams, 0, sizeof(DXVA_PicParams_H264));
	memset(&m_H264QuantaMatrix, 0, sizeof(DXVA_Qmatrix_H264));
	memset(m_H264SliceShort, 0, MAX_SUB_SLICE * sizeof(DXVA_Slice_H264_Short));

	m_pD3D9Ex = NULL;
	m_pDevice9Ex = NULL;
	m_pResetToken = 0;
	m_pDXVAManager = NULL;
	m_pVideoDecoder = NULL;
	m_hD3d9Device = NULL;
	m_pDecoderService = NULL;
	m_pConfigs = NULL;
	m_gH264Vld = DXVA2_ModeH264_E;
	m_dwCurPictureId = 0;
	m_pDXVAVP = NULL;
	m_iPrevTopFieldOrderCount = 0;
	m_eNalUnitType = NAL_UNIT_UNSPEC_0;
	m_btNalRefIdc = 0x00;
	m_dwPicturePresent = 0;
	m_dwPauseDuration = 40;
	m_dwStatusReportFeedbackNumber = 1;

#ifdef USE_DXVA2_SURFACE_INDEX
	for(int i = 0; i < NUM_DXVA2_SURFACE; i++){
		g_Dxva2SurfaceIndex[i].bUsed = FALSE;
		g_Dxva2SurfaceIndex[i].bNalRef = FALSE;
	}
#endif
}

HRESULT CDxva2Decoder::InitDXVA2(const HWND hWnd, const SPS_DATA& sps, const UINT uiWidth, const UINT uiHeight, const UINT uiNumerator, const UINT uiDenominator){

	HRESULT hr;

	assert(m_pD3D9Ex == NULL && m_pDevice9Ex == NULL);

	D3DPRESENT_PARAMETERS d3dpp;

	IF_FAILED_RETURN(Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D9Ex));

	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferWidth = uiWidth;
	d3dpp.BackBufferHeight = uiHeight;
	d3dpp.Windowed = TRUE;
	d3dpp.Flags = D3DPRESENTFLAG_VIDEO;

	IF_FAILED_RETURN(m_pD3D9Ex->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, NULL, &m_pDevice9Ex));

	IF_FAILED_RETURN(m_pDevice9Ex->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
	IF_FAILED_RETURN(m_pDevice9Ex->SetRenderState(D3DRS_ZENABLE, FALSE));
	IF_FAILED_RETURN(m_pDevice9Ex->SetRenderState(D3DRS_LIGHTING, FALSE));

	DXVA2_Frequency Dxva2Freq;
	Dxva2Freq.Numerator = uiNumerator;
	Dxva2Freq.Denominator = uiDenominator;

	m_dwPauseDuration = (1000 / (uiNumerator / uiDenominator)) - 4;

	m_Dxva2Desc.SampleWidth = uiWidth;
	m_Dxva2Desc.SampleHeight = uiHeight;

	m_Dxva2Desc.SampleFormat.SampleFormat = MFVideoInterlace_Progressive;

	m_Dxva2Desc.Format = static_cast<D3DFORMAT>(D3DFMT_NV12);
	m_Dxva2Desc.InputSampleFreq = Dxva2Freq;
	m_Dxva2Desc.OutputFrameFreq = Dxva2Freq;

	IF_FAILED_RETURN(DXVA2CreateDirect3DDeviceManager9(&m_pResetToken, &m_pDXVAManager));

	IF_FAILED_RETURN(m_pDXVAManager->ResetDevice(m_pDevice9Ex, m_pResetToken));

	IF_FAILED_RETURN(m_pDXVAManager->OpenDeviceHandle(&m_hD3d9Device));

	IF_FAILED_RETURN(m_pDXVAManager->GetVideoService(m_hD3d9Device, IID_IDirectXVideoDecoderService, reinterpret_cast<void**>(&m_pDecoderService)));

	IF_FAILED_RETURN(InitDecoderService());

	IF_FAILED_RETURN(InitVideoDecoder(sps));

	IF_FAILED_RETURN(InitVideoProcessor());

	return hr;
}

void CDxva2Decoder::OnRelease(){

	if(m_pDevice9Ex == NULL)
		return;

	TRACE((L"PicturePresent = %u - Poc = %d - PicturePresentation = %d\r\n", m_dwPicturePresent, m_dqPoc.size(), m_dqPicturePresentation.size()));

	memset(&m_Dxva2Desc, 0, sizeof(DXVA2_VideoDesc));
	memset(&m_ExecuteParams, 0, sizeof(DXVA2_DecodeExecuteParams));
	memset(m_BufferDesc, 0, 4 * sizeof(DXVA2_DecodeBufferDesc));
	memset(&m_H264PictureParams, 0, sizeof(DXVA_PicParams_H264));
	memset(&m_H264QuantaMatrix, 0, sizeof(DXVA_Qmatrix_H264));
	memset(m_H264SliceShort, 0, MAX_SUB_SLICE * sizeof(DXVA_Slice_H264_Short));

	m_dwCurPictureId = 0;
	m_iPrevTopFieldOrderCount = 0;
	m_eNalUnitType = NAL_UNIT_UNSPEC_0;
	m_btNalRefIdc = 0x00;

	if(m_pDXVAManager && m_hD3d9Device){

		LOG_HRESULT(m_pDXVAManager->CloseDeviceHandle(m_hD3d9Device));
		m_hD3d9Device = NULL;
	}

	for(int i = 0; i < NUM_DXVA2_SURFACE; i++){
		SAFE_RELEASE(m_pSurface9[i]);
	}

	if(m_pConfigs){
		CoTaskMemFree(m_pConfigs);
		m_pConfigs = NULL;
	}

	SAFE_RELEASE(m_pDecoderService);
	SAFE_RELEASE(m_pDXVAManager);
	SAFE_RELEASE(m_pD3D9Ex);
	SAFE_RELEASE(m_pVideoDecoder);
	m_pResetToken = 0;
	m_dwStatusReportFeedbackNumber = 1;
	m_dwPicturePresent = 0;

	SAFE_RELEASE(m_pDXVAVP);

	m_dqPoc.clear();
	m_dqPicturePresentation.clear();

#ifdef USE_DXVA2_SURFACE_INDEX
	for(int i = 0; i < NUM_DXVA2_SURFACE; i++){
		g_Dxva2SurfaceIndex[i].bUsed = FALSE;
		g_Dxva2SurfaceIndex[i].bNalRef = FALSE;
	}
#endif

	if(m_pDevice9Ex){

		ULONG ulCount = m_pDevice9Ex->Release();
		m_pDevice9Ex = NULL;
		assert(ulCount == 0);
	}
}

void CDxva2Decoder::Reset(){

	m_dwCurPictureId = 0;
	m_iPrevTopFieldOrderCount = 0;
	m_eNalUnitType = NAL_UNIT_UNSPEC_0;
	m_btNalRefIdc = 0x00;
	m_dwStatusReportFeedbackNumber = 1;
	m_dwPicturePresent = 0;

	m_dqPoc.clear();
	m_dqPicturePresentation.clear();

#ifdef USE_DXVA2_SURFACE_INDEX
	for(int i = 0; i < NUM_DXVA2_SURFACE; i++){
		g_Dxva2SurfaceIndex[i].bUsed = FALSE;
		g_Dxva2SurfaceIndex[i].bNalRef = FALSE;
	}
#endif
}

HRESULT CDxva2Decoder::DecodeFrame(CMFBuffer& cMFNaluBuffer, const PICTURE_INFO& Picture, const LONGLONG& llTime, const int iSubSliceCount){

	HRESULT hr = S_OK;
	IDirect3DDevice9* pDevice = NULL;
	void* pBuffer = NULL;
	UINT uiSize = 0;
	static BOOL bFirst = TRUE;

	assert(m_pVideoDecoder != NULL);

#ifndef USE_DXVA2_SURFACE_INDEX
	if(m_dwCurPictureId >= NUM_DXVA2_SURFACE){
		m_dwCurPictureId = 0;
	}
#else
	m_dwCurPictureId = (DWORD)-1;
	for(DWORD i = 0; i < NUM_DXVA2_SURFACE; i++){
		if(g_Dxva2SurfaceIndex[i].bUsed == FALSE){
			m_dwCurPictureId = i;
			break;
		}
	}
	//TRACE((L"CurPictureId = %lu", m_dwCurPictureId));
	assert(m_dwCurPictureId != (DWORD)-1);
#endif

	try{

		IF_FAILED_THROW(m_pDXVAManager->TestDevice(m_hD3d9Device));
		IF_FAILED_THROW(m_pDXVAManager->LockDevice(m_hD3d9Device, &pDevice, TRUE));

		do{

			hr = m_pVideoDecoder->BeginFrame(m_pSurface9[m_dwCurPictureId], NULL);
			Sleep(1);
		}
		while(hr == E_PENDING);

		IF_FAILED_THROW(hr);

		// Picture
		InitPictureParams(m_dwCurPictureId, Picture);
		HandlePOC(m_dwCurPictureId, Picture, llTime);
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

		IF_FAILED_THROW(m_pDXVAManager->UnlockDevice(m_hD3d9Device, FALSE));

#ifndef USE_DXVA2_SURFACE_INDEX
		m_dwCurPictureId++;
#endif
	}
	catch(HRESULT){}

	SAFE_RELEASE(pDevice);

	return hr;
}

HRESULT CDxva2Decoder::RenderFrame(){

	HRESULT hr = S_OK;
	IDirect3DSurface9* pRT = NULL;
	DXVAHD_STREAM_DATA stream_data;
	DWORD dwSurfaceIndex = 0;
	BOOL bHasPicture = FALSE;
	LONGLONG llTime = 0;
	ZeroMemory(&stream_data, sizeof(DXVAHD_STREAM_DATA));

	assert(m_pDXVAVP);

	for(deque<PICTURE_PRESENTATION>::const_iterator it = m_dqPicturePresentation.begin(); it != m_dqPicturePresentation.end(); ++it){

		if(it->TopFieldOrderCnt == 0 || it->TopFieldOrderCnt == (m_iPrevTopFieldOrderCount + 2) || it->TopFieldOrderCnt == (m_iPrevTopFieldOrderCount + 1)){

			dwSurfaceIndex = it->dwDXVA2Index;
			m_iPrevTopFieldOrderCount = it->TopFieldOrderCnt;
			llTime = it->llTime;
			m_dqPicturePresentation.erase(it);
			bHasPicture = TRUE;
			break;
		}
	}

	// Use assert to check m_dqPicturePresentation size
	//assert(m_dqPicturePresentation.size() < NUM_DXVA2_SURFACE);

	if(bHasPicture == FALSE){
		return hr;
	}

	// Check past frames, sometimes needed...
	ErasePastFrames(llTime);

	m_dwPicturePresent++;

	try{

		stream_data.Enable = TRUE;
		stream_data.OutputIndex = 0;
		stream_data.InputFrameOrField = dwSurfaceIndex;
		stream_data.pInputSurface = m_pSurface9[dwSurfaceIndex];

		IF_FAILED_THROW(m_pDevice9Ex->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pRT));

		IF_FAILED_THROW(m_pDXVAVP->VideoProcessBltHD(pRT, dwSurfaceIndex, 1, &stream_data));

#ifdef HANDLE_DIRECTX_ERROR_UNDOCUMENTED
		hr = m_pDevice9Ex->Present(NULL, NULL, NULL, NULL);

		// Present can return 0x88760872, i don't know what it is, it is OK to continue.
		// Move window border very quickly and continually to get this error
		if(hr == DIRECTX_ERROR_UNDOCUMENTED)
			hr = S_OK;
		else
			IF_FAILED_THROW(hr);
#else
		IF_FAILED_THROW(m_pDevice9Ex->Present(NULL, NULL, NULL, NULL));
#endif
	}
	catch(HRESULT){}

	SAFE_RELEASE(pRT);

	Sleep(m_dwPauseDuration);

	return hr;
}

HRESULT CDxva2Decoder::RenderBlackFrame(){

	HRESULT hr;
	IDirect3DSurface9* pRT = NULL;

	IF_FAILED_RETURN(m_pDevice9Ex ? S_OK : E_UNEXPECTED);

	try{

		// Clear the render target seems not to work as expected, ColorFill seems to be OK
		//IF_FAILED_RETURN(m_pDevice9Ex->Clear(0L, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0L));
		IF_FAILED_RETURN(m_pDevice9Ex->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pRT));
		IF_FAILED_RETURN(m_pDevice9Ex->ColorFill(pRT, NULL, 0xff000000));
		IF_FAILED_RETURN(m_pDevice9Ex->Present(NULL, NULL, NULL, NULL));

		// It seems we should also clear dxva2 surfaces
	}
	catch(HRESULT){}

	SAFE_RELEASE(pRT);

	return hr;
}

HRESULT CDxva2Decoder::AddSliceShortInfo(const int iSubSliceCount, const DWORD dwSize){

	HRESULT hr;
	IF_FAILED_RETURN(iSubSliceCount > 0 && iSubSliceCount < MAX_SUB_SLICE ? S_OK : E_INVALIDARG);

	const int iIndex = iSubSliceCount - 1;

	m_H264SliceShort[iIndex].SliceBytesInBuffer = dwSize;
	m_H264SliceShort[iIndex].BSNALunitDataLocation = 0;
	m_H264SliceShort[iIndex].wBadSliceChopping = 0;

	for(int i = 1; i < iSubSliceCount; i++)
		m_H264SliceShort[iIndex].BSNALunitDataLocation += m_H264SliceShort[i - 1].SliceBytesInBuffer;

	return hr;
}

HRESULT CDxva2Decoder::InitDecoderService(){

	HRESULT hr = S_OK;
	GUID* pGuids = NULL;
	D3DFORMAT* pFormats = NULL;
	UINT uiCount;
	BOOL bDecoderH264 = FALSE;

	try{

		IF_FAILED_THROW(m_pDecoderService->GetDecoderDeviceGuids(&uiCount, &pGuids));

		for(UINT ui = 0; ui < uiCount; ui++){

			if(pGuids[ui] == m_gH264Vld){
				bDecoderH264 = TRUE;
				break;
			}
		}

		IF_FAILED_THROW(bDecoderH264 ? S_OK : E_FAIL);
		bDecoderH264 = FALSE;

		IF_FAILED_THROW(m_pDecoderService->GetDecoderRenderTargets(m_gH264Vld, &uiCount, &pFormats));

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

HRESULT CDxva2Decoder::InitVideoDecoder(const SPS_DATA& sps){

	HRESULT hr;
	DXVA2_ConfigPictureDecode* pConfigs = NULL;
	UINT uiCount;
	UINT uiIndex = 0;

	IF_FAILED_RETURN(m_pDecoderService->GetDecoderConfigurations(m_gH264Vld, &m_Dxva2Desc, NULL, &uiCount, &pConfigs));

	IF_FAILED_RETURN((pConfigs == NULL || uiCount == 0) ? E_FAIL : S_OK);

	for(; uiIndex < uiCount; uiIndex++){

		// Configuration i use with NVIDIA 700 series

		// GUID guidConfigBitstreamEncryption		: {1B81BED0-A0C7-11D3-B984-00C04F2E73C5} (DXVA_NoEncrypt)
		// GUID guidConfigMBcontrolEncryption		: {1B81BED0-A0C7-11D3-B984-00C04F2E73C5} (DXVA_NoEncrypt)
		// GUID guidConfigResidDiffEncryption		: {1B81BED0-A0C7-11D3-B984-00C04F2E73C5} (DXVA_NoEncrypt)
		// UINT ConfigBitstreamRaw					: 2
		// UINT ConfigMBcontrolRasterOrder			: 0
		// UINT ConfigResidDiffHost					: 0
		// UINT ConfigSpatialResid8					: 0
		// UINT ConfigResid8Subtraction				: 0
		// UINT ConfigSpatialHost8or9Clipping		: 0
		// UINT ConfigSpatialResidInterleaved		: 0
		// UINT ConfigIntraResidUnsigned			: 0
		// UINT ConfigResidDiffAccelerator			: 1
		// UINT ConfigHostInverseScan				: 1
		// UINT ConfigSpecificIDCT					: 2
		// UINT Config4GroupedCoefs					: 0
		// USHORT ConfigMinRenderTargetBuffCount	: 3
		// USHORT ConfigDecoderSpecific				: 0

		if(pConfigs[uiIndex].ConfigBitstreamRaw == 2){

			// if ConfigBitstreamRaw == 2, we can use DXVA_Slice_H264_Short
			m_pConfigs = pConfigs;
			break;
		}
	}

	IF_FAILED_RETURN(m_pConfigs == NULL ? E_FAIL : S_OK);

	assert(m_pVideoDecoder == NULL && m_pSurface9[0] == NULL);

	IF_FAILED_RETURN(m_pDecoderService->CreateSurface(m_Dxva2Desc.SampleWidth, m_Dxva2Desc.SampleHeight, NUM_DXVA2_SURFACE - 1,
		static_cast<D3DFORMAT>(D3DFMT_NV12), D3DPOOL_DEFAULT, 0, DXVA2_VideoDecoderRenderTarget, m_pSurface9, NULL));

	IF_FAILED_RETURN(m_pDecoderService->CreateVideoDecoder(m_gH264Vld, &m_Dxva2Desc, &m_pConfigs[uiIndex], m_pSurface9, NUM_DXVA2_SURFACE, &m_pVideoDecoder));

	InitDxva2Struct(sps);

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

#ifdef USE_DXVA2_SURFACE_INDEX
		for(int i = 0; i < NUM_DXVA2_SURFACE; i++){
			g_Dxva2SurfaceIndex[i].bUsed = FALSE;
			g_Dxva2SurfaceIndex[i].bNalRef = FALSE;
		}
#endif
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

#ifdef USE_DXVA2_SURFACE_INDEX
		if(m_dqPoc.size() != 0){

			UCHAR uc = m_dqPoc.back().bRefFrameList;
			g_Dxva2SurfaceIndex[uc].bUsed = FALSE;
			g_Dxva2SurfaceIndex[uc].bNalRef = FALSE;
		}
#endif
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

#ifdef USE_DXVA2_SURFACE_INDEX
		for(int i = 0; i < NUM_DXVA2_SURFACE; i++){

			if(g_Dxva2SurfaceIndex[i].bUsed && g_Dxva2SurfaceIndex[i].bNalRef == FALSE)
				g_Dxva2SurfaceIndex[i].bUsed = FALSE;
		}
#endif
	}

#ifdef USE_DXVA2_SURFACE_INDEX
	g_Dxva2SurfaceIndex[dwIndex].bUsed = TRUE;
	g_Dxva2SurfaceIndex[dwIndex].bNalRef = m_btNalRefIdc != 0x00;
#endif

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

#ifdef USE_DXVA2_SURFACE_INDEX
				g_Dxva2SurfaceIndex[it->bRefFrameList].bUsed = FALSE;
				g_Dxva2SurfaceIndex[it->bRefFrameList].bNalRef = FALSE;
#endif
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

void CDxva2Decoder::ErasePastFrames(const LONGLONG& llTime){

	for(deque<PICTURE_PRESENTATION>::const_iterator it = m_dqPicturePresentation.begin(); it != m_dqPicturePresentation.end();){

		if(it->llTime <= llTime)
			it = m_dqPicturePresentation.erase(it);
		else
			++it;
	}
}

HRESULT CDxva2Decoder::InitVideoProcessor(){

	HRESULT hr;

	IDXVAHD_Device* pDXVAHD = NULL;
	D3DFORMAT* pFormats = NULL;
	DXVAHD_VPCAPS* pVPCaps = NULL;
	UINT uiIndex;

	DXVAHD_CONTENT_DESC desc;

	desc.InputFrameFormat = DXVAHD_FRAME_FORMAT_PROGRESSIVE;
	desc.InputFrameRate.Numerator = m_Dxva2Desc.InputSampleFreq.Numerator;
	desc.InputFrameRate.Denominator = m_Dxva2Desc.InputSampleFreq.Denominator;
	desc.InputWidth = m_Dxva2Desc.SampleWidth;
	desc.InputHeight = m_Dxva2Desc.SampleHeight;
	desc.OutputFrameRate.Numerator = m_Dxva2Desc.OutputFrameFreq.Numerator;
	desc.OutputFrameRate.Denominator = m_Dxva2Desc.OutputFrameFreq.Denominator;
	desc.OutputWidth = m_Dxva2Desc.SampleWidth;
	desc.OutputHeight = m_Dxva2Desc.SampleHeight;

	DXVAHD_VPDEVCAPS caps;
	ZeroMemory(&caps, sizeof(caps));

	try{

		IF_FAILED_THROW(DXVAHD_CreateDevice(m_pDevice9Ex, &desc, DXVAHD_DEVICE_USAGE_PLAYBACK_NORMAL, NULL, &pDXVAHD));

		IF_FAILED_THROW(pDXVAHD->GetVideoProcessorDeviceCaps(&caps));

		IF_FAILED_THROW(caps.MaxInputStreams < 1 ? E_FAIL : S_OK);

		pFormats = new (std::nothrow)D3DFORMAT[caps.OutputFormatCount];

		IF_FAILED_THROW(pFormats == NULL ? E_OUTOFMEMORY : S_OK);

		IF_FAILED_THROW(pDXVAHD->GetVideoProcessorOutputFormats(caps.OutputFormatCount, pFormats));

		for(uiIndex = 0; uiIndex < caps.OutputFormatCount; uiIndex++){

			if(pFormats[uiIndex] == D3DFMT_X8R8G8B8){
				break;
			}
		}

		IF_FAILED_THROW(uiIndex == caps.OutputFormatCount ? E_FAIL : S_OK);

		SAFE_DELETE_ARRAY(pFormats);

		pFormats = new (std::nothrow)D3DFORMAT[caps.InputFormatCount];

		IF_FAILED_THROW(pFormats == NULL ? E_OUTOFMEMORY : S_OK);

		IF_FAILED_THROW(pDXVAHD->GetVideoProcessorInputFormats(caps.InputFormatCount, pFormats));

		for(uiIndex = 0; uiIndex < caps.InputFormatCount; uiIndex++){

			if(pFormats[uiIndex] == m_Dxva2Desc.Format){
				break;
			}
		}

		IF_FAILED_THROW(uiIndex == caps.InputFormatCount ? E_FAIL : S_OK);

		pVPCaps = new (std::nothrow)DXVAHD_VPCAPS[caps.VideoProcessorCount];

		IF_FAILED_THROW(pVPCaps == NULL ? E_OUTOFMEMORY : S_OK);

		IF_FAILED_THROW(pDXVAHD->GetVideoProcessorCaps(caps.VideoProcessorCount, pVPCaps));

		IF_FAILED_THROW(pDXVAHD->CreateVideoProcessor(&pVPCaps[0].VPGuid, &m_pDXVAVP));

		IF_FAILED_THROW(m_pDevice9Ex->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 0, 0), 1.0f, 0));

		IF_FAILED_THROW(ConfigureVideoProcessor());
	}
	catch(HRESULT){}

	SAFE_DELETE_ARRAY(pFormats);
	SAFE_DELETE_ARRAY(pVPCaps);

	SAFE_RELEASE(pDXVAHD);

	return hr;
}

HRESULT CDxva2Decoder::ConfigureVideoProcessor(){

	HRESULT hr = S_OK;

	const RECT FrameRect = {0L, 0L, (LONG)m_Dxva2Desc.SampleWidth, (LONG)m_Dxva2Desc.SampleHeight};

	DXVAHD_STREAM_STATE_FRAME_FORMAT_DATA FrameFormat = {DXVAHD_FRAME_FORMAT_PROGRESSIVE};
	DXVAHD_STREAM_STATE_LUMA_KEY_DATA Luma = {FALSE, 1.0f, 1.0f};
	DXVAHD_STREAM_STATE_ALPHA_DATA Alpha = {TRUE, float(0xFF) / 0xFF};
	DXVAHD_STREAM_STATE_SOURCE_RECT_DATA Src = {TRUE, FrameRect};
	DXVAHD_STREAM_STATE_DESTINATION_RECT_DATA Dest = {TRUE, FrameRect};
	DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE_DATA Color = {0, 0, 1, 0};
	DXVAHD_BLT_STATE_TARGET_RECT_DATA Tr = {TRUE, FrameRect};

	try{

		IF_FAILED_THROW(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_D3DFORMAT, sizeof(m_Dxva2Desc.Format), &m_Dxva2Desc.Format));
		IF_FAILED_THROW(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_FRAME_FORMAT, sizeof(FrameFormat), &FrameFormat));
		IF_FAILED_THROW(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_LUMA_KEY, sizeof(Luma), &Luma));
		IF_FAILED_THROW(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_ALPHA, sizeof(Alpha), &Alpha));
		IF_FAILED_THROW(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_SOURCE_RECT, sizeof(Src), &Src));
		IF_FAILED_THROW(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_DESTINATION_RECT, sizeof(Dest), &Dest));
		IF_FAILED_THROW(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE, sizeof(Color), &Color));
		IF_FAILED_THROW(m_pDXVAVP->SetVideoProcessBltState(DXVAHD_BLT_STATE_OUTPUT_COLOR_SPACE, sizeof(Color), &Color));
		IF_FAILED_THROW(m_pDXVAVP->SetVideoProcessBltState(DXVAHD_BLT_STATE_TARGET_RECT, sizeof(Tr), &Tr));
	}
	catch(HRESULT){}

	return hr;
}