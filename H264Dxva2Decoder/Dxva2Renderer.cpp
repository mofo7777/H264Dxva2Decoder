//----------------------------------------------------------------------------------------------
// Dxva2Renderer.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

CDxva2Renderer::CDxva2Renderer() :
	m_pDXVAVP(NULL),
	m_pD3D9Ex(NULL),
	m_pDevice9Ex(NULL),
	m_pDirect3DDeviceManager9(NULL),
	m_pResetToken(0),
	m_dwPicturePresent(0),
	m_DxvaDeviceType(DXVAHD_DEVICE_TYPE_HARDWARE),
	m_InputPool(D3DPOOL_DEFAULT),
	m_bUseBT709(FALSE)
{
	ResetDxvaCaps();
	memset(&m_LastPresentation, 0, sizeof(m_LastPresentation));
}

HRESULT CDxva2Renderer::InitDXVA2(const HWND hWnd, const UINT uiWidth, const UINT uiHeight, const UINT uiNumerator, const UINT uiDenominator, DXVA2_VideoDesc& Dxva2Desc){

	HRESULT hr;

	// todo
	assert(m_pD3D9Ex == NULL && m_pDevice9Ex == NULL);

	D3DPRESENT_PARAMETERS d3dpp;

	IF_FAILED_RETURN(Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D9Ex));

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	ZeroMemory(&Dxva2Desc, sizeof(DXVA2_VideoDesc));

	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferWidth = uiWidth;
	d3dpp.BackBufferHeight = uiHeight;
	d3dpp.Windowed = TRUE;
	d3dpp.Flags = D3DPRESENTFLAG_VIDEO;

	IF_FAILED_RETURN(m_pD3D9Ex->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, NULL, &m_pDevice9Ex));

	IF_FAILED_RETURN(m_pDevice9Ex->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
	IF_FAILED_RETURN(m_pDevice9Ex->SetRenderState(D3DRS_ZENABLE, FALSE));
	IF_FAILED_RETURN(m_pDevice9Ex->SetRenderState(D3DRS_LIGHTING, FALSE));

#ifdef USE_DIRECTX9
	RECT rcMovieTimeText = {0, 0, 100, 50};
	IF_FAILED_RETURN(m_cMovieTime.OnRestore(m_pDevice9Ex, rcMovieTimeText, 32.0f));
#endif

	DXVA2_Frequency Dxva2Freq;
	Dxva2Freq.Numerator = uiNumerator;
	Dxva2Freq.Denominator = uiDenominator;

	Dxva2Desc.SampleWidth = uiWidth;
	Dxva2Desc.SampleHeight = uiHeight;

	Dxva2Desc.SampleFormat.SampleFormat = MFVideoInterlace_Progressive;

	Dxva2Desc.Format = static_cast<D3DFORMAT>(D3DFMT_NV12);
	Dxva2Desc.InputSampleFreq = Dxva2Freq;
	Dxva2Desc.OutputFrameFreq = Dxva2Freq;

	IF_FAILED_RETURN(DXVA2CreateDirect3DDeviceManager9(&m_pResetToken, &m_pDirect3DDeviceManager9));

	IF_FAILED_RETURN(m_pDirect3DDeviceManager9->ResetDevice(m_pDevice9Ex, m_pResetToken));

	IF_FAILED_RETURN(InitVideoProcessor(Dxva2Desc));

	return hr;
}

void CDxva2Renderer::OnRelease(){

	if(m_pDevice9Ex == NULL)
		return;

	m_pResetToken = 0;
	SAFE_RELEASE(m_pDirect3DDeviceManager9);

	ResetDxvaCaps();
	m_dwPicturePresent = 0;
	memset(&m_LastPresentation, 0, sizeof(m_LastPresentation));

#ifdef USE_DIRECTX9
	m_cMovieTime.OnDelete();
#endif

	SAFE_RELEASE(m_pD3D9Ex);
	SAFE_RELEASE(m_pDXVAVP);

	if(m_pDevice9Ex){

		ULONG ulCount = m_pDevice9Ex->Release();
		m_pDevice9Ex = NULL;
		assert(ulCount == 0);
	}
}

void CDxva2Renderer::Reset(){

	m_dwPicturePresent = 0;
	memset(&m_LastPresentation, 0, sizeof(m_LastPresentation));
}

