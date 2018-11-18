//----------------------------------------------------------------------------------------------
// MFByteStream.cpp
//----------------------------------------------------------------------------------------------
#include "StdAfx.h"

CMFByteStream::CMFByteStream() :
	m_nRefCount(1),
	m_wszFile(L""),
	m_hFile(INVALID_HANDLE_VALUE),
	m_pReadParam(NULL)
{
	TRACE_BYTESTREAM((L"MFByteStream::CTOR"));
}

HRESULT CMFByteStream::CreateInstance(CMFByteStream** ppByteStream){

	TRACE_BYTESTREAM((L"MFByteStream::CreateInstance"));

	HRESULT hr;
	IF_FAILED_RETURN(ppByteStream == NULL ? E_POINTER : S_OK);

	CMFByteStream* pByteStream = new (std::nothrow)CMFByteStream();

	IF_FAILED_RETURN(pByteStream == NULL ? E_OUTOFMEMORY : S_OK);

	if(SUCCEEDED(hr)){

		*ppByteStream = pByteStream;
		(*ppByteStream)->AddRef();
	}

	SAFE_RELEASE(pByteStream);

	return hr;
}

HRESULT CMFByteStream::QueryInterface(REFIID riid, void** ppv){

	TRACE_BYTESTREAM((L"MFByteStream::QI : riid = %s", GetIIDString(riid)));

	static const QITAB qit[] = {
		QITABENT(CMFByteStream, IMFAsyncCallback),
	{0}
	};

	return QISearch(this, qit, riid, ppv);
}

ULONG CMFByteStream::AddRef(){

	LONG lRef = InterlockedIncrement(&m_nRefCount);

	TRACE_REFCOUNT((L"MFByteStream::AddRef m_nRefCount = %d", lRef));

	return lRef;
}

ULONG CMFByteStream::Release(){

	ULONG uCount = InterlockedDecrement(&m_nRefCount);

	TRACE_REFCOUNT((L"MFByteStream::Release m_nRefCount = %d", uCount));

	if(uCount == 0){
		delete this;
	}

	return uCount;
}

HRESULT CMFByteStream::Invoke(IMFAsyncResult* pResult){

	TRACE_BYTESTREAM((L"MFByteStream::Invoke"));

	HRESULT hr;
	IF_FAILED_RETURN(pResult == NULL ? E_POINTER : S_OK);

	IUnknown* pState = NULL;
	IUnknown* pUnk = NULL;
	IMFAsyncResult* pCallerResult = NULL;

	try{

		IF_FAILED_THROW(pResult->GetState(&pState));

		IF_FAILED_THROW(pState->QueryInterface(IID_PPV_ARGS(&pCallerResult)));

		IF_FAILED_THROW(pCallerResult->GetObject(&pUnk));

		CMFReadParam* pReadParam = static_cast<CMFReadParam*>(pUnk);

		IF_FAILED_THROW(Read(pReadParam));
	}
	catch(HRESULT){}

	if(pCallerResult){

		LOG_HRESULT(pCallerResult->SetStatus(hr));
		LOG_HRESULT(MFInvokeCallback(pCallerResult));
	}

	SAFE_RELEASE(pState);
	SAFE_RELEASE(pUnk);
	SAFE_RELEASE(pCallerResult);

	return hr;
}

