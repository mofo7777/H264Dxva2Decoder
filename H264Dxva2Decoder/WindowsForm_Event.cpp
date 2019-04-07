//----------------------------------------------------------------------------------------------
// WindowsForm_Event.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

LRESULT CALLBACK CWindowsForm::WindowsFormProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){

	switch(msg){

		case WM_PAINT:
			OnPaint(hWnd);
			break;

		case WM_ERASEBKGND:
			// Suppress window erasing, to reduce flickering while the video is playing.
			return 1L;

		case WM_LBUTTONDBLCLK:
			if(wParam == MK_LBUTTON)
				OnFullScreen(TRUE);
			break;

		case WM_KEYDOWN:
			OnKeyDown(wParam);
			break;

		case WM_COMMAND:

			if(OnCommand(LOWORD(wParam))){
				return DefWindowProc(hWnd, msg, wParam, lParam);
			}
			break;

		case WM_DROPFILES:
			OnDrop(wParam);
			break;

		case WM_DESTROY:
			OnExit();
			PostQuitMessage(0);
			break;

		case WM_QUERYENDSESSION:
			OnExit();
			return DefWindowProc(hWnd, msg, TRUE, lParam);

		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return 0L;
}