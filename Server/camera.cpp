#include "camera.h"
Media::Media()
{
	InitializeCriticalSection(&criticalSection);
	referenceCount = 1;
	wSymbolicLink = NULL;
	cchSymbolicLink = 0;
	width = 0;
	height = 0;
	sourceReader = NULL;
	rawData = NULL;

}
Media::~Media()
{

	if (wSymbolicLink)
	{
		delete wSymbolicLink;
		wSymbolicLink = NULL;
	}
	EnterCriticalSection(&criticalSection);

	if (sourceReader)
	{
		sourceReader->Release();
		sourceReader = NULL;
	}


	if (rawData)
	{
		delete rawData;
		rawData = NULL;
	}

	CoTaskMemFree(wSymbolicLink);
	wSymbolicLink = NULL;
	cchSymbolicLink = 0;

	LeaveCriticalSection(&criticalSection);
	DeleteCriticalSection(&criticalSection);
}

HRESULT Media::CreateCaptureDevice()
{
	HRESULT hr = S_OK;

	//this is important!!
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	UINT32 count = 0;
	IMFAttributes* attributes = NULL;
	IMFActivate** devices = NULL;

	if (FAILED(hr)) { CLEAN_ATTRIBUTES() }
	// Create an attribute store to specify enumeration parameters.
	hr = MFCreateAttributes(&attributes, 1);

	if (FAILED(hr)) { CLEAN_ATTRIBUTES() }

	//The attribute to be requested is devices that can capture video
	hr = attributes->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
	);
	if (FAILED(hr)) { CLEAN_ATTRIBUTES() }
	//Enummerate the video capture devices
	hr = MFEnumDeviceSources(attributes, &devices, &count);

	if (FAILED(hr)) { CLEAN_ATTRIBUTES() }
	//if there are any available devices
	if (count > 0)
	{
		/*If you actually need to select one of the available devices
		this is the place to do it. For this example the first device
		is selected
		*/
		//Get a source reader from the first available device
		SetSourceReader(devices[0]);

		WCHAR* nameString = NULL;
		// Get the human-friendly name of the device
		UINT32 cchName;
		hr = devices[0]->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
			&nameString, &cchName);

		if (SUCCEEDED(hr))
		{
			//allocate a byte buffer for the raw pixel data
			bytesPerPixel = abs(stride) / width;
			if (videoFormat == MFVideoFormat_NV12) {
				rawData = new BYTE[width * height * bytesPerPixel * 3 / 2];
			}
			else if (videoFormat == MFVideoFormat_YUY2) {
				rawData = new BYTE[width * height * bytesPerPixel];
			}
			else if (videoFormat == MFVideoFormat_RGB24) {
				rawData = new BYTE[width * height * bytesPerPixel];
			}
			else if (videoFormat == MFVideoFormat_RGB32) {
				rawData = new BYTE[width * height * bytesPerPixel];
			}
			wcscpy_s(deviceNameString, nameString);
		}
		CoTaskMemFree(nameString);
	}

	//clean
	CLEAN_ATTRIBUTES()
}


HRESULT Media::SetSourceReader(IMFActivate* device)
{
	HRESULT hr = S_OK;

	IMFMediaSource* source = NULL;
	IMFAttributes* attributes = NULL;
	IMFMediaType* mediaType = NULL;

	EnterCriticalSection(&criticalSection);

	hr = device->ActivateObject(__uuidof(IMFMediaSource), (void**)&source);

	//get symbolic link for the device
	if (SUCCEEDED(hr))
		hr = device->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &wSymbolicLink, &cchSymbolicLink);
	//Allocate attributes
	if (SUCCEEDED(hr))
		hr = MFCreateAttributes(&attributes, 2);
	//get attributes
	if (SUCCEEDED(hr))
		hr = attributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);
	// Set the callback pointer.
	if (SUCCEEDED(hr))
		hr = attributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this);
	//Create the source reader
	if (SUCCEEDED(hr))
		hr = MFCreateSourceReaderFromMediaSource(source, attributes, &sourceReader);
	// Try to find a suitable output type.
	if (SUCCEEDED(hr))
	{
		for (DWORD i = 0; ; i++)
		{
			hr = sourceReader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &mediaType);
			if (FAILED(hr)) { break; }

			hr = IsMediaTypeSupported(mediaType);
			if (FAILED(hr)) { break; }
			//Get width and height
			MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height);
			if (mediaType)
			{
				mediaType->Release(); mediaType = NULL;
			}

			if (SUCCEEDED(hr))// Found an output type.
				break;
		}
	}
	if (SUCCEEDED(hr))
	{
		// Ask for the first sample.
		hr = sourceReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
	}

	if (FAILED(hr))
	{
		if (source)
		{
			source->Shutdown();
		}
		Close();
	}
	if (source) { source->Release(); source = NULL; }
	if (attributes) { attributes->Release(); attributes = NULL; }
	if (mediaType) { mediaType->Release(); mediaType = NULL; }

	LeaveCriticalSection(&criticalSection);
	return hr;
}

