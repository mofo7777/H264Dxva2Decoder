//----------------------------------------------------------------------------------------------
// MFLightBuffer.h
//----------------------------------------------------------------------------------------------
#ifndef MFLIGHTBUFFER_H
#define MFLIGHTBUFFER_H

class CMFLightBuffer{

public:

	CMFLightBuffer() : m_pBuffer(NULL), m_dwSize(0){}
	~CMFLightBuffer(){ if(m_pBuffer) delete[] m_pBuffer; }

	HRESULT Initialize(const DWORD);
	HRESULT Reserve(const DWORD);
	HRESULT DecreaseEndPosition();
	void Delete();

	BYTE* GetBuffer(){ return m_pBuffer; }
	DWORD GetBufferSize() const{ return m_dwSize; }

private:

	BYTE * m_pBuffer;
	DWORD m_dwSize;
};

inline HRESULT CMFLightBuffer::Initialize(const DWORD dwSize){

	// Initialize only once
	if(m_pBuffer)
		return S_OK;

	m_pBuffer = new (std::nothrow)BYTE[dwSize];

	if(m_pBuffer == NULL){
		return E_OUTOFMEMORY;
	}

	m_dwSize = dwSize;

	return S_OK;
}

inline HRESULT CMFLightBuffer::Reserve(const DWORD dwSize){

	// Initialize must be called first
	assert(m_pBuffer);

	if(dwSize == 0)
		return S_OK;

	if(dwSize > MAXDWORD - m_dwSize){
		return E_UNEXPECTED;
	}

	DWORD dwNewSize = m_dwSize + dwSize;

	BYTE* pBuffer = new (std::nothrow)BYTE[dwNewSize];

	if(pBuffer != NULL){

		if(m_pBuffer != NULL){

			memcpy(pBuffer, m_pBuffer, m_dwSize);
			delete[] m_pBuffer;
		}

		m_pBuffer = pBuffer;
		m_dwSize = dwNewSize;
	}
	else{

		return E_OUTOFMEMORY;
	}

	return S_OK;
}

inline HRESULT CMFLightBuffer::DecreaseEndPosition(){

	if(m_dwSize == 0){
		return E_UNEXPECTED;
	}

	m_dwSize--;

	return S_OK;
}

inline void CMFLightBuffer::Delete(){

	if(m_pBuffer){

		delete[] m_pBuffer;
		m_pBuffer = NULL;
		m_dwSize = 0;
	}
}

#endif