HRESULT CDxva2Renderer::RenderFrame(IDirect3DSurface9** pSurface9, const SAMPLE_PRESENTATION& SamplePresentation){

	HRESULT hr = S_OK;
	IDirect3DSurface9* pRT = NULL;
	DXVAHD_STREAM_DATA stream_data;
	ZeroMemory(&stream_data, sizeof(DXVAHD_STREAM_DATA));

	assert(m_pDXVAVP);

	m_dwPicturePresent++;
	m_LastPresentation.dwDXVA2Index = SamplePresentation.dwDXVA2Index;
	m_LastPresentation.llTime = SamplePresentation.llTime;

	try{

		stream_data.Enable = TRUE;
		stream_data.OutputIndex = 0;
		stream_data.InputFrameOrField = SamplePresentation.dwDXVA2Index;
		stream_data.pInputSurface = pSurface9[SamplePresentation.dwDXVA2Index];

		IF_FAILED_THROW(m_pDevice9Ex->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pRT));

		IF_FAILED_THROW(m_pDXVAVP->VideoProcessBltHD(pRT, SamplePresentation.dwDXVA2Index, 1, &stream_data));

#ifdef USE_DIRECTX9
		IF_FAILED_THROW(DrawMovieText(m_pDevice9Ex, SamplePresentation.llTime));
#endif

#ifdef HANDLE_DIRECTX_ERROR_UNDOCUMENTED
		hr = m_pDevice9Ex->Present(NULL, NULL, NULL, NULL);

		// Present can return 0x88760872, it is OK to continue.
		// When resizing the window, the rendering thread may fail with this error.
		// So when resizing, the strategy is to pause the video if playing (see WindowsFormProc and WM_ENTERSIZEMOVE/WM_SYSCOMMAND), also, this avoid flickering
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

	return hr;
}

HRESULT CDxva2Renderer::RenderBlackFrame(){

	HRESULT hr;
	IDirect3DSurface9* pRT = NULL;

	IF_FAILED_RETURN(m_pDevice9Ex ? S_OK : E_UNEXPECTED);

	try{

		// Clear the render target seems not to work as expected, ColorFill seems to be OK
		//IF_FAILED_RETURN(m_pDevice9Ex->Clear(0L, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0L));
		IF_FAILED_THROW(m_pDevice9Ex->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pRT));
		IF_FAILED_THROW(m_pDevice9Ex->ColorFill(pRT, NULL, 0xff000000));
		IF_FAILED_THROW(m_pDevice9Ex->Present(NULL, NULL, NULL, NULL));

		// It seems we should also clear dxva2 surfaces
		// todo : HANDLE_DIRECTX_ERROR_UNDOCUMENTED
	}
	catch(HRESULT){}

	SAFE_RELEASE(pRT);

	return hr;
}

HRESULT CDxva2Renderer::RenderLastFrame(){

	HRESULT hr;

	IF_FAILED_RETURN(m_pDevice9Ex ? S_OK : E_UNEXPECTED);
	IF_FAILED_RETURN(m_pDevice9Ex->Present(NULL, NULL, NULL, NULL));
	// todo : HANDLE_DIRECTX_ERROR_UNDOCUMENTED

	return hr;
}

HRESULT CDxva2Renderer::RenderLastFramePresentation(IDirect3DSurface9** pSurface9){

	HRESULT hr = S_OK;
	IDirect3DSurface9* pRT = NULL;
	DXVAHD_STREAM_DATA stream_data;
	ZeroMemory(&stream_data, sizeof(DXVAHD_STREAM_DATA));

	assert(m_pDXVAVP);

	try{

		stream_data.Enable = TRUE;
		stream_data.OutputIndex = 0;
		stream_data.InputFrameOrField = m_LastPresentation.dwDXVA2Index;
		stream_data.pInputSurface = pSurface9[m_LastPresentation.dwDXVA2Index];

		IF_FAILED_THROW(m_pDevice9Ex->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pRT));

		IF_FAILED_THROW(m_pDXVAVP->VideoProcessBltHD(pRT, m_LastPresentation.dwDXVA2Index, 1, &stream_data));

#ifdef USE_DIRECTX9
		IF_FAILED_THROW(DrawMovieText(m_pDevice9Ex, m_LastPresentation.llTime));
#endif

#ifdef HANDLE_DIRECTX_ERROR_UNDOCUMENTED
		hr = m_pDevice9Ex->Present(NULL, NULL, NULL, NULL);

		// Present can return 0x88760872, it is OK to continue.
		// When resizing the window, the rendering thread may fail with this error.
		// When resizing, the strategy is to pause the video if playing (see WindowsFormProc and WM_ENTERSIZEMOVE/WM_SYSCOMMAND), also, this avoid flickering
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

	return hr;
}

