//----------------------------------------------------------------------------------------------
// MFLogging.h
//----------------------------------------------------------------------------------------------
#ifndef MFLOGGING_H
#define MFLOGGING_H

#ifdef _DEBUG

class DebugLog{

public:

	static void Initialize(){
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
		//_CrtSetBreakAlloc(157);
	}

	static void Trace(const WCHAR* sFormatString, ...){

		HRESULT hr = S_OK;
		va_list va;

		const DWORD TRACE_STRING_LEN = 512;

		WCHAR message[TRACE_STRING_LEN];

		va_start(va, sFormatString);
		hr = StringCchVPrintf(message, TRACE_STRING_LEN, sFormatString, va);
		va_end(va);

		if(SUCCEEDED(hr)){

			size_t size = _tcslen(message);

			if(size != 0 && size < TRACE_STRING_LEN){
				message[size] = '\n';
				message[size + 1] = '\0';
			}

			_CrtDbgReport(_CRT_WARN, NULL, NULL, NULL, "%S", message);
		}
	}

	static void TraceNoEndLine(const WCHAR* sFormatString, ...){

		HRESULT hr = S_OK;
		va_list va;

		const DWORD TRACE_STRING_LEN = 512;

		WCHAR message[TRACE_STRING_LEN];

		va_start(va, sFormatString);
		hr = StringCchVPrintf(message, TRACE_STRING_LEN, sFormatString, va);
		va_end(va);

		if(SUCCEEDED(hr)){

			_CrtDbgReport(_CRT_WARN, NULL, NULL, NULL, "%S", message);
		}
	}

	static void Close(){
		int bLeak = _CrtDumpMemoryLeaks();
		assert(bLeak == FALSE);
	}
};

#define TRACE_INIT() DebugLog::Initialize()
#define TRACE(x) DebugLog::Trace x
#define TRACE_NO_END_LINE(x) DebugLog::TraceNoEndLine x
#define TRACE_CLOSE() DebugLog::Close()

inline HRESULT _LOG_HRESULT(HRESULT hr, const char* sFileName, long lLineNo){

	if(FAILED(hr)){
		TRACE((L"\n%S - Line: %d hr = %s\n", sFileName, lLineNo, MFErrorString(hr)));
	}

	return hr;
}

#define LOG_HRESULT(hr)		_LOG_HRESULT(hr, __FILE__, __LINE__)
#define LOG_LAST_ERROR()	_LOG_HRESULT(HRESULT_FROM_WIN32(GetLastError()), __FILE__, __LINE__)

#ifdef MF_USE_LOGREFCOUNT
#define TRACE_REFCOUNT(x) DebugLog::Trace x
#else
#define TRACE_REFCOUNT(x)
#endif

#ifdef MF_TRACE_BYTESTREAM
#define TRACE_BYTESTREAM(x) DebugLog::Trace x
#else
#define TRACE_BYTESTREAM(x)
#endif

#else
#define TRACE_INIT()
#define TRACE(x)
#define TRACE_NO_END_LINE(x)
#define TRACE_CLOSE()
#define LOG_HRESULT(hr) hr
#define LOG_LAST_ERROR()
#define TRACE_REFCOUNT(x)
#define TRACE_BYTESTREAM(x)
#endif

#endif
