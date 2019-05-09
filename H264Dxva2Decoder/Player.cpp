//----------------------------------------------------------------------------------------------
// Player.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

CPlayer::CPlayer(HRESULT& hr, IMFAsyncCallback* pWindowCallback) :
	m_nRefCount(1),
	m_pWindowCallback(NULL),
	m_bStop(TRUE),
	m_bPause(FALSE),
	m_bSeek(FALSE),
	m_bStep(FALSE),
	m_bFinish(FALSE),
	m_hEventEndRendering(NULL),
	m_iNaluLenghtSize(0),
	m_dwTrackId(0),
	m_dwWorkQueueRendering(0),
	m_dwWorkQueueDecoding(0),
	m_dwMessageQueue(0),
	m_bIsShutdown(FALSE),
	m_llPerFrame_1_4th(0LL),
	m_llPerFrame_3_4th(333333LL)
#ifdef USE_MMCSS_WORKQUEUE
	, m_hMMCSS(NULL),
	m_bIsMMCSS(FALSE)
#endif
{
	if(pWindowCallback == NULL){

		hr = E_POINTER;
	}
	else{

		m_pWindowCallback = pWindowCallback;
		m_pWindowCallback->AddRef();
		hr = S_OK;
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
	IUnknown* pUnk = NULL;
	CCallbackMessage* pMessage;
	int iMessage;

	// todo : return error
	assert(pResult);
	assert(SUCCEEDED(pResult->GetStatus()));

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	IF_FAILED_RETURN(pResult->GetState(&pUnk));
	IF_FAILED_RETURN(pUnk == NULL ? E_UNEXPECTED : S_OK);

	pMessage = static_cast<CCallbackMessage*>(pUnk);
	iMessage = pMessage->GetCallbackMessage();
	SAFE_RELEASE(pUnk);

	if(iMessage == MSG_PROCESS_DECODING){

		IF_FAILED_RETURN(ProcessDecoding());
	}
	else if(iMessage == MSG_PROCESS_RENDERING){

		// todo : if FAILED, stop playing
		IF_FAILED_RETURN(ProcessRendering());
	}
#ifdef USE_MMCSS_WORKQUEUE
	else if(iMessage == PLAYER_MSG_REGISTER_MMCSS){

		DWORD dwTaskId = 0;

		if(SUCCEEDED(hr = MFEndRegisterWorkQueueWithMMCSS(pResult, &dwTaskId)))
			m_bIsMMCSS = TRUE;

		SetEvent(m_hMMCSS);
	}
	else if(iMessage == PLAYER_MSG_UNREGISTER_MMCSS){

		if(SUCCEEDED(hr = MFEndUnregisterWorkQueueWithMMCSS(pResult)))
			m_bIsMMCSS = FALSE;

		SetEvent(m_hMMCSS);
	}
#endif

	return hr;
}

HRESULT CPlayer::Init(){

	HRESULT hr;
	const DWORD H264_BUFFER_SIZE = 262144;
#ifdef USE_MMCSS_WORKQUEUE
	DWORD dwTaskId = 0;
	CCallbackMessage* pMessage = NULL;
#endif

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	try{

		IF_FAILED_THROW(m_pVideoBuffer.Initialize(H264_BUFFER_SIZE));
		IF_FAILED_THROW(m_pNalUnitBuffer.Initialize(H264_BUFFER_SIZE));

		// On windows7, only single-threaded queues
		IF_FAILED_THROW(MFAllocateWorkQueue(&m_dwWorkQueueRendering));
		IF_FAILED_THROW(MFAllocateWorkQueue(&m_dwWorkQueueDecoding));
		IF_FAILED_THROW(MFAllocateWorkQueue(&m_dwMessageQueue));

#ifdef USE_MMCSS_WORKQUEUE
		m_hMMCSS = CreateEvent(NULL, FALSE, FALSE, NULL);
		IF_FAILED_THROW(m_hMMCSS == NULL ? E_FAIL : S_OK);

		pMessage = new (std::nothrow)CCallbackMessage();
		IF_FAILED_THROW(pMessage == NULL ? E_OUTOFMEMORY : S_OK);
		pMessage->SetCallbackMessage(PLAYER_MSG_REGISTER_MMCSS);

		IF_FAILED_THROW(MFBeginRegisterWorkQueueWithMMCSS(m_dwWorkQueueRendering, L"Playback", dwTaskId, this, pMessage));

		WaitForSingleObject(m_hMMCSS, INFINITE);

		IF_FAILED_THROW(m_bIsMMCSS ? S_OK : E_FAIL);
#endif
	}
	catch(HRESULT){}

#ifdef USE_MMCSS_WORKQUEUE
	SAFE_RELEASE(pMessage);
	CLOSE_HANDLE_NULL_IF(m_hMMCSS);
#endif

	return hr;
}

HRESULT CPlayer::OpenFile(const HWND hWnd, LPCWSTR lpwszFile){

	HRESULT hr;
	BYTE* pVideoData = NULL;
	DWORD dwBufferSize;
	MFTIME llMovieDuration = 0LL;
	DXVA2_VideoDesc Dxva2Desc;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);
	IF_FAILED_RETURN(m_hEventEndRendering == NULL ? S_OK : E_UNEXPECTED);

	IF_FAILED_RETURN(m_cH264AtomParser.Initialize(lpwszFile));
	IF_FAILED_RETURN(m_cH264AtomParser.ParseMp4());
	m_iNaluLenghtSize = m_cH264AtomParser.GetNaluLenghtSize();
	m_cH264NaluParser.SetNaluLenghtSize(m_iNaluLenghtSize);

	IF_FAILED_RETURN(m_cH264AtomParser.GetFirstVideoStream(&m_dwTrackId));
	IF_FAILED_RETURN(m_cH264AtomParser.GetVideoConfigDescriptor(m_dwTrackId, &pVideoData, &dwBufferSize));
	IF_FAILED_RETURN(m_cH264NaluParser.ParseVideoConfigDescriptor(pVideoData, dwBufferSize));
	IF_FAILED_RETURN(m_cH264AtomParser.GetVideoDuration(m_dwTrackId, llMovieDuration));
	IF_FAILED_RETURN(llMovieDuration == 0 ? E_UNEXPECTED : S_OK);

	DXVA2_Frequency Dxva2Freq;
	IF_FAILED_RETURN(m_cH264AtomParser.GetVideoFrameRate(m_dwTrackId, &Dxva2Freq.Numerator, &Dxva2Freq.Denominator));
	IF_FAILED_RETURN(m_cDxva2Renderer.InitDXVA2(hWnd, m_cH264NaluParser.GetWidth(), m_cH264NaluParser.GetHeight(), Dxva2Freq.Numerator, Dxva2Freq.Denominator, Dxva2Desc));
	IF_FAILED_RETURN(m_cDxva2Decoder.InitVideoDecoder(m_cDxva2Renderer.GetDeviceManager9(), &Dxva2Desc, m_cH264NaluParser.GetSPS()));

	UINT64 AvgTimePerFrame;
	IF_FAILED_RETURN(MFFrameRateToAverageTimePerFrame(Dxva2Freq.Numerator, Dxva2Freq.Denominator, &AvgTimePerFrame));
	m_llPerFrame_1_4th = AvgTimePerFrame / 4;
	m_llPerFrame_3_4th = AvgTimePerFrame - m_llPerFrame_1_4th;

	return hr;
}