BOOL CDxva2Renderer::GetDxva2Settings(DXVAHD_FILTER_RANGE_DATA_EX* pFilters, BOOL& bUseBT709){

	if(m_pDXVAVP == NULL)
		return FALSE;

	DXVAHD_FILTER_RANGE_DATA_EX* pFilter = pFilters;

	memcpy(pFilter, &m_RangeBrightness, sizeof(DXVAHD_FILTER_RANGE_DATA_EX));
	pFilter++;
	memcpy(pFilter, &m_RangeContrast, sizeof(DXVAHD_FILTER_RANGE_DATA_EX));
	pFilter++;
	memcpy(pFilter, &m_RangeHue, sizeof(DXVAHD_FILTER_RANGE_DATA_EX));
	pFilter++;
	memcpy(pFilter, &m_RangeSaturation, sizeof(DXVAHD_FILTER_RANGE_DATA_EX));
	pFilter++;
	memcpy(pFilter, &m_RangeNoiseReduction, sizeof(DXVAHD_FILTER_RANGE_DATA_EX));
	pFilter++;
	memcpy(pFilter, &m_RangeEdgeEnhancement, sizeof(DXVAHD_FILTER_RANGE_DATA_EX));
	pFilter++;
	memcpy(pFilter, &m_RangeAnamorphicScaling, sizeof(DXVAHD_FILTER_RANGE_DATA_EX));

	bUseBT709 = m_bUseBT709;

	return TRUE;
}

HRESULT CDxva2Renderer::ResetDxva2Settings(){

	HRESULT hr;
	IF_FAILED_RETURN(m_pDXVAVP ? S_OK : E_UNEXPECTED);

	IF_FAILED_RETURN(SetFilter(ID_FILTER_BRIGHTNESS, m_RangeBrightness.Default));
	IF_FAILED_RETURN(SetFilter(ID_FILTER_CONTRAST, m_RangeContrast.Default));
	IF_FAILED_RETURN(SetFilter(ID_FILTER_HUE, m_RangeHue.Default));
	IF_FAILED_RETURN(SetFilter(ID_FILTER_SATURATION, m_RangeSaturation.Default));
	IF_FAILED_RETURN(SetFilter(ID_FILTER_NOISE_REDUCTION, m_RangeNoiseReduction.Default));
	IF_FAILED_RETURN(SetFilter(ID_FILTER_EDGE_ENHANCEMENT, m_RangeEdgeEnhancement.Default));
	IF_FAILED_RETURN(SetFilter(ID_FILTER_ANAMORPHIC_SCALING, m_RangeAnamorphicScaling.Default));

	SetBT709(m_bUseBT709 = FALSE);

	return hr;
}

HRESULT CDxva2Renderer::SetFilter(const UINT uiItem, const INT iValue){

	HRESULT hr;
	IF_FAILED_RETURN(m_pDXVAVP ? S_OK : E_UNEXPECTED);

	DXVAHD_STREAM_STATE_FILTER_DATA FilterData = {TRUE, 0};
	DXVAHD_STREAM_STATE state = (DXVAHD_STREAM_STATE)-1;
	DXVAHD_FILTER_RANGE_DATA_EX* pRangeDataEx = NULL;

	if(uiItem == ID_FILTER_BRIGHTNESS){
		state = DXVAHD_STREAM_STATE_FILTER_BRIGHTNESS;
		pRangeDataEx = &m_RangeBrightness;
	}
	else if(uiItem == ID_FILTER_CONTRAST){
		state = DXVAHD_STREAM_STATE_FILTER_CONTRAST;
		pRangeDataEx = &m_RangeContrast;
	}
	else if(uiItem == ID_FILTER_HUE){
		state = DXVAHD_STREAM_STATE_FILTER_HUE;
		pRangeDataEx = &m_RangeHue;
	}
	else if(uiItem == ID_FILTER_SATURATION){
		state = DXVAHD_STREAM_STATE_FILTER_SATURATION;
		pRangeDataEx = &m_RangeSaturation;
	}
	else if(uiItem == ID_FILTER_NOISE_REDUCTION){
		state = DXVAHD_STREAM_STATE_FILTER_NOISE_REDUCTION;
		pRangeDataEx = &m_RangeNoiseReduction;
	}
	else if(uiItem == ID_FILTER_EDGE_ENHANCEMENT){
		state = DXVAHD_STREAM_STATE_FILTER_EDGE_ENHANCEMENT;
		pRangeDataEx = &m_RangeEdgeEnhancement;
	}
	else if(uiItem == ID_FILTER_ANAMORPHIC_SCALING){
		state = DXVAHD_STREAM_STATE_FILTER_ANAMORPHIC_SCALING;
		pRangeDataEx = &m_RangeAnamorphicScaling;
	}
	else if(uiItem == IDC_RADIO_BT601){
		return SetBT709(iValue);
	}
	else{
		IF_FAILED_RETURN(E_FAIL);
	}

	if(pRangeDataEx->bAvailable == FALSE)
		return hr;

	FilterData.Level = pRangeDataEx->iCurrent = iValue;

	IF_FAILED_RETURN(m_pDXVAVP->SetVideoProcessStreamState(0, state, sizeof(FilterData), &FilterData));

	return hr;
}