HRESULT Media::IsMediaTypeSupported(IMFMediaType* pType)
{
	HRESULT hr = S_OK;

	BOOL bFound = FALSE;
	GUID subtype = { 0 };

	//Get the stride for this format so we can calculate the number of bytes per pixel
	GetDefaultStride(pType, &stride);

	if (FAILED(hr)) { return hr; }
	hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);

	videoFormat = subtype;

	if (FAILED(hr)) { return hr; }

	if (subtype == MFVideoFormat_RGB32 || subtype == MFVideoFormat_RGB24 || subtype == MFVideoFormat_YUY2 || subtype == MFVideoFormat_NV12)
		return S_OK;
	else
		return S_FALSE;

	return hr;
}

HRESULT Media::Close()
{
	EnterCriticalSection(&criticalSection);
	if (sourceReader)
	{
		sourceReader->Release(); sourceReader = NULL;
	}

	CoTaskMemFree(wSymbolicLink);
	wSymbolicLink = NULL;
	cchSymbolicLink = 0;

	LeaveCriticalSection(&criticalSection);
	return S_OK;
}

//From IUnknown 
STDMETHODIMP Media::QueryInterface(REFIID riid, void** ppvObject)
{
	static const QITAB qit[] = { QITABENT(Media, IMFSourceReaderCallback),{ 0 }, };
	return QISearch(this, qit, riid, ppvObject);
}
//From IUnknown
ULONG Media::Release()
{
	ULONG count = InterlockedDecrement(&referenceCount);
	if (count == 0)
		delete this;
	// For thread safety
	return count;
}
//From IUnknown
ULONG Media::AddRef()
{
	return InterlockedIncrement(&referenceCount);
}


//Calculates the default stride based on the format and size of the frames
HRESULT Media::GetDefaultStride(IMFMediaType* type, LONG* stride)
{
	LONG tempStride = 0;

	// Try to get the default stride from the media type.
	HRESULT hr = type->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&tempStride);
	if (FAILED(hr))
	{
		//Setting this atribute to NULL we can obtain the default stride
		GUID subtype = GUID_NULL;

		UINT32 width = 0;
		UINT32 height = 0;

		// Obtain the subtype
		hr = type->GetGUID(MF_MT_SUBTYPE, &subtype);
		//obtain the width and height
		if (SUCCEEDED(hr))
			hr = MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height);
		//Calculate the stride based on the subtype and width
		if (SUCCEEDED(hr))
			hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &tempStride);
		// set the attribute so it can be read
		if (SUCCEEDED(hr))
			(void)type->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(tempStride));
	}

	if (SUCCEEDED(hr))
		*stride = tempStride;
	return hr;
}

