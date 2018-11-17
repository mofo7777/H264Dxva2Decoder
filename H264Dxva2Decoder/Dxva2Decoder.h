//----------------------------------------------------------------------------------------------
// Dxva2Decoder.h
//----------------------------------------------------------------------------------------------
#ifndef DXVA2DECODER_H
#define DXVA2DECODER_H

// todo : check max NUM_DXVA2_SURFACE/h264/dxva2 capabilities
// todo : some files just need 4 NUM_DXVA2_SURFACE, others need 32, and perhaps others 64 (check how to known)
// the NUM_DXVA2_SURFACE seems to be determined by the number of times where a frame stays in the short ref buffer
#define NUM_DXVA2_SURFACE 32

class CDxva2Decoder{

public:

	CDxva2Decoder();
	~CDxva2Decoder(){ OnRelease(); }

	HRESULT InitDXVA2(const SPS_DATA&, const UINT, const UINT, const UINT, const UINT);
	void OnRelease();
	HRESULT DecodeFrame(CMFBuffer&, const PICTURE_INFO&);
	HRESULT DisplayFrame();
	DWORD PictureToDisplayCount() const{ return m_dqPicturePresentation.size(); }

private:

	// Window
	HINSTANCE m_hInst;
	HWND      m_hWnd;

	// DirectX9Ex
	IDirect3D9Ex* m_pD3D9Ex;
	IDirect3DDevice9Ex* m_pDevice9Ex;

	// Dxva2
	UINT m_pResetToken;
	IDirect3DDeviceManager9* m_pDXVAManager;
	IDirectXVideoDecoderService* m_pDecoderService;
	IDirectXVideoDecoder* m_pVideoDecoder;
	IDirect3DSurface9* m_pSurface9[NUM_DXVA2_SURFACE];
	DXVA2_VideoDesc m_Dxva2Desc;
	HANDLE m_hD3d9Device;
	DXVA2_ConfigPictureDecode* m_pConfigs;
	GUID m_gH264Vld;

	DXVA2_DecodeExecuteParams m_ExecuteParams;
	DXVA2_DecodeBufferDesc m_BufferDesc[4];
	DXVA_PicParams_H264 m_H264PictureParams;
	DXVA_Qmatrix_H264 m_H264QuantaMatrix;
	DXVA_Slice_H264_Short m_H264SliceInfo[MAX_SLICE];// Todo check MAX_SLICE for h264
	DWORD m_dwCurPictureId;
	DWORD m_dwPicturePresent;
	DWORD m_dwPauseDuration;

	IDXVAHD_VideoProcessor* m_pDXVAVP;

	struct POC{

		UINT TopFieldOrderCnt;
		USHORT usFrameList;
		UCHAR bRefFrameList;
		USHORT usSliceType;
		BOOL bLongRef;
	};

	struct PICTURE_PRESENTATION{

		INT TopFieldOrderCnt;
		DWORD dwDXVA2Index;
		SLICE_TYPE SliceType;
	};

	deque<POC> m_dqPoc;
	deque<PICTURE_PRESENTATION> m_dqPicturePresentation;
	INT m_iPrevTopFieldOrderCount;

	HRESULT InitForm(const UINT, const UINT);
	HRESULT InitDecoderService();
	HRESULT InitVideoDecoder(const SPS_DATA&);
	void InitDxva2Struct(const SPS_DATA&);
	void InitPictureParams(const DWORD, const PICTURE_INFO&);
	void InitQuantaMatrixParams(const SPS_DATA&);
	HRESULT AddNalUnitBufferPadding(CMFBuffer&, const UINT);
	void HandlePOC(const DWORD, const PICTURE_INFO&);

	// Dxva2Decoder_DisplayFrame.cpp
	HRESULT RenderFrame();
	HRESULT InitVideoProcessor();
	HRESULT ConfigureVideoProcessor();
};

#endif