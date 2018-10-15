//----------------------------------------------------------------------------------------------
// Golomb.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

#define SWAP16(x) (((x) & 0x00ff) << 8 | ((x) & 0xff00) >> 8)

void CGolomb::InitBitContext(BYTE* buffer, const int bit_size){

	const int buffer_size = (bit_size + 7) >> 3;

	m_sBitContext.buffer = buffer;
	m_sBitContext.size_in_bits = bit_size;
	m_sBitContext.buffer_end = buffer + buffer_size;
	m_sBitContext.buffer_ptr = buffer;
	m_sBitContext.bit_count = 16;
	m_sBitContext.cache = 0;

	int re_bit_count = m_sBitContext.bit_count;
	int re_cache = m_sBitContext.cache;
	unsigned char* re_buffer_ptr = m_sBitContext.buffer_ptr;

	if(re_bit_count >= 0){

		re_cache += (int)SWAP16(*(unsigned short*)re_buffer_ptr) << re_bit_count;
		re_buffer_ptr += 2;
		re_bit_count -= 16;
	}

	if(re_bit_count >= 0){

		re_cache += (int)SWAP16(*(unsigned short*)re_buffer_ptr) << re_bit_count;
		re_buffer_ptr += 2;
		re_bit_count -= 16;
	}

	m_sBitContext.bit_count = re_bit_count;
	m_sBitContext.cache = re_cache;
	m_sBitContext.buffer_ptr = re_buffer_ptr;
}

void CGolomb::AdvanceBits(const int n){

	int re_bit_count = m_sBitContext.bit_count;
	int re_cache = m_sBitContext.cache;
	unsigned char* re_buffer_ptr = m_sBitContext.buffer_ptr;

	if(re_bit_count >= 0){

		re_cache += (int)SWAP16(*(unsigned short*)re_buffer_ptr) << re_bit_count;
		re_buffer_ptr += 2;
		re_bit_count -= 16;
	}

	re_cache <<= (n);
	re_bit_count += (n);
	m_sBitContext.bit_count = re_bit_count;
	m_sBitContext.cache = re_cache;
	m_sBitContext.buffer_ptr = re_buffer_ptr;
}

DWORD CGolomb::DecodeUGolomb(DWORD dwData, DWORD* dwCount){

	DWORD dwResult = 0;
	BYTE bZeroCount = 0;
	BYTE bBitCount = 0;

	while(!(dwData & 0x1)){

		bZeroCount++;

		if(bZeroCount > 8){

			TRACE((L"DecodeUGolomb : bZeroCount > 8 (TODO)"));
		}

		dwData >>= 1;
	}

	bBitCount = bZeroCount + 1;

	for(BYTE i = 0; i < bBitCount; i++){

		dwResult <<= 1;
		dwResult |= dwData & 0x1;
		dwData >>= 1;
	}

	dwResult -= 1;

	if(dwCount){
		*dwCount = bBitCount + bZeroCount;
	}

	return dwResult;
}