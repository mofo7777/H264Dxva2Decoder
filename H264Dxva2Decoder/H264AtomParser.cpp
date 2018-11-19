//----------------------------------------------------------------------------------------------
// H264AtomParser.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

CH264AtomParser::CH264AtomParser() :
	m_pByteStream(NULL),
	m_dwCurrentSample(0),
	m_dwTimeScale(0),
	m_ui64VideoDuration(0),
	m_iNaluLenghtSize(0)
{
	ZeroMemory(&m_DisplayMatrix, sizeof(m_DisplayMatrix));
}

HRESULT CH264AtomParser::Initialize(LPCWSTR wszFile){

	HRESULT hr;

	IF_FAILED_RETURN(wszFile ? S_OK : E_INVALIDARG);
	IF_FAILED_RETURN(m_pByteStream ? ERROR_ALREADY_INITIALIZED : S_OK);

	CMFByteStream* pByteStream = NULL;

	try{

		IF_FAILED_THROW(CMFByteStream::CreateInstance(&pByteStream));
		IF_FAILED_THROW(pByteStream->Initialize(wszFile));
		IF_FAILED_THROW(m_cMp4ParserBuffer.Initialize(READ_SIZE));

		m_pByteStream = pByteStream;
		m_pByteStream->AddRef();
	}
	catch(HRESULT){}

	SAFE_RELEASE(pByteStream);

	return hr;
}

HRESULT CH264AtomParser::ParseMp4(){

	HRESULT hr;
	ROOT_ATOM sRootAtom = {0};

	IF_FAILED_RETURN(m_pByteStream ? S_OK : MF_E_NOT_INITIALIZED);

	try{

		IF_FAILED_THROW(m_pByteStream->Reset());
		m_cMp4ParserBuffer.Reset();

		IF_FAILED_THROW(ParseAtoms(sRootAtom));

		IF_FAILED_THROW(sRootAtom.bFTYP && sRootAtom.bMOOV && sRootAtom.bMDAT ? S_OK : E_FAIL);

		for(auto& TrackInfo : m_vTrackInfo){

			if(TrackInfo.dwTypeHandler == HANDLER_TYPE_VIDEO){

				IF_FAILED_THROW(FinalizeSampleTime(TrackInfo));
				IF_FAILED_THROW(FinalizeSampleOffset(TrackInfo));
				IF_FAILED_THROW(FinalizeSampleSync(TrackInfo));
			}

			TrackInfo.vChunks.clear();
			TrackInfo.vSyncSamples.clear();
			TrackInfo.vChunkOffset.clear();
			TrackInfo.vTimeSample.clear();
			TrackInfo.vCompositionTime.clear();
			TrackInfo.vEditList.clear();
		}
	}
	catch(HRESULT){}

	return hr;
}

void CH264AtomParser::Delete(){

	SAFE_RELEASE(m_pByteStream);
	m_cMp4ParserBuffer.Delete();

	ZeroMemory(&m_DisplayMatrix, sizeof(m_DisplayMatrix));

	for(auto& TrackInfo : m_vTrackInfo){

		// todo : all vector should be clear when m_vTrackInfo.clear(), we can just do SAFE_DELETE
		TrackInfo.vChunks.clear();
		TrackInfo.vSamples.clear();
		TrackInfo.vSyncSamples.clear();
		TrackInfo.vChunkOffset.clear();
		TrackInfo.vTimeSample.clear();
		TrackInfo.vCompositionTime.clear();
		TrackInfo.vEditList.clear();

		SAFE_DELETE(TrackInfo.pConfig);
	}

	m_vTrackInfo.clear();

	m_dwCurrentSample = 0;
	m_dwTimeScale = 0;
	m_ui64VideoDuration = 0;
	m_iNaluLenghtSize = 0;
}

HRESULT CH264AtomParser::GetNextSample(const DWORD dwTrackId, BYTE** ppData, DWORD* pSize, LONGLONG* pllTime){

	HRESULT hr;
	DWORD dwRead;
	DWORD dwChunkSize;
	DWORD dwOffset;

	IF_FAILED_RETURN(ppData && pSize ? S_OK : E_FAIL);

	const vector<SAMPLE_INFO>* vSamples = GetSamples(dwTrackId);

	IF_FAILED_RETURN(vSamples ? S_OK : E_FAIL);

	if(m_dwCurrentSample >= vSamples->size()){
		return S_FALSE;
	}

	IF_FAILED_RETURN(m_pByteStream->Reset());
	IF_FAILED_RETURN((*vSamples)[m_dwCurrentSample].dwSize > 4 ? S_OK : E_FAIL);

	dwOffset = (*vSamples)[m_dwCurrentSample].dwOffset;

	// Perhaps just use SeekHigh
	if(dwOffset <= LONG_MAX){

		IF_FAILED_RETURN(m_pByteStream->Seek(dwOffset));
	}
	else{

		LARGE_INTEGER liFilePosToRead = {0};
		liFilePosToRead.QuadPart = dwOffset;
		IF_FAILED_RETURN(m_pByteStream->SeekHigh(liFilePosToRead));
	}

	dwChunkSize = (*vSamples)[m_dwCurrentSample].dwSize;

	m_cMp4ParserBuffer.Reset();

	IF_FAILED_RETURN(m_cMp4ParserBuffer.Reserve(dwChunkSize));

	IF_FAILED_RETURN(m_pByteStream->Read(m_cMp4ParserBuffer.GetStartBuffer(), dwChunkSize, &dwRead));

	if(dwRead == dwChunkSize){

		IF_FAILED_RETURN(m_cMp4ParserBuffer.SetEndPosition(dwRead));

		*ppData = m_cMp4ParserBuffer.GetStartBuffer();
		*pSize = m_cMp4ParserBuffer.GetBufferSize();
		*pllTime = (*vSamples)[m_dwCurrentSample].llTime + (*vSamples)[m_dwCurrentSample].llDuration;
		m_dwCurrentSample++;
	}
	else{

		TRACE((L"GetNextSample : ChunkSize = %u - Read = %u", dwChunkSize, dwRead));
		hr = S_FALSE;
	}

	return hr;
}

