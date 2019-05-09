//----------------------------------------------------------------------------------------------
// Player.h
//----------------------------------------------------------------------------------------------
#ifndef PLAYER_H
#define PLAYER_H

class CPlayer : public IMFAsyncCallback{

public:

	CPlayer(HRESULT& hr, IMFAsyncCallback*);
	~CPlayer();
	
	// IUnknown
	STDMETHODIMP QueryInterface(REFIID, void**);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFAsyncCallback
	STDMETHODIMP GetParameters(DWORD*, DWORD*){ return E_NOTIMPL; }
	STDMETHODIMP Invoke(IMFAsyncResult*);

	HRESULT Init();
	HRESULT OpenFile(const HWND, LPCWSTR);
	HRESULT Play();
	HRESULT Pause();
	HRESULT Stop();
	HRESULT Step();
	HRESULT PlayPause();
	HRESULT Close();
	HRESULT Shutdown();
	HRESULT SeekVideo(const MFTIME);
	HRESULT RepaintVideo();
	HRESULT OnFilter(const UINT, const INT);
	HRESULT OnResetDxva2Settings();

	// Inline
	const BOOL IsShutdown() const{ return m_bIsShutdown; }
	BOOL GetDxva2Settings(DXVAHD_FILTER_RANGE_DATA_EX* pFilters, BOOL& bUseBT709){ return m_cDxva2Renderer.GetDxva2Settings(pFilters, bUseBT709); }

private:

	volatile long m_nRefCount;
	CriticSection m_CriticSection;
	CriticSection m_CriticSectionDecoding;
	CH264AtomParser m_cH264AtomParser;
	CH264NaluParser m_cH264NaluParser;
	CDxva2Decoder m_cDxva2Decoder;
	CDxva2Renderer m_cDxva2Renderer;
	IMFAsyncCallback* m_pWindowCallback;
	CMFBuffer m_pVideoBuffer;
	CMFBuffer m_pNalUnitBuffer;
	BOOL m_bStop;
	BOOL m_bPause;
	BOOL m_bSeek;
	BOOL m_bStep;
	BOOL m_bFinish;
	HANDLE m_hEventEndRendering;
	int m_iNaluLenghtSize;
	DWORD m_dwTrackId;
	DWORD m_dwWorkQueueRendering;
	DWORD m_dwWorkQueueDecoding;
	DWORD m_dwMessageQueue;
	BOOL m_bIsShutdown;
	deque<SAMPLE_PRESENTATION> m_dqPresentation;
	LONGLONG m_llPerFrame_1_4th;
	LONGLONG m_llPerFrame_3_4th;

#ifdef USE_MMCSS_WORKQUEUE
	HANDLE m_hMMCSS;
	BOOL m_bIsMMCSS;

	HRESULT UnregisterMMCSS();
#endif

	HRESULT ProcessDecoding();
	HRESULT ProcessRendering();
	HRESULT SendMessageToPipeline(const DWORD, const int);
	HRESULT SendMessageToWindow(const int);
	void StopEvent();
	HRESULT AddByteAndConvertAvccToAnnexB(CMFBuffer&);
};

#endif