HRESULT CDxva2Renderer::InitVideoProcessor(const DXVA2_VideoDesc Dxva2Desc){

	HRESULT hr;

	IDXVAHD_Device* pDXVAHD = NULL;
	D3DFORMAT* pFormats = NULL;
	DXVAHD_VPCAPS* pVPCaps = NULL;
	UINT uiIndex;

	DXVAHD_CONTENT_DESC desc;

	desc.InputFrameFormat = DXVAHD_FRAME_FORMAT_PROGRESSIVE;
	desc.InputFrameRate.Numerator = Dxva2Desc.InputSampleFreq.Numerator;
	desc.InputFrameRate.Denominator = Dxva2Desc.InputSampleFreq.Denominator;
	desc.InputWidth = Dxva2Desc.SampleWidth;
	desc.InputHeight = Dxva2Desc.SampleHeight;
	desc.OutputFrameRate.Numerator = Dxva2Desc.OutputFrameFreq.Numerator;
	desc.OutputFrameRate.Denominator = Dxva2Desc.OutputFrameFreq.Denominator;
	desc.OutputWidth = Dxva2Desc.SampleWidth;
	desc.OutputHeight = Dxva2Desc.SampleHeight;

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

			if(pFormats[uiIndex] == Dxva2Desc.Format){
				break;
			}
		}

		IF_FAILED_THROW(uiIndex == caps.InputFormatCount ? E_FAIL : S_OK);

		pVPCaps = new (std::nothrow)DXVAHD_VPCAPS[caps.VideoProcessorCount];

		IF_FAILED_THROW(pVPCaps == NULL ? E_OUTOFMEMORY : S_OK);

		IF_FAILED_THROW(pDXVAHD->GetVideoProcessorCaps(caps.VideoProcessorCount, pVPCaps));

		IF_FAILED_THROW(pDXVAHD->CreateVideoProcessor(&pVPCaps[0].VPGuid, &m_pDXVAVP));

		IF_FAILED_THROW(m_pDevice9Ex->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 0, 0), 1.0f, 0));

		// Should be DXVAHD_DEVICE_TYPE_HARDWARE
		m_DxvaDeviceType = caps.DeviceType;
		// Should be D3DPOOL_DEFAULT
		m_InputPool = caps.InputPool;

		/*
		// It seems there is an undocumented value, i get 0x17 with my Nvidia GPU card
		enum _DXVAHD_DEVICE_CAPS
		{
			DXVAHD_DEVICE_CAPS_LINEAR_SPACE = 0x1,
			DXVAHD_DEVICE_CAPS_xvYCC = 0x2,
			DXVAHD_DEVICE_CAPS_RGB_RANGE_CONVERSION = 0x4,
			DXVAHD_DEVICE_CAPS_YCbCr_MATRIX_CONVERSION = 0x8
			DXVAHD_DEVICE_CAPS_?????????? = 0x16
		} 	DXVAHD_DEVICE_CAPS;
		*/

		if(caps.MaxStreamStates > 0){

			m_RangeBrightness.bAvailable = caps.FilterCaps & DXVAHD_FILTER_CAPS_BRIGHTNESS ? TRUE : FALSE;
			m_RangeContrast.bAvailable = caps.FilterCaps & DXVAHD_FILTER_CAPS_CONTRAST ? TRUE : FALSE;
			m_RangeHue.bAvailable = caps.FilterCaps & DXVAHD_FILTER_CAPS_HUE ? TRUE : FALSE;
			m_RangeSaturation.bAvailable = caps.FilterCaps & DXVAHD_FILTER_CAPS_SATURATION ? TRUE : FALSE;
			m_RangeNoiseReduction.bAvailable = caps.FilterCaps & DXVAHD_FILTER_CAPS_NOISE_REDUCTION ? TRUE : FALSE;
			m_RangeEdgeEnhancement.bAvailable = caps.FilterCaps & DXVAHD_FILTER_CAPS_EDGE_ENHANCEMENT ? TRUE : FALSE;
			m_RangeAnamorphicScaling.bAvailable = caps.FilterCaps & DXVAHD_FILTER_CAPS_ANAMORPHIC_SCALING ? TRUE : FALSE;

			// Informative
			m_BltStateAlphaFill.bAvailable = caps.FeatureCaps & DXVAHD_FEATURE_CAPS_ALPHA_FILL ? TRUE : FALSE;
			m_StateConstriction.bAvailable = caps.FeatureCaps & DXVAHD_FEATURE_CAPS_CONSTRICTION ? TRUE : FALSE;
			m_StreamStateLumaKey.bAvailable = ((caps.FeatureCaps & DXVAHD_FEATURE_CAPS_LUMA_KEY) && (caps.InputFormatCaps & DXVAHD_INPUT_FORMAT_CAPS_RGB_LUMA_KEY)) ? TRUE : FALSE;
			m_StreamStateAlpha.bAvailable = caps.FeatureCaps & DXVAHD_FEATURE_CAPS_ALPHA_PALETTE ? TRUE : FALSE;
		}

		IF_FAILED_THROW(ConfigureVideoProcessor(pDXVAHD, Dxva2Desc));
	}
	catch(HRESULT){}

	SAFE_DELETE_ARRAY(pFormats);
	SAFE_DELETE_ARRAY(pVPCaps);

	SAFE_RELEASE(pDXVAHD);

	return hr;
}

