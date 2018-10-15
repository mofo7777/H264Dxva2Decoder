//----------------------------------------------------------------------------------------------
// H264AtomParser.h
//----------------------------------------------------------------------------------------------
#ifndef H264ATOMPARSER_H
#define H264ATOMPARSER_H

class CH264AtomParser{

public:

	CH264AtomParser();
	~CH264AtomParser(){ Delete(); }

	HRESULT Initialize(LPCWSTR);
	void Delete();
	HRESULT GetNextSample(BYTE**, DWORD*);
	HRESULT GetVideoConfigDescriptor(BYTE**, DWORD*);
	HRESULT ParseMp4();
	HRESULT GetVideoFrameRate(UINT*, UINT*);
	BOOL IsEndOfStream() const{ return m_bEOS; }
	const int GetNaluLenghtSize() const{ return m_iNaluLenghtSize; }

private:

	CMFByteStream* m_pByteStream;
	CMFBuffer m_cMp4ParserBuffer;
	BOOL m_bEOS;

	struct ATOM_HEADER{

		LARGE_INTEGER liFtypFilePos;
		LARGE_INTEGER liMoovFilePos;
		LARGE_INTEGER liMdatFilePos;
		DWORD dwMoovSize;
		BOOL bFTYP;
		BOOL bMOOV;
		BOOL bMDAT;
	};

	struct TRACK_HEADER{

		DWORD dwTrackId;
		DWORD dwFlags;
		DWORD dwDuration;
		DWORD dwWidth;
		DWORD dwHeight;
	};

	struct CHUNCK_INFO{

		DWORD dwFirstChunk;
		DWORD dwSamplesPerChunk;
		DWORD dwSampleDescId;
	};

	struct SAMPLE_INFO{

		DWORD dwSize;
		DWORD dwOffset;
		BOOL bKeyFrame;
	};

	struct COMPOSITION_INFO{

		DWORD dwCount;
		DWORD dwOffset;
	};

	ATOM_HEADER m_sAtomHeader;
	TRACK_HEADER m_sTrackHeader[2];
	BOOL m_bHasVideoOffset;
	vector<CHUNCK_INFO> m_vChunks;
	vector<SAMPLE_INFO> m_vSamples;
	vector<DWORD> m_vSyncSamples;
	vector<DWORD> m_vChunkOffset;
	vector<COMPOSITION_INFO> m_vCompositionOffset;
	DWORD m_dwCurrentSample;
	CMFLightBuffer m_pSyncHeader;
	DWORD m_dwTimeScale;
	DWORD m_dwDuration;
	int m_DisplayMatrix[3][3];
	int m_iNaluLenghtSize;

	HRESULT FindAtoms();
	HRESULT ReadMoov();
	HRESULT FinalizeSample();
	HRESULT ReadTrack(BYTE*, const DWORD);
	HRESULT ReadMovieHeader(BYTE*, const DWORD);
	HRESULT ReadTrackHeader(BYTE*, const DWORD);
	HRESULT ReadEditAtoms(BYTE*, const DWORD);
	HRESULT ReadMediaAtom(BYTE*, const DWORD);
	HRESULT ReadVideoMediaInfoHeader(BYTE*, const DWORD);
	HRESULT ReadSampleTableHeader(BYTE*, const DWORD);
	HRESULT ReadSampleDescHeader(BYTE*, const DWORD);
	HRESULT ReadSampleTimeHeader(BYTE*, const DWORD);
	HRESULT ReadSyncSampleHeader(BYTE*, const DWORD);
	HRESULT ReadCompositionOffsetHeader(BYTE*, const DWORD);
	HRESULT ReadSampleChunckHeader(BYTE*, const DWORD);
	HRESULT ReadSampleSizeHeader(BYTE*, const DWORD);
	HRESULT ReadChunckOffsetHeader(BYTE*, const DWORD);
	HRESULT ParseConfigDescriptor(BYTE* pData, const DWORD dwSize);
};

#endif