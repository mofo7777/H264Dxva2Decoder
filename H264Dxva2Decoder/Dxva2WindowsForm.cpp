//----------------------------------------------------------------------------------------------
// Dxva2WindowsForm.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

CDxva2WindowsForm* CDxva2WindowsForm::m_pDxva2WindowsForm = NULL;

INT_PTR CALLBACK Dxva2WindowsFormMsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){

	return CDxva2WindowsForm::GetDxva2WindowsForm()->Dxva2WindowsFormProc(hWnd, msg, wParam, lParam);
}

void CDxva2WindowsForm::Open(const HINSTANCE hInst, const HWND hwndParent){

	if(m_hwndDlg != NULL)
		return;

	m_hwndDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG), hwndParent, Dxva2WindowsFormMsgProc, 0);

	if(m_hwndDlg == NULL)
		return;

	m_hwndParent = hwndParent;

	HWND hDlgItem = GetDlgItem(m_hwndDlg, IDC_RADIO_BT601);
	SendMessage(hDlgItem, BM_SETCHECK, BST_CHECKED, 0);
}

void CDxva2WindowsForm::Close(){

	if(m_hwndDlg == NULL)
		return;

	DestroyWindow(m_hwndDlg);
	m_hwndDlg = NULL;
	m_hwndParent = NULL;
}

void CDxva2WindowsForm::UpdateDxva2Settings(DXVAHD_FILTER_RANGE_DATA_EX* pFilters, const BOOL bUseBT709){

	if(m_hwndDlg == NULL)
		return;

	UpdateDxva2Filter(ID_FILTER_BRIGHTNESS, IDC_STATIC_LBRIGHTNESS, IDC_STATIC_RBRIGHTNESS, pFilters[0]);
	UpdateDxva2Filter(ID_FILTER_CONTRAST, IDC_STATIC_LCONTRAST, IDC_STATIC_RCONTRAST, pFilters[1]);
	UpdateDxva2Filter(ID_FILTER_HUE, IDC_STATIC_LHUE, IDC_STATIC_RHUE, pFilters[2]);
	UpdateDxva2Filter(ID_FILTER_SATURATION, IDC_STATIC_LSATURATION, IDC_STATIC_RSATURATION, pFilters[3]);
	UpdateDxva2Filter(ID_FILTER_NOISE_REDUCTION, IDC_STATIC_LNOISE_REDUCTION, IDC_STATIC_RNOISE_REDUCTION, pFilters[4]);
	UpdateDxva2Filter(ID_FILTER_EDGE_ENHANCEMENT, IDC_STATIC_LEDGE_ENHANCEMENT, IDC_STATIC_REDGE_ENHANCEMENT, pFilters[5]);
	UpdateDxva2Filter(ID_FILTER_ANAMORPHIC_SCALING, IDC_STATIC_LANAMORPHIC_SCALING, IDC_STATIC_RANAMORPHIC_SCALING, pFilters[6]);

	HWND hDlgItem = GetDlgItem(m_hwndDlg, IDC_BUTTON_RESET);
	EnableWindow(hDlgItem, TRUE);

	hDlgItem = GetDlgItem(m_hwndDlg, IDC_RADIO_BT601);
	EnableWindow(hDlgItem, TRUE);
	SendMessage(hDlgItem, BM_SETCHECK, bUseBT709 ? BST_UNCHECKED : BST_CHECKED, 0);

	hDlgItem = GetDlgItem(m_hwndDlg, IDC_RADIO_BT709);
	EnableWindow(hDlgItem, TRUE);
	SendMessage(hDlgItem, BM_SETCHECK, bUseBT709 ? BST_CHECKED : BST_UNCHECKED, 0);
}

HRESULT CDxva2WindowsForm::UpdateDxva2Filter(const int iDlgItem, const int iStaticLeft, const int iStaticRight, DXVAHD_FILTER_RANGE_DATA_EX& Filter){

	HWND hDlgItem = GetDlgItem(m_hwndDlg, iDlgItem);
	EnableWindow(hDlgItem, Filter.bAvailable);

	if(Filter.bAvailable){

		SendMessage(hDlgItem, TBM_SETPAGESIZE, 0, 10);
		SendMessage(hDlgItem, TBM_SETRANGEMIN, (WPARAM)TRUE, (LPARAM)Filter.Minimum);
		SendMessage(hDlgItem, TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)Filter.Maximum);
		SendMessage(hDlgItem, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)Filter.iCurrent);
	}

	HRESULT hr;
	WCHAR wszValue[12];

	IF_FAILED_RETURN(StringCchPrintf(wszValue, 12, L"%d", Filter.Minimum));
	hDlgItem = GetDlgItem(m_hwndDlg, iStaticLeft);
	SendMessage(hDlgItem, WM_SETTEXT, 0, (LPARAM)wszValue);

	IF_FAILED_RETURN(StringCchPrintf(wszValue, 12, L"%d", Filter.Maximum));
	hDlgItem = GetDlgItem(m_hwndDlg, iStaticRight);
	SendMessage(hDlgItem, WM_SETTEXT, 0, (LPARAM)wszValue);

	return hr;
}

void CDxva2WindowsForm::OnHorizontalScroll(const HWND hDlgItem){

	UINT uiValue;
	int iControlId;

	if((uiValue = (UINT)SendMessage(hDlgItem, TBM_GETPOS, 0, 0)) == CB_ERR)
		return;

	if((iControlId = GetDlgCtrlID(hDlgItem)) == 0)
		return;

	PostMessage(m_hwndParent, WM_DXVA2_SETTINGS, iControlId, uiValue);
}

void CDxva2WindowsForm::OnClicked(LPARAM lParam){

	HWND hDlgItem = GetDlgItem(m_hwndDlg, IDC_BUTTON_RESET);

	if(hDlgItem == (HWND)lParam){

		PostMessage(m_hwndParent, WM_DXVA2_SETTINGS_RESET, 0, 0);
		return;
	}

	hDlgItem = GetDlgItem(m_hwndDlg, IDC_RADIO_BT601);

	if(hDlgItem == (HWND)lParam){

		PostMessage(m_hwndParent, WM_DXVA2_SETTINGS, IDC_RADIO_BT601, 0);
		return;
	}

	hDlgItem = GetDlgItem(m_hwndDlg, IDC_RADIO_BT709);

	if(hDlgItem == (HWND)lParam){

		PostMessage(m_hwndParent, WM_DXVA2_SETTINGS, IDC_RADIO_BT601, 1);
		return;
	}
}

BOOL CALLBACK CDxva2WindowsForm::Dxva2WindowsFormProc(HWND, UINT uiMsg, WPARAM wParam, LPARAM lParam){

	switch(uiMsg){

		case WM_HSCROLL:
			if(LOWORD(wParam) == TB_THUMBTRACK || LOWORD(wParam) == TB_PAGEUP || LOWORD(wParam) == TB_PAGEDOWN){

				OnHorizontalScroll((HWND)lParam);
			}
			return TRUE;

		case WM_COMMAND:
			if(HIWORD(wParam) == BN_CLICKED)
				OnClicked(lParam);
			return TRUE;

		case WM_CLOSE:
			Close();
			return TRUE;

		case WM_DESTROY:
			return TRUE;
	}

	return FALSE;
}