HRESULT CH264AtomParser::GetVideoConfigDescriptor(const DWORD dwTrackId, BYTE** ppData, DWORD* pSize){

	HRESULT hr;

	CMFLightBuffer* pConfig = GetConfig(dwTrackId);

	IF_FAILED_RETURN(!pConfig || pConfig->GetBufferSize() == 0 ? E_FAIL : S_OK);

	*ppData = pConfig->GetBuffer();
	*pSize = pConfig->GetBufferSize();

	return hr;
}

HRESULT CH264AtomParser::GetVideoFrameRate(const DWORD dwTrackId, UINT* puiNumerator, UINT* puiDenominator){

	HRESULT hr;
	UINT64 ui64Time = 0;

	for(auto& TrackInfo : m_vTrackInfo){

		if(TrackInfo.dwTrackId == dwTrackId){

			const vector<SAMPLE_INFO>::const_reverse_iterator rit = TrackInfo.vSamples.rbegin();

			if(rit != TrackInfo.vSamples.rend()){

				ui64Time = rit->llTime + rit->llDuration;

				if(TrackInfo.vSamples.size() != 0)
					ui64Time /= TrackInfo.vSamples.size();
			}

			break;
		}
	}

	// Frames per second(floating point)	Frames per second(fractional)	Average time per frame
	// 59.94								60000 / 1001					166833
	// 29.97								30000 / 1001					333667
	// 23.976								24000 / 1001					417188
	// 60									60 / 1							166667
	// 30									30 / 1							333333
	// 50									50 / 1							200000
	// 25									25 / 1							400000
	// 24									24 / 1							416667

	IF_FAILED_RETURN(ui64Time ? S_OK : E_FAIL);
	IF_FAILED_RETURN(MFAverageTimePerFrameToFrameRate(ui64Time, puiNumerator, puiDenominator));

	return hr;
}

HRESULT CH264AtomParser::GetFirstVideoStream(DWORD* pdwTrackId){

	HRESULT hr = E_FAIL;

	for(auto& TrackInfo : m_vTrackInfo){

		if(TrackInfo.dwTypeHandler == HANDLER_TYPE_VIDEO){

			*pdwTrackId = TrackInfo.dwTrackId;
			hr = S_OK;
			break;
		}
	}

	return hr;
}

HRESULT CH264AtomParser::FinalizeSampleTime(TRACK_INFO& TrackInfo){

	HRESULT hr;
	DWORD dwCompositionTimeIndex = 1;
	DWORD dwTimeSampleIndex = 0;
	DWORD dwTimeScale = 0;
	LONGLONG llTime = 0;
	LONGLONG llDuration = 0;

	// todo
	//assert(TrackInfo.vEditList.size() == 0);

	IF_FAILED_RETURN(TrackInfo.vTimeSample.size() == 0 ? E_FAIL : S_OK);

	dwTimeScale = TrackInfo.dwTimeScale ? TrackInfo.dwTimeScale : m_dwTimeScale;

	IF_FAILED_RETURN(dwTimeScale ? S_OK : E_FAIL);

	vector<TIME_INFO>::const_iterator itTime = TrackInfo.vTimeSample.begin();

	if(TrackInfo.vCompositionTime.size() != 0){

		vector<TIME_INFO>::const_iterator itComposition = TrackInfo.vCompositionTime.begin();

		for(auto& itSample : TrackInfo.vSamples){

			llDuration = itTime->dwOffset * (10000000 / dwTimeScale);
			itSample.llDuration = llDuration;

			dwTimeSampleIndex++;

			if(itTime->dwCount == dwTimeSampleIndex){

				if((itTime + 1) != TrackInfo.vTimeSample.end())
					++itTime;

				dwTimeSampleIndex = 0;
			}

			itSample.llTime = llTime + (itComposition->dwOffset * (10000000 / dwTimeScale));

			if(itComposition->dwCount == 1){

				if((itComposition + 1) != TrackInfo.vCompositionTime.end())
					++itComposition;
			}
			else if(itComposition->dwCount == dwCompositionTimeIndex){

				if((itComposition + 1) != TrackInfo.vCompositionTime.end())
					++itComposition;

				dwCompositionTimeIndex = 1;
			}
			else{

				dwCompositionTimeIndex++;
			}

			llTime += llDuration;
		}
	}
	else{

		for(auto& itSample : TrackInfo.vSamples){

			llDuration = itTime->dwOffset * (10000000 / dwTimeScale);
			itSample.llDuration = llDuration;

			dwTimeSampleIndex++;

			if(itTime->dwCount == dwTimeSampleIndex){

				if((itTime + 1) != TrackInfo.vTimeSample.end())
					++itTime;

				dwTimeSampleIndex = 0;
			}

			itSample.llTime = llTime;
			llTime += llDuration;
		}
	}

	return hr;
}

