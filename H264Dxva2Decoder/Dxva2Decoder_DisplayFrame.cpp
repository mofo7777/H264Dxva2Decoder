//----------------------------------------------------------------------------------------------
// Dxva2Decoder_DisplayFrame.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

HRESULT CDxva2Decoder::DisplayFrame(){

	HRESULT hr = S_OK;

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while(msg.message != WM_QUIT){

		if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)){

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else{

			hr = RenderFrame();
			break;
		}
	}

	return hr;
}

HRESULT CDxva2Decoder::RenderFrame(){

	HRESULT hr = S_OK;
	IDirect3DSurface9 *pRT = NULL;
	DXVAHD_STREAM_DATA stream_data;
	DWORD dwSurfaceIndex = 0;
	BOOL bHasPicture = FALSE;
	ZeroMemory(&stream_data, sizeof(DXVAHD_STREAM_DATA));

	assert(m_pDXVAVP);

	for(deque<PICTURE_PRESENTATION>::const_iterator it = m_dqPicturePresentation.begin(); it != m_dqPicturePresentation.end(); ++it){

		if(it->TopFieldOrderCnt == 0 || it->TopFieldOrderCnt == (m_iPrevTopFieldOrderCount + 2) || it->TopFieldOrderCnt == (m_iPrevTopFieldOrderCount + 1)){

			dwSurfaceIndex = it->dwDXVA2Index;
			m_iPrevTopFieldOrderCount = it->TopFieldOrderCnt;
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

	m_dwPicturePresent++;

	try{

		stream_data.Enable = TRUE;
		stream_data.OutputIndex = 0;
		stream_data.InputFrameOrField = dwSurfaceIndex;
		stream_data.pInputSurface = m_pSurface9[dwSurfaceIndex];

		IF_FAILED_THROW(m_pDevice9Ex->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pRT));

		IF_FAILED_THROW(m_pDXVAVP->VideoProcessBltHD(pRT, dwSurfaceIndex, 1, &stream_data));

		IF_FAILED_THROW(m_pDevice9Ex->Present(NULL, NULL, NULL, NULL));
	}
	catch(HRESULT){}

	SAFE_RELEASE(pRT);

	Sleep(m_dwPauseDuration);

	return hr;
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