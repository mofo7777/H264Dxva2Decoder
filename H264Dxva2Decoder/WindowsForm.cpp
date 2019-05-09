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
	m_pPlayer(NULL),
	m_bPlayerState(STATE_STOPPING),
	m_bWasPlaying(FALSE)
{
	m_pWindowsForm = this;
}

CWindowsForm::~CWindowsForm(){

	m_cDxva2WindowsForm.Close();

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

	// todo : return error
	assert(pResult);
	assert(SUCCEEDED(pResult->GetStatus()));

	hr = pResult->GetObject(&pUnk);

	if(pUnk != NULL){

		CCallbackMessage* pMessage = static_cast<CCallbackMessage*>(pUnk);

		OnWindowMessage(pMessage->GetCallbackMessage());

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
	WndClassEx.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON));
	WndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClassEx.hbrBackground = NULL;
	WndClassEx.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	WndClassEx.lpszClassName = WINDOWSFORM_CLASS;
	WndClassEx.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON));

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

	IF_FAILED_RETURN(InitPlayer());

	GetWindowRect(m_hWnd, &m_rcLastWindowPos);

	m_hMenu = GetMenu(m_hWnd);

	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	UpdateWindow(m_hWnd);

	return S_OK;
}

HRESULT CWindowsForm::Shutdown(){

	HRESULT hr = S_OK;

	if(m_pPlayer){

		LOG_HRESULT(hr = m_pPlayer->Shutdown());
		SAFE_RELEASE(m_pPlayer);
	}

	LOG_HRESULT(MFShutdown());

	CoUninitialize();

	return hr;
}

HRESULT CWindowsForm::InitPlayer(){

	HRESULT hr = S_OK;
	HRESULT hrInitPlayer = S_OK;

	try{

		IF_FAILED_THROW(CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE));
		IF_FAILED_THROW(MFStartup(MF_VERSION, MFSTARTUP_LITE));

		m_pPlayer = new (std::nothrow)CPlayer(hrInitPlayer, this);

		IF_FAILED_THROW(m_pPlayer == NULL ? E_OUTOFMEMORY : S_OK);
		IF_FAILED_THROW(hrInitPlayer);

		IF_FAILED_THROW(m_pPlayer->Init());
	}
	catch(HRESULT){}

	return hr;
}

void CWindowsForm::OnExit(){

	if(m_pPlayer){

		LOG_HRESULT(m_pPlayer->Shutdown());
	}
}

HRESULT CWindowsForm::OnOpenFile(LPCWSTR lpwszFile){

	HRESULT hr;

	IF_FAILED_RETURN(m_pPlayer ? S_OK : E_UNEXPECTED);
	IF_FAILED_RETURN(m_pPlayer->Close());
	IF_FAILED_RETURN(m_pPlayer->OpenFile(m_hWnd, lpwszFile));

	UpdateDxva2Settings();

	IF_FAILED_RETURN(m_pPlayer->Play());

	return hr;
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

void CWindowsForm::OnStep(){

	if(m_pPlayer == NULL)
		return;

	m_pPlayer->Step();
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

void CWindowsForm::OnKeyDown(const WPARAM wParam){

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
		case VK_RIGHT: OnStep(); break;
		case F_KEY: OnFullScreen(TRUE); break;
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

		case ID_DXVA2PARAMETERS_SETTINGS:
			m_cDxva2WindowsForm.Open(m_hInst, m_hWnd);
			UpdateDxva2Settings();
			break;

		default:
			bNoCmd = TRUE;
	}

	return bNoCmd;
}

void CWindowsForm::OnSysCommand(const WPARAM wParam){

	if(wParam == SC_MINIMIZE || wParam == SC_MAXIMIZE){

		m_bWasPlaying = (m_bPlayerState == STATE_PLAYING);
		if(m_bWasPlaying)
			OnPause();
	}
	else if(wParam == SC_RESTORE){

		if(m_bWasPlaying)
			OnPlay();
	}
}

void CWindowsForm::OnPaint(const HWND hWnd){

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);

	if(m_pPlayer && m_bPlayerState != STATE_STOPPING){

		if(m_bPlayerState == STATE_PAUSING){

			m_pPlayer->RepaintVideo();
		}
	}
	else{

		RECT rc;

		GetClientRect(hWnd, &rc);
		FillRect(hdc, &rc, (HBRUSH)COLOR_WINDOW + 1);
	}

	EndPaint(hWnd, &ps);
}

