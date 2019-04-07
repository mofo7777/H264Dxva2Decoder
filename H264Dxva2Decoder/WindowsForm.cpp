//----------------------------------------------------------------------------------------------
// WindowsForm.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

CWindowsForm* CWindowsForm::m_pWindowsForm = NULL;

LRESULT CALLBACK WindowsFormMsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){

	return CWindowsForm::GetWindowsForm()->WindowsFormProc(hWnd, msg, wParam, lParam);
}

CWindowsForm::CWindowsForm() :
	m_hInst(NULL),
	m_hWnd(NULL),
	m_hMenu(NULL),
	m_nRefCount(1),
	m_pPlayer(NULL)
{
	m_pWindowsForm = this;
}

CWindowsForm::~CWindowsForm(){

	if(IsWindow(m_hWnd)){
		DestroyWindow(m_hWnd);
	}

	UnregisterClass(WINDOWSFORM_CLASS, m_hInst);
}

HRESULT CWindowsForm::QueryInterface(REFIID riid, void** ppv){

	static const QITAB qit[] = {
		QITABENT(CWindowsForm, IMFAsyncCallback),
	{0}
	};

	return QISearch(this, qit, riid, ppv);
}

ULONG CWindowsForm::AddRef(){

	LONG lRef = InterlockedIncrement(&m_nRefCount);

	TRACE_REFCOUNT((L"CWindowsForm::AddRef m_nRefCount = %d", lRef));

	return lRef;
}

ULONG CWindowsForm::Release(){

	ULONG uCount = InterlockedDecrement(&m_nRefCount);

	TRACE_REFCOUNT((L"CWindowsForm::Release m_nRefCount = %d", uCount));

	if(uCount == 0){
		delete this;
	}

	return uCount;
}

HRESULT CWindowsForm::Invoke(IMFAsyncResult* pResult){

	HRESULT hr = S_OK;
	IUnknown* pUnk = NULL;

	assert(pResult);
	assert(SUCCEEDED(pResult->GetStatus()));

	hr = pResult->GetObject(&pUnk);

	if(pUnk != NULL){

		CWindowMessage* pWindowMessage = static_cast<CWindowMessage*>(pUnk);

		OnWindowMessage(pWindowMessage->GetWindowMessage());

		SAFE_RELEASE(pUnk);
	}

	return hr;
}

HRESULT CWindowsForm::InitWindowsForm(const HINSTANCE hInst){

	HRESULT hr = E_FAIL;
	m_hInst = hInst;

	WNDCLASSEX WndClassEx;

	WndClassEx.cbSize = sizeof(WNDCLASSEX);
	WndClassEx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	WndClassEx.lpfnWndProc = WindowsFormMsgProc;
	WndClassEx.cbClsExtra = 0L;
	WndClassEx.cbWndExtra = 0L;
	WndClassEx.hInstance = hInst;
	WndClassEx.hIcon = NULL;
	WndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClassEx.hbrBackground = NULL;
	WndClassEx.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	WndClassEx.lpszClassName = WINDOWSFORM_CLASS;
	WndClassEx.hIconSm = NULL;

	if(!RegisterClassEx(&WndClassEx)){
		return hr;
	}

	int iWndL = 1280 + GetSystemMetrics(SM_CXSIZEFRAME) * 4,
		iWndH = 720 + GetSystemMetrics(SM_CYSIZEFRAME) * 4 + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU);

	int iXWnd = (GetSystemMetrics(SM_CXSCREEN) - iWndL) / 2,
		iYWnd = (GetSystemMetrics(SM_CYSCREEN) - iWndH) / 2;

	if((m_hWnd = CreateWindowEx(WS_EX_ACCEPTFILES, WINDOWSFORM_CLASS, WINDOWSFORM_CLASS, WS_OVERLAPPEDWINDOW, iXWnd, iYWnd,
		iWndL, iWndH, GetDesktopWindow(), NULL, m_hInst, NULL)) == NULL){
		return hr;
	}

	IF_FAILED_RETURN(InitMediaFoundation());

	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	UpdateWindow(m_hWnd);

	m_hMenu = GetMenu(m_hWnd);

	/*RECT rc;
	GetClientRect(m_hWnd, &rc);*/

	return S_OK;
}

