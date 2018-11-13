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
	HRESULT ParseMp4();
	void Delete();
	HRESULT GetNextSample(const DWORD, BYTE**, DWORD*);
	HRESULT GetVideoConfigDescriptor(const DWORD, BYTE**, DWORD*);
	HRESULT GetVideoFrameRate(const DWORD, UINT*, UINT*);
	HRESULT GetFirstVideoStream(DWORD*);
	const int GetNaluLenghtSize() const{ return m_iNaluLenghtSize; }

private:

	struct SAMPLE_INFO{

		DWORD dwSize;
		DWORD dwOffset;
		BOOL bKeyFrame;
		LONGLONG llTime;
		LONGLONG llDuration;
	};

	struct ROOT_ATOM{

		BOOL bFTYP;
		BOOL bMOOV;
		BOOL bMDAT;
	};

	struct CHUNCK_INFO{

		DWORD dwFirstChunk;
		DWORD dwSamplesPerChunk;
		DWORD dwSampleDescId;
	};

	struct TIME_INFO{

		DWORD dwCount;
		DWORD dwOffset;
	};

	struct EDIT_INFO{

		DWORD dwSegmentDuration;
		DWORD dwMediaTime;
		DWORD dwMediaRate;
	};

	struct TRACK_INFO{

		DWORD dwTrackId;
		DWORD dwFlags;// todo : Track enabled/Track in movie/Track in preview/Track in poster
		DWORD dwTypeHandler;
		DWORD dwTimeScale;
		UINT64 ui64VideoDuration;
		DWORD dwWidth;
		DWORD dwHeight;
		vector<CHUNCK_INFO> vChunks;
		vector<SAMPLE_INFO> vSamples;
		vector<DWORD> vSyncSamples;
		vector<DWORD> vChunkOffset;
		vector<TIME_INFO> vTimeSample;
		vector<TIME_INFO> vCompositionTime;
		vector<EDIT_INFO> vEditList;
		CMFLightBuffer* pConfig;
	};

	CMFByteStream* m_pByteStream;
	CMFBuffer m_cMp4ParserBuffer;
	int m_DisplayMatrix[3][3];
	vector<TRACK_INFO> m_vTrackInfo;
	DWORD m_dwCurrentSample;
	DWORD m_dwTimeScale;
	UINT64 m_ui64VideoDuration;
	int m_iNaluLenghtSize;

	HRESULT FinalizeSampleTime(TRACK_INFO&);
	HRESULT FinalizeSampleOffset(TRACK_INFO&);
	HRESULT FinalizeSampleSync(TRACK_INFO&);
	HRESULT SeekAtom(LARGE_INTEGER&, const DWORD, const DWORD, const UINT64);
	HRESULT ParseAtoms(ROOT_ATOM&);
	HRESULT ParseMoov(const DWORD);
	HRESULT ParseTrack(TRACK_INFO&, BYTE*, const DWORD);
	HRESULT ParseMovieHeader(BYTE*, const DWORD);
	HRESULT ParseTrackHeader(TRACK_INFO&, BYTE*, const DWORD);
	HRESULT ParseEditAtoms(vector<EDIT_INFO>& vEditList, BYTE*, const DWORD);
	HRESULT ParseMediaAtom(TRACK_INFO&, BYTE*, const DWORD);
	HRESULT ParseMediaInfoHeader(TRACK_INFO&, BYTE*, const DWORD);
	HRESULT ParseMediaHeader(TRACK_INFO&, BYTE*, const DWORD);
	HRESULT ParseSampleTableHeader(TRACK_INFO&, BYTE*, const DWORD);
	HRESULT ParseSampleDescHeader(TRACK_INFO&, BYTE*, const DWORD);
	HRESULT ParseSampleTimeHeader(vector<TIME_INFO>&, BYTE*, const DWORD);
	HRESULT ParseSyncSampleHeader(vector<DWORD>&, BYTE*, const DWORD);
	HRESULT ParseCompositionOffsetHeader(vector<TIME_INFO>&, BYTE*, const DWORD);
	HRESULT ParseSampleChunckHeader(vector<CHUNCK_INFO>&, BYTE*, const DWORD);
	HRESULT ParseSampleSizeHeader(vector<SAMPLE_INFO>&, BYTE*, const DWORD);
	HRESULT ParseChunckOffsetHeader(vector<DWORD>&, BYTE*, const DWORD);
	HRESULT ParseChunckOffset64Header(vector<DWORD>&, BYTE*, const DWORD);

	HRESULT ParseAvc1Format(CMFLightBuffer**, const BYTE*, const DWORD);
	HRESULT ParseVideoConfigDescriptor(CMFLightBuffer**, const BYTE*, const DWORD);
	void RemoveEmulationPreventionByte(CMFLightBuffer*, int*, const DWORD);

	const vector<SAMPLE_INFO>* GetSamples(const DWORD) const;
	CMFLightBuffer* GetConfig(const DWORD) const;
};

#endif