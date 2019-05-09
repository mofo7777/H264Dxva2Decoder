//----------------------------------------------------------------------------------------------
// WinMain.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int){

	CWindowsForm* pWindowsForm = new (std::nothrow)CWindowsForm();

	if(pWindowsForm == NULL)
		return -1;

	if(FAILED(pWindowsForm->InitWindowsForm(hInst))){

		LOG_HRESULT(pWindowsForm->Shutdown());
		SAFE_RELEASE(pWindowsForm);
		return -1;
	}

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while(GetMessage(&msg, NULL, 0, 0) > 0){

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	LOG_HRESULT(pWindowsForm->Shutdown());
	SAFE_RELEASE(pWindowsForm);

	return static_cast<int>(msg.wParam);
}