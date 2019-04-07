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

	static CWindowsForm* m_pWindowsForm;
	CPlayer* m_pPlayer;

	HINSTANCE m_hInst;
	HWND m_hWnd;
	HMENU m_hMenu;
	RECT m_rcWindow;
	volatile long m_nRefCount;

	// WindowsForm.cpp
	HRESULT InitMediaFoundation();
	HRESULT OnOpenFile(LPCWSTR);
	void OnExit();
	void OnPaint(const HWND);
	void OnKeyDown(WPARAM);
	BOOL OnCommand(const DWORD);
	void OnDrop(WPARAM);
	void OnPlay();
	void OnPause();
	void OnStop();
	void OnPlayPause();
	void OnSeekVideo(const int);
	void OnChooseFile();
	void OnFullScreen(const BOOL);
	void OnWindowMessage(const int);
};

#endif
