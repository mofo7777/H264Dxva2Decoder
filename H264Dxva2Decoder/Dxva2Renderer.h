//----------------------------------------------------------------------------------------------
// Dxva2Renderer.h
//----------------------------------------------------------------------------------------------
#ifndef DXVA2RENDERER_H
#define DXVA2RENDERER_H

class CDxva2Renderer{

public:

	CDxva2Renderer();
	~CDxva2Renderer(){ OnRelease(); }

	// Dxva2Renderer.cpp
	HRESULT InitDXVA2(const HWND, const UINT, const UINT, const UINT, const UINT, DXVA2_VideoDesc&);
	void OnRelease();
	void Reset();
	HRESULT RenderFrame(IDirect3DSurface9**, const SAMPLE_PRESENTATION&);
	HRESULT RenderBlackFrame();
	HRESULT RenderLastFrame();
	HRESULT RenderLastFramePresentation(IDirect3DSurface9**);
	BOOL GetDxva2Settings(DXVAHD_FILTER_RANGE_DATA_EX*, BOOL&);
	HRESULT ResetDxva2Settings();
	HRESULT SetFilter(const UINT, const INT);

	// Inline
	IDirect3DDeviceManager9* GetDeviceManager9(){ return m_pDirect3DDeviceManager9; }
	const BOOL IsInitialized() const{ return m_pDXVAVP != NULL; }

private:

	// DirectX9Ex
	IDirect3D9Ex* m_pD3D9Ex;
	IDirect3DDevice9Ex* m_pDevice9Ex;
	IDirect3DDeviceManager9* m_pDirect3DDeviceManager9;
	UINT m_pResetToken;
	IDXVAHD_VideoProcessor* m_pDXVAVP;
	DWORD m_dwPicturePresent;
	SAMPLE_PRESENTATION m_LastPresentation;
#ifdef USE_DIRECTX9
	CText2D m_cMovieTime;
#endif

	DXVAHD_FILTER_RANGE_DATA_EX m_RangeBrightness;
	DXVAHD_FILTER_RANGE_DATA_EX m_RangeContrast;
	DXVAHD_FILTER_RANGE_DATA_EX m_RangeHue;
	DXVAHD_FILTER_RANGE_DATA_EX m_RangeSaturation;
	DXVAHD_FILTER_RANGE_DATA_EX m_RangeNoiseReduction;
	DXVAHD_FILTER_RANGE_DATA_EX m_RangeEdgeEnhancement;
	DXVAHD_FILTER_RANGE_DATA_EX m_RangeAnamorphicScaling;
	BOOL m_bUseBT709;

	// Informative
	DXVAHD_DEVICE_TYPE m_DxvaDeviceType;
	D3DPOOL m_InputPool;
	DXVAHD_BLT_STATE_ALPHA_FILL_DATA_EX m_BltStateAlphaFill;
	DXVAHD_BLT_STATE_CONSTRICTION_DATA_EX m_StateConstriction;
	DXVAHD_STREAM_STATE_LUMA_KEY_DATA_EX m_StreamStateLumaKey;
	DXVAHD_STREAM_STATE_ALPHA_DATA_EX m_StreamStateAlpha;

	// Dxva2Renderer.cpp
	HRESULT InitVideoProcessor(const DXVA2_VideoDesc);
	HRESULT ConfigureVideoProcessor(IDXVAHD_Device*, const DXVA2_VideoDesc);
	void ResetDxvaCaps();
	HRESULT GetDxva2Filter(IDXVAHD_Device*, const DXVAHD_FILTER, DXVAHD_FILTER_RANGE_DATA_EX*);
	HRESULT SetBT709(const BOOL);
#ifdef USE_DIRECTX9
	HRESULT DrawMovieText(IDirect3DDevice9Ex*, const LONGLONG);
	HRESULT GetMovieTimeText(STRSAFE_LPWSTR, const LONGLONG);
#endif
};

#endif