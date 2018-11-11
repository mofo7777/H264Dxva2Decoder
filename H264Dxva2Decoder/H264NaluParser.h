//----------------------------------------------------------------------------------------------
// H264NaluParser.h
//----------------------------------------------------------------------------------------------
#ifndef H264NALUPARSER_H
#define H264NALUPARSER_H

class CH264NaluParser{

public:

	CH264NaluParser();
	~CH264NaluParser(){}

	HRESULT ParseVideoConfigDescriptor(const BYTE*, const DWORD);
	HRESULT ParseNaluHeader(CMFBuffer&);

	const BOOL IsNalUnitCodedSlice() const{ return (m_Picture.NalUnitType == NAL_UNIT_CODED_SLICE || m_Picture.NalUnitType == NAL_UNIT_CODED_SLICE_IDR); }
	const PICTURE_INFO& GetPicture() const { return m_Picture; }
	const DWORD GetWidth() const{ return m_dwWidth; }
	const DWORD GetHeight() const{ return m_dwHeight; }
	void SetNaluLenghtSize(const int iNaluLenghtSize){ m_iNaluLenghtSize = iNaluLenghtSize; };

private:

	PICTURE_INFO m_Picture;
	CBitStream m_cBitStream;
	BOOL m_bHasSPS;
	BOOL m_bHasPPS;
	DWORD m_dwWidth;
	DWORD m_dwHeight;
	int m_iNaluLenghtSize;

	HRESULT ParseSPS();
	HRESULT ParsePPS();
	HRESULT ParseCodedSlice();
	void HandlePOC();
	HRESULT ParseVuiParameters(SPS_DATA*);
	HRESULT ParsePredWeightTable();
	HRESULT ParseDecRefPicMarking();
	void hrd_parameters();
};

#endif