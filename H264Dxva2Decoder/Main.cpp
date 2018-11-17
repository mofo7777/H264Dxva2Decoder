//----------------------------------------------------------------------------------------------
// Main.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

// You can download mp4 files for free (Creative Commons CC0) here : https://pixabay.com/
// From Life-Of-Vids : https://pixabay.com/fr/videos/plage-sable-ondes-roches-nature-10890/
#define H264_INPUT_FILE L"Beach - 10890.mp4"

HRESULT ProcessDecode();
HRESULT AddByteAndConvertAvccToAnnexB(CMFBuffer&);

void main(){

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if(SUCCEEDED(hr)){

		hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);

		if(SUCCEEDED(hr)){

			hr = ProcessDecode();

			LOG_HRESULT(MFShutdown());
		}

		CoUninitialize();
	}
}

HRESULT ProcessDecode(){

	HRESULT hr = S_OK;

	CH264AtomParser* pH264AtomParser = NULL;
	CDxva2Decoder* pDxva2Decoder = NULL;
	CH264NaluParser cH264NaluParser;
	CMFBuffer pVideoBuffer;
	CMFBuffer pNalUnitBuffer;
	BYTE* pVideoData = NULL;
	DWORD dwBufferSize;
	DWORD dwTotalSample = 0;
	DWORD dwTotalPicture = 0;
	int iNaluLenghtSize;
	DWORD dwMaxSampleBufferSize = 0;
	DWORD dwTrackId = 0;
	BYTE btStartCode[4] = {0x00, 0x00, 0x00, 0x01};

	try{

		pH264AtomParser = new (std::nothrow)CH264AtomParser;

		IF_FAILED_THROW(hr = (pH264AtomParser == NULL ? E_OUTOFMEMORY : S_OK));

		pDxva2Decoder = new (std::nothrow)CDxva2Decoder;

		IF_FAILED_THROW(hr = (pDxva2Decoder == NULL ? E_OUTOFMEMORY : S_OK));

		IF_FAILED_THROW(hr = pH264AtomParser->Initialize(H264_INPUT_FILE));
		IF_FAILED_THROW(hr = pH264AtomParser->ParseMp4());
		IF_FAILED_THROW(hr = pH264AtomParser->GetFirstVideoStream(&dwTrackId));

		iNaluLenghtSize = pH264AtomParser->GetNaluLenghtSize();
		cH264NaluParser.SetNaluLenghtSize(iNaluLenghtSize);

		IF_FAILED_THROW(hr = pH264AtomParser->GetVideoConfigDescriptor(dwTrackId, &pVideoData, &dwBufferSize));
		IF_FAILED_THROW(hr = cH264NaluParser.ParseVideoConfigDescriptor(pVideoData, dwBufferSize));

		DXVA2_Frequency Dxva2Freq;
		IF_FAILED_THROW(hr = pH264AtomParser->GetVideoFrameRate(dwTrackId, &Dxva2Freq.Numerator, &Dxva2Freq.Denominator));
		IF_FAILED_THROW(hr = pDxva2Decoder->InitDXVA2(cH264NaluParser.GetSPS(), cH264NaluParser.GetWidth(), cH264NaluParser.GetHeight(), Dxva2Freq.Numerator, Dxva2Freq.Denominator));

		IF_FAILED_THROW(hr = pVideoBuffer.Initialize(H264_BUFFER_SIZE));
		IF_FAILED_THROW(hr = pNalUnitBuffer.Initialize(H264_BUFFER_SIZE));

		while(pH264AtomParser->GetNextSample(dwTrackId, &pVideoData, &dwBufferSize) == S_OK){

			if(dwMaxSampleBufferSize < dwBufferSize)
				dwMaxSampleBufferSize = dwBufferSize;

			if(g_bStopApplication){
				break;
			}

			IF_FAILED_THROW(hr = pVideoBuffer.Reserve(dwBufferSize));
			memcpy(pVideoBuffer.GetReadStartBuffer(), pVideoData, dwBufferSize);
			IF_FAILED_THROW(hr = pVideoBuffer.SetEndPosition(dwBufferSize));

			dwTotalSample++;

			do{

				pNalUnitBuffer.Reset();
				IF_FAILED_THROW(hr = pNalUnitBuffer.Reserve(pVideoBuffer.GetBufferSize()));
				memcpy(pNalUnitBuffer.GetStartBuffer(), pVideoBuffer.GetStartBuffer(), pVideoBuffer.GetBufferSize());
				IF_FAILED_THROW(hr = pNalUnitBuffer.SetEndPosition(pVideoBuffer.GetBufferSize()));

				IF_FAILED_THROW(hr = cH264NaluParser.ParseNaluHeader(pVideoBuffer));

				if(cH264NaluParser.IsNalUnitCodedSlice()){

					dwTotalPicture++;

					// DXVA2 needs start code
					if(iNaluLenghtSize == 4){
						memcpy(pNalUnitBuffer.GetStartBuffer(), btStartCode, 4);
					}
					else{
						IF_FAILED_THROW(hr = AddByteAndConvertAvccToAnnexB(pNalUnitBuffer));
					}

					IF_FAILED_THROW(hr = pDxva2Decoder->DecodeFrame(pNalUnitBuffer, cH264NaluParser.GetPicture()));
					IF_FAILED_THROW(hr = pDxva2Decoder->DisplayFrame());
				}

				if(pVideoBuffer.GetBufferSize() != 0){

					hr = S_FALSE;
				}
			}
			while(hr == S_FALSE);
		}
	}
	catch(HRESULT){}

	if(pDxva2Decoder){

		// Display last pictures if any
		DWORD dwPictureToDisplay = pDxva2Decoder->PictureToDisplayCount();

		while(SUCCEEDED(hr) && dwPictureToDisplay){

			dwPictureToDisplay--;
			dwTotalPicture++;

			LOG_HRESULT(hr = pDxva2Decoder->DisplayFrame());
		}
	}

	// if(pVideoBuffer.GetAllocatedSize == H264_BUFFER_SIZE and pNalUnitBuffer.GetBufferSize == H264_BUFFER_SIZE), no memory allocation, but perhaps, we reserved too much memory
	// H264_BUFFER_SIZE : value arbitrary choosen after testing few files
	TRACE((L"\r\nTotal sample = %lu - Total picture = %lu - Total video buffer Size = %lu (%lu) - Total nalunit buffer Size = %lu (%lu) - Max sample size = %lu\r\n",
		dwTotalSample, dwTotalPicture, pVideoBuffer.GetAllocatedSize(), H264_BUFFER_SIZE, pNalUnitBuffer.GetAllocatedSize(), H264_BUFFER_SIZE, dwMaxSampleBufferSize));

	pVideoBuffer.Delete();
	pNalUnitBuffer.Delete();
	SAFE_DELETE(pDxva2Decoder);
	SAFE_DELETE(pH264AtomParser);

	LOG_HRESULT(MFShutdown());
	CoUninitialize();

	return hr;
}

HRESULT AddByteAndConvertAvccToAnnexB(CMFBuffer& pNalUnitBuffer){

	HRESULT hr;
	BYTE btStartCode[3] = {0x00, 0x00, 0x01};

	IF_FAILED_RETURN(hr = pNalUnitBuffer.Reserve(1));
	memcpy(pNalUnitBuffer.GetStartBuffer() + 1, pNalUnitBuffer.GetStartBuffer(), pNalUnitBuffer.GetBufferSize());
	memcpy(pNalUnitBuffer.GetStartBuffer(), btStartCode, 3);
	IF_FAILED_RETURN(hr = pNalUnitBuffer.SetEndPosition(1));

	return hr;
}