void CWindowsForm::OnFullScreen(const BOOL bToggleFullScreen){

	long lStyleEx;
	long lstyle;
	BOOL bCursor;
	RECT rc;
	HWND hTop;

	if(bToggleFullScreen && GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) != WS_EX_TOPMOST){

		// Remember the last position
		if(GetWindowRect(m_hWnd, &m_rcLastWindowPos) == FALSE)
			return;

		lStyleEx = WS_EX_TOPMOST;
		lstyle = 0;
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

		rc.left = m_rcLastWindowPos.left;
		rc.top = m_rcLastWindowPos.top;
		rc.right = m_rcLastWindowPos.right - m_rcLastWindowPos.left;
		rc.bottom = m_rcLastWindowPos.bottom - m_rcLastWindowPos.top;

		SetMenu(m_hWnd, m_hMenu);
	}

	SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, lStyleEx);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, lstyle);

	ShowCursor(bCursor);

	SetWindowPos(m_hWnd, hTop, rc.left, rc.top, rc.right, rc.bottom, SWP_SHOWWINDOW);
}

void CWindowsForm::OnWindowMessage(const int iMessage){

	switch(iMessage){

		case WND_MSG_PLAYING:
			SetWindowText(m_hWnd, L"H264Dxva2Decoder 1.1.0.0 : playing");
			m_bPlayerState = STATE_PLAYING;
			break;

		case WND_MSG_PAUSING:
			SetWindowText(m_hWnd, L"H264Dxva2Decoder 1.1.0.0 : pausing");
			m_bPlayerState = STATE_PAUSING;
			break;

		case WND_MSG_STOPPING:
			SetWindowText(m_hWnd, L"H264Dxva2Decoder 1.1.0.0 : stopping");
			m_bPlayerState = STATE_STOPPING;
			break;

		case WND_MSG_FINISH:
			SetWindowText(m_hWnd, L"H264Dxva2Decoder 1.1.0.0 : finish");
			m_bPlayerState = STATE_STOPPING;
			break;
	}
}

void CWindowsForm::OnDrop(const WPARAM wParam){

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

void CWindowsForm::OnChooseFile(){

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	WCHAR wszBuffer[MAX_PATH];
	wszBuffer[0] = NULL;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = wszBuffer;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = TEXT("Select video file");
	ofn.Flags = OFN_HIDEREADONLY;

	if(GetOpenFileName(&ofn)){

		if(FAILED(OnOpenFile(wszBuffer)))
			MessageBox(m_hWnd, L"Failed to load video file", L"Error", MB_OK);
	}
}

void CWindowsForm::OnDxva2Settings(const UINT uiControlId, const INT iValue){

	if(m_pPlayer == NULL)
		return;

	m_pPlayer->OnFilter(uiControlId, iValue);
}

void CWindowsForm::OnResetDxva2Settings(){

	if(m_pPlayer == NULL)
		return;

	if(SUCCEEDED(m_pPlayer->OnResetDxva2Settings()))
		UpdateDxva2Settings();
}

void CWindowsForm::UpdateDxva2Settings(){

	if(m_pPlayer == NULL)
		return;

	DXVAHD_FILTER_RANGE_DATA_EX Filters[7];
	ZeroMemory(Filters, 7 * sizeof(DXVAHD_FILTER_RANGE_DATA_EX));

	BOOL bUseBT709 = FALSE;

	if(m_pPlayer->GetDxva2Settings(Filters, bUseBT709))
		m_cDxva2WindowsForm.UpdateDxva2Settings(Filters, bUseBT709);
}