//----------------------------------------------------------------------------------------------
// MFCriticSection.h
//----------------------------------------------------------------------------------------------
#ifndef MFCRITICSECTION_H
#define MFCRITICSECTION_H

class CriticSection{

private:

	CRITICAL_SECTION m_Section;

public:

	CriticSection(){ InitializeCriticalSection(&m_Section); }
	~CriticSection(){ DeleteCriticalSection(&m_Section); }

	void Lock(){ EnterCriticalSection(&m_Section); }
	void Unlock(){ LeaveCriticalSection(&m_Section); }
};

class AutoLock{

private:

	CriticSection* m_pSection;

public:

	AutoLock(CriticSection& critic){ m_pSection = &critic; m_pSection->Lock(); }
	~AutoLock(){ m_pSection->Unlock(); }
};

#endif
