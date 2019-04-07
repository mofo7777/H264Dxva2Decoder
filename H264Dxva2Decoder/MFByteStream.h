//----------------------------------------------------------------------------------------------
// MFByteStream.h
//----------------------------------------------------------------------------------------------
#ifndef MFBYTESTREAM_H
#define MFBYTESTREAM_H

class CMFByteStream : public IMFAsyncCallback{

public:

	static HRESULT CreateInstance(CMFByteStream**);

	// IUnknown - MFByteStream.cpp
	STDMETHODIMP QueryInterface(REFIID, void**);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFAsyncCallback - MFByteStream.cpp
	STDMETHODIMP GetParameters(DWORD*, DWORD*){ TRACE_BYTESTREAM((L"MFByteStream::GetParameters")); return E_NOTIMPL; }
	STDMETHODIMP Invoke(IMFAsyncResult*);

	// MFByteStream.cpp
	HRESULT Initialize(LPCWSTR);
	HRESULT Start();
	void Close();
	HRESULT Read(BYTE*, const DWORD, DWORD*);
	HRESULT Read(CMFReadParam*);
	HRESULT BeginRead(BYTE*, ULONG, IMFAsyncCallback*);
	HRESULT EndRead(IMFAsyncResult*, ULONG*);
	HRESULT Seek(const LONG);
	HRESULT SeekHigh(LARGE_INTEGER);
	HRESULT SeekEnd(const LONG);
	HRESULT Reset();
	const BOOL IsInitialized() const{ return (m_hFile != INVALID_HANDLE_VALUE && m_pReadParam != NULL); }

private:

	// MFByteStream.cpp
	CMFByteStream();
	virtual ~CMFByteStream(){ TRACE_BYTESTREAM((L"MFByteStream::DTOR")); Close(); }

	CriticSection m_CriticSection;
	volatile long m_nRefCount;
	wstring m_wszFile;
	HANDLE m_hFile;
	CMFReadParam* m_pReadParam;
};

#endif