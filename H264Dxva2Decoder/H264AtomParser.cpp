//----------------------------------------------------------------------------------------------
// H264AtomParser.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

CH264AtomParser::CH264AtomParser() :
	m_pByteStream(NULL),
	m_bEOS(FALSE),
	m_bHasVideoOffset(FALSE),
	m_dwCurrentSample(0),
	m_dwTimeScale(0),
	m_dwDuration(0),
	m_iNaluLenghtSize(0)
{
	ZeroMemory(&m_sAtomHeader, sizeof(ATOM_HEADER));
	ZeroMemory(&m_sTrackHeader, 2 * sizeof(TRACK_HEADER));
	ZeroMemory(&m_DisplayMatrix, sizeof(m_DisplayMatrix));
}

HRESULT CH264AtomParser::Initialize(LPCWSTR wszFile){

	HRESULT hr;

	IF_FAILED_RETURN(hr = wszFile ? S_OK : E_INVALIDARG);
	IF_FAILED_RETURN(hr = m_pByteStream ? ERROR_ALREADY_INITIALIZED : S_OK);

	CMFByteStream* pByteStream = NULL;

	try{

		IF_FAILED_THROW(hr = CMFByteStream::CreateInstance(&pByteStream));

		DWORD dwFlags;
		LARGE_INTEGER liFileSize;

		IF_FAILED_THROW(hr = pByteStream->Initialize(wszFile, &dwFlags, &liFileSize));
		IF_FAILED_THROW(hr = m_cMp4ParserBuffer.Initialize(READ_SIZE));
		IF_FAILED_THROW(hr = m_cMp4ParserBuffer.Reserve(READ_SIZE));

		m_pByteStream = pByteStream;
		m_pByteStream->AddRef();
	}
	catch(HRESULT){}

	SAFE_RELEASE(pByteStream);

	return hr;
}

void CH264AtomParser::Delete(){

	m_cMp4ParserBuffer.Delete();
	SAFE_RELEASE(m_pByteStream);
	m_pSyncHeader.Delete();

	m_vChunks.clear();
	m_vSamples.clear();
	m_vSyncSamples.clear();
	m_vChunkOffset.clear();
	m_vCompositionOffset.clear();
}

HRESULT CH264AtomParser::GetNextSample(BYTE** ppData, DWORD* pSize){

	HRESULT hr;
	DWORD dwRead;
	DWORD dwChunkSize;

	IF_FAILED_RETURN(hr = (ppData && pSize ? S_OK : E_FAIL));

	if(m_dwCurrentSample >= m_vSamples.size()){
		return S_FALSE;
	}

	IF_FAILED_RETURN(hr = m_pByteStream->Reset());
	IF_FAILED_RETURN(hr = (m_vSamples[m_dwCurrentSample].dwSize > 8 ? S_OK : E_FAIL));

	// todo : SeekHigh
	assert((m_vSamples[m_dwCurrentSample].dwOffset) <= LONG_MAX);
	IF_FAILED_RETURN(hr = m_pByteStream->Seek(m_vSamples[m_dwCurrentSample].dwOffset));

	dwChunkSize = m_vSamples[m_dwCurrentSample].dwSize;

	m_cMp4ParserBuffer.Reset();

	IF_FAILED_RETURN(hr = m_cMp4ParserBuffer.Reserve(dwChunkSize));

	IF_FAILED_RETURN(hr = m_pByteStream->Read(m_cMp4ParserBuffer.GetStartBuffer(), dwChunkSize, &dwRead));
	IF_FAILED_RETURN(hr = (dwRead == dwChunkSize ? S_OK : E_FAIL));

	IF_FAILED_RETURN(hr = m_cMp4ParserBuffer.SetEndPosition(dwRead));

	*ppData = m_cMp4ParserBuffer.GetStartBuffer();
	*pSize = dwRead;
	m_dwCurrentSample++;

	return hr;
}

HRESULT CH264AtomParser::GetVideoConfigDescriptor(BYTE** ppData, DWORD* pSize){

	HRESULT hr;

	IF_FAILED_RETURN(hr = (m_pSyncHeader.GetBufferSize() == 0 ? E_FAIL : S_OK));

	*ppData = m_pSyncHeader.GetBuffer();
	*pSize = m_pSyncHeader.GetBufferSize();

	return hr;
}

