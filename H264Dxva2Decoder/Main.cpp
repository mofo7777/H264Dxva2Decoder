//----------------------------------------------------------------------------------------------
// Main.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

// You can download mp4 files for free (Creative Commons CC0) here : https://pixabay.com/
// From Life-Of-Vids : https://pixabay.com/fr/videos/plage-sable-ondes-roches-nature-10890/
#define H264_INPUT_FILE L"Beach - 10890.mp4"

HRESULT ProcessDecode();

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
	DWORD dwNalBufferSize;
	DWORD dwTotalSample = 0;
	DWORD dwTotalPicture = 0;
	int iNaluLenghtSize;
	DWORD dwMaxSampleBufferSize = 0;
	BYTE btStartCode[3] = {0x00, 0x00, 0x01};

	try{

		pH264AtomParser = new (std::nothrow)CH264AtomParser;

		IF_FAILED_THROW(hr = (pH264AtomParser == NULL ? E_OUTOFMEMORY : S_OK));

		pDxva2Decoder = new (std::nothrow)CDxva2Decoder;

		IF_FAILED_THROW(hr = (pDxva2Decoder == NULL ? E_OUTOFMEMORY : S_OK));

		IF_FAILED_THROW(hr = pH264AtomParser->Initialize(H264_INPUT_FILE));
		IF_FAILED_THROW(hr = pH264AtomParser->ParseMp4());

		IF_FAILED_THROW(hr = pVideoBuffer.Initialize(H264_BUFFER_SIZE));
		IF_FAILED_THROW(hr = pNalUnitBuffer.Initialize(H264_BUFFER_SIZE));

		IF_FAILED_THROW(hr = pH264AtomParser->GetVideoConfigDescriptor(&pVideoData, &dwBufferSize));

		iNaluLenghtSize = pH264AtomParser->GetNaluLenghtSize();
		cH264NaluParser.SetNaluLenghtSize(iNaluLenghtSize);

		IF_FAILED_THROW(hr = cH264NaluParser.ParseVideoConfigDescriptor(pVideoData, dwBufferSize));

		DXVA2_Frequency Dxva2Freq;
		IF_FAILED_THROW(hr = pH264AtomParser->GetVideoFrameRate(&Dxva2Freq.Numerator, &Dxva2Freq.Denominator));
		IF_FAILED_THROW(hr = pDxva2Decoder->InitDXVA2(cH264NaluParser.GetWidth(), cH264NaluParser.GetHeight(), Dxva2Freq.Numerator, Dxva2Freq.Denominator));

		while(pH264AtomParser->GetNextSample(&pVideoData, &dwBufferSize) == S_OK){

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

				// DXVA2 needs StartCode
				pNalUnitBuffer.Reset();
				IF_FAILED_THROW(hr = pNalUnitBuffer.Reserve(3));
				memcpy(pNalUnitBuffer.GetReadStartBuffer(), btStartCode, 3);
				IF_FAILED_THROW(hr = pNalUnitBuffer.SetEndPosition(3));

				dwNalBufferSize = pVideoBuffer.GetBufferSize() - iNaluLenghtSize;
				IF_FAILED_THROW(hr = pNalUnitBuffer.Reserve(dwNalBufferSize));
				memcpy(pNalUnitBuffer.GetReadStartBuffer(), pVideoBuffer.GetStartBuffer() + iNaluLenghtSize, dwNalBufferSize);
				IF_FAILED_THROW(hr = pNalUnitBuffer.SetEndPosition(dwNalBufferSize));

				IF_FAILED_THROW(hr = cH264NaluParser.ParseNaluHeader(pVideoBuffer));

				if(cH264NaluParser.IsNalUnitCodedSlice()){

					dwTotalPicture++;

					IF_FAILED_THROW(hr = pDxva2Decoder->DecodeFrame(pNalUnitBuffer, cH264NaluParser.GetPicture()));
					IF_FAILED_THROW(hr = pDxva2Decoder->DisplayFrame());
				}

				if(pVideoBuffer.GetBufferSize() != 0){

					hr = S_FALSE;
				}
			} while(hr == S_FALSE);
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
		dwTotalSample, dwTotalPicture, pVideoBuffer.GetBufferSize(), H264_BUFFER_SIZE, pNalUnitBuffer.GetBufferSize(), H264_BUFFER_SIZE, dwMaxSampleBufferSize));

	pVideoBuffer.Delete();
	pNalUnitBuffer.Delete();
	SAFE_DELETE(pDxva2Decoder);
	SAFE_DELETE(pH264AtomParser);

	LOG_HRESULT(MFShutdown());
	CoUninitialize();

	return hr;
}