//----------------------------------------------------------------------------------------------
// StdAfx.h
//----------------------------------------------------------------------------------------------
#ifndef STDAFX_H
#define STDAFX_H

#pragma once
#define WIN32_LEAN_AND_MEAN
#define STRICT

//----------------------------------------------------------------------------------------------
// Pragma
#pragma comment(lib, "mfplat")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "dxva2")
#pragma comment(lib, "d3d9")

//----------------------------------------------------------------------------------------------
// Microsoft Windows SDK for Windows 7
#include <WinSDKVer.h>
#include <new>
#include <windows.h>
#include <tchar.h>
#include <assert.h>
#include <strsafe.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#include <commdlg.h>
#include <Shellapi.h>
#include <initguid.h>
#include <Shlwapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <evr.h>
#include <uuids.h>
#include <d3d9.h>
#include <Evr9.h>
#include <dxva.h>
#include <dxvahd.h>

//----------------------------------------------------------------------------------------------
// STL
#include <vector>
using std::vector;
#include <string>
using std::wstring;
#include <deque>
using std::deque;

//----------------------------------------------------------------------------------------------
// Common Project Files
#ifdef _DEBUG
#define MF_USE_LOGGING 1
//#define MF_USE_LOGREFCOUNT
//#define MF_TRACE_BYTESTREAM
#else
#define MF_USE_LOGGING 0
#endif

#include ".\Common\MFMacro.h"
#include ".\Common\MFTrace.h"
#include ".\Common\MFLogging.h"
#include ".\Common\MFTExternTrace.h"
#include ".\Common\MFBuffer.h"
#include ".\Common\MFLightBuffer.h"
#include ".\Common\MFCriticSection.h"
#include ".\Common\MFReadParam.h"

// Optimize the use of DXVA2 surfaces
// I need to check with more movie files before including it
#define USE_DXVA2_SURFACE_INDEX

//----------------------------------------------------------------------------------------------
// Project Files
#include "Resource.h"
#include "PlayerDefinition.h"
#include "Mp4Definition.h"
#include "H264Definition.h"
#include "MFByteStream.h"
#include "BitStream.h"
#include "WindowMessage.h"
#include "H264AtomParser.h"
#include "H264NaluParser.h"
#include "Dxva2Decoder.h"
#include "Player.h"
#include "WindowsForm.h"

#define WINDOWSFORM_CLASS L"H264Dxva2Decoder 1.0.0.0"

// Workaround for Intel GPU : nalu buffer with start code 0x00,0x00,0x00,0x01 are not handled.
// This workaround will not be enough if nalu contains more than one sub-slice.
//#define USE_WORKAROUND_FOR_INTEL_GPU

//#define HANDLE_DIRECTX_ERROR_UNDOCUMENTED

#endif