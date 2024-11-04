#include "camera.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
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