HRESULT CH264AtomParser::ParseMp4(){

	HRESULT hr;

	IF_FAILED_RETURN(hr = m_pByteStream ? S_OK : E_FAIL);
	ZeroMemory(&m_sAtomHeader, sizeof(ATOM_HEADER));
	ZeroMemory(&m_sTrackHeader, 2 * sizeof(TRACK_HEADER));
	ZeroMemory(&m_DisplayMatrix, sizeof(m_DisplayMatrix));
	m_bHasVideoOffset = FALSE;
	m_vChunks.clear();
	m_vSamples.clear();
	m_vSyncSamples.clear();
	m_vChunkOffset.clear();
	m_vCompositionOffset.clear();
	m_dwCurrentSample = 0;
	m_dwTimeScale = 0;
	m_dwDuration = 0;
	m_iNaluLenghtSize = 0;
	m_pSyncHeader.Delete();

	try{

		IF_FAILED_THROW(hr = m_pByteStream->Reset());
		m_cMp4ParserBuffer.Reset();

		IF_FAILED_THROW(hr = FindAtoms());

		IF_FAILED_THROW(hr = (m_sAtomHeader.bFTYP && m_sAtomHeader.bMOOV && m_sAtomHeader.bMDAT ? S_OK : E_FAIL));

		IF_FAILED_THROW(hr = ReadMoov());

		IF_FAILED_THROW(hr = FinalizeSample());
	}
	catch(HRESULT){}

	return hr;
}

HRESULT CH264AtomParser::FindAtoms(){

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
				if(m_sAtomHeader.bFTYP == FALSE && dwAtomSize > ATOM_MIN_SIZE_HEADER){

					m_sAtomHeader.bFTYP = TRUE;
					m_sAtomHeader.liFtypFilePos = liCurrentFilePos;
				}
				break;

			case ATOM_TYPE_MOOV:
				if(m_sAtomHeader.bMOOV == FALSE && dwAtomSize > ATOM_MIN_SIZE_HEADER){

					m_sAtomHeader.bMOOV = TRUE;
					m_sAtomHeader.dwMoovSize = dwAtomSize;
					m_sAtomHeader.liMoovFilePos = liCurrentFilePos;
				}
				break;

			case ATOM_TYPE_MDAT:
				if(m_sAtomHeader.bMDAT == FALSE && (dwAtomSize > ATOM_MIN_SIZE_HEADER || dwAtomSize == 1)){

					m_sAtomHeader.bMDAT = TRUE;
					m_sAtomHeader.liMdatFilePos = liCurrentFilePos;
				}
				break;
		}

		if(dwAtomSize == 1){

			if(ui64AtomSize > LONG_MAX){

				LARGE_INTEGER liFilePosToSeek = {0};
				liFilePosToSeek.QuadPart = ui64AtomSize - dwRead;
				IF_FAILED_RETURN(hr = m_pByteStream->SeekHigh(liFilePosToSeek));
			}
			else{

				IF_FAILED_RETURN(hr = m_pByteStream->Seek(LONG(ui64AtomSize - dwRead)));
			}

			liCurrentFilePos.QuadPart += ui64AtomSize;
		}
		else{

			if(dwAtomSize > LONG_MAX){

				LARGE_INTEGER liFilePosToSeek = {0};
				liFilePosToSeek.QuadPart = dwAtomSize - dwRead;
				IF_FAILED_RETURN(hr = m_pByteStream->SeekHigh(liFilePosToSeek));
			}
			else{

				IF_FAILED_RETURN(hr = m_pByteStream->Seek(dwAtomSize - dwRead));
			}

			liCurrentFilePos.QuadPart += dwAtomSize;
		}
	}

	return hr;
}