HRESULT CH264AtomParser::FinalizeSampleOffset(TRACK_INFO& TrackInfo){

	HRESULT hr;
	DWORD dwChunkIndex = 1;
	DWORD dwChunkOffsetIndex = 1;
	DWORD dwPrevOffset = 0;
	DWORD dwPrevSize = 0;

	IF_FAILED_RETURN(TrackInfo.vSamples.size() == 0 || TrackInfo.vChunks.size() == 0 || TrackInfo.vChunkOffset.size() == 0 ? E_FAIL : S_OK);

	// todo : fixed sample size (for video, should not happen)
	IF_FAILED_RETURN(TrackInfo.dwFixedSampleSize == 0 ? S_OK : E_FAIL);

	vector<CHUNCK_INFO>::const_iterator itChunk = TrackInfo.vChunks.begin();
	vector<DWORD>::const_iterator itChunkOffset = TrackInfo.vChunkOffset.begin();
	vector<CHUNCK_INFO>::const_iterator itChunkNext;

	// Skip bad first chunk
	// Some stupid mp4 files have multiple dwFirstChunk == 1, just skip them, seems to be ok
	while((itChunkNext = itChunk + 1) != TrackInfo.vChunks.end()){

		if(itChunkNext->dwFirstChunk == itChunk->dwFirstChunk)
			itChunk = itChunkNext;
		else
			break;
	}

	for(auto& itSample : TrackInfo.vSamples){

		if(itChunk->dwSamplesPerChunk == 1){

			itSample.dwOffset = *itChunkOffset;

			if((itChunkOffset + 1) != TrackInfo.vChunkOffset.end())
				++itChunkOffset;

			dwChunkIndex++;
		}
		else if(itChunk->dwSamplesPerChunk == dwChunkOffsetIndex){

			itSample.dwOffset = dwPrevOffset + dwPrevSize;

			if((itChunkOffset + 1) != TrackInfo.vChunkOffset.end())
				++itChunkOffset;

			dwChunkOffsetIndex = 1;
			dwChunkIndex++;
		}
		else{

			if(dwChunkOffsetIndex == 1){

				itSample.dwOffset = *itChunkOffset;
			}
			else{

				itSample.dwOffset = dwPrevOffset + dwPrevSize;
			}

			dwPrevOffset = itSample.dwOffset;
			dwPrevSize = itSample.dwSize;

			dwChunkOffsetIndex++;
		}

		if((itChunkNext = itChunk + 1) != TrackInfo.vChunks.end()){

			if(itChunkNext->dwFirstChunk == dwChunkIndex)
				itChunk = itChunkNext;
		}
	}

	return hr;
}

HRESULT CH264AtomParser::FinalizeSampleSync(TRACK_INFO& TrackInfo){

	HRESULT hr;
	IF_FAILED_RETURN(TrackInfo.vSamples.size() == 0 || TrackInfo.vSyncSamples.size() == 0 ? E_FAIL : S_OK);

	vector<SAMPLE_INFO>::iterator itSamples = TrackInfo.vSamples.begin();

	for(auto& itSyncSample : TrackInfo.vSyncSamples){

		if(itSyncSample && itSyncSample <= TrackInfo.vSamples.size())
			(itSamples + (itSyncSample - 1))->bKeyFrame = TRUE;
	}

	return hr;
}

HRESULT CH264AtomParser::SeekAtom(LARGE_INTEGER& liCurrentFilePos, const DWORD dwAtomSize, const DWORD dwRead, const UINT64 ui64AtomSize){

	HRESULT hr;

	if(dwAtomSize == 1){

		if(ui64AtomSize > LONG_MAX){

			LARGE_INTEGER liFilePosToSeek = {0};
			liFilePosToSeek.QuadPart = ui64AtomSize - dwRead;
			IF_FAILED_RETURN(m_pByteStream->SeekHigh(liFilePosToSeek));
		}
		else{

			IF_FAILED_RETURN(m_pByteStream->Seek(LONG(ui64AtomSize - dwRead)));
		}

		liCurrentFilePos.QuadPart += ui64AtomSize;
	}
	else{

		if(dwAtomSize > LONG_MAX){

			LARGE_INTEGER liFilePosToSeek = {0};
			liFilePosToSeek.QuadPart = dwAtomSize - dwRead;
			IF_FAILED_RETURN(m_pByteStream->SeekHigh(liFilePosToSeek));
		}
		else{

			IF_FAILED_RETURN(m_pByteStream->Seek(dwAtomSize - dwRead));
		}

		liCurrentFilePos.QuadPart += dwAtomSize;
	}

	return hr;
}

HRESULT CH264AtomParser::ParseAtoms(ROOT_ATOM& sRootAtom){

	HRESULT hr;
	DWORD dwRead;
	BYTE* pData;
	DWORD dwAtomSize;
	DWORD dwAtomType;
	UINT64 ui64AtomSize = 0;
	LARGE_INTEGER liCurrentFilePos = {0};

	while(SUCCEEDED(hr = m_pByteStream->Read(m_cMp4ParserBuffer.GetReadStartBuffer(), ATOM_MIN_READ_SIZE_HEADER, &dwRead)) && dwRead == ATOM_MIN_READ_SIZE_HEADER){

		pData = m_cMp4ParserBuffer.GetStartBuffer();

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// todo : atom size is the lenght of the rest of the file, this is the last atom
		assert(dwAtomSize != 0);

		if(dwAtomSize == 1){

			// todo : ui64AtomSize is the real atom size, we need to change the code to handle 64 bits atom size
			// For now, can be OK with only MDAT atom and ui64AtomSize < LONG_MAX
			ui64AtomSize = MAKE_DWORD64(pData + 8);
		}

		switch(dwAtomType){

			case ATOM_TYPE_FTYP:
				// Only one
				IF_FAILED_RETURN(sRootAtom.bFTYP ? E_FAIL : S_OK);
				sRootAtom.bFTYP = TRUE;
				break;

			case ATOM_TYPE_MOOV:
				// Only one
				IF_FAILED_RETURN(sRootAtom.bMOOV ? E_FAIL : S_OK);
				// todo : just one call here
				assert(dwAtomSize != 1 && dwAtomSize > ATOM_MIN_READ_SIZE_HEADER);
				IF_FAILED_RETURN(m_pByteStream->Seek(-ATOM_MIN_SIZE_HEADER));
				IF_FAILED_RETURN(ParseMoov(dwAtomSize - ATOM_MIN_SIZE_HEADER));
				sRootAtom.bMOOV = TRUE;
				// Seek is already done in ParseMoov, so continue
				continue;

			case ATOM_TYPE_MDAT:
				// One or more
				sRootAtom.bMDAT = TRUE;
				break;

			case ATOM_TYPE_FREE:
			case ATOM_TYPE_UUID:
			case ATOM_TYPE_WIDE:
				break;
		}

		// todo : sometimes ATOM_TYPE_UNKNOWN after last root, check why
		if(sRootAtom.bFTYP && sRootAtom.bMOOV && sRootAtom.bMDAT)
			break;

		IF_FAILED_RETURN(SeekAtom(liCurrentFilePos, dwAtomSize, dwRead, ui64AtomSize));
	}

	return hr;
}