//Method from IMFSourceReaderCallback
HRESULT Media::OnReadSample(HRESULT status, DWORD streamIndex, DWORD streamFlags, LONGLONG timeStamp, IMFSample* sample)
{
	HRESULT hr = S_OK;
	IMFMediaBuffer* mediaBuffer = NULL;

	EnterCriticalSection(&criticalSection);

	if (FAILED(status))
		hr = status;

	if (SUCCEEDED(hr))
	{
		if (sample)
		{// Get the video frame buffer from the sample.
			hr = sample->GetBufferByIndex(0, &mediaBuffer);
			// Draw the frame.
			if (SUCCEEDED(hr))
			{
				BYTE* data;
				mediaBuffer->Lock(&data, NULL, NULL);
				//This is a good place to perform color conversion and drawing
				//Instead we're copying the data to a buffer
				if (videoFormat == MFVideoFormat_NV12) CopyMemory(rawData, data, width * height * bytesPerPixel * 1.5);
				else if (videoFormat == MFVideoFormat_YUY2) CopyMemory(rawData, data, width * height * bytesPerPixel);
				else if (videoFormat == MFVideoFormat_RGB24) CopyMemory(rawData, data, width * height * bytesPerPixel);
				else if (videoFormat == MFVideoFormat_RGB32) CopyMemory(rawData, data, width * height * bytesPerPixel);

			}
		}
	}
	// Request the next frame.
	if (SUCCEEDED(hr))
		hr = sourceReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);

	if (FAILED(hr))
	{
		//Notify there was an error
		printf("Error HRESULT = 0x%d", hr);
		PostMessage(NULL, 1, (WPARAM)hr, 0L);
	}
	if (mediaBuffer) { mediaBuffer->Release(); mediaBuffer = NULL; }

	LeaveCriticalSection(&criticalSection);
	return hr;
}
//Method from IMFSourceReaderCallback 
STDMETHODIMP Media::OnEvent(DWORD, IMFMediaEvent*) { return S_OK; }
//Method from IMFSourceReaderCallback 
STDMETHODIMP Media::OnFlush(DWORD) { return S_OK; }

// RENDERING MEDIA

RenderingWindow::RenderingWindow(LPWSTR name, int width, int height, int cmd)
{
	factory = NULL;
	d2dBitmap = NULL;
	renderTarget = NULL;

	WNDCLASSEX wc{ 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor = (HCURSOR)LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc = RenderingWindow::MsgRouter;
	wc.lpszClassName = TEXT("WindowClass");
	wc.style = CS_VREDRAW | CS_HREDRAW;

	RegisterClassEx(&wc);
	windowHandle = CreateWindowEx(NULL, wc.lpszClassName, name, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, NULL, this);

	ShowWindow(windowHandle, cmd);
	UpdateWindow(windowHandle);

	//Initialize D2D
	if (renderTarget == NULL)
	{
		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
		D2D1_SIZE_U size = D2D1::SizeU(width, height);
		D2D1_RENDER_TARGET_PROPERTIES rtProperties = D2D1::RenderTargetProperties();
		rtProperties.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
		rtProperties.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
		factory->CreateHwndRenderTarget(rtProperties, D2D1::HwndRenderTargetProperties(windowHandle, size), &renderTarget);
		renderTarget->CreateBitmap(size, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
			&d2dBitmap);
	}
}
RenderingWindow::~RenderingWindow()
{
	if (factory)
	{
		factory->Release();
		factory = NULL;
	}
	if (renderTarget)
	{
		renderTarget->Release();
		renderTarget = NULL;
	}
	if (d2dBitmap)
	{
		d2dBitmap->Release();
		d2dBitmap = NULL;
	}

}
void RenderingWindow::Draw(BYTE* pixels, int width, int height)
{
	d2dBitmap->CopyFromMemory(NULL, pixels, width);
	renderTarget->BeginDraw();
	renderTarget->DrawBitmap(d2dBitmap);
	renderTarget->EndDraw();
}

LRESULT CALLBACK RenderingWindow::MsgRouter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RenderingWindow* app;
	if (msg == WM_CREATE)
	{
		app = (RenderingWindow*)(((LPCREATESTRUCT)(lParam))->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)app);
	}
	else
	{
		app = (RenderingWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}
	return app->WndProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK RenderingWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		CloseWindow(hWnd);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
		break;
	}
	return 0;
}


//-------------------------------------------------------------------
//
// Conversion functions
//
//-------------------------------------------------------------------

