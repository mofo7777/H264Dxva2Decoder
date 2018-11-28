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
	CMFBuffer VideoBuffer;
	CMFBuffer NalUnitBuffer;
	BYTE* pVideoData = NULL;
	DWORD dwBufferSize;
	DWORD dwTotalSample = 0;
	DWORD dwTotalPicture = 0;
	int iNaluLenghtSize;
	DWORD dwMaxSampleBufferSize = 0;
	DWORD dwTrackId = 0;
	LONGLONG llTime = 0;
	BYTE btStartCode[4] = {0x00, 0x00, 0x00, 0x01};
	int iSubSliceCount;
	DWORD dwParsed;

	const DWORD H264_BUFFER_SIZE = 262144;

	try{

		pH264AtomParser = new (std::nothrow)CH264AtomParser;

		IF_FAILED_THROW(pH264AtomParser == NULL ? E_OUTOFMEMORY : S_OK);

		pDxva2Decoder = new (std::nothrow)CDxva2Decoder;

		IF_FAILED_THROW(pDxva2Decoder == NULL ? E_OUTOFMEMORY : S_OK);

		IF_FAILED_THROW(pH264AtomParser->Initialize(H264_INPUT_FILE));
		IF_FAILED_THROW(pH264AtomParser->ParseMp4());
		IF_FAILED_THROW(pH264AtomParser->GetFirstVideoStream(&dwTrackId));

		iNaluLenghtSize = pH264AtomParser->GetNaluLenghtSize();
		cH264NaluParser.SetNaluLenghtSize(iNaluLenghtSize);

		IF_FAILED_THROW(pH264AtomParser->GetVideoConfigDescriptor(dwTrackId, &pVideoData, &dwBufferSize));
		IF_FAILED_THROW(cH264NaluParser.ParseVideoConfigDescriptor(pVideoData, dwBufferSize));

		DXVA2_Frequency Dxva2Freq;
		IF_FAILED_THROW(pH264AtomParser->GetVideoFrameRate(dwTrackId, &Dxva2Freq.Numerator, &Dxva2Freq.Denominator));
		IF_FAILED_THROW(pDxva2Decoder->InitDXVA2(cH264NaluParser.GetSPS(), cH264NaluParser.GetWidth(), cH264NaluParser.GetHeight(), Dxva2Freq.Numerator, Dxva2Freq.Denominator));

		IF_FAILED_THROW(VideoBuffer.Initialize(H264_BUFFER_SIZE));
		IF_FAILED_THROW(NalUnitBuffer.Initialize(H264_BUFFER_SIZE));

		while(pH264AtomParser->GetNextSample(dwTrackId, &pVideoData, &dwBufferSize, &llTime) == S_OK){

			if(dwMaxSampleBufferSize < dwBufferSize)
				dwMaxSampleBufferSize = dwBufferSize;

			if(g_bStopApplication){
				break;
			}

			IF_FAILED_THROW(VideoBuffer.Reserve(dwBufferSize));
			memcpy(VideoBuffer.GetStartBuffer(), pVideoData, dwBufferSize);
			IF_FAILED_THROW(VideoBuffer.SetEndPosition(dwBufferSize));

			NalUnitBuffer.Reset();
			iSubSliceCount = 0;

			dwTotalSample++;

			do{

				if(iSubSliceCount == 0){

					// todo : we can avoid the use of NalUnitBuffer and memcpy. Just use VideoBuffer
					IF_FAILED_THROW(NalUnitBuffer.Reserve(VideoBuffer.GetBufferSize()));
					memcpy(NalUnitBuffer.GetStartBuffer(), VideoBuffer.GetStartBuffer(), VideoBuffer.GetBufferSize());
					IF_FAILED_THROW(NalUnitBuffer.SetEndPosition(VideoBuffer.GetBufferSize()));
				}

				IF_FAILED_THROW(cH264NaluParser.ParseNaluHeader(VideoBuffer, &dwParsed));

				if(cH264NaluParser.IsNalUnitCodedSlice()){

					iSubSliceCount++;
					dwTotalPicture++;

					if(iSubSliceCount == 1)
						pDxva2Decoder->SetCurrentNalu(cH264NaluParser.GetPicture().NalUnitType, cH264NaluParser.GetPicture().btNalRefIdc);

					// DXVA2 needs start code
					if(iNaluLenghtSize == 4){
						memcpy(NalUnitBuffer.GetStartBuffer(), btStartCode, 4);
					}
					else{
						IF_FAILED_THROW(AddByteAndConvertAvccToAnnexB(NalUnitBuffer));
						dwParsed += 1;
					}

					IF_FAILED_THROW(pDxva2Decoder->AddSliceShortInfo(iSubSliceCount, dwParsed));

					if(VideoBuffer.GetBufferSize() == 0){

						NalUnitBuffer.SetStartPositionAtBeginning();
						IF_FAILED_THROW(pDxva2Decoder->DecodeFrame(NalUnitBuffer, cH264NaluParser.GetPicture(), llTime, iSubSliceCount));
						IF_FAILED_THROW(pDxva2Decoder->DisplayFrame());
					}
					else{

						// Sub-slices
						IF_FAILED_THROW(NalUnitBuffer.SetStartPosition(dwParsed));
					}
				}
				else{

					if(iSubSliceCount > 0){

						// Can be NAL_UNIT_FILLER_DATA after sub-slices
						NalUnitBuffer.SetStartPositionAtBeginning();

						IF_FAILED_THROW(pDxva2Decoder->DecodeFrame(NalUnitBuffer, cH264NaluParser.GetPicture(), llTime, iSubSliceCount));
						IF_FAILED_THROW(pDxva2Decoder->DisplayFrame());

						// We assume sub-slices are contiguous, so skip others
						VideoBuffer.Reset();
					}
					else{

						NalUnitBuffer.Reset();
					}
				}

				if(hr == S_FALSE){

					// S_FALSE means slice is corrupted. Just clear previous frames presentation, sometimes it's ok to continue
					pDxva2Decoder->ClearPresentation();
					hr = S_OK;
				}
				else if(VideoBuffer.GetBufferSize() != 0){

					// Some slice contains SEI message and sub-slices, continue parsing
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
		dwTotalSample, dwTotalPicture, VideoBuffer.GetAllocatedSize(), H264_BUFFER_SIZE, NalUnitBuffer.GetAllocatedSize(), H264_BUFFER_SIZE, dwMaxSampleBufferSize));

	VideoBuffer.Delete();
	NalUnitBuffer.Delete();
	SAFE_DELETE(pDxva2Decoder);
	SAFE_DELETE(pH264AtomParser);

	LOG_HRESULT(MFShutdown());
	CoUninitialize();

	return hr;
}

HRESULT AddByteAndConvertAvccToAnnexB(CMFBuffer& pNalUnitBuffer){

	HRESULT hr;
	BYTE btStartCode[3] = {0x00, 0x00, 0x01};

	IF_FAILED_RETURN(pNalUnitBuffer.Reserve(1));
	// Normally memmove here, but seems to be ok with memcpy
	memcpy(pNalUnitBuffer.GetStartBuffer() + 1, pNalUnitBuffer.GetStartBuffer(), pNalUnitBuffer.GetBufferSize());
	memcpy(pNalUnitBuffer.GetStartBuffer(), btStartCode, 3);
	IF_FAILED_RETURN(pNalUnitBuffer.SetEndPosition(1));

	return hr;
}