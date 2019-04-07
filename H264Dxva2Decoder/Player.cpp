//----------------------------------------------------------------------------------------------
// Player.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

CPlayer::CPlayer(HRESULT& hr, IMFAsyncCallback* pWindowCallback) :
	m_nRefCount(1),
	m_bStop(TRUE),
	m_bPause(FALSE),
	m_bSeek(FALSE),
	m_hEvent(NULL),
	m_iNaluLenghtSize(0),
	m_dwTrackId(0),
	m_dwWorkQueue(0),
	m_dwMessageQueue(0),
	m_bIsShutdown(FALSE)
{
	if(pWindowCallback == NULL){

		hr = E_POINTER;
	}
	else{

		// todo : MFBeginRegisterWorkQueueWithMMCSS/AvSetMmThreadCharacteristics/AvSetMmThreadPriority
		// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile\Tasks
		// Audio/Capture/Distribution/Games/Playback/Pro Audio/Window Manager
		// MFGetWorkQueueMMCSSPriority
		// For video, we should use : Playback
		// On windows7, only single-threaded queues
		if(SUCCEEDED(hr = MFAllocateWorkQueue(&m_dwWorkQueue))){

			if(SUCCEEDED(hr = MFAllocateWorkQueue(&m_dwMessageQueue))){

				m_pWindowCallback = pWindowCallback;
				m_pWindowCallback->AddRef();
			}
		}
	}
}

CPlayer::~CPlayer(){

	assert(m_bIsShutdown);
}

HRESULT CPlayer::QueryInterface(REFIID riid, void** ppv){

	static const QITAB qit[] = {
		QITABENT(CPlayer, IMFAsyncCallback),
	{0}
	};

	return QISearch(this, qit, riid, ppv);
}

ULONG CPlayer::AddRef(){

	LONG lRef = InterlockedIncrement(&m_nRefCount);

	TRACE_REFCOUNT((L"CPlayer::AddRef m_nRefCount = %d", lRef));

	return lRef;
}

ULONG CPlayer::Release(){

	ULONG uCount = InterlockedDecrement(&m_nRefCount);

	TRACE_REFCOUNT((L"CPlayer::Release m_nRefCount = %d", uCount));

	if(uCount == 0){
		delete this;
	}

	return uCount;
}