__forceinline BYTE Clip(int clr)
{
	return (BYTE)(clr < 0 ? 0 : (clr > 255 ? 255 : clr));
}

__forceinline RGBQUAD ConvertYCrCbToRGB(
	int y,
	int cr,
	int cb
)
{
	RGBQUAD rgbq;

	int c = y - 16;
	int d = cb - 128;
	int e = cr - 128;

	rgbq.rgbRed = Clip((298 * c + 409 * e + 128) >> 8);
	rgbq.rgbGreen = Clip((298 * c - 100 * d - 208 * e + 128) >> 8);
	rgbq.rgbBlue = Clip((298 * c + 516 * d + 128) >> 8);

	return rgbq;
}


//-------------------------------------------------------------------
// TransformImage_RGB24 
//
// RGB-24 to RGB-32
//-------------------------------------------------------------------

void TransformImage_RGB24(
	BYTE* pDest,
	LONG        lDestStride,
	const BYTE* pSrc,
	LONG        lSrcStride,
	DWORD       dwWidthInPixels,
	DWORD       dwHeightInPixels
)
{
	for (DWORD y = 0; y < dwHeightInPixels; y++)
	{
		RGBTRIPLE* pSrcPel = (RGBTRIPLE*)pSrc;
		DWORD* pDestPel = (DWORD*)pDest;

		for (DWORD x = 0; x < dwWidthInPixels; x++)
		{
			pDestPel[x] = D3DCOLOR_XRGB(
				pSrcPel[x].rgbtRed,
				pSrcPel[x].rgbtGreen,
				pSrcPel[x].rgbtBlue
			);
		}

		pSrc += lSrcStride;
		pDest += lDestStride;
	}
}

//-------------------------------------------------------------------
// TransformImage_RGB32
//
// RGB-32 to RGB-32 
//
// Note: This function is needed to copy the image from system
// memory to the Direct3D surface.
//-------------------------------------------------------------------

void TransformImage_RGB32(
	BYTE* pDest,
	LONG        lDestStride,
	const BYTE* pSrc,
	LONG        lSrcStride,
	DWORD       dwWidthInPixels,
	DWORD       dwHeightInPixels
)
{
	MFCopyImage(pDest, lDestStride, pSrc, lSrcStride, dwWidthInPixels * 4, dwHeightInPixels);
}

//-------------------------------------------------------------------
// TransformImage_YUY2 
//
// YUY2 to RGB-32
//-------------------------------------------------------------------

void TransformImage_YUY2(
	BYTE* pDest,
	LONG        lDestStride,
	const BYTE* pSrc,
	LONG        lSrcStride,
	DWORD       dwWidthInPixels,
	DWORD       dwHeightInPixels
)
{
	for (DWORD y = 0; y < dwHeightInPixels; y++)
	{
		RGBQUAD* pDestPel = (RGBQUAD*)pDest;
		WORD* pSrcPel = (WORD*)pSrc;

		for (DWORD x = 0; x < dwWidthInPixels; x += 2)
		{
			// Byte order is U0 Y0 V0 Y1

			int y0 = (int)LOBYTE(pSrcPel[x]);
			int u0 = (int)HIBYTE(pSrcPel[x]);
			int y1 = (int)LOBYTE(pSrcPel[x + 1]);
			int v0 = (int)HIBYTE(pSrcPel[x + 1]);

			pDestPel[x] = ConvertYCrCbToRGB(y0, v0, u0);
			pDestPel[x + 1] = ConvertYCrCbToRGB(y1, v0, u0);
		}

		pSrc += lSrcStride;
		pDest += lDestStride;
	}

}


//-------------------------------------------------------------------
// TransformImage_NV12
//
// NV12 to RGB-32
//-------------------------------------------------------------------

