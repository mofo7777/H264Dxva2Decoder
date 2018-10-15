//----------------------------------------------------------------------------------------------
// Golomb.h
//----------------------------------------------------------------------------------------------
#ifndef GOLOMB_H
#define GOLOMB_H

class CGolomb{

public:

	CGolomb(){ ZeroMemory(&m_sBitContext, sizeof(SBitContext)); }
	~CGolomb(){}

	void InitBitContext(BYTE*, const int);
	void AdvanceBits(const int);
	DWORD DecodeUGolomb(DWORD, DWORD*);
	DWORD GetBufferData(){ return m_sBitContext.cache; }

private:

	struct SBitContext{

		const unsigned char* buffer;
		const unsigned char* buffer_end;
		unsigned char* buffer_ptr;
		unsigned int cache;
		int bit_count;
		int size_in_bits;
	};

	SBitContext m_sBitContext;
};

#endif