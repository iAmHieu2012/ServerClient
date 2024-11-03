#pragma once
//media foundation headers
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <mfidl.h> 
#include <Mfapi.h> 
#include <Mfreadwrite.h>
#include <Shlwapi.h>
#include <d2d1.h>
#include <D3d9types.h>
//include and lib dependencies for Media Foundation
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mf.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"shlwapi.lib")

#include <stdio.h>

#define CLEAN_ATTRIBUTES() if (attributes) { attributes->Release(); attributes = NULL; }for (DWORD i = 0; i < count; i++){if (&devices[i]) { devices[i]->Release(); devices[i] = NULL; }}CoTaskMemFree(devices);return hr;


class Media : public IMFSourceReaderCallback //this class inhertis from IMFSourceReaderCallback
{
	CRITICAL_SECTION criticalSection;
	long referenceCount;
	WCHAR* wSymbolicLink;
	UINT32                  cchSymbolicLink;
	IMFSourceReader* sourceReader;


public:
	LONG stride;
	int bytesPerPixel;
	GUID videoFormat;
	UINT height;
	UINT width;
	WCHAR deviceNameString[2048];
	BYTE* rawData;

	HRESULT CreateCaptureDevice();
	HRESULT SetSourceReader(IMFActivate* device);
	HRESULT IsMediaTypeSupported(IMFMediaType* type);
	HRESULT GetDefaultStride(IMFMediaType* pType, LONG* plStride);
	HRESULT Close();
	Media();
	~Media();

	// the class must implement the methods from IUnknown 
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//  the class must implement the methods from IMFSourceReaderCallback 
	STDMETHODIMP OnReadSample(HRESULT status, DWORD streamIndex, DWORD streamFlags, LONGLONG timeStamp, IMFSample* sample);
	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent*);
	STDMETHODIMP OnFlush(DWORD);

};


class RenderingWindow
{
	static LRESULT CALLBACK MsgRouter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	//D2D stuff
	ID2D1Factory* factory;
	ID2D1HwndRenderTarget* renderTarget;
	ID2D1Bitmap* d2dBitmap;
public:
	HWND windowHandle;
	RenderingWindow(LPWSTR name, int width, int height, int cmd);
	~RenderingWindow();
	void Draw(BYTE* pixels, int width, int height);
};

__forceinline BYTE Clip(int clr);
__forceinline RGBQUAD ConvertYCrCbToRGB(
	int y,
	int cr,
	int cb
);
void TransformImage_RGB24(
	BYTE* pDest,
	LONG        lDestStride,
	const BYTE* pSrc,
	LONG        lSrcStride,
	DWORD       dwWidthInPixels,
	DWORD       dwHeightInPixels
);
void TransformImage_RGB32(
	BYTE* pDest,
	LONG        lDestStride,
	const BYTE* pSrc,
	LONG        lSrcStride,
	DWORD       dwWidthInPixels,
	DWORD       dwHeightInPixels
);
void TransformImage_YUY2(
	BYTE* pDest,
	LONG        lDestStride,
	const BYTE* pSrc,
	LONG        lSrcStride,
	DWORD       dwWidthInPixels,
	DWORD       dwHeightInPixels
);
void TransformImage_NV12(
	BYTE* pDst,
	LONG dstStride,
	const BYTE* pSrc,
	LONG srcStride,
	DWORD dwWidthInPixels,
	DWORD dwHeightInPixels
);