HRESULT CH264AtomParser::ParseMoov(const DWORD dwAtomMoovSize){

	HRESULT hr;
	DWORD dwRead;
	BYTE* pData;
	DWORD dwAtomSize;
	DWORD dwAtomType;

	IF_FAILED_RETURN(m_cMp4ParserBuffer.Reserve(dwAtomMoovSize));
	IF_FAILED_RETURN(m_pByteStream->Read(m_cMp4ParserBuffer.GetReadStartBuffer(), dwAtomMoovSize, &dwRead));
	IF_FAILED_RETURN(dwRead == dwAtomMoovSize ? S_OK : E_FAIL);
	IF_FAILED_RETURN(m_cMp4ParserBuffer.SetEndPosition(dwRead));

	while(m_cMp4ParserBuffer.GetBufferSize() >= ATOM_MIN_SIZE_HEADER){

		pData = m_cMp4ParserBuffer.GetStartBuffer();
		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		IF_FAILED_RETURN(m_cMp4ParserBuffer.GetBufferSize() < dwAtomSize);

		switch(dwAtomType){

			case ATOM_TYPE_TRAK:
				{
					TRACK_INFO TrackInfo = {0};
					if(FAILED(hr = ParseTrack(TrackInfo, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER))){

						SAFE_DELETE(TrackInfo.pConfig);
						return hr;
					}
					m_vTrackInfo.push_back(TrackInfo);
				}
				break;

			case ATOM_TYPE_MVHD:
				IF_FAILED_RETURN(ParseMovieHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_UDTA:
			case ATOM_TYPE_IODS:
			case ATOM_TYPE_FREE:
			case ATOM_TYPE_UUID:
				break;
		}

		IF_FAILED_RETURN(m_cMp4ParserBuffer.SetStartPosition(dwAtomSize));
	}

	return hr;
}

HRESULT CH264AtomParser::ParseTrack(TRACK_INFO& TrackInfo, BYTE* pData, const DWORD dwAtomTrackSize){

	HRESULT hr = S_OK;

	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwByteDone = 0;

	while(dwAtomTrackSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		IF_FAILED_RETURN(dwAtomSize > dwAtomTrackSize - dwByteDone ? E_FAIL : S_OK);

		switch(dwAtomType){

			case ATOM_TYPE_TKHD:
				IF_FAILED_RETURN(ParseTrackHeader(TrackInfo, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_EDTS:
				IF_FAILED_RETURN(ParseEditAtoms(TrackInfo.vEditList, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_MDIA:
				IF_FAILED_RETURN(ParseMediaAtom(TrackInfo, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_UDTA:
			case ATOM_TYPE_TREF:
			case ATOM_TYPE_UUID:
				break;
		}

		dwByteDone += dwAtomSize;
		pData += dwAtomSize;
	}

	return hr;
}

HRESULT CH264AtomParser::ParseMovieHeader(BYTE* pData, const DWORD dwAtomMovieHeaderSize){

	HRESULT hr;

	BYTE btVersion = *pData >> 6;

	DWORD dwTotalSize = btVersion ? 112 : 100;

	IF_FAILED_RETURN(dwTotalSize == dwAtomMovieHeaderSize ? S_OK : E_FAIL);

	// Skip Version/Flag
	pData += 4;

	// Skip Creation time and Modification time
	if(btVersion == 0x01)
		pData += 16;
	else
		pData += 8;

	m_dwTimeScale = MAKE_DWORD(pData);
	pData += 4;

	if(btVersion == 0x01){

		m_ui64VideoDuration = MAKE_DWORD64(pData);
		pData += 8;
	}
	else{

		m_ui64VideoDuration = MAKE_DWORD(pData);
		pData += 4;
	}

	// Skip Preferred Rate/Volume/Reserved
	//pData += 16;

	// Skip Matrix
	//pData += 36;

	/*
	// Preview time
	MAKE_DWORD(pData);
	pData += 4;
	// Preview duration
	MAKE_DWORD(pData);
	pData += 4;
	// Poster time
	MAKE_DWORD(pData);
	pData += 4;
	// Selection time
	MAKE_DWORD(pData);
	pData += 4;
	// Selection duration
	MAKE_DWORD(pData);
	pData += 4;
	// Current time
	MAKE_DWORD(pData);
	pData += 4;
	// Next track ID
	MAKE_DWORD(pData);
	*/

	return hr;
}

HRESULT CH264AtomParser::ParseTrackHeader(TRACK_INFO& TrackInfo, BYTE* pData, const DWORD dwAtomTrackHeaderSize){

	HRESULT hr;

	BYTE btVersion = *pData >> 6;

	DWORD dwTotalSize = btVersion ? 96 : 84;

	IF_FAILED_RETURN(dwTotalSize == dwAtomTrackHeaderSize ? S_OK : E_FAIL);

	// todo : flags
	// if flags == 0, all enable
	// BYTE btFlags = *pData & 0x0f; (if (btFlags != 0) -> todo)

	// skip version/flags
	pData += 4;

	// Skip Creation/Modification Time
	if(btVersion == 0x01){
		pData += 16;
	}
	else{
		pData += 8;
	}

	TrackInfo.dwTrackId = MAKE_DWORD(pData);
	pData += 4;

	// Skip Reserved/Duration/Reserved x 2
	if(btVersion == 0x01){
		pData += 20;
	}
	else{
		pData += 16;
	}

	// Skip Layer/Alternate_group/Reserved
	pData += 8;

	for(int i = 0; i < 3; i++){

		m_DisplayMatrix[i][0] = (int)MAKE_DWORD(pData);
		pData += 4;
		m_DisplayMatrix[i][1] = (int)MAKE_DWORD(pData);
		pData += 4;
		m_DisplayMatrix[i][2] = (int)MAKE_DWORD(pData);
		pData += 4;
	}

	TrackInfo.dwWidth = MAKE_DWORD(pData) >> 16;
	pData += 4;

	TrackInfo.dwHeight = MAKE_DWORD(pData) >> 16;

	return hr;
}

HRESULT CH264AtomParser::ParseEditAtoms(vector<EDIT_INFO>& vEditList, BYTE* pData, const DWORD dwAtomEditSize){

	HRESULT hr;

	// The Edit List Box provides the initial CT value if it is non-empty (non-zero).

	DWORD dwAtomSize;
	DWORD dwAtomType;

	dwAtomSize = MAKE_DWORD(pData);
	dwAtomType = MAKE_DWORD(pData + 4);

	IF_FAILED_RETURN(dwAtomType == ATOM_TYPE_ELST ? S_OK : E_FAIL);
	// todo : check dwAtomSize 0/1
	IF_FAILED_RETURN(dwAtomSize > dwAtomEditSize ? E_FAIL : S_OK);

	// skip atom : size + type
	pData += 8;

	BYTE btVersion = *pData >> 6;

	// todo : version 1 (0x01) -> Track duration and Media time are MAKE_DWORD64
	IF_FAILED_RETURN(btVersion != 0x00 ? E_FAIL : S_OK);

	// skip version + flag
	pData += 4;

	const DWORD dwEntries = MAKE_DWORD(pData);

	DWORD dwTotalSize = btVersion ? (dwEntries * 20) : (dwEntries * 12);

	IF_FAILED_RETURN(dwTotalSize <= (dwAtomSize - 16) ? S_OK : E_FAIL);

	for(DWORD dwCount = 0; dwCount < dwEntries; dwCount++){

		EDIT_INFO sEditInfo = {0};

		pData += 4;
		sEditInfo.dwSegmentDuration = MAKE_DWORD(pData);
		pData += 4;
		sEditInfo.dwMediaTime = MAKE_DWORD(pData);
		pData += 4;
		sEditInfo.dwMediaRate = MAKE_DWORD(pData);

		vEditList.push_back(sEditInfo);
	}

	return hr;
}

HRESULT CH264AtomParser::ParseMediaAtom(TRACK_INFO& TrackInfo, BYTE* pData, const DWORD dwAtomMovieHeaderSize){

	HRESULT hr = S_OK;

	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwByteDone = 0;

	while(dwAtomMovieHeaderSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		IF_FAILED_RETURN(dwAtomSize > dwAtomMovieHeaderSize - dwByteDone ? E_FAIL : S_OK);

		switch(dwAtomType){

			case ATOM_TYPE_HDLR:
				IF_FAILED_RETURN(dwAtomSize < 32 ? E_FAIL : S_OK);
				TrackInfo.dwTypeHandler = MAKE_DWORD(pData + 16);
				break;

			case ATOM_TYPE_MINF:
				IF_FAILED_RETURN(ParseMediaInfoHeader(TrackInfo, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_MDHD:
				IF_FAILED_RETURN(ParseMediaHeader(TrackInfo, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;
		}

		dwByteDone += dwAtomSize;
		pData += dwAtomSize;
	}

	return hr;
}

HRESULT CH264AtomParser::ParseMediaInfoHeader(TRACK_INFO& TrackInfo, BYTE* pData, const DWORD dwAtomMediaHeaderSize){

	HRESULT hr = S_OK;

	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwByteDone = 0;

	while(dwAtomMediaHeaderSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		IF_FAILED_RETURN(dwAtomSize > dwAtomMediaHeaderSize - dwByteDone ? E_FAIL : S_OK);

		switch(dwAtomType){

			case ATOM_TYPE_STBL:
				IF_FAILED_RETURN(ParseSampleTableHeader(TrackInfo, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_VMHD:
			case ATOM_TYPE_SMHD:
			case ATOM_TYPE_GMHD:
			case ATOM_TYPE_DINF:
			case ATOM_TYPE_NMHD:
			case ATOM_TYPE_HMHD:
			case ATOM_TYPE_HDLR:
				break;
		}

		dwByteDone += dwAtomSize;
		pData += dwAtomSize;
	}

	return hr;
}

HRESULT CH264AtomParser::ParseMediaHeader(TRACK_INFO& TrackInfo, BYTE* pData, const DWORD dwAtomMediaHeaderSize){

	HRESULT hr;

	BYTE btVersion = *pData;

	DWORD dwTotalSize = btVersion ? 36 : 24;

	IF_FAILED_RETURN(dwTotalSize == dwAtomMediaHeaderSize ? S_OK : E_FAIL);

	// Skip Version/Flag
	pData += 4;

	// Skip Creation time and Modification time
	if(btVersion == 0x01)
		pData += 16;
	else
		pData += 8;

	TrackInfo.dwTimeScale = MAKE_DWORD(pData);
	pData += 4;

	if(btVersion == 0x01){

		TrackInfo.ui64VideoDuration = MAKE_DWORD64(pData);
		pData += 8;
	}
	else{

		TrackInfo.ui64VideoDuration = MAKE_DWORD(pData);
		pData += 4;
	}

	// Pad/Langage/pre_defined

	return hr;
}

HRESULT CH264AtomParser::ParseSampleTableHeader(TRACK_INFO& TrackInfo, BYTE* pData, const DWORD dwAtomSampleHeaderSize){

	HRESULT hr = S_OK;

	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwByteDone = 0;

	while(dwAtomSampleHeaderSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		IF_FAILED_RETURN(dwAtomSize > dwAtomSampleHeaderSize - dwByteDone ? E_FAIL : S_OK);

		switch(dwAtomType){

			case ATOM_TYPE_STSD:
				IF_FAILED_RETURN(ParseSampleDescHeader(TrackInfo, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_STTS:
				IF_FAILED_RETURN(ParseSampleTimeHeader(TrackInfo.vTimeSample, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_STSS:
				IF_FAILED_RETURN(ParseSyncSampleHeader(TrackInfo.vSyncSamples, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_CTTS:
				IF_FAILED_RETURN(ParseCompositionOffsetHeader(TrackInfo.vCompositionTime, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_STSC:
				IF_FAILED_RETURN(ParseSampleChunckHeader(TrackInfo.vChunks, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_STSZ:
				IF_FAILED_RETURN(ParseSampleSizeHeader(TrackInfo.vSamples, &TrackInfo.dwFixedSampleSize, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_STCO:
				IF_FAILED_RETURN(ParseChunckOffsetHeader(TrackInfo.vChunkOffset, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_CO64:
				IF_FAILED_RETURN(ParseChunckOffset64Header(TrackInfo.vChunkOffset, pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
				break;

			case ATOM_TYPE_SDTP:// todo
			case ATOM_TYPE_STPS:// todo
			case ATOM_TYPE_FREE:
			case ATOM_TYPE_SBGP:
			case ATOM_TYPE_SGPD:
				break;
		}

		dwByteDone += dwAtomSize;
		pData += dwAtomSize;
	}

	return hr;
}

HRESULT CH264AtomParser::ParseSampleDescHeader(TRACK_INFO& TrackInfo, BYTE* pData, const DWORD dwAtomSampleDescSize){

	HRESULT hr;

	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwAtomCount;
	DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(dwByteDone < dwAtomSampleDescSize ? S_OK : E_FAIL);

	pData += 4;

	dwAtomCount = MAKE_DWORD(pData);

	// todo : if more than one STSD. For now just get the first
	if(dwAtomCount > 1)
		TRACE((L"error : dwAtomCount = %u", dwAtomCount));

	pData += 4;

	while(dwAtomSampleDescSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		switch(dwAtomType){

			case ATOM_TYPE_AVC1:
				IF_FAILED_RETURN(ParseAvc1Format(&TrackInfo.pConfig, pData, dwAtomSize));
				break;

			case ATOM_TYPE_MP4V:
			case ATOM_TYPE_MP4A:
			case ATOM_TYPE_TEXT:
			case ATOM_TYPE_MP4S:
			case ATOM_TYPE_AC_3:
			case ATOM_TYPE_RTP_:
			case ATOM_TYPE_EC_3:
				break;
		}

		dwByteDone += dwAtomSize;
		pData += dwAtomSize;

		// todo : if more than one STSD. For now just get the first
		break;
	}

	return hr;
}

HRESULT CH264AtomParser::ParseSampleTimeHeader(vector<TIME_INFO>& vTimeSample, BYTE* pData, const DWORD dwAtomSampleTimeSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(dwByteDone < dwAtomSampleTimeSize ? S_OK : E_FAIL);

	pData += 4;
	const DWORD dwTimeSamples = MAKE_DWORD(pData);

	IF_FAILED_RETURN((dwTimeSamples > 0) && ((dwTimeSamples * 8) == (dwAtomSampleTimeSize - dwByteDone)) ? S_OK : E_FAIL);

	vTimeSample.reserve(dwTimeSamples);

	for(DWORD dwCount = 0; dwCount < dwTimeSamples; dwCount++){

		TIME_INFO sTimeInfo = {0};

		pData += 4;
		sTimeInfo.dwCount = MAKE_DWORD(pData);
		pData += 4;
		sTimeInfo.dwOffset = MAKE_DWORD(pData);

		vTimeSample.push_back(sTimeInfo);
	}

	return hr;
}

HRESULT CH264AtomParser::ParseSyncSampleHeader(vector<DWORD>& vSyncSamples, BYTE* pData, const DWORD dwAtomSyncSampleSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(dwByteDone <= dwAtomSyncSampleSize ? S_OK : E_FAIL);

	pData += 4;
	const DWORD dwSyncSamples = MAKE_DWORD(pData);

	if(dwSyncSamples == 0)
		return hr;

	IF_FAILED_RETURN((dwSyncSamples * 4) <= (dwAtomSyncSampleSize - dwByteDone) ? S_OK : E_FAIL);

	vSyncSamples.reserve(dwSyncSamples);

	for(DWORD dwCount = 0; dwCount < dwSyncSamples; dwCount++){

		pData += 4;

		vSyncSamples.push_back(MAKE_DWORD(pData));
	}

	return hr;
}

HRESULT CH264AtomParser::ParseCompositionOffsetHeader(vector<TIME_INFO>& vCompositionTime, BYTE* pData, const DWORD dwAtomCompositionOffsetSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(dwByteDone < dwAtomCompositionOffsetSize ? S_OK : E_FAIL);

	pData += 4;
	const DWORD dwCompositionOffsetCount = MAKE_DWORD(pData);

	IF_FAILED_RETURN((dwCompositionOffsetCount > 0) && ((dwCompositionOffsetCount * 8) == (dwAtomCompositionOffsetSize - dwByteDone)) ? S_OK : E_FAIL);

	vCompositionTime.reserve(dwCompositionOffsetCount);

	for(DWORD dwCount = 0; dwCount < dwCompositionOffsetCount; dwCount++){

		TIME_INFO sTimeInfo = {0};

		pData += 4;
		sTimeInfo.dwCount = MAKE_DWORD(pData);
		pData += 4;
		sTimeInfo.dwOffset = MAKE_DWORD(pData);

		vCompositionTime.push_back(sTimeInfo);
	}

	return hr;
}

HRESULT CH264AtomParser::ParseSampleChunckHeader(vector<CHUNCK_INFO>& vChunks, BYTE* pData, const DWORD dwAtomSampleChunckSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(dwByteDone < dwAtomSampleChunckSize ? S_OK : E_FAIL);

	pData += 4;
	const DWORD dwSampleChunckSize = MAKE_DWORD(pData);

	IF_FAILED_RETURN((dwSampleChunckSize > 0) && ((dwSampleChunckSize * 12) <= (dwAtomSampleChunckSize - dwByteDone)) ? S_OK : E_FAIL);

	vChunks.reserve(dwSampleChunckSize);

	for(DWORD dwCount = 0; dwCount < dwSampleChunckSize; dwCount++){

		CHUNCK_INFO sChunckInfo = {0};

		pData += 4;
		sChunckInfo.dwFirstChunk = MAKE_DWORD(pData);

		pData += 4;
		sChunckInfo.dwSamplesPerChunk = MAKE_DWORD(pData);

		pData += 4;
		sChunckInfo.dwSampleDescId = MAKE_DWORD(pData);

		vChunks.push_back(sChunckInfo);
	}

	return hr;
}

HRESULT CH264AtomParser::ParseSampleSizeHeader(vector<SAMPLE_INFO>& vSamples, DWORD* pdwFixedSampleSize, BYTE* pData, const DWORD dwAtomSampleSize){

	HRESULT hr;

	DWORD dwSampleSize;
	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER + 4;

	IF_FAILED_RETURN(dwByteDone <= dwAtomSampleSize ? S_OK : E_FAIL);

	// Skip version + flags
	pData += 4;
	dwSampleSize = MAKE_DWORD(pData);

	if(dwSampleSize != 0){

		*pdwFixedSampleSize = dwSampleSize;
		return hr;
	}

	pData += 4;
	dwSampleSize = MAKE_DWORD(pData);

	IF_FAILED_RETURN((dwSampleSize > 0) && ((dwSampleSize * 4) <= (dwAtomSampleSize - dwByteDone)) ? S_OK : E_FAIL);

	pData += 4;

	vSamples.reserve(dwSampleSize);

	for(DWORD dwCount = 0; dwCount < dwSampleSize; dwCount++){

		SAMPLE_INFO sSampleInfo = {0};
		sSampleInfo.dwSize = MAKE_DWORD(pData);

		vSamples.push_back(sSampleInfo);
		pData += 4;
	}

	return hr;
}

HRESULT CH264AtomParser::ParseChunckOffsetHeader(vector<DWORD>& vChunkOffset, BYTE* pData, const DWORD dwAtomChunckOffsetSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(dwByteDone <= dwAtomChunckOffsetSize ? S_OK : E_FAIL);

	pData += 4;

	const DWORD dwChuncks = MAKE_DWORD(pData);

	IF_FAILED_RETURN((dwChuncks * 4) <= (dwAtomChunckOffsetSize - dwByteDone) ? S_OK : E_FAIL);

	vChunkOffset.reserve(dwChuncks);

	for(DWORD dwCount = 0; dwCount < dwChuncks; dwCount++){

		pData += 4;
		vChunkOffset.push_back(MAKE_DWORD(pData));
	}

	return hr;
}

HRESULT CH264AtomParser::ParseChunckOffset64Header(vector<DWORD>& vChunkOffset, BYTE* pData, const DWORD dwAtomChunckOffsetSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(dwByteDone <= dwAtomChunckOffsetSize ? S_OK : E_FAIL);

	pData += 4;

	const DWORD dwChuncks = MAKE_DWORD(pData);

	IF_FAILED_RETURN((dwChuncks * 8) <= (dwAtomChunckOffsetSize - dwByteDone) ? S_OK : E_FAIL);

	vChunkOffset.reserve(dwChuncks);
	pData += 4;

	for(DWORD dwCount = 0; dwCount < dwChuncks; dwCount++){

		UINT64 uiTest = MAKE_DWORD64(pData);
		assert(uiTest <= ULONG_MAX);
		vChunkOffset.push_back((DWORD)uiTest);
		pData += 8;
	}

	return hr;
}

HRESULT CH264AtomParser::ParseAvc1Format(CMFLightBuffer** ppConfig, const BYTE* pData, const DWORD dwAvc1AtomSize){

	HRESULT hr = S_OK;
	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwByteDone = 86;
	pData += 86;

	while(dwAvc1AtomSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		switch(dwAtomType){

			case ATOM_TYPE_AVCC:
				IF_FAILED_RETURN(ParseVideoConfigDescriptor(ppConfig, pData + 8, dwAtomSize - 8));
				break;

			case ATOM_TYPE_PASP:
			case ATOM_TYPE_BTRT:
			case ATOM_TYPE_COLR:
			case ATOM_TYPE_GAMA:
			case ATOM_TYPE_UUID:
				break;
		}

		dwByteDone += dwAtomSize;
		pData += dwAtomSize;
	}

	return hr;
}

HRESULT CH264AtomParser::ParseVideoConfigDescriptor(CMFLightBuffer** ppConfig, const BYTE* pData, const DWORD /*dwSize*/){

	HRESULT hr;
	CMFLightBuffer* pConfig = NULL;
	BYTE m_btNaluSize[4] = {0x00, 0x00, 0x00, 0x00};
	int iDecrease;
	DWORD dwLenght;
	DWORD dwLastLenght;

	// we must check pData does not exceed dwSize
	// todo : what are we skipping ?
	pData += 4;

	// todo : expect all video stream same NaluLenghtSize
	m_iNaluLenghtSize = (*pData++ & 0x03) + 1;

	// todo : m_iNaluLenghtSize == 1
	IF_FAILED_RETURN((m_iNaluLenghtSize != 1 && m_iNaluLenghtSize != 2 && m_iNaluLenghtSize != 4) ? E_FAIL : S_OK);

	pConfig = new (std::nothrow)CMFLightBuffer;
	IF_FAILED_RETURN(pConfig ? S_OK : E_OUTOFMEMORY);

	*ppConfig = pConfig;

	IF_FAILED_RETURN(pConfig->Initialize(m_iNaluLenghtSize));
	memcpy(pConfig->GetBuffer(), m_btNaluSize, m_iNaluLenghtSize);

	int iCurrentPos = m_iNaluLenghtSize;

	WORD wParameterSets = *pData++ & 0x1F;
	WORD wParameterSetLength;

	for(int i = 0; i < wParameterSets; i++){

		wParameterSetLength = ((*pData++) << 8);
		wParameterSetLength += *pData++;

		IF_FAILED_RETURN(pConfig->Reserve(wParameterSetLength));
		memcpy(pConfig->GetBuffer() + iCurrentPos, pData, wParameterSetLength);

		pData += wParameterSetLength;
		iCurrentPos += wParameterSetLength;
	}

	RemoveEmulationPreventionByte(pConfig, &iDecrease, 0);

	IF_FAILED_RETURN(iDecrease < (iCurrentPos + 4) ? S_OK : E_UNEXPECTED);
	iCurrentPos -= iDecrease;

	dwLenght = iCurrentPos - m_iNaluLenghtSize;

	if(m_iNaluLenghtSize == 4){

		pConfig->GetBuffer()[0] = (dwLenght >> 24) & 0xff;
		pConfig->GetBuffer()[1] = (dwLenght >> 16) & 0xff;
		pConfig->GetBuffer()[2] = (dwLenght >> 8) & 0xff;
		pConfig->GetBuffer()[3] = dwLenght & 0xff;
	}
	else if(m_iNaluLenghtSize == 2){

		pConfig->GetBuffer()[0] = (dwLenght >> 8) & 0xff;
		pConfig->GetBuffer()[1] = dwLenght & 0xff;
	}

	IF_FAILED_RETURN(pConfig->Reserve(m_iNaluLenghtSize));
	memcpy(pConfig->GetBuffer() + iCurrentPos, m_btNaluSize, m_iNaluLenghtSize);
	dwLenght = iCurrentPos;
	iCurrentPos += m_iNaluLenghtSize;
	wParameterSets = *pData++;

	for(int i = 0; i < wParameterSets; i++){

		wParameterSetLength = ((*pData++) << 8);
		wParameterSetLength += *pData++;

		IF_FAILED_RETURN(pConfig->Reserve(wParameterSetLength));
		memcpy(pConfig->GetBuffer() + iCurrentPos, pData, wParameterSetLength);

		pData += wParameterSetLength;
		iCurrentPos += wParameterSetLength;
	}

	RemoveEmulationPreventionByte(pConfig, &iDecrease, dwLenght);

	IF_FAILED_RETURN(iDecrease < (iCurrentPos + 4) ? S_OK : E_UNEXPECTED);
	iCurrentPos -= iDecrease;

	dwLastLenght = iCurrentPos - dwLenght - m_iNaluLenghtSize;

	if(m_iNaluLenghtSize == 4){

		pConfig->GetBuffer()[dwLenght] = (dwLastLenght >> 24) & 0xff;
		pConfig->GetBuffer()[dwLenght + 1] = (dwLastLenght >> 16) & 0xff;
		pConfig->GetBuffer()[dwLenght + 2] = (dwLastLenght >> 8) & 0xff;
		pConfig->GetBuffer()[dwLenght + 3] = dwLastLenght & 0xff;
	}
	else if(m_iNaluLenghtSize == 2){

		pConfig->GetBuffer()[dwLenght] = (dwLastLenght >> 8) & 0xff;
		pConfig->GetBuffer()[dwLenght + 1] = dwLastLenght & 0xff;
	}

	return hr;
}

void CH264AtomParser::RemoveEmulationPreventionByte(CMFLightBuffer* pConfig, int* piDecrease, const DWORD dwPosition){

	assert(pConfig);

	DWORD dwValue;
	DWORD dwIndex = dwPosition;
	const DWORD dwSize = pConfig->GetBufferSize();
	BYTE* pData = pConfig->GetBuffer();
	*piDecrease = 0;

	assert(dwIndex < dwSize);
	pData += dwIndex;

	while(dwIndex < dwSize){

		dwValue = MAKE_DWORD(pData);

		if(dwValue == 0x00000300 || dwValue == 0x00000301 || dwValue == 0x00000302 || dwValue == 0x00000303){

			BYTE* pDataTmp = pConfig->GetBuffer();

			memcpy(pDataTmp + (dwIndex + 2), pDataTmp + (dwIndex + 3), pConfig->GetBufferSize() - (dwIndex + 2));
			pConfig->DecreaseEndPosition();
			*piDecrease += 1;

			dwIndex += 3;
			pData += 3;
		}
		else
		{
			dwIndex += 1;
			pData += 1;
		}
	}
}

const vector<CH264AtomParser::SAMPLE_INFO>* CH264AtomParser::GetSamples(const DWORD dwTrackId) const{

	for(auto& TrackInfo : m_vTrackInfo){

		if(TrackInfo.dwTrackId == dwTrackId){

			return &TrackInfo.vSamples;
		}
	}

	return NULL;
}

CMFLightBuffer* CH264AtomParser::GetConfig(const DWORD dwTrackId) const{

	for(auto& TrackInfo : m_vTrackInfo){

		if(TrackInfo.dwTrackId == dwTrackId){

			return TrackInfo.pConfig;
		}
	}

	return NULL;
}