HRESULT CPlayer::Play(){

	HRESULT hr;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);
	IF_FAILED_RETURN(m_cDxva2Decoder.IsInitialized() && m_cDxva2Renderer.IsInitialized() ? S_OK : MF_E_NOT_INITIALIZED);

	// movie playing
	if(m_hEventEndRendering != NULL){

		if(m_bPause)
			m_bPause = FALSE;

		if(m_bStop || m_bFinish){
			Stop();
		}
		else{

			SendMessageToWindow(WND_MSG_PLAYING);
			return hr;
		}
	}

	m_hEventEndRendering = CreateEvent(NULL, FALSE, FALSE, NULL);
	IF_FAILED_THROW(m_hEventEndRendering == NULL ? E_FAIL : S_OK);

	m_bStop = FALSE;
	m_bFinish = FALSE;
	IF_FAILED_RETURN(SendMessageToPipeline(m_dwWorkQueueRendering, MSG_PROCESS_RENDERING));
	SendMessageToWindow(WND_MSG_PLAYING);

	return hr;
}

HRESULT CPlayer::Pause(){

	HRESULT hr;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	if(m_hEventEndRendering == NULL){
		return hr;
	}

	m_bPause = TRUE;

	SendMessageToWindow(WND_MSG_PAUSING);

	return hr;
}

