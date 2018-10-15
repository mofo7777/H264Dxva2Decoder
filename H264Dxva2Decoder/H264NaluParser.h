//----------------------------------------------------------------------------------------------
// H264NaluParser.h
//----------------------------------------------------------------------------------------------
#ifndef H264NALUPARSER_H
#define H264NALUPARSER_H

class CH264NaluParser{

public:

	CH264NaluParser();
	~CH264NaluParser(){}

	HRESULT ParseVideoConfigDescriptor(BYTE*, const DWORD);
	HRESULT ParseNaluHeader(CMFBuffer&);

	const BOOL HasSPS() const{ return m_bHasSPS; }
	const BOOL HasPPS() const{ return m_bHasPPS; }
	const BOOL IsNalUnitCodedSlice() const{ return (m_Picture.NalUnitType == NAL_UNIT_CODED_SLICE || m_Picture.NalUnitType == NAL_UNIT_CODED_SLICE_IDR); }
	const PICTURE_INFO& GetPicture() const { return m_Picture; }
	void ResetPPS(){ m_bHasPPS = FALSE; }
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

	void RemoveEmulationPreventionByte(CMFBuffer&, const DWORD);
	HRESULT ParseSPS();
	HRESULT ParsePPS();
	HRESULT ParseCodedSlice();
	void HandlePOC();
	HRESULT ParseVuiParameters(SPS_DATA*);
	HRESULT ParsePredWeightTable();
	HRESULT ParseDecRefPicMarking();
	void hrd_parameters();
	void CheckEmulationByte();
};

#endif