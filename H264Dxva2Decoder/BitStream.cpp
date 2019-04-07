//----------------------------------------------------------------------------------------------
// BitStream.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

static const DWORD mask[33] = {

	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};

static BYTE exp_golomb_bits[256] = {

	8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
};

void CBitStream::Init(const BYTE* pBuffer, const DWORD dwBitLen){

	m_chDecBuffer = pBuffer;
	m_chDecBufferSize = dwBitLen;
	m_uNumOfBitsInBuffer = 0;
	m_chDecData = 0x00;

	m_bBookmarkOn = 0;
	m_uNumOfBitsInBuffer_bookmark = 0;
	m_chDecBuffer_bookmark = 0x00;
	m_chDecBufferSize_bookmark = 0;
	m_chDecData_bookmark = 0x00;
}

DWORD CBitStream::GetBits(const DWORD numBits){

	DWORD retData;
	HRESULT hr;

	if(numBits > 32){
		IF_FAILED_THROW(BITSTREAM_TOO_MANY_BITS);
	}

	if(numBits == 0){
		return 0;
	}

	if(m_uNumOfBitsInBuffer >= numBits){

		m_uNumOfBitsInBuffer -= numBits;
		retData = m_chDecData >> m_uNumOfBitsInBuffer;
		// wmay - this gets done below...retData &= mask[numBits];
	}
	else{

		DWORD nbits;

		nbits = numBits - m_uNumOfBitsInBuffer;

		if(nbits == 32)
			retData = 0;
		else
			retData = m_chDecData << nbits;

		switch((nbits - 1) / 8){

		case 3:

			nbits -= 8;

			if(m_chDecBufferSize < 8)
				IF_FAILED_THROW(BITSTREAM_PAST_END);

			retData |= *m_chDecBuffer++ << nbits;
			m_chDecBufferSize -= 8;
			// fall through

		case 2:

			nbits -= 8;

			if(m_chDecBufferSize < 8)
				IF_FAILED_THROW(BITSTREAM_PAST_END);

			retData |= *m_chDecBuffer++ << nbits;
			m_chDecBufferSize -= 8;

		case 1:

			nbits -= 8;

			if(m_chDecBufferSize < 8)
				IF_FAILED_THROW(BITSTREAM_PAST_END);

			retData |= *m_chDecBuffer++ << nbits;
			m_chDecBufferSize -= 8;

		case 0:
			break;
		}

		if(m_chDecBufferSize < nbits){
			IF_FAILED_THROW(BITSTREAM_PAST_END);
		}

		m_chDecData = *m_chDecBuffer++;
		m_uNumOfBitsInBuffer = MIN(8, m_chDecBufferSize) - nbits;
		m_chDecBufferSize -= MIN(8, m_chDecBufferSize);
		retData |= (m_chDecData >> m_uNumOfBitsInBuffer) & mask[nbits];
	}

	return (retData & mask[numBits]);
}

void CBitStream::CheckZeroStream(const int count){

	DWORD val;
	val = GetBits(count);
	HRESULT hr;

	if(val != 0){
		IF_FAILED_THROW(BITSTREAM_NON_ZERO);
	}
}

DWORD CBitStream::UGolomb(){

	DWORD bits;
	DWORD read = 0;
	int bits_left;
	BYTE coded;
	bool done = false;
	bits = 0;

	// we want to read 8 bits at a time - if we don't have 8 bits, read what's left, and shift.
	// The exp_golomb_bits calc remains the same.
	while(done == false){

		bits_left = BitsRemain();

		if(bits_left < 8){

			read = PeekBits(bits_left) << (8 - bits_left);
			done = true;
		}
		else{

			read = PeekBits(8);

			if(read == 0){
				GetBits(8);
				bits += 8;
			}
			else{
				done = true;
			}
		}
	}

	coded = exp_golomb_bits[read];
	GetBits(coded);
	bits += coded;

	return GetBits(bits + 1) - 1;
}

DWORD CBitStream::SGolomb(){

	DWORD ret;
	ret = UGolomb();

	if((ret & 0x1) == 0){

		ret >>= 1;
		DWORD temp = 0 - ret;
		return temp;
	}

	return (ret + 1) >> 1;
}

DWORD CBitStream::PeekBits(const DWORD bits){

	DWORD ret;
	bookmark(1);
	ret = GetBits(bits);
	bookmark(0);
	return ret;
}

void CBitStream::bookmark(const int bSet){

	if(bSet){

		m_uNumOfBitsInBuffer_bookmark = m_uNumOfBitsInBuffer;
		m_chDecBuffer_bookmark = m_chDecBuffer;
		m_chDecBufferSize_bookmark = m_chDecBufferSize;
		m_bBookmarkOn = 1;
		m_chDecData_bookmark = m_chDecData;
	}
	else{

		m_uNumOfBitsInBuffer = m_uNumOfBitsInBuffer_bookmark;
		m_chDecBuffer = m_chDecBuffer_bookmark;
		m_chDecBufferSize = m_chDecBufferSize_bookmark;
		m_chDecData = m_chDecData_bookmark;
		m_bBookmarkOn = 0;
	}
}