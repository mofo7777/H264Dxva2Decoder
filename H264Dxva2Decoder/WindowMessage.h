//----------------------------------------------------------------------------------------------
// WindowMessage.h
//----------------------------------------------------------------------------------------------
#ifndef WINDOWMESSAGE_H
#define WINDOWMESSAGE_H

class CWindowMessage : public IUnknown{

public:

	CWindowMessage() : m_nRefCount(1), m_iMessage(WND_MSG_NOT_SET){ /*LOG_HRESULT(MFLockPlatform());*/ }
	~CWindowMessage(){ /*LOG_HRESULT(MFUnlockPlatform());*/ }

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv){

		static const QITAB qit[] = {
			QITABENT(CWindowMessage, IUnknown),
		{0}
		};

		return QISearch(this, qit, riid, ppv);
	}

	STDMETHODIMP_(ULONG) AddRef(){ return InterlockedIncrement(&m_nRefCount); }

	STDMETHODIMP_(ULONG) Release(){

		LONG cRef = InterlockedDecrement(&m_nRefCount);

		if(cRef == 0){
			delete this;
		}
		return cRef;
	}

	void SetWindowMessage(const int iMessage){ m_iMessage = iMessage; }
	int GetWindowMessage(){ return m_iMessage; }

private:

	volatile long m_nRefCount;
	int m_iMessage;
};

#endif