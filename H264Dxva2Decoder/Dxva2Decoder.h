//----------------------------------------------------------------------------------------------
// Dxva2Decoder.h
//----------------------------------------------------------------------------------------------
#ifndef DXVA2DECODER_H
#define DXVA2DECODER_H

// todo : check max NUM_DXVA2_SURFACE/h264/dxva2 capabilities
// todo : some files just need 4 NUM_DXVA2_SURFACE, others need 32, and perhaps others 64 (check how to known)
// the NUM_DXVA2_SURFACE seems to be determined by the number of times where a frame stays in the short ref buffer
#define NUM_DXVA2_SURFACE 64

const DWORD D3DFMT_NV12 = MAKEFOURCC('N', 'V', '1', '2');

#ifdef USE_D3D_SURFACE_ALIGMENT
#define D3DALIGN(x, a) (((x)+(a)-1)&~((a)-1))
#endif

class CDxva2Decoder{

public:

	CDxva2Decoder();
	~CDxva2Decoder(){ OnRelease(); }

	HRESULT InitVideoDecoder(IDirect3DDeviceManager9*, const DXVA2_VideoDesc*, const SPS_DATA&);
	void OnRelease();
	void Reset();
	HRESULT DecodeFrame(CMFBuffer&, const PICTURE_INFO&, const LONGLONG&, const int);
	BOOL CheckFrame(SAMPLE_PRESENTATION&);
	HRESULT AddSliceShortInfo(const int, const DWORD, const BOOL);
	void FreeSurfaceIndexRenderer(const DWORD);

	// Inline
	void ClearPresentation(){ m_dqPicturePresentation.clear(); memset(g_Dxva2SurfaceIndexV2, 0, sizeof(g_Dxva2SurfaceIndexV2)); }
	DWORD PictureToDisplayCount() const{ return (DWORD)m_dqPicturePresentation.size(); }
	void SetCurrentNalu(const NAL_UNIT_TYPE eNalUnitType, const BYTE btNalRefIdc){ m_eNalUnitType = eNalUnitType; m_btNalRefIdc = btNalRefIdc; }
	IDirect3DSurface9** GetDirect3DSurface9(){ return m_pSurface9; }
	const BOOL IsInitialized() const{ return m_pVideoDecoder != NULL; }

private:

	// Dxva2
	IDirectXVideoDecoder* m_pVideoDecoder;
	IDirect3DSurface9* m_pSurface9[NUM_DXVA2_SURFACE];
	IDirect3DDeviceManager9* m_pDirect3DDeviceManager9;
	HANDLE m_hD3d9Device;
	DXVA2_ConfigPictureDecode* m_pConfigs;
	GUID m_gH264Vld;

	DXVA2_DecodeExecuteParams m_ExecuteParams;
	DXVA2_DecodeBufferDesc m_BufferDesc[4];
	DXVA_PicParams_H264 m_H264PictureParams;
	DXVA_Qmatrix_H264 m_H264QuantaMatrix;
	DXVA_Slice_H264_Short m_H264SliceShort[MAX_SUB_SLICE];
	DWORD m_dwStatusReportFeedbackNumber;

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
		LONGLONG llTime;
	};

	deque<POC> m_dqPoc;
	deque<PICTURE_PRESENTATION> m_dqPicturePresentation;
	INT m_iPrevTopFieldOrderCount;
	NAL_UNIT_TYPE m_eNalUnitType;
	BYTE m_btNalRefIdc;

	CriticSection m_CriticSection;

	struct DXVA2_SURFACE_INDEX{

		BOOL bUsedByDecoder;
		BOOL bUsedByRenderer;
		BOOL bNalRef;
	};

	DXVA2_SURFACE_INDEX g_Dxva2SurfaceIndexV2[NUM_DXVA2_SURFACE];

	HRESULT InitDecoderService(IDirectXVideoDecoderService*);
	void InitDxva2Struct(const SPS_DATA&);
	void InitPictureParams(const DWORD, const PICTURE_INFO&);
	void InitQuantaMatrixParams(const SPS_DATA&);
	HRESULT AddNalUnitBufferPadding(CMFBuffer&, const UINT);
	void HandlePOC(const DWORD, const PICTURE_INFO&, const LONGLONG&);
	void ErasePastFrames(const LONGLONG);
	DWORD GetFreeSurfaceIndex();
	void ResetAllSurfaceIndex();
};

#endif