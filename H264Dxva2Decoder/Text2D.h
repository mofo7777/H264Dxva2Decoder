//----------------------------------------------------------------------------------------------
// Text2D.h
//----------------------------------------------------------------------------------------------
#ifndef TEXT2D_H
#define TEXT2D_H

#ifdef USE_DIRECTX9

class CText2D{

public:

	CText2D() : m_pTextFont(NULL){}
	~CText2D(){ OnDelete(); }

	HRESULT OnRestore(IDirect3DDevice9*, const RECT&, const FLOAT);
	HRESULT OnRender(LPCWSTR);
	void    OnDelete();

private:

	ID3DXFont* m_pTextFont;
	RECT m_rcText;
};

#endif

#endif