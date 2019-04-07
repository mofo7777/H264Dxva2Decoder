//----------------------------------------------------------------------------------------------
// MFTExternTrace.h
//----------------------------------------------------------------------------------------------
#ifndef MFTEXTERNTRACE_H
#define MFTEXTERNTRACE_H

// CExternTrace initializes the memory leaks detection (see "crtdbg.h").
// CExternTrace ensures that no object is allocated before TRACE_INIT is called.
// Because DllMain.cpp contains external objects, you will see "FAKE" memory leaks.
#ifdef _DEBUG

class CExternTrace{

public:

	CExternTrace(){ TRACE_INIT(); };
	~CExternTrace(){ TRACE_CLOSE(); };
};

CExternTrace cExternTrace;

#endif

#endif