HRESULT CPlayer::Invoke(IMFAsyncResult* pResult){

	HRESULT hr;

	assert(pResult);
	assert(SUCCEEDED(pResult->GetStatus()));

	BYTE* pVideoData = NULL;
	DWORD dwBufferSize;

	CMFBuffer pVideoBuffer;
	CMFBuffer pNalUnitBuffer;
	LONGLONG llTime = 0;
	int iSubSliceCount;
	DWORD dwParsed;
	BYTE btStartCode[4] = {0x00, 0x00, 0x00, 0x01};

	const DWORD H264_BUFFER_SIZE = 262144;

	IF_FAILED_RETURN(pVideoBuffer.Initialize(H264_BUFFER_SIZE));
	IF_FAILED_RETURN(pNalUnitBuffer.Initialize(H264_BUFFER_SIZE));

	try{

		while(m_cH264AtomParser.GetNextSample(m_dwTrackId, &pVideoData, &dwBufferSize, &llTime) == S_OK){

			if(m_bPause){

				while(m_bPause)
					Sleep(500);
			}

			if(m_bStop){
				break;
			}

			if(m_bSeek){

				m_cDxva2Decoder.ClearPresentation();
				m_bSeek = FALSE;
			}

			IF_FAILED_THROW(pVideoBuffer.Reserve(dwBufferSize));
			memcpy(pVideoBuffer.GetStartBuffer(), pVideoData, dwBufferSize);
			IF_FAILED_THROW(pVideoBuffer.SetEndPosition(dwBufferSize));

			pNalUnitBuffer.Reset();
			iSubSliceCount = 0;

			do{

				if(iSubSliceCount == 0){

					IF_FAILED_THROW(pNalUnitBuffer.Reserve(pVideoBuffer.GetBufferSize()));
					memcpy(pNalUnitBuffer.GetStartBuffer(), pVideoBuffer.GetStartBuffer(), pVideoBuffer.GetBufferSize());
					IF_FAILED_THROW(pNalUnitBuffer.SetEndPosition(pVideoBuffer.GetBufferSize()));
				}

				IF_FAILED_THROW(m_cH264NaluParser.ParseNaluHeader(pVideoBuffer, &dwParsed));

				if(m_cH264NaluParser.IsNalUnitCodedSlice()){

					iSubSliceCount++;

					if(iSubSliceCount == 1)
						m_cDxva2Decoder.SetCurrentNalu(m_cH264NaluParser.GetPicture().NalUnitType, m_cH264NaluParser.GetPicture().btNalRefIdc);

					// DXVA2 needs start code
					if(m_iNaluLenghtSize == 4){
						memcpy(pNalUnitBuffer.GetStartBuffer(), btStartCode, 4);
#ifdef USE_WORKAROUND_FOR_INTEL_GPU
						dwParsed--;
#endif
					}
					else{
						IF_FAILED_THROW(AddByteAndConvertAvccToAnnexB(pNalUnitBuffer));
						dwParsed += 1;
					}

					IF_FAILED_THROW(m_cDxva2Decoder.AddSliceShortInfo(iSubSliceCount, dwParsed));

					if(pVideoBuffer.GetBufferSize() == 0){

						pNalUnitBuffer.SetStartPositionAtBeginning();
#ifdef USE_WORKAROUND_FOR_INTEL_GPU
						IF_FAILED_THROW(pNalUnitBuffer.SetStartPosition(1));
#endif
						IF_FAILED_THROW(m_cDxva2Decoder.DecodeFrame(pNalUnitBuffer, m_cH264NaluParser.GetPicture(), llTime, iSubSliceCount));
						IF_FAILED_THROW(m_cDxva2Decoder.RenderFrame());
					}
					else{

						// Sub-slices
						IF_FAILED_THROW(pNalUnitBuffer.SetStartPosition(dwParsed));
					}
				}
				else{

					if(iSubSliceCount > 0){

						// Can be NAL_UNIT_FILLER_DATA after sub-slices
						pNalUnitBuffer.SetStartPositionAtBeginning();

						IF_FAILED_THROW(m_cDxva2Decoder.DecodeFrame(pNalUnitBuffer, m_cH264NaluParser.GetPicture(), llTime, iSubSliceCount));
						IF_FAILED_THROW(m_cDxva2Decoder.RenderFrame());

						// We assume sub-slices are contiguous, so skip others
						pVideoBuffer.Reset();
					}
					else{

						pNalUnitBuffer.Reset();
					}
				}

				if(hr == S_FALSE){

					// S_FALSE means slice is corrupted. Just clear previous frames presentation, sometimes it's ok to continue
					m_cDxva2Decoder.ClearPresentation();
					hr = S_OK;
				}
				else if(pVideoBuffer.GetBufferSize() != 0){

					// Some slice contains SEI message and sub-slices, continue parsing
					hr = S_FALSE;
				}
			}
			while(hr == S_FALSE);
		}
	}
	catch(HRESULT){}

	if(m_bStop == FALSE){

		DWORD dwPictureToDisplay = m_cDxva2Decoder.PictureToDisplayCount();

		while(SUCCEEDED(hr) && dwPictureToDisplay){

			dwPictureToDisplay--;

			LOG_HRESULT(hr = m_cDxva2Decoder.RenderFrame());
		}

		m_bStop = TRUE;

		SendMessageToWindow(WND_MSG_FINISH);
	}
	else{

		SendMessageToWindow(WND_MSG_STOPPING);
	}

	LOG_HRESULT(m_cDxva2Decoder.RenderBlackFrame());

	pVideoBuffer.Delete();
	pNalUnitBuffer.Delete();

	SetEvent(m_hEvent);

	return hr;
}

HRESULT CPlayer::OpenFile(const HWND hWnd, LPCWSTR lpwszFile){

	HRESULT hr;
	BYTE* pVideoData = NULL;
	DWORD dwBufferSize;

	IF_FAILED_RETURN(m_hEvent == NULL ? S_OK : E_UNEXPECTED);

	IF_FAILED_RETURN(m_cH264AtomParser.Initialize(lpwszFile));
	IF_FAILED_RETURN(m_cH264AtomParser.ParseMp4());
	m_iNaluLenghtSize = m_cH264AtomParser.GetNaluLenghtSize();
	m_cH264NaluParser.SetNaluLenghtSize(m_iNaluLenghtSize);

	IF_FAILED_RETURN(m_cH264AtomParser.GetFirstVideoStream(&m_dwTrackId));
	IF_FAILED_RETURN(m_cH264AtomParser.GetVideoConfigDescriptor(m_dwTrackId, &pVideoData, &dwBufferSize));
	IF_FAILED_RETURN(m_cH264NaluParser.ParseVideoConfigDescriptor(pVideoData, dwBufferSize));

	DXVA2_Frequency Dxva2Freq;
	IF_FAILED_RETURN(m_cH264AtomParser.GetVideoFrameRate(m_dwTrackId, &Dxva2Freq.Numerator, &Dxva2Freq.Denominator));
	IF_FAILED_RETURN(m_cDxva2Decoder.InitDXVA2(hWnd, m_cH264NaluParser.GetSPS(), m_cH264NaluParser.GetWidth(), m_cH264NaluParser.GetHeight(), Dxva2Freq.Numerator, Dxva2Freq.Denominator));

	IF_FAILED_RETURN(Play());

	return hr;
}

