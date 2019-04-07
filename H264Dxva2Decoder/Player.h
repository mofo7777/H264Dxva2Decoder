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

	HRESULT OpenFile(const HWND, LPCWSTR);
	HRESULT Play();
	HRESULT Pause();
	HRESULT Stop();
	HRESULT PlayPause();
	HRESULT Close();
	HRESULT Shutdown();
	HRESULT SeekVideo(const MFTIME);

	const BOOL IsShutdown() const{ return m_bIsShutdown; }

private:

	volatile long m_nRefCount;

	CH264AtomParser m_cH264AtomParser;
	CH264NaluParser m_cH264NaluParser;
	CDxva2Decoder m_cDxva2Decoder;
	IMFAsyncCallback* m_pWindowCallback;
	BOOL m_bStop;
	BOOL m_bPause;
	BOOL m_bSeek;
	HANDLE m_hEvent;
	int m_iNaluLenghtSize;
	DWORD m_dwTrackId;
	DWORD m_dwWorkQueue;
	DWORD m_dwMessageQueue;
	BOOL m_bIsShutdown;

	HRESULT SendMessageToWindow(const int);
	void StopEvent();
	HRESULT AddByteAndConvertAvccToAnnexB(CMFBuffer&);
};

#endif
