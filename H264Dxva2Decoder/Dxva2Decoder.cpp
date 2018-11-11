//----------------------------------------------------------------------------------------------
// Dxva2Decoder.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

BOOL g_bStopApplication = FALSE;

LRESULT CALLBACK WindowApplicationMsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){

	if(msg == WM_PAINT){

		ValidateRect(hWnd, NULL);
		return 0L;
	}
	else if(msg == WM_CLOSE){

		g_bStopApplication = TRUE;
		return 0L;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

CDxva2Decoder::CDxva2Decoder(){

	memset(&m_Dxva2Desc, 0, sizeof(DXVA2_VideoDesc));

	for(int i = 0; i < NUM_DXVA2_SURFACE; i++){
		m_pSurface9[i] = NULL;
	}

	memset(&m_ExecuteParams, 0, sizeof(DXVA2_DecodeExecuteParams));
	memset(m_BufferDesc, 0, 4 * sizeof(DXVA2_DecodeBufferDesc));
	memset(&m_H264PictureParams, 0, sizeof(DXVA_PicParams_H264));
	memset(&m_H264QuantaMatrix, 0, sizeof(DXVA_Qmatrix_H264));
	memset(m_H264SliceInfo, 0, MAX_SLICE * sizeof(DXVA_Slice_H264_Short));

	m_hInst = NULL;
	m_hWnd = NULL;
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
	m_dwPicturePresent = 0;
	m_dwPauseDuration = 40;
}

HRESULT CDxva2Decoder::InitForm(const UINT uiWidth, const UINT uiHeight){

	WNDCLASSEX WndClassEx;

	WndClassEx.cbSize = sizeof(WNDCLASSEX);
	WndClassEx.style = CS_HREDRAW | CS_VREDRAW;
	WndClassEx.lpfnWndProc = WindowApplicationMsgProc;
	WndClassEx.cbClsExtra = 0L;
	WndClassEx.cbWndExtra = 0L;
	WndClassEx.hInstance = m_hInst;
	WndClassEx.hIcon = NULL;
	WndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClassEx.hbrBackground = NULL;
	WndClassEx.lpszMenuName = NULL;
	WndClassEx.lpszClassName = WINDOWAPPLICATION_CLASS;
	WndClassEx.hIconSm = NULL;

	if(!RegisterClassEx(&WndClassEx)){
		return E_FAIL;
	}

	int iWndL = uiWidth + GetSystemMetrics(SM_CXSIZEFRAME) * 2;
	int iWndH = uiHeight + GetSystemMetrics(SM_CYSIZEFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);

	int iXWnd = (GetSystemMetrics(SM_CXSCREEN) - iWndL) / 2;
	int iYWnd = (GetSystemMetrics(SM_CYSCREEN) - iWndH) / 2;

	if((m_hWnd = CreateWindowEx(WS_EX_ACCEPTFILES, WINDOWAPPLICATION_CLASS, WINDOWAPPLICATION_CLASS, WS_OVERLAPPEDWINDOW, iXWnd, iYWnd, iWndL, iWndH, GetDesktopWindow(), NULL, m_hInst, NULL)) == NULL){
		return E_FAIL;
	}

	ShowWindow(m_hWnd, SW_SHOW);

	return S_OK;
}

HRESULT CDxva2Decoder::InitDXVA2(const UINT uiWidth, const UINT uiHeight, const UINT uiNumerator, const UINT uiDenominator){

	HRESULT hr;

	assert(m_hWnd == NULL && m_hInst == NULL);

	m_hInst = GetModuleHandle(NULL);

	assert(m_hInst != NULL);

	IF_FAILED_RETURN(hr = InitForm(uiWidth, uiHeight));

	assert(m_pD3D9Ex == NULL && m_pDevice9Ex == NULL);

	D3DPRESENT_PARAMETERS d3dpp;

	IF_FAILED_RETURN(hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D9Ex));

	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = m_hWnd;
	d3dpp.BackBufferWidth = uiWidth;
	d3dpp.BackBufferHeight = uiHeight;
	d3dpp.Windowed = TRUE;
	d3dpp.Flags = D3DPRESENTFLAG_VIDEO;

	IF_FAILED_RETURN(hr = m_pD3D9Ex->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, NULL, &m_pDevice9Ex));

	IF_FAILED_RETURN(hr = m_pDevice9Ex->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
	IF_FAILED_RETURN(hr = m_pDevice9Ex->SetRenderState(D3DRS_ZENABLE, FALSE));
	IF_FAILED_RETURN(hr = m_pDevice9Ex->SetRenderState(D3DRS_LIGHTING, FALSE));

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

	IF_FAILED_RETURN(hr = DXVA2CreateDirect3DDeviceManager9(&m_pResetToken, &m_pDXVAManager));

	IF_FAILED_RETURN(hr = m_pDXVAManager->ResetDevice(m_pDevice9Ex, m_pResetToken));

	IF_FAILED_RETURN(hr = m_pDXVAManager->OpenDeviceHandle(&m_hD3d9Device));

	IF_FAILED_RETURN(hr = m_pDXVAManager->GetVideoService(m_hD3d9Device, IID_IDirectXVideoDecoderService, reinterpret_cast<void**>(&m_pDecoderService)));

	IF_FAILED_RETURN(hr = InitDecoderService());

	IF_FAILED_RETURN(hr = InitVideoDecoder());

	IF_FAILED_RETURN(hr = InitVideoProcessor());

	return hr;
}

void CDxva2Decoder::OnRelease(){

	TRACE((L"PicturePresent = %u - Poc = %d - ShortRef = %d - LongRef = %d - PicturePresentation = %d\r\n", m_dwPicturePresent, m_dqPoc.size(), m_dqShortRef.size(), m_dqLongRef.size(), m_dqPicturePresentation.size()));

	memset(&m_Dxva2Desc, 0, sizeof(DXVA2_VideoDesc));
	memset(&m_ExecuteParams, 0, sizeof(DXVA2_DecodeExecuteParams));
	memset(m_BufferDesc, 0, 4 * sizeof(DXVA2_DecodeBufferDesc));
	memset(&m_H264PictureParams, 0, sizeof(DXVA_PicParams_H264));
	memset(&m_H264QuantaMatrix, 0, sizeof(DXVA_Qmatrix_H264));
	memset(m_H264SliceInfo, 0, MAX_SLICE * sizeof(DXVA_Slice_H264_Short));

	m_dwCurPictureId = 0;
	m_iPrevTopFieldOrderCount = 0;

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

	SAFE_RELEASE(m_pDXVAVP);

	m_dqPoc.clear();
	m_dqShortRef.clear();
	m_dqLongRef.clear();
	m_dqPicturePresentation.clear();

	if(m_pDevice9Ex){

		ULONG ulCount = m_pDevice9Ex->Release();
		m_pDevice9Ex = NULL;
		assert(ulCount == 0);
	}

	if(IsWindow(m_hWnd)){

		DestroyWindow(m_hWnd);
		UnregisterClass(WINDOWAPPLICATION_CLASS, m_hInst);
		m_hWnd = NULL;
		m_hInst = NULL;
	}
}

HRESULT CDxva2Decoder::DecodeFrame(CMFBuffer& cMFNaluBuffer, const PICTURE_INFO& Picture){

	HRESULT hr = S_OK;
	IDirect3DDevice9* pDevice = NULL;
	void* pBuffer = NULL;
	UINT uiSize = 0;
	static BOOL bFirst = TRUE;

	assert(m_pVideoDecoder != NULL);

	if(m_dwCurPictureId >= NUM_DXVA2_SURFACE){
		m_dwCurPictureId = 0;
	}

	try{

		IF_FAILED_THROW(hr = m_pDXVAManager->TestDevice(m_hD3d9Device));
		IF_FAILED_THROW(hr = m_pDXVAManager->LockDevice(m_hD3d9Device, &pDevice, TRUE));

		do{

			hr = m_pVideoDecoder->BeginFrame(m_pSurface9[m_dwCurPictureId], NULL);
			Sleep(1);
		}
		while(hr == E_PENDING);

		IF_FAILED_THROW(hr);

		// Picture
		InitPictureParams(m_dwCurPictureId, Picture);
		IF_FAILED_THROW(hr = m_pVideoDecoder->GetBuffer(DXVA2_PictureParametersBufferType, &pBuffer, &uiSize));
		assert(sizeof(DXVA_PicParams_H264) <= uiSize);
		memcpy(pBuffer, &m_H264PictureParams, sizeof(DXVA_PicParams_H264));
		IF_FAILED_THROW(hr = m_pVideoDecoder->ReleaseBuffer(DXVA2_PictureParametersBufferType));

		// QuantaMatrix
		InitQuantaMatrixParams(Picture.pps);
		IF_FAILED_THROW(hr = m_pVideoDecoder->GetBuffer(DXVA2_InverseQuantizationMatrixBufferType, &pBuffer, &uiSize));
		assert(sizeof(DXVA_Qmatrix_H264) <= uiSize);
		memcpy(pBuffer, &m_H264QuantaMatrix, sizeof(DXVA_Qmatrix_H264));
		IF_FAILED_THROW(hr = m_pVideoDecoder->ReleaseBuffer(DXVA2_InverseQuantizationMatrixBufferType));

		// BitStream
		IF_FAILED_THROW(hr = m_pVideoDecoder->GetBuffer(DXVA2_BitStreamDateBufferType, &pBuffer, &uiSize));
		IF_FAILED_THROW(hr = AddNalUnitBufferPadding(cMFNaluBuffer, uiSize));
		assert(cMFNaluBuffer.GetBufferSize() <= uiSize);
		memcpy(pBuffer, cMFNaluBuffer.GetStartBuffer(), cMFNaluBuffer.GetBufferSize());
		IF_FAILED_THROW(hr = m_pVideoDecoder->ReleaseBuffer(DXVA2_BitStreamDateBufferType));

		// Slices
		IF_FAILED_THROW(hr = m_pVideoDecoder->GetBuffer(DXVA2_SliceControlBufferType, &pBuffer, &uiSize));
		assert(sizeof(DXVA_Slice_H264_Short) <= uiSize);
		DXVA_Slice_H264_Short SliceH264Short = {0};
		SliceH264Short.SliceBytesInBuffer = cMFNaluBuffer.GetBufferSize();
		memcpy(pBuffer, &SliceH264Short, sizeof(DXVA_Slice_H264_Short));
		IF_FAILED_THROW(hr = m_pVideoDecoder->ReleaseBuffer(DXVA2_SliceControlBufferType));

		// CompBuffers
		// Allready set
		//--> m_BufferDesc[0].DataSize = sizeof(DXVA_PicParams_H264);
		//--> m_BufferDesc[1].DataSize = sizeof(DXVA_Qmatrix_H264);
		m_BufferDesc[2].DataSize = cMFNaluBuffer.GetBufferSize();
		m_BufferDesc[2].NumMBsInBuffer = (Picture.sps.pic_width_in_mbs_minus1 + 1) * (Picture.sps.pic_height_in_map_units_minus1 + 1);
		m_BufferDesc[3].DataSize = sizeof(DXVA_Slice_H264_Short);
		m_BufferDesc[3].NumMBsInBuffer = m_BufferDesc[2].NumMBsInBuffer;

		IF_FAILED_THROW(hr = m_pVideoDecoder->Execute(&m_ExecuteParams));

		IF_FAILED_THROW(hr = m_pVideoDecoder->EndFrame(NULL));

		IF_FAILED_THROW(hr = m_pDXVAManager->UnlockDevice(m_hD3d9Device, FALSE));

		m_dwCurPictureId++;
	}
	catch(HRESULT){}

	SAFE_RELEASE(pDevice);

	return hr;
}

HRESULT CDxva2Decoder::InitDecoderService(){

	HRESULT hr = S_OK;
	GUID* pGuids = NULL;
	D3DFORMAT* pFormats = NULL;
	UINT uiCount;
	BOOL bDecoderH264 = FALSE;

	try{

		IF_FAILED_THROW(hr = m_pDecoderService->GetDecoderDeviceGuids(&uiCount, &pGuids));

		for(UINT ui = 0; ui < uiCount; ui++){

			if(pGuids[ui] == m_gH264Vld){
				bDecoderH264 = TRUE;
				break;
			}
		}

		IF_FAILED_THROW(hr = (bDecoderH264 ? S_OK : E_FAIL));
		bDecoderH264 = FALSE;

		IF_FAILED_THROW(hr = m_pDecoderService->GetDecoderRenderTargets(m_gH264Vld, &uiCount, &pFormats));

		for(UINT ui = 0; ui < uiCount; ui++){

			if(pFormats[ui] == (D3DFORMAT)D3DFMT_NV12){
				bDecoderH264 = TRUE;
				break;
			}
		}

		IF_FAILED_THROW(hr = (bDecoderH264 ? S_OK : E_FAIL));
	}
	catch(HRESULT){}

	CoTaskMemFree(pFormats);
	CoTaskMemFree(pGuids);

	return hr;
}

HRESULT CDxva2Decoder::InitVideoDecoder(){

	HRESULT hr;
	DXVA2_ConfigPictureDecode* pConfigs = NULL;
	UINT uiCount;
	UINT uiIndex = 0;

	IF_FAILED_RETURN(hr = m_pDecoderService->GetDecoderConfigurations(m_gH264Vld, &m_Dxva2Desc, NULL, &uiCount, &pConfigs));

	IF_FAILED_RETURN(hr = ((pConfigs == NULL || uiCount == 0) ? E_FAIL : S_OK));

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

	IF_FAILED_RETURN(hr = (m_pConfigs == NULL ? E_FAIL : S_OK));

	assert(m_pVideoDecoder == NULL && m_pSurface9[0] == NULL);

	IF_FAILED_RETURN(hr = m_pDecoderService->CreateSurface(m_Dxva2Desc.SampleWidth, m_Dxva2Desc.SampleHeight, NUM_DXVA2_SURFACE - 1,
		static_cast<D3DFORMAT>(D3DFMT_NV12), D3DPOOL_DEFAULT, 0, DXVA2_VideoDecoderRenderTarget, m_pSurface9, NULL));

	IF_FAILED_RETURN(hr = m_pDecoderService->CreateVideoDecoder(m_gH264Vld, &m_Dxva2Desc, &m_pConfigs[uiIndex], m_pSurface9, NUM_DXVA2_SURFACE, &m_pVideoDecoder));

	InitDxva2Struct();

	return hr;
}

void CDxva2Decoder::InitDxva2Struct(){

	memset(&m_ExecuteParams, 0, sizeof(DXVA2_DecodeExecuteParams));
	memset(m_BufferDesc, 0, 4 * sizeof(DXVA2_DecodeBufferDesc));
	memset(&m_H264PictureParams, 0, sizeof(DXVA_PicParams_H264));
	memset(&m_H264QuantaMatrix, 0, sizeof(DXVA_Qmatrix_H264));
	memset(m_H264SliceInfo, 0, MAX_SLICE * sizeof(DXVA_Slice_H264_Short));

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

	static DWORD dwStatusReportFeedbackNumber = 1;
	int iIndex = 0;

	if(Picture.NalUnitType == NAL_UNIT_CODED_SLICE_IDR){
		m_dqPoc.clear();
		m_dqShortRef.clear();
		m_dqLongRef.clear();
	}

	m_H264PictureParams.wFrameWidthInMbsMinus1 = (USHORT)Picture.sps.pic_width_in_mbs_minus1;
	m_H264PictureParams.wFrameHeightInMbsMinus1 = (USHORT)Picture.sps.pic_height_in_map_units_minus1;
	m_H264PictureParams.CurrPic.Index7Bits = dwIndex;
	m_H264PictureParams.CurrPic.AssociatedFlag = Picture.pps.pic_order_present_flag;
	m_H264PictureParams.num_ref_frames = (UCHAR)Picture.sps.num_ref_frames;

	//m_H264PictureParams.wBitFields = 0;
	//m_H264PictureParams.field_pic_flag = 0;
	//m_H264PictureParams.MbaffFrameFlag = 0;
	m_H264PictureParams.residual_colour_transform_flag = Picture.sps.residual_colour_transform_flag;
	//m_H264PictureParams.sp_for_switch_flag = 0;
	m_H264PictureParams.chroma_format_idc = 1;
	m_H264PictureParams.RefPicFlag = Picture.btNalRefIdc != 0;
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
	m_H264PictureParams.StatusReportFeedbackNumber = dwStatusReportFeedbackNumber++;

	for(int i = 0; i < 16; i++){

		m_H264PictureParams.RefFrameList[i].bPicEntry = 0xff;
		m_H264PictureParams.FieldOrderCntList[i][0] = m_H264PictureParams.FieldOrderCntList[i][1] = 0;
		m_H264PictureParams.FrameNumList[i] = 0;
	}

	m_H264PictureParams.CurrFieldOrderCnt[1] = m_H264PictureParams.CurrFieldOrderCnt[0] = Picture.slice.TopFieldOrderCnt;
	m_H264PictureParams.UsedForReferenceFlags = 0;

	for(auto it = m_dqPoc.begin(); it != m_dqPoc.end(); iIndex++, ++it){

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
	m_H264PictureParams.pic_order_present_flag = (UCHAR)Picture.pps.pic_order_present_flag;
	m_H264PictureParams.num_slice_groups_minus1 = (UCHAR)Picture.pps.num_slice_groups_minus1;
	//m_H264PictureParams.slice_group_map_type = 0;
	m_H264PictureParams.deblocking_filter_control_present_flag = (UCHAR)Picture.pps.deblocking_filter_control_present_flag;
	m_H264PictureParams.redundant_pic_cnt_present_flag = (UCHAR)Picture.pps.redundant_pic_cnt_present_flag;
	//m_H264PictureParams.Reserved8BitsB = 0;
	m_H264PictureParams.slice_group_change_rate_minus1 = (USHORT)Picture.pps.num_slice_groups_minus1;

	if(dwStatusReportFeedbackNumber == ULONG_MAX){
		dwStatusReportFeedbackNumber = 1;
	}

	HandlePOC(dwIndex, Picture);
}

void CDxva2Decoder::InitQuantaMatrixParams(const PPS_DATA& /*pps*/){

	/*static const BYTE zigzag_scan[16 + 1] = {
		0 + 0 * 4, 1 + 0 * 4, 0 + 1 * 4, 0 + 2 * 4,
		1 + 1 * 4, 2 + 0 * 4, 3 + 0 * 4, 2 + 1 * 4,
		1 + 2 * 4, 0 + 3 * 4, 1 + 3 * 4, 2 + 2 * 4,
		3 + 1 * 4, 3 + 2 * 4, 2 + 3 * 4, 3 + 3 * 4,
	};

	static const BYTE ff_zigzag_direct[64] = {
		0, 1, 8, 16, 9, 2, 3, 10,
		17, 24, 32, 25, 18, 11, 4, 5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13, 6, 7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63
	};*/

	memset(m_H264QuantaMatrix.bScalingLists4x4, 16, sizeof(m_H264QuantaMatrix.bScalingLists4x4));
	memset(m_H264QuantaMatrix.bScalingLists8x8, 16, sizeof(m_H264QuantaMatrix.bScalingLists8x8));

	/*for(int i = 0; i < 6; i++){

	  for(int j = 0; j < 16; j++){

		m_H264QuantaMatrix.bScalingLists4x4[i][j] = pps.bScalingLists4x4[i][zigzag_scan[j]];
	  }
	}

	for(int i = 0; i < 64; i++) {
	  m_H264QuantaMatrix.bScalingLists8x8[0][i] = pps.bScalingLists8x8[0][ff_zigzag_direct[i]];
	  m_H264QuantaMatrix.bScalingLists8x8[1][i] = pps.bScalingLists8x8[1][ff_zigzag_direct[i]];
	}*/
}

HRESULT CDxva2Decoder::AddNalUnitBufferPadding(CMFBuffer& cMFNaluBuffer, const UINT uiSize){

	HRESULT hr;

	UINT uiPadding = MIN(128 - (cMFNaluBuffer.GetBufferSize() & 127), uiSize);

	IF_FAILED_RETURN(hr = cMFNaluBuffer.Reserve(uiPadding));
	memset(cMFNaluBuffer.GetReadStartBuffer(), 0, uiPadding);
	IF_FAILED_RETURN(hr = cMFNaluBuffer.SetEndPosition(uiPadding));

	return hr;
}

void CDxva2Decoder::HandlePOC(const DWORD dwIndex, const PICTURE_INFO& Picture){

	if(Picture.btNalRefIdc && Picture.slice.PicMarking.adaptive_ref_pic_marking_mode_flag == FALSE && m_dqPoc.size() >= Picture.sps.num_ref_frames){

		m_dqPoc.pop_back();
	}

	if(Picture.btNalRefIdc){

		POC NewPoc;

		NewPoc.usFrameList = Picture.slice.frame_num;
		NewPoc.bRefFrameList = (UCHAR)dwIndex;
		NewPoc.TopFieldOrderCnt = Picture.slice.TopFieldOrderCnt;
		NewPoc.usSliceType = Picture.slice.slice_type;
		NewPoc.bLongRef = FALSE;

		m_dqPoc.push_front(NewPoc);
	}

	PICTURE_PRESENTATION pp;
	pp.TopFieldOrderCnt = Picture.slice.TopFieldOrderCnt;
	pp.dwDXVA2Index = dwIndex;
	pp.SliceType = (SLICE_TYPE)Picture.slice.slice_type;
	if(Picture.slice.TopFieldOrderCnt != 0)
		m_dqPicturePresentation.push_front(pp);
	else
		m_dqPicturePresentation.push_back(pp);

	if(!Picture.btNalRefIdc)
		return;

	UINT uiCurIndex;
	MMCO_OP op;

	//todo 8.2.4 list reordering
	for(int iIndex = 0; iIndex < Picture.slice.PicMarking.iNumOPMode; iIndex++){

		op = Picture.slice.PicMarking.mmcoOPMode[iIndex].mmcoOP;

		if(op == MMCO_OP_SHORT2UNUSED){

			uiCurIndex = Picture.slice.PicMarking.mmcoOPMode[iIndex].difference_of_pic_nums_minus1 + 1;

			if(uiCurIndex >= m_dqPoc.size())
				uiCurIndex = m_dqPoc.size() - 1;

			if(uiCurIndex < m_dqPoc.size()){

				auto it = m_dqPoc.begin() + uiCurIndex;

				m_dqPoc.erase(m_dqPoc.begin() + uiCurIndex);
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