HRESULT CMFByteStream::Initialize(LPCWSTR pwszFile){

	TRACE_BYTESTREAM((L"MFByteStream::Initialize"));

	HRESULT hr;

	IF_FAILED_RETURN(pwszFile == NULL ? E_POINTER : S_OK);

	AutoLock lock(m_CriticSection);

	IF_FAILED_RETURN(m_hFile != INVALID_HANDLE_VALUE ? E_UNEXPECTED : S_OK);

	m_hFile = CreateFile(pwszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	IF_FAILED_RETURN(m_hFile == INVALID_HANDLE_VALUE ? E_UNEXPECTED : S_OK);

	LARGE_INTEGER liFileSize;

	if(!GetFileSizeEx(m_hFile, &liFileSize)){

		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
		IF_FAILED_RETURN(E_UNEXPECTED);
	}

	m_pReadParam = new (std::nothrow)CMFReadParam();
	IF_FAILED_RETURN(m_pReadParam == NULL ? E_OUTOFMEMORY : S_OK);

	m_wszFile = pwszFile;

	return hr;
}

HRESULT CMFByteStream::Start(){

	TRACE_BYTESTREAM((L"MFByteStream::Start"));

	HRESULT hr;

	AutoLock lock(m_CriticSection);

	IF_FAILED_RETURN(m_hFile != INVALID_HANDLE_VALUE ? E_UNEXPECTED : S_OK);
	IF_FAILED_RETURN(m_pReadParam != NULL ? E_UNEXPECTED : S_OK);

	m_hFile = CreateFile(m_wszFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	IF_FAILED_RETURN(m_hFile == INVALID_HANDLE_VALUE ? E_UNEXPECTED : S_OK);

	LARGE_INTEGER liFileSize;

	if(!GetFileSizeEx(m_hFile, &liFileSize)){

		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
		IF_FAILED_RETURN(E_UNEXPECTED);
	}

	m_pReadParam = new (std::nothrow)CMFReadParam();
	IF_FAILED_RETURN(m_pReadParam == NULL ? E_OUTOFMEMORY : S_OK);

	return hr;
}

void CMFByteStream::Close(){

	TRACE_BYTESTREAM((L"MFByteStream::Close"));

	AutoLock lock(m_CriticSection);

	SAFE_RELEASE(m_pReadParam);

	if(m_hFile == INVALID_HANDLE_VALUE)
		return;

	//m_wszFile = L"";

	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
}

HRESULT CMFByteStream::Read(BYTE* pData, const DWORD dwSize, DWORD* pdwSize){

	TRACE_BYTESTREAM((L"MFByteStream::Read"));

	HRESULT hr;

	if(ReadFile(m_hFile, pData, dwSize, pdwSize, 0)){
		hr = S_OK;
	}
	else{
		hr = E_FAIL;
	}

	return hr;
}

HRESULT CMFByteStream::Read(CMFReadParam* pReadParam){

	TRACE_BYTESTREAM((L"MFByteStream::Read"));

	HRESULT hr;
	IF_FAILED_RETURN(pReadParam == NULL ? E_POINTER : S_OK);

	if(ReadFile(m_hFile, pReadParam->GetDataPtr(), pReadParam->GetByteToRead(), pReadParam->GetpByteRead(), 0)){
		hr = S_OK;
	}
	else{
		hr = E_FAIL;
	}

	return hr;
}

HRESULT CMFByteStream::BeginRead(BYTE* pData, ULONG ulToRead, IMFAsyncCallback* pCallback){

	TRACE_BYTESTREAM((L"MFByteStream::BeginRead"));

	HRESULT hr;

	// ToDo : Check BeginRead parameters and inside variables

	AutoLock lock(m_CriticSection);

	m_pReadParam->SetDataPtr(pData);
	m_pReadParam->SetByteToRead(ulToRead);

	IMFAsyncResult* pResult = NULL;

	// Do MFCreateAsyncResult Release pReadParam if Failed ?
	IF_FAILED_RETURN(MFCreateAsyncResult(m_pReadParam, pCallback, NULL, &pResult));

	LOG_HRESULT(MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, this, pResult));

	pResult->Release();

	return hr;
}

HRESULT CMFByteStream::EndRead(IMFAsyncResult* pResult, ULONG* pulRead){

	TRACE_BYTESTREAM((L"MFByteStream::EndRead"));

	HRESULT hr;
	IF_FAILED_RETURN(pResult == NULL || pulRead == NULL ? E_POINTER : S_OK);

	*pulRead = 0;
	IUnknown* pUnk = NULL;

	// ToDo : Check EndRead parameters and inside variables

	AutoLock lock(m_CriticSection);

	try{

		IF_FAILED_THROW(pResult->GetStatus());

		IF_FAILED_THROW(pResult->GetObject(&pUnk));

		CMFReadParam* pReadParam = static_cast<CMFReadParam*>(pUnk);

		*pulRead = pReadParam->GetByteRead();
	}
	catch(HRESULT){}

	SAFE_RELEASE(pUnk);

	return hr;
}

HRESULT CMFByteStream::Seek(const LONG lDistance){

	TRACE_BYTESTREAM((L"MFByteStream::Seek"));

	HRESULT hr;

	AutoLock lock(m_CriticSection);

	IF_FAILED_RETURN(m_hFile == INVALID_HANDLE_VALUE ? E_UNEXPECTED : S_OK);

	DWORD dwPosition = SetFilePointer(m_hFile, lDistance, NULL, FILE_CURRENT);

	IF_FAILED_RETURN(dwPosition == INVALID_SET_FILE_POINTER ? E_FAIL : S_OK);

	return hr;
}

HRESULT CMFByteStream::SeekHigh(LARGE_INTEGER liDistance){

	TRACE_BYTESTREAM((L"MFByteStream::SeekHigh"));

	HRESULT hr;

	AutoLock lock(m_CriticSection);

	IF_FAILED_RETURN(m_hFile == INVALID_HANDLE_VALUE ? E_UNEXPECTED : S_OK);

	DWORD dwPosition = SetFilePointer(m_hFile, liDistance.LowPart, &liDistance.HighPart, FILE_CURRENT);

	IF_FAILED_RETURN(dwPosition == INVALID_SET_FILE_POINTER || GetLastError() != NO_ERROR ? E_FAIL : S_OK);

	return hr;
}

HRESULT CMFByteStream::SeekEnd(const LONG lDistance){

	TRACE_BYTESTREAM((L"MFByteStream::SeekEnd"));

	HRESULT hr;

	AutoLock lock(m_CriticSection);

	IF_FAILED_RETURN(m_hFile == INVALID_HANDLE_VALUE ? E_UNEXPECTED : S_OK);

	DWORD dwPosition = SetFilePointer(m_hFile, -lDistance, NULL, FILE_END);

	IF_FAILED_RETURN(dwPosition == INVALID_SET_FILE_POINTER ? E_FAIL : S_OK);

	return hr;
}

HRESULT CMFByteStream::Reset(){

	TRACE_BYTESTREAM((L"MFByteStream::Reset"));

	HRESULT hr;

	AutoLock lock(m_CriticSection);

	IF_FAILED_RETURN(m_hFile == INVALID_HANDLE_VALUE ? E_UNEXPECTED : S_OK);

	DWORD dwPosition = SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN);

	IF_FAILED_RETURN(dwPosition == INVALID_SET_FILE_POINTER ? E_FAIL : S_OK);

	return hr;
}