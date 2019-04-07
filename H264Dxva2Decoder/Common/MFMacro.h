//----------------------------------------------------------------------------------------------
// MFMacro.h
//----------------------------------------------------------------------------------------------
#ifndef MFMACRO_H
#define MFMACRO_H

#ifndef MF_SAFE_RELEASE
#define MF_SAFE_RELEASE
template <class T> inline void SAFE_RELEASE(T*& p){

	if(p){
		p->Release();
		p = NULL;
	}
}
#endif

#ifndef MF_SAFE_DELETE
#define MF_SAFE_DELETE
template<class T> inline void SAFE_DELETE(T*& p){

	if(p){
		delete p;
		p = NULL;
	}
}
#endif

#ifndef MF_SAFE_DELETE_ARRAY
#define MF_SAFE_DELETE_ARRAY
template<class T> inline void SAFE_DELETE_ARRAY(T*& p){

	if(p){
		delete[] p;
		p = NULL;
	}
}
#endif

#ifndef IF_FAILED_RETURN
#if(_DEBUG && MF_USE_LOGGING)
#define IF_FAILED_RETURN(X) if(FAILED(hr = (X))){ LOG_HRESULT(hr); return hr; }
#else
#define IF_FAILED_RETURN(X) if(FAILED(hr = (X))){ return hr; }
#endif
#endif

#ifndef IF_FAILED_THROW
#if(_DEBUG && MF_USE_LOGGING)
#define IF_FAILED_THROW(X) if(FAILED(hr = (X))){ LOG_HRESULT(hr); throw hr; }
#else
#define IF_FAILED_THROW(X) if(FAILED(hr = (X))){ throw hr; }
#endif
#endif

#ifndef IF_ERROR_RETURN
#if (_DEBUG && MF_USE_LOGGING)
#define IF_ERROR_RETURN(b) if(b == FALSE){ LOG_LAST_ERROR(); return b; }
#else
#define IF_ERROR_RETURN(b) if(b == FALSE){ return b; }
#endif
#endif

#ifndef CLOSE_HANDLE_IF
#if (_DEBUG && MF_USE_LOGGING)
#define CLOSE_HANDLE_IF(h) if(h != INVALID_HANDLE_VALUE){ if(CloseHandle(h) == FALSE){ LOG_LAST_ERROR(); } h = INVALID_HANDLE_VALUE; }
#else
#define CLOSE_HANDLE_IF(h) if(h != INVALID_HANDLE_VALUE){ CloseHandle(h); h = INVALID_HANDLE_VALUE; }
#endif
#endif

#ifndef CLOSE_HANDLE_NULL_IF
#if (_DEBUG && MF_USE_LOGGING)
#define CLOSE_HANDLE_NULL_IF(h) if(h != NULL){ if(CloseHandle(h) == FALSE){ LOG_LAST_ERROR(); } h = NULL; }
#else
#define CLOSE_HANDLE_NULL_IF(h) if(h != NULL){ CloseHandle(h); h = NULL; }
#endif
#endif

#ifndef RETURN_STRING
#define RETURN_STRING(x) case x: return L#x
#endif

#ifndef IF_EQUAL_RETURN
#define IF_EQUAL_RETURN(param, val) if(val == param) return L#val
#endif

#ifndef VALUE_NOT_FOUND
#define VALUE_NOT_FOUND(val) return L#val
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#endif