HRESULT CDxva2Renderer::ConfigureVideoProcessor(IDXVAHD_Device* pDXVAHD, const DXVA2_VideoDesc Dxva2Desc){

	HRESULT hr = S_OK;

	const RECT FrameRect = {0L, 0L, (LONG)Dxva2Desc.SampleWidth, (LONG)Dxva2Desc.SampleHeight};
	const DXVAHD_STREAM_STATE_FRAME_FORMAT_DATA FrameFormat = {DXVAHD_FRAME_FORMAT_PROGRESSIVE};
	const DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE_DATA Color = {0, 0, (UINT)m_bUseBT709, 0};
	const DXVAHD_STREAM_STATE_SOURCE_RECT_DATA Src = {TRUE, FrameRect};
	const DXVAHD_STREAM_STATE_DESTINATION_RECT_DATA Dest = {TRUE, FrameRect};

	IF_FAILED_RETURN(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_D3DFORMAT, sizeof(Dxva2Desc.Format), &Dxva2Desc.Format));
	IF_FAILED_RETURN(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_FRAME_FORMAT, sizeof(FrameFormat), &FrameFormat));
	IF_FAILED_RETURN(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE, sizeof(Color), &Color));
	IF_FAILED_RETURN(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_SOURCE_RECT, sizeof(Src), &Src));
	IF_FAILED_RETURN(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_DESTINATION_RECT, sizeof(Dest), &Dest));

	IF_FAILED_RETURN(GetDxva2Filter(pDXVAHD, DXVAHD_FILTER_BRIGHTNESS, &m_RangeBrightness));
	IF_FAILED_RETURN(GetDxva2Filter(pDXVAHD, DXVAHD_FILTER_CONTRAST, &m_RangeContrast));
	IF_FAILED_RETURN(GetDxva2Filter(pDXVAHD, DXVAHD_FILTER_HUE, &m_RangeHue));
	IF_FAILED_RETURN(GetDxva2Filter(pDXVAHD, DXVAHD_FILTER_SATURATION, &m_RangeSaturation));
	IF_FAILED_RETURN(GetDxva2Filter(pDXVAHD, DXVAHD_FILTER_NOISE_REDUCTION, &m_RangeNoiseReduction));
	IF_FAILED_RETURN(GetDxva2Filter(pDXVAHD, DXVAHD_FILTER_EDGE_ENHANCEMENT, &m_RangeEdgeEnhancement));
	IF_FAILED_RETURN(GetDxva2Filter(pDXVAHD, DXVAHD_FILTER_ANAMORPHIC_SCALING, &m_RangeAnamorphicScaling));

	return hr;
}

