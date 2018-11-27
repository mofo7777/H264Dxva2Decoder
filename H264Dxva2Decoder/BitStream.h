//----------------------------------------------------------------------------------------------
// BitStream.h
//----------------------------------------------------------------------------------------------
#ifndef BITSTREAM_H
#define BITSTREAM_H

#define BITSTREAM_TOO_MANY_BITS  _HRESULT_TYPEDEF_(0xA0000001L)
#define BITSTREAM_PAST_END       _HRESULT_TYPEDEF_(0xA0000002L)
#define BITSTREAM_NON_ZERO       _HRESULT_TYPEDEF_(0xA0000003L)

class CBitStream{

public:

	CBitStream(){}
	~CBitStream(){}

	void Init(const BYTE*, const DWORD);
	DWORD GetBits(const DWORD);
	void CheckZeroStream(const int);
	DWORD UGolomb();
	DWORD SGolomb();
	int BitsRemain(){ return m_chDecBufferSize + m_uNumOfBitsInBuffer; }
	DWORD PeekBits(const DWORD);
	void bookmark(const int);

private:

	DWORD m_uNumOfBitsInBuffer;
	DWORD m_chDecBufferSize;
	const BYTE* m_chDecBuffer;
	BYTE m_chDecData;

	int m_bBookmarkOn;
	DWORD m_uNumOfBitsInBuffer_bookmark;
	const BYTE* m_chDecBuffer_bookmark;
	DWORD m_chDecBufferSize_bookmark;
	BYTE m_chDecData_bookmark;
};

#endif