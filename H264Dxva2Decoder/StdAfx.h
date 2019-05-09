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
#pragma comment(lib, "Mf")
#pragma comment(lib, "strmiids")
#pragma comment(lib, "mfuuid")

// Comment this, if you don't have the Directx SDK (june 2010) installed
#define USE_DIRECTX9

//----------------------------------------------------------------------------------------------
// Microsoft DirectX SDK (June 2010)
#ifdef USE_DIRECTX9
#ifdef _WIN64
#pragma comment(lib, "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x64\\d3dx9")
#else
#pragma comment(lib, "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x86\\d3dx9")
#endif
#endif

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
#include <CommCtrl.h>
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
// Microsoft DirectX SDK (June 2010)
#ifdef USE_DIRECTX9
#include "C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\d3dx9.h"
#endif

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

// Some GPU need this (Intel)
#define USE_D3D_SURFACE_ALIGMENT

// this is not really needed, if your system has good performance
//#define USE_MMCSS_WORKQUEUE

//----------------------------------------------------------------------------------------------
// Project Files
#include "Resource.h"
#include "PlayerDefinition.h"
#include "Mp4Definition.h"
#include "H264Definition.h"
#include "Dxva2Definition.h"
#include "MFByteStream.h"
#include "BitStream.h"
#include "CallbackMessage.h"
#include "H264AtomParser.h"
#include "H264NaluParser.h"
#include "Text2D.h"
#include "Dxva2Decoder.h"
#include "Dxva2Renderer.h"
#include "Player.h"
#include "Dxva2WindowsForm.h"
#include "WindowsForm.h"

#define WINDOWSFORM_CLASS L"H264Dxva2Decoder 1.1.0.0"

// See CDxva2Renderer::RenderFrame (IDirect3DDevice9Ex::Present)
//#define HANDLE_DIRECTX_ERROR_UNDOCUMENTED

#endif