void TransformImage_NV12(
	BYTE* pDst,
	LONG dstStride,
	const BYTE* pSrc,
	LONG srcStride,
	DWORD dwWidthInPixels,
	DWORD dwHeightInPixels
)
{
	const BYTE* lpBitsY = pSrc;
	const BYTE* lpBitsCb = lpBitsY + (dwHeightInPixels * srcStride);;
	const BYTE* lpBitsCr = lpBitsCb + 1;

	for (UINT y = 0; y < dwHeightInPixels; y += 2)
	{
		const BYTE* lpLineY1 = lpBitsY;
		const BYTE* lpLineY2 = lpBitsY + srcStride;
		const BYTE* lpLineCr = lpBitsCr;
		const BYTE* lpLineCb = lpBitsCb;

		LPBYTE lpDibLine1 = pDst;
		LPBYTE lpDibLine2 = pDst + dstStride;

		for (UINT x = 0; x < dwWidthInPixels; x += 2)
		{
			int  y0 = (int)lpLineY1[0];
			int  y1 = (int)lpLineY1[1];
			int  y2 = (int)lpLineY2[0];
			int  y3 = (int)lpLineY2[1];
			int  cb = (int)lpLineCb[0];
			int  cr = (int)lpLineCr[0];

			RGBQUAD r = ConvertYCrCbToRGB(y0, cr, cb);
			lpDibLine1[0] = r.rgbBlue;
			lpDibLine1[1] = r.rgbGreen;
			lpDibLine1[2] = r.rgbRed;
			lpDibLine1[3] = 0; // Alpha

			r = ConvertYCrCbToRGB(y1, cr, cb);
			lpDibLine1[4] = r.rgbBlue;
			lpDibLine1[5] = r.rgbGreen;
			lpDibLine1[6] = r.rgbRed;
			lpDibLine1[7] = 0; // Alpha

			r = ConvertYCrCbToRGB(y2, cr, cb);
			lpDibLine2[0] = r.rgbBlue;
			lpDibLine2[1] = r.rgbGreen;
			lpDibLine2[2] = r.rgbRed;
			lpDibLine2[3] = 0; // Alpha

			r = ConvertYCrCbToRGB(y3, cr, cb);
			lpDibLine2[4] = r.rgbBlue;
			lpDibLine2[5] = r.rgbGreen;
			lpDibLine2[6] = r.rgbRed;
			lpDibLine2[7] = 0; // Alpha

			lpLineY1 += 2;
			lpLineY2 += 2;
			lpLineCr += 2;
			lpLineCb += 2;

			lpDibLine1 += 8;
			lpDibLine2 += 8;
		}

		pDst += (2 * dstStride);
		lpBitsY += (2 * srcStride);
		lpBitsCr += srcStride;
		lpBitsCb += srcStride;
	}
}

int WINAPI turnOnCamera(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Media* m = new Media();
	m->CreateCaptureDevice();

	BYTE* bgraBuffer = new BYTE[m->width * m->height * 4];
	wchar_t titleString[2048];
	wsprintf(titleString, L"Camera Preview: %ls %d x %d", m->deviceNameString, m->width, m->height);
	RenderingWindow window(titleString, m->width, m->height, nCmdShow);

	MSG msg{ 0 };
	while (msg.message != WM_QUIT)
	{
		//RGB24_to_BGRA32(bgraBuffer, m->rawData, m->width, m->height);
		if (m->videoFormat == MFVideoFormat_NV12) {
			TransformImage_NV12(bgraBuffer, m->width * 4, m->rawData, m->stride, m->width, m->height);
		}
		else if (m->videoFormat == MFVideoFormat_YUY2) {
			TransformImage_YUY2(bgraBuffer, m->width * 4, m->rawData, m->stride, m->width, m->height);
		}
		else if (m->videoFormat == MFVideoFormat_RGB24) {
			TransformImage_RGB24(bgraBuffer, m->width * 4, m->rawData, m->stride, m->width, m->height);
		}
		else if (m->videoFormat == MFVideoFormat_RGB32) {
			TransformImage_RGB32(bgraBuffer, m->width * 4, m->rawData, m->stride, m->width, m->height);
		}
		window.Draw(bgraBuffer, m->width * 4, m->height);

		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (window.windowHandle && IsDialogMessage(window.windowHandle, &msg))
			{
				continue;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

	}

	m->Release();
	delete[] bgraBuffer;
	return 0;
}