HRESULT CH264AtomParser::ReadMoov(){

	HRESULT hr;
	DWORD dwRead;
	BYTE* pData;
	DWORD dwAtomSize;
	DWORD dwAtomType;

	IF_FAILED_RETURN(hr = (m_sAtomHeader.dwMoovSize <= ATOM_MIN_SIZE_HEADER ? E_FAIL : S_OK));

	IF_FAILED_RETURN(hr = m_cMp4ParserBuffer.Reserve(m_sAtomHeader.dwMoovSize));
	IF_FAILED_RETURN(hr = m_pByteStream->Reset());
	IF_FAILED_RETURN(hr = m_pByteStream->SeekFile(m_sAtomHeader.liMoovFilePos));
	IF_FAILED_RETURN(hr = m_pByteStream->Read(m_cMp4ParserBuffer.GetReadStartBuffer(), m_sAtomHeader.dwMoovSize, &dwRead));
	IF_FAILED_RETURN(hr = (dwRead == m_sAtomHeader.dwMoovSize ? S_OK : E_FAIL));
	IF_FAILED_RETURN(hr = m_cMp4ParserBuffer.SetEndPosition(dwRead));

	pData = m_cMp4ParserBuffer.GetStartBuffer();
	dwAtomSize = MAKE_DWORD(pData);
	dwAtomType = MAKE_DWORD(pData + 4);

	IF_FAILED_RETURN(hr = (dwAtomType == ATOM_TYPE_MOOV && dwAtomSize == dwRead ? S_OK : E_FAIL));
	IF_FAILED_RETURN(hr = m_cMp4ParserBuffer.SetStartPosition(ATOM_MIN_SIZE_HEADER));

	while(m_cMp4ParserBuffer.GetBufferSize() >= ATOM_MIN_SIZE_HEADER){

		pData = m_cMp4ParserBuffer.GetStartBuffer();
		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		IF_FAILED_RETURN(hr = (m_cMp4ParserBuffer.GetBufferSize() < dwAtomSize));

		if(dwAtomType == ATOM_TYPE_TRAC){

			IF_FAILED_RETURN(hr = ReadTrack(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}
		else if(dwAtomType == ATOM_TYPE_MVHD){

			IF_FAILED_RETURN(hr = ReadMovieHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}

		IF_FAILED_RETURN(hr = m_cMp4ParserBuffer.SetStartPosition(dwAtomSize));
	}

	return hr;
}

HRESULT CH264AtomParser::FinalizeSample(){

	HRESULT hr;
	DWORD dwCurrentKeyFrame = 0;
	DWORD dwSampleCount = 0;
	DWORD dwChunkOffsetIndex = 0;

	IF_FAILED_RETURN(hr = (m_vSyncSamples.size() == 0 || m_vSamples.size() == 0 || m_vChunks.size() == 0 || m_vChunkOffset.size() == 0 ? E_FAIL : S_OK));

	vector<CHUNCK_INFO>::const_iterator itChunk = m_vChunks.begin();
	vector<CHUNCK_INFO>::const_iterator itChunkNext;
	vector<DWORD>::const_iterator itChunkOffset = m_vChunkOffset.begin();

	do{

		// Set KeyFrame
		if(dwCurrentKeyFrame < m_vSyncSamples.size() && (m_vSyncSamples[dwCurrentKeyFrame] - 1) == dwSampleCount){

			m_vSamples[dwSampleCount].bKeyFrame = TRUE;
			dwCurrentKeyFrame++;
		}

		if(itChunk->dwSamplesPerChunk > 1){

			DWORD dwPrevOffset = m_vSamples[dwSampleCount].dwOffset = *itChunkOffset;
			DWORD dwPrevSize = m_vSamples[dwSampleCount].dwSize;

			dwSampleCount++;
			itChunkOffset++;
			dwChunkOffsetIndex++;

			for(DWORD dwCount = 1; dwCount < itChunk->dwSamplesPerChunk && dwSampleCount < m_vSamples.size(); dwCount++, dwSampleCount++){

				m_vSamples[dwSampleCount].dwOffset = dwPrevOffset + dwPrevSize;

				dwPrevOffset = m_vSamples[dwSampleCount].dwOffset;
				dwPrevSize = m_vSamples[dwSampleCount].dwSize;

				// Set KeyFrame
				if(dwCurrentKeyFrame < m_vSyncSamples.size() && (m_vSyncSamples[dwCurrentKeyFrame] - 1) == dwSampleCount){

					m_vSamples[dwSampleCount].bKeyFrame = TRUE;
					dwCurrentKeyFrame++;
				}
			}

			if((itChunkNext = itChunk + 1) != m_vChunks.end()){

				if((itChunkNext->dwFirstChunk - 1) == dwChunkOffsetIndex)
					itChunk = itChunkNext;
			}
		}
		else{

			m_vSamples[dwSampleCount].dwOffset = *itChunkOffset;

			dwSampleCount++;
			itChunkOffset++;
			dwChunkOffsetIndex++;

			if((itChunkNext = itChunk + 1) != m_vChunks.end()){

				if((itChunkNext->dwFirstChunk - 1) == dwChunkOffsetIndex)
					itChunk = itChunkNext;
			}
		}
	}
	while(dwSampleCount < m_vSamples.size());

	return hr;
}

HRESULT CH264AtomParser::ReadTrack(BYTE* pData, const DWORD dwAtomTrackSize){

	HRESULT hr = S_OK;

	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwByteDone = 0;

	while(dwAtomTrackSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		IF_FAILED_RETURN(hr = (dwAtomSize > dwAtomTrackSize - dwByteDone ? E_FAIL : S_OK));

		if(dwAtomType == ATOM_TYPE_TKHD){

			IF_FAILED_RETURN(hr = ReadTrackHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}
		else if(dwAtomType == ATOM_TYPE_EDTS){

			IF_FAILED_RETURN(hr = ReadEditAtoms(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}
		else if(dwAtomType == ATOM_TYPE_MDIA){

			IF_FAILED_RETURN(hr = ReadMediaAtom(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}

		dwByteDone += dwAtomSize;
		pData += dwAtomSize;
	}

	return hr;
}

HRESULT CH264AtomParser::ReadMovieHeader(BYTE* pData, const DWORD /*dwAtomMovieHeaderSize*/){

	HRESULT hr = S_OK;

	BYTE btVersion = *pData;

	// Skip Version + Flag
	pData += 4;

	// Skip Creation time and Modification time
	if(btVersion == 0x01)
		pData += 16;
	else
		pData += 8;

	m_dwTimeScale = MAKE_DWORD(pData);
	pData += 4;

	// Skip Duration
	if(btVersion == 0x01)
	{
		// todo
		//UINT64 ui64Duration = MAKE_DWORD64(pData);
		pData += 8;
	}
	else{

		m_dwDuration = MAKE_DWORD(pData);
		pData += 4;
	}

	// Skip Preferred rate/Volume and Reserved
	pData += 16;

	for(int i = 0; i < 3; i++){

		m_DisplayMatrix[i][0] = (int)MAKE_DWORD(pData);
		pData += 4;
		m_DisplayMatrix[i][1] = (int)MAKE_DWORD(pData);
		pData += 4;
		m_DisplayMatrix[i][2] = (int)MAKE_DWORD(pData);
		pData += 4;
	}

	/*
	// Preview time
	MAKE_DWORD(pData);
	pData++;
	// Preview duration
	MAKE_DWORD(pData);
	pData++;
	// Poster time
	MAKE_DWORD(pData);
	pData++;
	// Selection time
	MAKE_DWORD(pData);
	pData++;
	// Selection duration
	MAKE_DWORD(pData);
	pData++;
	// Current time
	MAKE_DWORD(pData);
	pData++;
	// Next track ID
	MAKE_DWORD(pData);
	*/

	return hr;
}

HRESULT CH264AtomParser::ReadTrackHeader(BYTE* /*pData*/, const DWORD /*dwAtomTrackHeaderSize*/){

	HRESULT hr = S_OK;

	/*DWORD dwCurrentData = MAKE_DWORD(pData);

	TRACE((L"Flags : 0x%08x", dwCurrentData));
	pData += 4;
	dwCurrentData = MAKE_DWORD(pData);

	time_t now = dwCurrentData - 2082844800;
	tm* timeinfo = gmtime(&now);
	//tm* timeinfo = localtime(&now);
	char buffer[80];
	strftime(buffer, 80, "%a %b %d %H:%M : %S %Y", timeinfo);

	TRACE((L"Creation Time : 0x%08x", dwCurrentData));
	pData += 4;
	dwCurrentData = MAKE_DWORD(pData);

	TRACE((L"Modification Time : 0x%08x", dwCurrentData));
	pData += 4;
	dwCurrentData = MAKE_DWORD(pData);

	TRACE((L"Track ID : 0x%08x", dwCurrentData));
	pData += 4;
	dwCurrentData = MAKE_DWORD(pData);

	TRACE((L"Reserved : 0x%08x", dwCurrentData));
	pData += 4;
	dwCurrentData = MAKE_DWORD(pData);

	TRACE((L"Duration : 0x%08x", dwCurrentData));
	pData += 4;
	dwCurrentData = MAKE_DWORD(pData);

	TRACE((L"Reserved : 0x%08x", dwCurrentData));
	pData += 4;
	dwCurrentData = MAKE_DWORD(pData);

	TRACE((L"Reserved : 0x%08x", dwCurrentData));
	pData += 4;
	dwCurrentData = MAKE_DWORD(pData);*/

	return hr;
}

HRESULT CH264AtomParser::ReadEditAtoms(BYTE* pData, const DWORD dwAtomEditSize){

	HRESULT hr = S_OK;

	DWORD dwAtomSize;
	DWORD dwAtomType;

	dwAtomSize = MAKE_DWORD(pData);
	dwAtomType = MAKE_DWORD(pData + 4);

	IF_FAILED_RETURN(hr = (dwAtomType == ATOM_TYPE_ELST ? S_OK : E_FAIL));
	IF_FAILED_RETURN(hr = (dwAtomSize > dwAtomEditSize ? E_FAIL : S_OK));

	// skip atom : size + type
	pData += 8;

	BYTE btVersion = *pData;

	// todo : version 1 (0x01) -> Track duration and Media time are MAKE_DWORD64
	IF_FAILED_RETURN(hr = (btVersion != 0x00 ? E_FAIL : S_OK));

	// skip version + flag
	pData += 4;

	const DWORD dwEntries = MAKE_DWORD(pData);

	// todo : check dwAtomSize with (dwEntries * 12)

	for(DWORD dwCount = 0; dwCount < dwEntries; dwCount++){

		/*
		pData += 4;
		// Track duration
		pData += 4;
		// Media time
		pData += 4;
		// Media rate
		*/

		pData += 12;
	}

	return hr;
}

HRESULT CH264AtomParser::ReadMediaAtom(BYTE* pData, const DWORD dwAtomMovieHeaderSize){

	HRESULT hr = S_OK;

	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwByteDone = 0;
	BOOL bIsVideoHandler = FALSE;

	while(dwAtomMovieHeaderSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		IF_FAILED_RETURN(hr = (dwAtomSize > dwAtomMovieHeaderSize - dwByteDone ? E_FAIL : S_OK));

		if(!m_bHasVideoOffset && dwAtomType == ATOM_TYPE_HDLR){

			IF_FAILED_RETURN(hr = (dwAtomSize < 20 ? E_FAIL : S_OK));

			if(MAKE_DWORD(pData + 16) == HANDLER_TYPE_VIDEO)
				bIsVideoHandler = TRUE;
		}
		else if(!m_bHasVideoOffset && bIsVideoHandler && dwAtomType == ATOM_TYPE_MINF){

			IF_FAILED_RETURN(hr = ReadVideoMediaInfoHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}

		dwByteDone += dwAtomSize;
		pData += dwAtomSize;
	}

	return hr;
}

HRESULT CH264AtomParser::ReadVideoMediaInfoHeader(BYTE* pData, const DWORD dwAtomMediaHeaderSize){

	HRESULT hr = S_OK;

	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwByteDone = 0;

	while(dwAtomMediaHeaderSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		IF_FAILED_RETURN(hr = (dwAtomSize > dwAtomMediaHeaderSize - dwByteDone ? E_FAIL : S_OK));

		if(dwAtomType == ATOM_TYPE_STBL){

			IF_FAILED_RETURN(hr = ReadSampleTableHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}

		dwByteDone += dwAtomSize;
		pData += dwAtomSize;
	}

	return hr;
}

HRESULT CH264AtomParser::ReadSampleTableHeader(BYTE* pData, const DWORD dwAtomSampleHeaderSize){

	HRESULT hr = S_OK;

	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwByteDone = 0;

	while(dwAtomSampleHeaderSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		IF_FAILED_RETURN(hr = (dwAtomSize > dwAtomSampleHeaderSize - dwByteDone ? E_FAIL : S_OK));

		if(dwAtomType == ATOM_TYPE_STSD){

			IF_FAILED_RETURN(hr = ReadSampleDescHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}
		else if(dwAtomType == ATOM_TYPE_STTS){

			IF_FAILED_RETURN(hr = ReadSampleTimeHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}
		else if(dwAtomType == ATOM_TYPE_STSS){

			IF_FAILED_RETURN(hr = ReadSyncSampleHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}
		else if(dwAtomType == ATOM_TYPE_CTTS){

			IF_FAILED_RETURN(hr = ReadCompositionOffsetHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}
		else if(dwAtomType == ATOM_TYPE_STSC){

			IF_FAILED_RETURN(hr = ReadSampleChunckHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}
		else if(dwAtomType == ATOM_TYPE_STSZ){

			IF_FAILED_RETURN(hr = ReadSampleSizeHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}
		else if(dwAtomType == ATOM_TYPE_STCO){

			IF_FAILED_RETURN(hr = ReadChunckOffsetHeader(pData + ATOM_MIN_SIZE_HEADER, dwAtomSize - ATOM_MIN_SIZE_HEADER));
		}
		else if(dwAtomType == ATOM_TYPE_SDTP){

			// todo
			//IF_FAILED_RETURN(hr = E_FAIL);
		}

		dwByteDone += dwAtomSize;
		pData += dwAtomSize;
	}

	return hr;
}

HRESULT CH264AtomParser::ReadSampleDescHeader(BYTE* pData, const DWORD dwAtomSampleDescSize){

	HRESULT hr;

	DWORD dwAtomSize;
	DWORD dwAtomType;
	DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(hr = (dwByteDone < dwAtomSampleDescSize ? S_OK : E_FAIL));

	pData += 4;

	IF_FAILED_RETURN(hr = (MAKE_DWORD(pData) == 1 ? S_OK : E_FAIL));

	pData += 4;

	while(dwAtomSampleDescSize - dwByteDone >= ATOM_MIN_SIZE_HEADER){

		dwAtomSize = MAKE_DWORD(pData);
		dwAtomType = MAKE_DWORD(pData + 4);

		// Todo
		assert(dwAtomSize != 0 && dwAtomSize != 1);

		if(dwAtomType == ATOM_TYPE_AVC1){

			dwByteDone += 86;
			pData += 86;

			IF_FAILED_RETURN(hr = (dwAtomSampleDescSize - dwByteDone >= ATOM_MIN_SIZE_HEADER ? S_OK : E_FAIL));

			dwAtomSize = MAKE_DWORD(pData);
			dwAtomType = MAKE_DWORD(pData + 4);

			// Rarely, ATOM_TYPE_AVCC is not the first : we should loop to find ATOM_TYPE_AVCC
			/*if(dwAtomType != ATOM_TYPE_AVCC){

				dwByteDone += dwAtomSize;
				pData += dwAtomSize;
				IF_FAILED_RETURN(hr = (dwByteDone <= dwAtomSampleDescSize ? S_OK : E_FAIL));

				dwAtomSize = MAKE_DWORD(pData);
				dwAtomType = MAKE_DWORD(pData + 4);
			}*/

			IF_FAILED_RETURN(hr = (dwAtomType == ATOM_TYPE_AVCC ? S_OK : E_FAIL));

			dwByteDone += dwAtomSize;

			IF_FAILED_RETURN(hr = (dwByteDone <= dwAtomSampleDescSize ? S_OK : E_FAIL));

			// we must check pData does not exceed dwAtomSampleDescSize
			pData += 8;

			IF_FAILED_RETURN(hr = ParseConfigDescriptor(pData, dwAtomSize - 8));
		}
		else{

			hr = E_FAIL;
		}

		break;
	}

	return hr;
}

HRESULT CH264AtomParser::ReadSampleTimeHeader(BYTE* pData, const DWORD dwAtomSampleTimeSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(hr = (dwByteDone < dwAtomSampleTimeSize ? S_OK : E_FAIL));

	pData += 4;
	const DWORD dwTimeSamples = MAKE_DWORD(pData);

	IF_FAILED_RETURN(hr = ((dwTimeSamples > 0) && ((dwTimeSamples * 8) == (dwAtomSampleTimeSize - dwByteDone)) ? S_OK : E_FAIL));

	for(DWORD dwCount = 0; dwCount < dwTimeSamples; dwCount++){

		/*
		pData += 4;
		// Sample Count
		pData += 4;
		// Sample Duration
		*/

		pData += 8;
	}

	return hr;
}

HRESULT CH264AtomParser::ReadSyncSampleHeader(BYTE* pData, const DWORD dwAtomSyncSampleSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(hr = (dwByteDone < dwAtomSyncSampleSize ? S_OK : E_FAIL));

	pData += 4;
	const DWORD dwSyncSamples = MAKE_DWORD(pData);

	IF_FAILED_RETURN(hr = ((dwSyncSamples > 0) && ((dwSyncSamples * 4) == (dwAtomSyncSampleSize - dwByteDone)) ? S_OK : E_FAIL));

	m_vSyncSamples.reserve(dwSyncSamples);

	for(DWORD dwCount = 0; dwCount < dwSyncSamples; dwCount++){

		pData += 4;

		m_vSyncSamples.push_back(MAKE_DWORD(pData));
	}

	return hr;
}

HRESULT CH264AtomParser::ReadCompositionOffsetHeader(BYTE* pData, const DWORD dwAtomCompositionOffsetSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(hr = (dwByteDone < dwAtomCompositionOffsetSize ? S_OK : E_FAIL));

	pData += 4;
	const DWORD dwCompositionOffsetCount = MAKE_DWORD(pData);

	IF_FAILED_RETURN(hr = ((dwCompositionOffsetCount > 0) && ((dwCompositionOffsetCount * 8) == (dwAtomCompositionOffsetSize - dwByteDone)) ? S_OK : E_FAIL));

	m_vCompositionOffset.reserve(dwCompositionOffsetCount);

	for(DWORD dwCount = 0; dwCount < dwCompositionOffsetCount; dwCount++){

		COMPOSITION_INFO sCompoInfo = {0};

		pData += 4;
		sCompoInfo.dwCount = MAKE_DWORD(pData);
		pData += 4;
		sCompoInfo.dwOffset = MAKE_DWORD(pData);

		m_vCompositionOffset.push_back(sCompoInfo);
	}

	return hr;
}

HRESULT CH264AtomParser::ReadSampleChunckHeader(BYTE* pData, const DWORD dwAtomSampleChunckSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(hr = (dwByteDone < dwAtomSampleChunckSize ? S_OK : E_FAIL));

	pData += 4;
	const DWORD dwSampleChunckSize = MAKE_DWORD(pData);

	IF_FAILED_RETURN(hr = ((dwSampleChunckSize > 0) && ((dwSampleChunckSize * 12) == (dwAtomSampleChunckSize - dwByteDone)) ? S_OK : E_FAIL));

	for(DWORD dwCount = 0; dwCount < dwSampleChunckSize; dwCount++){

		CHUNCK_INFO sChunckInfo = {0};

		pData += 4;
		sChunckInfo.dwFirstChunk = MAKE_DWORD(pData);

		pData += 4;
		sChunckInfo.dwSamplesPerChunk = MAKE_DWORD(pData);

		pData += 4;
		sChunckInfo.dwSampleDescId = MAKE_DWORD(pData);

		m_vChunks.push_back(sChunckInfo);
	}

	return hr;
}

HRESULT CH264AtomParser::ReadSampleSizeHeader(BYTE* pData, const DWORD dwAtomSampleSize){

	HRESULT hr;

	DWORD dwSampleSize;
	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER + 4;

	IF_FAILED_RETURN(hr = (dwByteDone < dwAtomSampleSize ? S_OK : E_FAIL));

	// Skip version + flags
	pData += 4;
	dwSampleSize = MAKE_DWORD(pData);

	// Todo : if not 0, all samples same size
	IF_FAILED_RETURN(hr = (dwSampleSize == 0 ? S_OK : E_FAIL));

	pData += 4;
	dwSampleSize = MAKE_DWORD(pData);

	IF_FAILED_RETURN(hr = ((dwSampleSize > 0) && ((dwSampleSize * 4) == (dwAtomSampleSize - dwByteDone)) ? S_OK : E_FAIL));

	pData += 4;
	m_vSamples.reserve(dwSampleSize);

	for(DWORD dwCount = 0; dwCount < dwSampleSize; dwCount++){

		SAMPLE_INFO sSampleInfo = {0};
		sSampleInfo.dwSize = MAKE_DWORD(pData);

		m_vSamples.push_back(sSampleInfo);
		pData += 4;
	}

	return hr;
}

HRESULT CH264AtomParser::ReadChunckOffsetHeader(BYTE* pData, const DWORD dwAtomChunckOffsetSize){

	HRESULT hr;

	const DWORD dwByteDone = ATOM_MIN_SIZE_HEADER;

	IF_FAILED_RETURN(hr = (dwByteDone < dwAtomChunckOffsetSize ? S_OK : E_FAIL));

	pData += 4;

	const DWORD dwChuncks = MAKE_DWORD(pData);

	IF_FAILED_RETURN(hr = ((dwChuncks * 4) == (dwAtomChunckOffsetSize - dwByteDone) ? S_OK : E_FAIL));

	m_vChunkOffset.reserve(dwChuncks);

	for(DWORD dwCount = 0; dwCount < dwChuncks; dwCount++){

		pData += 4;
		m_vChunkOffset.push_back(MAKE_DWORD(pData));
	}

	m_bHasVideoOffset = TRUE;

	return hr;
}

HRESULT CH264AtomParser::ParseConfigDescriptor(BYTE* pData, const DWORD /*dwSize*/){

	HRESULT hr;
	BYTE m_btNaluSize[4] = {0x00, 0x00, 0x00, 0x00};

	IF_FAILED_RETURN(hr = (m_pSyncHeader.GetBufferSize() == 0 ? S_OK : E_FAIL));

	pData += 4;

	// we must check pData does not exceed dwSize
	m_iNaluLenghtSize = (*pData++ & 0x03) + 1;

	IF_FAILED_RETURN(hr = ((m_iNaluLenghtSize != 1 && m_iNaluLenghtSize != 2 && m_iNaluLenghtSize != 4) ? E_FAIL : S_OK));

	IF_FAILED_RETURN(hr = m_pSyncHeader.Initialize(m_iNaluLenghtSize));
	memcpy(m_pSyncHeader.GetBuffer(), m_btNaluSize, m_iNaluLenghtSize);

	int iCurrentPos = m_iNaluLenghtSize;

	WORD wParameterSets = *pData++ & 0x1F;
	WORD wParameterSetLength;

	for(int i = 0; i < wParameterSets; i++){

		wParameterSetLength = ((*pData++) << 8);
		wParameterSetLength += *pData++;

		IF_FAILED_RETURN(hr = m_pSyncHeader.Reserve(wParameterSetLength));
		memcpy(m_pSyncHeader.GetBuffer() + iCurrentPos, pData, wParameterSetLength);

		pData += wParameterSetLength;
		iCurrentPos += wParameterSetLength;
	}

	DWORD dwLenght = iCurrentPos - m_iNaluLenghtSize;

	if(m_iNaluLenghtSize == 4){

		m_pSyncHeader.GetBuffer()[0] = (dwLenght >> 24) & 0xff;
		m_pSyncHeader.GetBuffer()[1] = (dwLenght >> 16) & 0xff;
		m_pSyncHeader.GetBuffer()[2] = (dwLenght >> 8) & 0xff;
		m_pSyncHeader.GetBuffer()[3] = dwLenght & 0xff;
	}
	else if(m_iNaluLenghtSize == 2){

		m_pSyncHeader.GetBuffer()[0] = (dwLenght >> 8) & 0xff;
		m_pSyncHeader.GetBuffer()[1] = dwLenght & 0xff;
	}
	else{

		// todo : size 1
		assert(FALSE);
	}

	IF_FAILED_RETURN(hr = m_pSyncHeader.Reserve(m_iNaluLenghtSize));
	memcpy(m_pSyncHeader.GetBuffer() + iCurrentPos, m_btNaluSize, m_iNaluLenghtSize);
	dwLenght = iCurrentPos;
	iCurrentPos += m_iNaluLenghtSize;
	wParameterSets = *pData++;

	for(int i = 0; i < wParameterSets; i++){

		wParameterSetLength = ((*pData++) << 8);
		wParameterSetLength += *pData++;

		IF_FAILED_RETURN(hr = m_pSyncHeader.Reserve(wParameterSetLength));
		memcpy(m_pSyncHeader.GetBuffer() + iCurrentPos, pData, wParameterSetLength);

		pData += wParameterSetLength;
		iCurrentPos += wParameterSetLength;
	}

	DWORD dwLastLenght = iCurrentPos - dwLenght - m_iNaluLenghtSize;

	if(m_iNaluLenghtSize == 4){

		m_pSyncHeader.GetBuffer()[dwLenght] = (dwLastLenght >> 24) & 0xff;
		m_pSyncHeader.GetBuffer()[dwLenght + 1] = (dwLastLenght >> 16) & 0xff;
		m_pSyncHeader.GetBuffer()[dwLenght + 2] = (dwLastLenght >> 8) & 0xff;
		m_pSyncHeader.GetBuffer()[dwLenght + 3] = dwLastLenght & 0xff;
	}
	else if(m_iNaluLenghtSize == 2){

		m_pSyncHeader.GetBuffer()[dwLenght] = (dwLastLenght >> 8) & 0xff;
		m_pSyncHeader.GetBuffer()[dwLenght + 1] = dwLastLenght & 0xff;
	}
	else{

		// todo : size 1
		assert(FALSE);
	}

	return hr;
}

HRESULT CH264AtomParser::GetVideoFrameRate(UINT* puiNumerator, UINT* puiDenominator){

	HRESULT hr;

	// FPS = (SampleCount * TimeScale) / Duration
	// Time = Duration / TimeScale
	// Time per Frame (100-nanosecond units) = (Time * 10000000) / SampleCount

	IF_FAILED_RETURN(hr = (m_dwDuration == 0 || m_dwTimeScale == 0 || m_vSamples.size() == 0 ? E_FAIL : S_OK));

	//double dFPS = (m_vSamples.size() * m_dwTimeScale) / (double)m_dwDuration;
	double dTime = (m_dwDuration / (double)m_dwTimeScale) * 10000000;
	dTime /= m_vSamples.size();

	// Frames per second(floating point)	Frames per second(fractional)	Average time per frame
	// 59.94								60000 / 1001					166833
	// 29.97								30000 / 1001					333667
	// 23.976								24000 / 1001					417188
	// 60									60 / 1							166667
	// 30									30 / 1							333333
	// 50									50 / 1							200000
	// 25									25 / 1							400000
	// 24									24 / 1							416667

	IF_FAILED_RETURN(hr = MFAverageTimePerFrameToFrameRate((UINT64)dTime, puiNumerator, puiDenominator));

	return hr;
}