HRESULT CPlayer::Stop(){

	HRESULT hr;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	if(m_hEventEndRendering == NULL){
		return hr;
	}

	StopEvent();

	m_cH264AtomParser.Reset();
	m_cDxva2Decoder.Reset();
	m_cDxva2Renderer.Reset();

	SendMessageToWindow(WND_MSG_STOPPING);

	return hr;
}

HRESULT CPlayer::Step(){

	HRESULT hr;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	if(m_hEventEndRendering == NULL){
		return hr;
	}

	if(m_bStop == FALSE)
		m_bStep = TRUE;

	return hr;
}

HRESULT CPlayer::PlayPause(){

	HRESULT hr;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	if(m_hEventEndRendering == NULL){
		return hr;
	}

	m_bPause = !m_bPause;

	SendMessageToWindow(m_bPause ? WND_MSG_PAUSING : WND_MSG_PLAYING);

	return hr;
}

HRESULT CPlayer::Close(){

	HRESULT hr;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	StopEvent();

	m_cH264AtomParser.Delete();
	m_cH264NaluParser.Reset();
	m_cDxva2Decoder.OnRelease();
	m_cDxva2Renderer.OnRelease();
	m_bSeek = FALSE;
	m_bFinish = FALSE;
	m_iNaluLenghtSize = 0;
	m_dwTrackId = 0;

	return hr;
}

HRESULT CPlayer::Shutdown(){

	HRESULT hr = S_OK;

	if(m_bIsShutdown == FALSE){

		Close();

#ifdef USE_MMCSS_WORKQUEUE
		if(m_bIsMMCSS)
			LOG_HRESULT(UnregisterMMCSS());
#endif

		LOG_HRESULT(MFUnlockWorkQueue(m_dwWorkQueueRendering));
		m_dwWorkQueueRendering = 0;

		LOG_HRESULT(MFUnlockWorkQueue(m_dwWorkQueueDecoding));
		m_dwWorkQueueDecoding = 0;

		LOG_HRESULT(MFUnlockWorkQueue(m_dwMessageQueue));
		m_dwMessageQueue = 0;

		m_pVideoBuffer.Delete();
		m_pNalUnitBuffer.Delete();

		SAFE_RELEASE(m_pWindowCallback);

		// Do not use CPlayer after Shutdown...
		m_bIsShutdown = TRUE;
	}

	return hr;
}

HRESULT CPlayer::SeekVideo(const MFTIME llDuration){

	HRESULT hr;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	if(m_bStop || m_bPause || m_bFinish)
		return hr;

	AutoLock lockDecoding(m_CriticSectionDecoding);
	AutoLock lock(m_CriticSection);

	m_cDxva2Decoder.ClearPresentation();
	m_dqPresentation.clear();

	IF_FAILED_RETURN(m_cH264AtomParser.SeekVideo(llDuration, m_dwTrackId));

	m_bSeek = TRUE;

	return hr;
}

HRESULT CPlayer::RepaintVideo(){

	HRESULT hr;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	IF_FAILED_RETURN(m_cDxva2Renderer.RenderLastFrame());

	return hr;
}

HRESULT CPlayer::OnFilter(const UINT uiFilter, const INT iValue){

	HRESULT hr;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	IF_FAILED_RETURN(m_cDxva2Renderer.SetFilter(uiFilter, iValue));

	if(m_bPause || m_bStep){
		IF_FAILED_RETURN(m_cDxva2Renderer.RenderLastFramePresentation(m_cDxva2Decoder.GetDirect3DSurface9()));
	}

	return hr;
}

HRESULT CPlayer::OnResetDxva2Settings(){

	HRESULT hr;

	IF_FAILED_RETURN(IsShutdown() ? MF_E_SHUTDOWN : S_OK);

	m_cDxva2Renderer.ResetDxva2Settings();

	if(m_bPause || m_bStep){
		IF_FAILED_RETURN(m_cDxva2Renderer.RenderLastFramePresentation(m_cDxva2Decoder.GetDirect3DSurface9()));
	}

	return hr;
}

