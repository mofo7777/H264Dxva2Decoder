//----------------------------------------------------------------------------------------------
// CallbackMessage.h
//----------------------------------------------------------------------------------------------
#ifndef CALLBACKMESSAGE_H
#define CALLBACKMESSAGE_H

class CCallbackMessage : public IUnknown{

public:

	CCallbackMessage() : m_nRefCount(1), m_iMessage(CALLBACK_MSG_NOT_SET){ /*LOG_HRESULT(MFLockPlatform());*/ }
	~CCallbackMessage(){ /*LOG_HRESULT(MFUnlockPlatform());*/ }

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv){

		static const QITAB qit[] = {
			QITABENT(CCallbackMessage, IUnknown),
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

	void SetCallbackMessage(const int iMessage){ m_iMessage = iMessage; }
	const int GetCallbackMessage() const{ return m_iMessage; }

private:

	volatile long m_nRefCount;
	int m_iMessage;
};

#endif