void CDxva2Renderer::ResetDxvaCaps(){

	memset(&m_RangeBrightness, 0, sizeof(m_RangeBrightness));
	memset(&m_RangeContrast, 0, sizeof(m_RangeContrast));
	memset(&m_RangeHue, 0, sizeof(m_RangeHue));
	memset(&m_RangeSaturation, 0, sizeof(m_RangeSaturation));
	memset(&m_RangeNoiseReduction, 0, sizeof(m_RangeNoiseReduction));
	memset(&m_RangeEdgeEnhancement, 0, sizeof(m_RangeEdgeEnhancement));
	memset(&m_RangeAnamorphicScaling, 0, sizeof(m_RangeAnamorphicScaling));
	memset(&m_BltStateAlphaFill, 0, sizeof(m_BltStateAlphaFill));
	memset(&m_StateConstriction, 0, sizeof(m_StateConstriction));
	memset(&m_StreamStateLumaKey, 0, sizeof(m_StreamStateLumaKey));
	memset(&m_StreamStateAlpha, 0, sizeof(m_StreamStateAlpha));
}

HRESULT CDxva2Renderer::GetDxva2Filter(IDXVAHD_Device* pDXVAHD, const DXVAHD_FILTER Filter, DXVAHD_FILTER_RANGE_DATA_EX* pFilterRange){

	HRESULT hr = S_OK;

	if(pFilterRange && pFilterRange->bAvailable){

		IF_FAILED_RETURN(pDXVAHD->GetVideoProcessorFilterRange(Filter, pFilterRange));
		pFilterRange->iCurrent = pFilterRange->Default;
		assert(pFilterRange->Multiplier == 1.0f);
	}

	return hr;
}

HRESULT CDxva2Renderer::SetBT709(const BOOL bUseBT709){

	HRESULT hr;
	IF_FAILED_RETURN(m_pDXVAVP ? S_OK : E_UNEXPECTED);

	const DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE_DATA Color = {0, 0, (UINT)bUseBT709, 0};

	IF_FAILED_RETURN(m_pDXVAVP->SetVideoProcessStreamState(0, DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE, sizeof(Color), &Color));

	m_bUseBT709 = bUseBT709;

	return hr;
}

#ifdef USE_DIRECTX9

HRESULT CDxva2Renderer::DrawMovieText(IDirect3DDevice9Ex* pDevice9Ex, const LONGLONG llMovieTime){

	HRESULT hr;
	WCHAR wszMovieTime[32];

	IF_FAILED_RETURN(GetMovieTimeText(wszMovieTime, llMovieTime));

	IF_FAILED_RETURN(pDevice9Ex->BeginScene());
	IF_FAILED_RETURN(m_cMovieTime.OnRender(wszMovieTime));
	IF_FAILED_RETURN(pDevice9Ex->EndScene());

	return hr;
}

HRESULT CDxva2Renderer::GetMovieTimeText(STRSAFE_LPWSTR wszBuffer, const LONGLONG llTime){

	HRESULT hr = S_OK;
	MFTIME Hours = 0;
	MFTIME Minutes = 0;
	MFTIME Seconds = 0;
	MFTIME MilliSeconds = 0;

	if(llTime < ONE_SECOND){

		MilliSeconds = llTime / ONE_MSEC;
	}
	else if(llTime < ONE_MINUTE){

		Seconds = llTime / ONE_SECOND;
		MilliSeconds = (llTime - (Seconds * ONE_SECOND)) / ONE_MSEC;
	}
	else if(llTime < ONE_HOUR){

		Minutes = llTime / ONE_MINUTE;
		LONGLONG llMinutes = Minutes * ONE_MINUTE;
		Seconds = (llTime - llMinutes) / ONE_SECOND;
		MilliSeconds = (llTime - (llMinutes + (Seconds * ONE_SECOND))) / ONE_MSEC;
	}
	else if(llTime < ONE_DAY){

		Hours = llTime / ONE_HOUR;
		LONGLONG llHours = Hours * ONE_HOUR;
		Minutes = (llTime - llHours) / ONE_MINUTE;
		LONGLONG llMinutes = Minutes * ONE_MINUTE;
		Seconds = (llTime - (llHours + llMinutes)) / ONE_SECOND;
		MilliSeconds = (llTime - (llHours + llMinutes + (Seconds * ONE_SECOND))) / ONE_MSEC;
	}
	else{

		memcpy(wszBuffer, L"Time more than one day", 46);
		return hr;
	}

	IF_FAILED_RETURN(StringCchPrintf(wszBuffer, 32, L"%02dh:%02dmn:%02ds:%03dms", (int)Hours, (int)Minutes, (int)Seconds, MilliSeconds));

	return hr;
}

#endif