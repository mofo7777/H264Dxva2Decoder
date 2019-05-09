//----------------------------------------------------------------------------------------------
// WindowsForm_Event.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

LRESULT CALLBACK CWindowsForm::WindowsFormProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam){

	switch(uiMsg){

		case WM_ERASEBKGND:
			return 1L;

		case WM_PAINT:
			OnPaint(hWnd);
			break;

		case WM_ENTERSIZEMOVE:
			// todo : OnCompletePause()
			m_bWasPlaying = (m_bPlayerState == STATE_PLAYING);
			if(m_bWasPlaying)
				OnPause();
			break;

		case WM_EXITSIZEMOVE:
			if(m_bWasPlaying)
				OnPlay();
			break;

		case WM_LBUTTONDBLCLK:
			if(wParam == MK_LBUTTON)
				OnFullScreen(TRUE);
			break;

		case WM_COMMAND:
			if(OnCommand(LOWORD(wParam))){
				return DefWindowProc(hWnd, uiMsg, wParam, lParam);
			}
			break;

		case WM_KEYDOWN:
			OnKeyDown(wParam);
			break;

		case WM_SYSCOMMAND:
			OnSysCommand(wParam);
			return DefWindowProc(hWnd, uiMsg, wParam, lParam);

		case WM_DXVA2_SETTINGS:
			OnDxva2Settings((UINT)wParam, (INT)lParam);
			break;

		case WM_DXVA2_SETTINGS_RESET:
			OnResetDxva2Settings();
			break;

		case WM_DROPFILES:
			OnDrop(wParam);
			break;

		case WM_CLOSE:
			if(IsWindow(m_hWnd)){
				DestroyWindow(m_hWnd);
				m_hWnd = NULL;
			}
			return DefWindowProc(hWnd, uiMsg, wParam, lParam);

		case WM_DESTROY:
			OnExit();
			PostQuitMessage(0);
			break;

		case WM_QUERYENDSESSION:
			OnExit();
			return DefWindowProc(hWnd, uiMsg, TRUE, lParam);

		default:
			return DefWindowProc(hWnd, uiMsg, wParam, lParam);
	}

	return 0L;
}