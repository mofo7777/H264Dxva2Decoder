//----------------------------------------------------------------------------------------------
// BitStream.h
//----------------------------------------------------------------------------------------------
#ifndef BITSTREAM_H
#define BITSTREAM_H

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