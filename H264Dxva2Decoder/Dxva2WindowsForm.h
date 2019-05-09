//----------------------------------------------------------------------------------------------
// Dxva2WindowsForm.h
//----------------------------------------------------------------------------------------------
#ifndef DXVA2WINDOWSFORM_H
#define DXVA2WINDOWSFORM_H

class CDxva2WindowsForm{

public:

	CDxva2WindowsForm() : m_hwndParent(NULL), m_hwndDlg(NULL){ m_pDxva2WindowsForm = this; }
	~CDxva2WindowsForm(){}

	void Open(const HINSTANCE, const HWND);
	void Close();
	void UpdateDxva2Settings(DXVAHD_FILTER_RANGE_DATA_EX*, const BOOL);
	HRESULT UpdateDxva2Filter(const int, const int, const int, DXVAHD_FILTER_RANGE_DATA_EX&);
	BOOL CALLBACK Dxva2WindowsFormProc(HWND, UINT, WPARAM, LPARAM);

	// inline
	static CDxva2WindowsForm* GetDxva2WindowsForm(){ return m_pDxva2WindowsForm; }

private:

	static CDxva2WindowsForm* m_pDxva2WindowsForm;
	HWND m_hwndParent;
	HWND m_hwndDlg;

	void OnHorizontalScroll(const HWND);
	void OnClicked(LPARAM);
};

#endif