//----------------------------------------------------------------------------------------------
// WindowsForm.h
//----------------------------------------------------------------------------------------------
#ifndef WINDOWSFORM_H
#define WINDOWSFORM_H

class CWindowsForm : public IMFAsyncCallback{

public:

	CWindowsForm();
	~CWindowsForm();

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID, void**);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFAsyncCallback
	STDMETHODIMP GetParameters(DWORD*, DWORD*){ return E_NOTIMPL; }
	STDMETHODIMP Invoke(IMFAsyncResult*);

	// WindowsForm.cpp
	HRESULT InitWindowsForm(const HINSTANCE);
	HRESULT Shutdown();
	// WindowsForm_Event.cpp
	LRESULT CALLBACK WindowsFormProc(HWND, UINT, WPARAM, LPARAM);

	// inline
	static CWindowsForm* GetWindowsForm(){ return m_pWindowsForm; }

private:

	volatile long m_nRefCount;
	static CWindowsForm* m_pWindowsForm;
	CPlayer* m_pPlayer;
	HINSTANCE m_hInst;
	HWND m_hWnd;
	HMENU m_hMenu;
	RECT m_rcLastWindowPos;
	PLAYER_STATE m_bPlayerState;
	BOOL m_bWasPlaying;
	CDxva2WindowsForm m_cDxva2WindowsForm;

	// WindowsForm.cpp
	HRESULT InitPlayer();
	void OnExit();
	HRESULT OnOpenFile(LPCWSTR);
	void OnPlay();
	void OnPause();
	void OnStop();
	void OnStep();
	void OnPlayPause();
	void OnSeekVideo(const int);
	void OnKeyDown(const WPARAM);
	BOOL OnCommand(const DWORD);
	void OnSysCommand(const WPARAM);
	void OnPaint(const HWND);
	void OnFullScreen(const BOOL);
	void OnWindowMessage(const int);
	void OnDrop(const WPARAM);
	void OnChooseFile();
	void OnDxva2Settings(const UINT, const INT);
	void OnResetDxva2Settings();
	void UpdateDxva2Settings();
};

#endif
