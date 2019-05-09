//----------------------------------------------------------------------------------------------
// Dxva2Definition.h
//----------------------------------------------------------------------------------------------
#ifndef DXVA2DEFINITION_H
#define DXVA2DEFINITION_H

#define ZERO_MOVIE_TIME_STRING L"00h00mn00s000ms"

struct SAMPLE_PRESENTATION{

	DWORD dwDXVA2Index;
	LONGLONG llTime;
};

#define FRAME_COUNT_TO_PREROLL	5

struct DXVAHD_FILTER_RANGE_DATA_EX : public DXVAHD_FILTER_RANGE_DATA{

	BOOL bEnable;
	BOOL bAvailable;
	INT iCurrent;
};

struct DXVAHD_BLT_STATE_ALPHA_FILL_DATA_EX : public DXVAHD_BLT_STATE_ALPHA_FILL_DATA{

	BOOL bAvailable;
};

struct DXVAHD_BLT_STATE_CONSTRICTION_DATA_EX : public DXVAHD_BLT_STATE_CONSTRICTION_DATA{

	BOOL bAvailable;
	SIZE CurrentSize;
};

struct DXVAHD_STREAM_STATE_LUMA_KEY_DATA_EX : public DXVAHD_STREAM_STATE_LUMA_KEY_DATA{

	BOOL bAvailable;
	FLOAT CurrentLower;
	FLOAT CurrentUpper;
};

struct DXVAHD_STREAM_STATE_ALPHA_DATA_EX : public DXVAHD_STREAM_STATE_ALPHA_DATA{

	BOOL bAvailable;
	FLOAT fCurrent;
};

#endif