#ifdef USE_MMCSS_WORKQUEUE
HRESULT CPlayer::UnregisterMMCSS(){

	HRESULT hr = S_OK;
	CCallbackMessage* pMessage = NULL;

	try{

		m_hMMCSS = CreateEvent(NULL, FALSE, FALSE, NULL);
		IF_FAILED_THROW(m_hMMCSS == NULL ? E_FAIL : S_OK);

		pMessage = new (std::nothrow)CCallbackMessage();
		IF_FAILED_THROW(pMessage == NULL ? E_OUTOFMEMORY : S_OK);
		pMessage->SetCallbackMessage(PLAYER_MSG_UNREGISTER_MMCSS);

		IF_FAILED_THROW(MFBeginUnregisterWorkQueueWithMMCSS(m_dwWorkQueueRendering, this, pMessage));

		WaitForSingleObject(m_hMMCSS, INFINITE);
	}
	catch(HRESULT){}

	SAFE_RELEASE(pMessage);
	CLOSE_HANDLE_NULL_IF(m_hMMCSS);

	return hr;
}
#endif

HRESULT CPlayer::ProcessDecoding(){

	HRESULT hr = S_OK;

	AutoLock lockDecoding(m_CriticSectionDecoding);

	if(m_bStop || m_bFinish){
		return hr;
	}

	BYTE* pVideoData = NULL;
	DWORD dwBufferSize;

	LONGLONG llTime = 0LL;
	int iSubSliceCount = 0;
	DWORD dwParsed;
	BYTE btStartCode[4] = {0x00, 0x00, 0x00, 0x01};

	IF_FAILED_RETURN(m_cH264AtomParser.GetNextSample(m_dwTrackId, &pVideoData, &dwBufferSize, &llTime));

	if(hr == S_FALSE){

		m_bFinish = TRUE;
		SendMessageToWindow(WND_MSG_FINISH);
		return hr;
	}

	try{

		IF_FAILED_THROW(m_pVideoBuffer.Reserve(dwBufferSize));
		memcpy(m_pVideoBuffer.GetStartBuffer(), pVideoData, dwBufferSize);
		IF_FAILED_THROW(m_pVideoBuffer.SetEndPosition(dwBufferSize));

		m_pNalUnitBuffer.Reset();

		do{

			if(iSubSliceCount == 0){

				IF_FAILED_THROW(m_pNalUnitBuffer.Reserve(m_pVideoBuffer.GetBufferSize()));
				memcpy(m_pNalUnitBuffer.GetStartBuffer(), m_pVideoBuffer.GetStartBuffer(), m_pVideoBuffer.GetBufferSize());
				IF_FAILED_THROW(m_pNalUnitBuffer.SetEndPosition(m_pVideoBuffer.GetBufferSize()));
			}

			IF_FAILED_THROW(m_cH264NaluParser.ParseNaluHeader(m_pVideoBuffer, &dwParsed));

			if(m_cH264NaluParser.IsNalUnitCodedSlice()){

				iSubSliceCount++;

				if(iSubSliceCount == 1)
					m_cDxva2Decoder.SetCurrentNalu(m_cH264NaluParser.GetPicture().NalUnitType, m_cH264NaluParser.GetPicture().btNalRefIdc);

				// DXVA2 needs start code
				if(m_iNaluLenghtSize == 4){
					memcpy(m_pNalUnitBuffer.GetStartBuffer(), btStartCode, 4);
				}
				else{
					IF_FAILED_THROW(AddByteAndConvertAvccToAnnexB(m_pNalUnitBuffer));
					dwParsed += 1;
				}

				IF_FAILED_THROW(m_cDxva2Decoder.AddSliceShortInfo(iSubSliceCount, dwParsed, m_iNaluLenghtSize == 4));

				if(m_pVideoBuffer.GetBufferSize() == 0){

					m_pNalUnitBuffer.SetStartPositionAtBeginning();
					IF_FAILED_THROW(m_cDxva2Decoder.DecodeFrame(m_pNalUnitBuffer, m_cH264NaluParser.GetPicture(), llTime, iSubSliceCount));

					SAMPLE_PRESENTATION SamplePresentation = {0};

					if(m_cDxva2Decoder.CheckFrame(SamplePresentation)){

						AutoLock lock(m_CriticSection);
						m_dqPresentation.push_front(SamplePresentation);
					}
				}
				else{

					// Sub-slices
					IF_FAILED_THROW(m_pNalUnitBuffer.SetStartPosition(dwParsed));
				}
			}
			else{

				if(iSubSliceCount > 0){

					// Can be NAL_UNIT_FILLER_DATA after sub-slices
					m_pNalUnitBuffer.SetStartPositionAtBeginning();

					IF_FAILED_THROW(m_cDxva2Decoder.DecodeFrame(m_pNalUnitBuffer, m_cH264NaluParser.GetPicture(), llTime, iSubSliceCount));

					SAMPLE_PRESENTATION SamplePresentation = {0};

					if(m_cDxva2Decoder.CheckFrame(SamplePresentation)){

						AutoLock lock(m_CriticSection);
						m_dqPresentation.push_front(SamplePresentation);
					}

					// We assume sub-slices are contiguous, so skip others
					m_pVideoBuffer.Reset();
				}
				else{

					m_pNalUnitBuffer.Reset();
				}
			}

			if(hr == S_FALSE){

				// S_FALSE means slice is corrupted. Just clear previous frames presentation, sometimes it's ok to continue
				m_cDxva2Decoder.ClearPresentation();
				hr = S_OK;
			}
			else if(m_pVideoBuffer.GetBufferSize() != 0){

				// Some slice contains SEI message and sub-slices, continue parsing
				hr = S_FALSE;
			}
		}
		while(hr == S_FALSE);
	}
	catch(HRESULT){}

	if(FAILED(hr)){

		m_bStop = TRUE;
		SendMessageToWindow(WND_MSG_STOPPING);
	}

	return hr;
}