HRESULT CPlayer::Play(){

	HRESULT hr = S_OK;

	if(m_hEvent != NULL){

		if(m_bPause)
			m_bPause = FALSE;

		if(m_bStop){

			Stop();
		}
		else{

			SendMessageToWindow(WND_MSG_PLAYING);
			return hr;
		}
	}

	m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	IF_FAILED_RETURN(m_hEvent == NULL ? E_FAIL : S_OK);

	SendMessageToWindow(WND_MSG_PLAYING);

	m_bStop = FALSE;
	IF_FAILED_RETURN(MFPutWorkItem(m_dwWorkQueue, this, NULL));

	return hr;
}

HRESULT CPlayer::Pause(){

	HRESULT hr = S_OK;

	if(m_hEvent == NULL){
		return hr;
	}

	m_bPause = TRUE;

	SendMessageToWindow(WND_MSG_PAUSING);

	return hr;
}

HRESULT CPlayer::Stop(){

	HRESULT hr = S_OK;

	if(m_hEvent == NULL){
		return hr;
	}

	StopEvent();

	m_cH264AtomParser.Reset();
	m_cDxva2Decoder.Reset();

	SendMessageToWindow(WND_MSG_STOPPING);

	return hr;
}

HRESULT CPlayer::PlayPause(){

	HRESULT hr = S_OK;

	if(m_hEvent == NULL){
		return hr;
	}

	m_bPause = !m_bPause;

	SendMessageToWindow(m_bPause ? WND_MSG_PAUSING : WND_MSG_PLAYING);

	return hr;
}

HRESULT CPlayer::Close(){

	HRESULT hr = S_OK;

	StopEvent();

	m_cH264AtomParser.Delete();
	m_cH264NaluParser.Reset();
	m_cDxva2Decoder.OnRelease();
	m_bSeek = FALSE;
	m_iNaluLenghtSize = 0;
	m_dwTrackId = 0;

	return hr;
}

HRESULT CPlayer::Shutdown(){

	HRESULT hr = S_OK;

	if(m_bIsShutdown == FALSE){

		Close();

		// Do not use CPlayer after Shutdown...
		LOG_HRESULT(hr = MFUnlockWorkQueue(m_dwWorkQueue));
		m_dwWorkQueue = 0;

		LOG_HRESULT(hr = MFUnlockWorkQueue(m_dwMessageQueue));
		m_dwMessageQueue = 0;

		SAFE_RELEASE(m_pWindowCallback);

		m_bIsShutdown = TRUE;
	}

	return hr;
}

HRESULT CPlayer::SeekVideo(const MFTIME llDuration){

	HRESULT hr = S_OK;

	if(m_bStop || m_bPause)
		return hr;

	hr = m_cH264AtomParser.SeekVideo(llDuration, m_dwTrackId);

	if(hr == S_OK)
		m_bSeek = TRUE;

	return hr;
}

HRESULT CPlayer::SendMessageToWindow(const int iMessage){

	HRESULT hr = S_OK;

	IMFAsyncResult* pAsyncResult = NULL;
	CWindowMessage* pMessage = NULL;

	try{

		pMessage = new (std::nothrow)CWindowMessage();

		IF_FAILED_RETURN(pMessage == NULL ? E_OUTOFMEMORY : S_OK);

		pMessage->SetWindowMessage(iMessage);

		IF_FAILED_THROW(MFCreateAsyncResult(pMessage, m_pWindowCallback, NULL, &pAsyncResult));

		IF_FAILED_THROW(MFPutWorkItemEx(m_dwMessageQueue, pAsyncResult));
	}
	catch(HRESULT){}

	SAFE_RELEASE(pMessage);
	SAFE_RELEASE(pAsyncResult);

	return hr;
}

void CPlayer::StopEvent(){

	if(m_hEvent){

		m_bStop = TRUE;
		m_bPause = FALSE;
		WaitForSingleObject(m_hEvent, INFINITE);
		CLOSE_HANDLE_NULL_IF(m_hEvent);
		m_hEvent = NULL;
	}
}

HRESULT CPlayer::AddByteAndConvertAvccToAnnexB(CMFBuffer& pNalUnitBuffer){

	HRESULT hr;
	BYTE btStartCode[3] = {0x00, 0x00, 0x01};

	IF_FAILED_RETURN(pNalUnitBuffer.Reserve(1));
	// Normally memmove here, but seems to be ok with memcpy
	memcpy(pNalUnitBuffer.GetStartBuffer() + 1, pNalUnitBuffer.GetStartBuffer(), pNalUnitBuffer.GetBufferSize());
	memcpy(pNalUnitBuffer.GetStartBuffer(), btStartCode, 3);
	IF_FAILED_RETURN(pNalUnitBuffer.SetEndPosition(1));

	return hr;
}