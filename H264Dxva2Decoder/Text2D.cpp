//----------------------------------------------------------------------------------------------
// Text2D.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

#ifdef USE_DIRECTX9

HRESULT CText2D::OnRestore(IDirect3DDevice9* pDevice, const RECT& rcText, const FLOAT fFontSize){

	HRESULT hr;
	IF_FAILED_RETURN(hr = (pDevice == NULL ? E_POINTER : S_OK));
	IF_FAILED_RETURN(hr = (m_pTextFont ? E_UNEXPECTED : S_OK));

	ID3DXFont* pTextFont = NULL;
	IDirect3DSurface9* pBackBuffer = NULL;
	HDC hDC = NULL;
	D3DSURFACE_DESC SurfDesc;
	int iLogPixelsY;
	int iFontSize;
	int iHeight;

	try{

		IF_FAILED_THROW(CopyRect(&m_rcText, &rcText) ? S_OK : E_FAIL);

		hDC = GetDC(NULL);
		IF_FAILED_THROW(hDC ? S_OK : E_POINTER);

		iLogPixelsY = GetDeviceCaps(hDC, LOGPIXELSY);

		IF_FAILED_THROW(pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer));
		IF_FAILED_THROW(pBackBuffer->GetDesc(&SurfDesc));
		
		iFontSize = static_cast<int>(SurfDesc.Height / fFontSize);
		iHeight = -MulDiv(iFontSize, iLogPixelsY, 72);

		IF_FAILED_THROW(hr = D3DXCreateFont(pDevice,
			iHeight,
			0,
			FW_NORMAL,
			1,
			FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE,
			L"Arial",
			&pTextFont
		));

		m_pTextFont = pTextFont;
		m_pTextFont->AddRef();
	}
	catch(HRESULT){}

	if(hDC)
		ReleaseDC(NULL, hDC);

	SAFE_RELEASE(pBackBuffer);
	SAFE_RELEASE(pTextFont);

	return hr;
}

HRESULT CText2D::OnRender(LPCWSTR wszText){

	HRESULT hr;
	IF_FAILED_RETURN(hr = (m_pTextFont ? S_OK : E_UNEXPECTED));

	m_pTextFont->DrawText(NULL, wszText, -1, &m_rcText, DT_NOCLIP, 0xffffff00);

	return hr;
}

void CText2D::OnDelete(){

	if(m_pTextFont){

		m_pTextFont->OnLostDevice();
		SAFE_RELEASE(m_pTextFont);
	}
}

#endif