HRESULT CWindowsForm::Shutdown(){

	HRESULT hr;

#ifdef _DEBUG
	if(m_pPlayer){

		assert(m_pPlayer->IsShutdown());
	}
#endif

	SAFE_RELEASE(m_pPlayer);

	LOG_HRESULT(hr = MFShutdown());

	CoUninitialize();

	return hr;
}

HRESULT CWindowsForm::InitMediaFoundation(){

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if(SUCCEEDED(hr))
		hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);

	return hr;
}

HRESULT CWindowsForm::OnOpenFile(LPCWSTR lpwszFile){

	HRESULT hr;

	if(m_pPlayer == NULL){

		HRESULT hrInit = S_OK;
		m_pPlayer = new (std::nothrow)CPlayer(hrInit, this);

		IF_FAILED_RETURN(hrInit);
		IF_FAILED_RETURN(m_pPlayer == NULL ? E_OUTOFMEMORY : S_OK);
	}
	else{

		IF_FAILED_RETURN(m_pPlayer->Close());
	}

	IF_FAILED_RETURN(m_pPlayer->OpenFile(m_hWnd, lpwszFile));

	return hr;
}

void CWindowsForm::OnExit(){

	if(m_pPlayer){

		LOG_HRESULT(m_pPlayer->Shutdown());
	}
}

void CWindowsForm::OnPaint(const HWND hWnd){

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);

	if(m_pPlayer == NULL){

		RECT rc;
		GetClientRect(hWnd, &rc);
		FillRect(hdc, &rc, (HBRUSH)COLOR_WINDOW + 1);
	}

	EndPaint(hWnd, &ps);
}

void CWindowsForm::OnKeyDown(WPARAM wParam){

	switch(wParam){

		case VK_NUMPAD0: OnSeekVideo(10); break;
		case VK_NUMPAD1: OnSeekVideo(1); break;
		case VK_NUMPAD2: OnSeekVideo(2); break;
		case VK_NUMPAD3: OnSeekVideo(3); break;
		case VK_NUMPAD4: OnSeekVideo(4); break;
		case VK_NUMPAD5: OnSeekVideo(5); break;
		case VK_NUMPAD6: OnSeekVideo(6); break;
		case VK_NUMPAD7: OnSeekVideo(7); break;
		case VK_NUMPAD8: OnSeekVideo(8); break;
		case VK_NUMPAD9: OnSeekVideo(9); break;
		case VK_SPACE: OnPlayPause(); break;
		case P_KEY: OnPlay(); break;
		case S_KEY: OnStop(); break;
		case VK_ESCAPE: OnFullScreen(FALSE); break;
	}
}

BOOL CWindowsForm::OnCommand(const DWORD dwCmd){

	BOOL bNoCmd = FALSE;

	switch(dwCmd){

		case ID_CONTROL_PLAY:
			OnPlay();
			break;

		case ID_CONTROL_PAUSE:
			OnPause();
			break;

		case ID_CONTROL_STOP:
			OnStop();
			break;

		case ID_FILE_EXIT:
			OnExit();
			PostQuitMessage(0);
			break;

		case ID_FILE_OPEN:
			OnChooseFile();
			break;

		default:
			bNoCmd = TRUE;
	}

	return bNoCmd;
}

void CWindowsForm::OnDrop(WPARAM wParam){

	HDROP hDrop = (HDROP)wParam;
	UINT uiFile = 0;

	uiFile = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, NULL);

	if(uiFile != 1){

		// Just one file at a time
		DragFinish(hDrop);
		return;
	}

	UINT uiCount = DragQueryFile(hDrop, 0, NULL, NULL);
	WCHAR* pwszFileName = NULL;

	if(uiCount != 0 && uiCount < MAX_PATH){

		UINT uiSize = MAX_PATH * sizeof(WCHAR);

		pwszFileName = new WCHAR[uiSize];

		if(DragQueryFile(hDrop, 0, pwszFileName, uiSize)){

			OnOpenFile(pwszFileName);
		}
	}

	DragFinish(hDrop);

	if(pwszFileName){
		delete[] pwszFileName;
		pwszFileName = NULL;
	}
}

