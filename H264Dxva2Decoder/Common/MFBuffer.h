//----------------------------------------------------------------------------------------------
// MFBuffer.h
//----------------------------------------------------------------------------------------------
#ifndef MFBUFFER_H
#define MFBUFFER_H

// Set the buffer according of the usual input buffer size
// For mpeg1/mpeg2 it seems to be ok. I should check more.
#define GROW_BUFFER_SIZE  4096

class CMFBuffer{

public:

	CMFBuffer() : m_pBuffer(NULL), m_dwTotalSize(0), m_dwStartAfterResize(0), m_dwStartPosition(0), m_dwEndPosition(0){}
	~CMFBuffer(){ if(m_pBuffer) delete[] m_pBuffer; }

	HRESULT Initialize();
	HRESULT Initialize(const DWORD);
	HRESULT Reserve(const DWORD);
	HRESULT SetStartPosition(const DWORD);
	HRESULT SetEndPosition(const DWORD);
	void Reset();
	void Delete();

	BYTE* GetReadStartBuffer(){ return m_pBuffer + (m_dwStartPosition + m_dwStartAfterResize); }
	BYTE* GetStartBuffer(){ return m_pBuffer + m_dwStartPosition; }
	DWORD GetBufferSize() const{ assert(m_dwEndPosition >= m_dwStartPosition); return (m_dwEndPosition - m_dwStartPosition); }
	DWORD GetAllocatedSize() const{ return m_dwTotalSize; }
	void SetStartPositionAtBeginning(){ m_dwStartPosition = 0; }

private:

	BYTE * m_pBuffer;

	DWORD m_dwTotalSize;
	DWORD m_dwStartAfterResize;

	DWORD m_dwStartPosition;
	DWORD m_dwEndPosition;

	HRESULT SetSize(const DWORD);
};

inline HRESULT CMFBuffer::Initialize(){

	// Initialize only once
	if(m_pBuffer)
		return S_OK;

	m_pBuffer = new (std::nothrow)BYTE[GROW_BUFFER_SIZE];

	if(m_pBuffer == NULL){
		return E_OUTOFMEMORY;
	}

	m_dwTotalSize = GROW_BUFFER_SIZE;

	return S_OK;
}

inline HRESULT CMFBuffer::Initialize(const DWORD dwSize){

	// Initialize only once
	if(m_pBuffer)
		return S_OK;

	m_pBuffer = new (std::nothrow)BYTE[dwSize];

	if(m_pBuffer == NULL){
		return E_OUTOFMEMORY;
	}

	m_dwTotalSize = dwSize;

	return S_OK;
}

inline HRESULT CMFBuffer::Reserve(const DWORD dwSize){

	// Initialize must be called first
	assert(m_pBuffer);

	if(dwSize == 0)
		return S_OK;

	if(dwSize > MAXDWORD - m_dwTotalSize){
		return E_UNEXPECTED;
	}

	return SetSize(dwSize);
}

inline HRESULT CMFBuffer::SetStartPosition(const DWORD dwPos){

	if(dwPos == 0)
		return S_OK;

	if(dwPos > MAXDWORD - m_dwStartPosition){
		return E_UNEXPECTED;
	}

	DWORD dwNewPos = m_dwStartPosition + dwPos;

	if(dwNewPos > m_dwEndPosition){
		return E_UNEXPECTED;
	}

	m_dwStartPosition = dwNewPos;

	return S_OK;
}

inline HRESULT CMFBuffer::SetEndPosition(const DWORD dwPos){

	if(dwPos == 0)
		return S_OK;

	if(dwPos > MAXDWORD - m_dwEndPosition){
		return E_UNEXPECTED;
	}

	DWORD dwNewPos = m_dwEndPosition + dwPos;

	if(dwNewPos > m_dwTotalSize){

		return E_UNEXPECTED;
	}

	m_dwEndPosition = dwNewPos;
	m_dwStartAfterResize = 0;

	return S_OK;
}

inline void CMFBuffer::Reset(){

	m_dwStartPosition = 0;
	m_dwEndPosition = 0;
	m_dwStartAfterResize = 0;
}

inline void CMFBuffer::Delete(){

	if(m_pBuffer){

		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}

	Reset();
	m_dwTotalSize = 0;
}

inline HRESULT CMFBuffer::SetSize(const DWORD dwSize){

	HRESULT hr = S_OK;

	DWORD dwCurrentSize = GetBufferSize();

	// Todo check
	//if(dwCurrentSize == dwSize)
		//return hr;

	DWORD dwRemainingSize = m_dwTotalSize - dwCurrentSize;

	if(dwSize > dwRemainingSize){

		// Grow enough to not go here too many times (avoid lot of new).
		// We could use a multiple of 16 and use an aligned buffer.
		DWORD dwNewTotalSize = dwSize + dwCurrentSize + GROW_BUFFER_SIZE;

		BYTE* pTmp = new (std::nothrow)BYTE[dwNewTotalSize];

		//ZeroMemory(pTmp, dwNewTotalSize);

		if(pTmp != NULL){

			if(m_pBuffer != NULL){

				memcpy(pTmp, GetStartBuffer(), dwCurrentSize);
				delete[] m_pBuffer;
			}

			m_pBuffer = pTmp;

			m_dwStartAfterResize = dwCurrentSize;
			m_dwStartPosition = 0;
			m_dwEndPosition = dwCurrentSize;
			m_dwTotalSize = dwNewTotalSize;
		}
		else{

			hr = E_OUTOFMEMORY;
		}
	}
	else{

		if(dwCurrentSize != 0)
			memcpy(m_pBuffer, GetStartBuffer(), dwCurrentSize);
		//ZeroMemory(m_pBuffer + dwCurrentSize, m_dwTotalSize - dwCurrentSize);

		m_dwStartAfterResize = dwCurrentSize;
		m_dwStartPosition = 0;
		m_dwEndPosition = dwCurrentSize;
	}

	return hr;
}

#endif