HRESULT CPlayer::ProcessRendering(){

	HRESULT hr = S_OK;
	BOOL bNotStarted = TRUE;
	IMFPresentationClock* pPresentationClock = NULL;
	IMFPresentationTimeSource* pSystemTimeSource = NULL;
	LONGLONG llClockTime;
	LONGLONG llDelta;
	DWORD dwSleep = 0;
	BOOL bDecode;

	try{

		// PreProcess FRAME_COUNT_TO_PREROLL decoding
		for(int i = 0; i < FRAME_COUNT_TO_PREROLL; i++){

			IF_FAILED_THROW(SendMessageToPipeline(m_dwWorkQueueDecoding, MSG_PROCESS_DECODING));
			Sleep(10);
		}

		IF_FAILED_THROW(MFCreateSystemTimeSource(&pSystemTimeSource));
		IF_FAILED_THROW(MFCreatePresentationClock(&pPresentationClock));
		IF_FAILED_THROW(pPresentationClock->SetTimeSource(pSystemTimeSource));

		while(TRUE){

			if(m_bPause){

				LONGLONG llCurrentTime;
				IF_FAILED_THROW(pPresentationClock->Pause());
				IF_FAILED_THROW(pPresentationClock->GetTime(&llCurrentTime));

				// todo : wait for event
				while(m_bPause && !m_bStep)
					Sleep(200);

				IF_FAILED_THROW(pPresentationClock->Start(llCurrentTime));
			}

			if(m_bStop || (m_bFinish && m_dqPresentation.empty())){
				break;
			}

			{
				AutoLock lock(m_CriticSection);

				bDecode = TRUE;

				if(m_dqPresentation.empty()){

					dwSleep = (DWORD)(m_llPerFrame_3_4th / (double)ONE_MSEC);
				}
				else{

					auto it = m_dqPresentation.back();

					if(bNotStarted){

						IF_FAILED_THROW(pPresentationClock->Start(0LL));
						bNotStarted = FALSE;
					}
					else if(m_bSeek){

						IF_FAILED_THROW(pPresentationClock->Start(it.llTime));
						m_bSeek = FALSE;
					}

					//IF_FAILED_THROW(pPresentationClock->GetCorrelatedTime(0, &llClockTime, &llSystemTime));
					IF_FAILED_THROW(pPresentationClock->GetTime(&llClockTime));
					llDelta = it.llTime - llClockTime;

					if(llDelta < -m_llPerFrame_1_4th){

						// Frame too late
						IF_FAILED_THROW(m_cDxva2Renderer.RenderFrame(m_cDxva2Decoder.GetDirect3DSurface9(), it));
						m_cDxva2Decoder.FreeSurfaceIndexRenderer(it.dwDXVA2Index);

						m_dqPresentation.pop_back();

						if(m_bStep){

							m_bStep = FALSE;
							m_bPause = TRUE;
						}

						dwSleep = 0;
					}
					else if(llDelta > m_llPerFrame_3_4th){

						// Frame too early
						dwSleep = (DWORD)((llDelta - m_llPerFrame_3_4th) / (double)ONE_MSEC);
						bDecode = FALSE;
					}
					else{

						// Time to render frame
						IF_FAILED_THROW(m_cDxva2Renderer.RenderFrame(m_cDxva2Decoder.GetDirect3DSurface9(), it));
						m_cDxva2Decoder.FreeSurfaceIndexRenderer(it.dwDXVA2Index);

						m_dqPresentation.pop_back();

						if(m_bStep){

							m_bStep = FALSE;
							m_bPause = TRUE;
						}

						dwSleep = (DWORD)(m_llPerFrame_1_4th / (double)ONE_MSEC);
					}
				}
			}

			// Outside m_CriticSection
			if(bDecode){

				// We rendered a video frame, so ask for another to be decoded
				IF_FAILED_THROW(SendMessageToPipeline(m_dwWorkQueueDecoding, MSG_PROCESS_DECODING));
			}

			Sleep(dwSleep);
		}
	}
	catch(HRESULT){}

	if(pPresentationClock){

		LOG_HRESULT(pPresentationClock->Stop());
		SAFE_RELEASE(pPresentationClock);
	}

	SAFE_RELEASE(pSystemTimeSource);

	LOG_HRESULT(m_cDxva2Renderer.RenderBlackFrame());

	SetEvent(m_hEventEndRendering);

	return hr;
}