void CWindowsForm::OnPlay(){

	if(m_pPlayer == NULL)
		return;

	m_pPlayer->Play();
}

void CWindowsForm::OnPause(){

	if(m_pPlayer == NULL)
		return;

	m_pPlayer->Pause();
}

void CWindowsForm::OnStop(){

	if(m_pPlayer == NULL)
		return;

	m_pPlayer->Stop();
}

void CWindowsForm::OnPlayPause(){

	if(m_pPlayer == NULL)
		return;

	m_pPlayer->PlayPause();
}

void CWindowsForm::OnSeekVideo(const int iDurationToSeek){

	if(m_pPlayer == NULL)
		return;

	m_pPlayer->SeekVideo(iDurationToSeek * ONE_MINUTE);
}

void CWindowsForm::OnChooseFile(){

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	WCHAR wszBuffer[MAX_PATH];
	wszBuffer[0] = NULL;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	//ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = wszBuffer;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = TEXT("Select video file");
	ofn.Flags = OFN_HIDEREADONLY;
	//ofn.lpstrDefExt = TEXT("WAV");

	if(GetOpenFileName(&ofn)){

		if(FAILED(OnOpenFile(wszBuffer)))
			MessageBox(m_hWnd, L"Failed to load video file", L"Error", MB_OK);
	}
}

void CWindowsForm::OnFullScreen(const BOOL bToggleFullScreen){

	long lStyleEx;
	long lstyle;
	BOOL bCursor;
	RECT rc;
	HWND hTop;

	if(bToggleFullScreen && GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) != WS_EX_TOPMOST){

		// Remember the last position
		if(GetWindowRect(m_hWnd, &m_rcWindow) == FALSE)
			return;

		lStyleEx = WS_EX_TOPMOST;
		lstyle = WS_OVERLAPPEDWINDOW & ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU);
		bCursor = FALSE;
		hTop = HWND_TOPMOST;

		rc.left = 0;
		rc.top = 0;
		// We should manage multiple monitors, if present
		rc.right = GetSystemMetrics(SM_CXSCREEN);
		rc.bottom = GetSystemMetrics(SM_CYSCREEN);

		SetMenu(m_hWnd, NULL);
	}
	else{

		if(GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) == WS_EX_ACCEPTFILES)
			return;

		lStyleEx = WS_EX_ACCEPTFILES;
		lstyle = WS_OVERLAPPEDWINDOW;
		bCursor = TRUE;
		hTop = HWND_NOTOPMOST;

		rc.left = m_rcWindow.left;
		rc.top = m_rcWindow.top;
		rc.right = m_rcWindow.right - m_rcWindow.left;
		rc.bottom = m_rcWindow.bottom - m_rcWindow.top;

		SetMenu(m_hWnd, m_hMenu);
	}

	SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, lStyleEx);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, lstyle);

	ShowCursor(bCursor);

	SetWindowPos(m_hWnd, hTop, rc.left, rc.top, rc.right, rc.bottom, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

	InvalidateRect(NULL, NULL, TRUE);
}

void CWindowsForm::OnWindowMessage(const int iMessage){

	switch(iMessage){

		case WND_MSG_PLAYING:
			SetWindowText(m_hWnd, L"H264Dxva2Decoder 1.0.0.0 : playing");
			break;

		case WND_MSG_PAUSING:
			SetWindowText(m_hWnd, L"H264Dxva2Decoder 1.0.0.0 : pausing");
			break;

		case WND_MSG_STOPPING:
			SetWindowText(m_hWnd, L"H264Dxva2Decoder 1.0.0.0 : stopping");
			break;

		case WND_MSG_FINISH:
			SetWindowText(m_hWnd, L"H264Dxva2Decoder 1.0.0.0 : finish");
			break;
	}
}