HRESULT CPlayer::SendMessageToPipeline(const DWORD dwQueue, const int iType){

	HRESULT hr = S_OK;

	IMFAsyncResult* pAsyncResult = NULL;
	CCallbackMessage* pMessage = NULL;

	try{

		pMessage = new (std::nothrow)CCallbackMessage();

		IF_FAILED_RETURN(pMessage == NULL ? E_OUTOFMEMORY : S_OK);

		pMessage->SetCallbackMessage(iType);

		IF_FAILED_THROW(MFCreateAsyncResult(NULL, this, pMessage, &pAsyncResult));

		IF_FAILED_THROW(MFPutWorkItemEx(dwQueue, pAsyncResult));
	}
	catch(HRESULT){}

	SAFE_RELEASE(pMessage);
	SAFE_RELEASE(pAsyncResult);

	return hr;
}

HRESULT CPlayer::SendMessageToWindow(const int iMessage){

	HRESULT hr = S_OK;

	IMFAsyncResult* pAsyncResult = NULL;
	CCallbackMessage* pMessage = NULL;

	try{

		pMessage = new (std::nothrow)CCallbackMessage();

		IF_FAILED_RETURN(pMessage == NULL ? E_OUTOFMEMORY : S_OK);

		pMessage->SetCallbackMessage(iMessage);

		IF_FAILED_THROW(MFCreateAsyncResult(pMessage, m_pWindowCallback, NULL, &pAsyncResult));

		IF_FAILED_THROW(MFPutWorkItemEx(m_dwMessageQueue, pAsyncResult));
	}
	catch(HRESULT){}

	SAFE_RELEASE(pMessage);
	SAFE_RELEASE(pAsyncResult);

	return hr;
}

void CPlayer::StopEvent(){

	if(m_hEventEndRendering == NULL)
		return;

	m_bStop = TRUE;
	m_bPause = FALSE;

	// todo : don't use INFINITE
	WaitForSingleObject(m_hEventEndRendering, INFINITE);
	CLOSE_HANDLE_NULL_IF(m_hEventEndRendering);

	AutoLock lockDecoding(m_CriticSectionDecoding);
	m_dqPresentation.clear();
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