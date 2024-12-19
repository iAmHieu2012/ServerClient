#include "Tasks.h"

class WebcamCapture {
private:
    IGraphBuilder* pGraph = NULL;
    ICaptureGraphBuilder2* pBuild = NULL;
    IMediaControl* pControl = NULL;
    IBaseFilter* pCap = NULL;
    IBaseFilter* pMux = NULL;
    IFileSinkFilter* pSink = NULL;

public:
    HRESULT InitCaptureGraphBuilder() {
        HRESULT hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL,
            CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2,
            (void**)&pBuild);

        if (SUCCEEDED(hr)) {
            hr = CoCreateInstance(CLSID_FilterGraph, NULL,
                CLSCTX_INPROC_SERVER, IID_IGraphBuilder,
                (void**)&pGraph);
            if (SUCCEEDED(hr)) {
                hr = pBuild->SetFiltergraph(pGraph);
            }
        }
        return hr;
    }

    HRESULT EnumerateDevices() {
        HRESULT hr = S_OK;
        ICreateDevEnum* pDevEnum = NULL;
        IEnumMoniker* pEnum = NULL;

        hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
            CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
            (void**)&pDevEnum);

        if (SUCCEEDED(hr)) {
            hr = pDevEnum->CreateClassEnumerator(
                CLSID_VideoInputDeviceCategory, &pEnum, 0);

            if (hr == S_FALSE) {
                hr = VFW_E_NOT_FOUND;
            }

            IMoniker* pMoniker = NULL;
            if (SUCCEEDED(hr)) {
                if (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
                    hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter,
                        (void**)&pCap);
                    pMoniker->Release();
                }
                pEnum->Release();
            }
            pDevEnum->Release();
        }
        return hr;
    }

    HRESULT SetupVideoFile(const wchar_t* filename) {
        // Add capture filter to graph
        HRESULT hr = pGraph->AddFilter(pCap, L"Capture Filter");
        if (FAILED(hr)) return hr;

        // Create the AVI multiplexer
        hr = CoCreateInstance(CLSID_AviDest, NULL, CLSCTX_INPROC_SERVER,
            IID_IBaseFilter, (void**)&pMux);
        if (FAILED(hr)) return hr;

        // Create the file writer
        hr = CoCreateInstance(CLSID_FileWriter, NULL, CLSCTX_INPROC_SERVER,
            IID_IFileSinkFilter, (void**)&pSink);
        if (FAILED(hr)) return hr;

        // Set the output filename
        hr = pSink->SetFileName(filename, NULL);
        if (FAILED(hr)) return hr;

        // Get the IBaseFilter interface from the sink filter
        IBaseFilter* pSinkFilter = NULL;
        hr = pSink->QueryInterface(IID_IBaseFilter, (void**)&pSinkFilter);
        if (FAILED(hr)) return hr;

        // Add the filters to the graph
        hr = pGraph->AddFilter(pMux, L"AVI Mux");
        if (FAILED(hr)) {
            pSinkFilter->Release();
            return hr;
        }

        hr = pGraph->AddFilter(pSinkFilter, L"File Writer");
        if (FAILED(hr)) {
            pSinkFilter->Release();
            return hr;
        }

        // Connect capture pin to mux input
        hr = pBuild->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
            pCap, pMux, pSinkFilter);

        pSinkFilter->Release();
        if (FAILED(hr)) return hr;

        // Get media control interface
        hr = pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
        return hr;
    }

    void Record(int sec) {
        if (pControl) {
            pControl->Run();
            clock_t start_t = clock();
            while ((clock() - start_t) / CLOCKS_PER_SEC <= sec) {
                continue;
            }
            pControl->Stop();
        }
    }

    void Cleanup() {
        if (pControl) pControl->Release();
        if (pSink) pSink->Release();
        if (pMux) pMux->Release();
        if (pCap) pCap->Release();
        if (pBuild) pBuild->Release();
        if (pGraph) pGraph->Release();
    }
};

int webcam(wchar_t* filename) {
    // Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        printf("Failed to initialize COM\n");
        return -1;
    }

    WebcamCapture capture;

    // Setup capture graph
    hr = capture.InitCaptureGraphBuilder();
    if (FAILED(hr)) {
        printf("Failed to create capture graph builder\n");
        CoUninitialize();
        return -1;
    }

    // Find webcam
    hr = capture.EnumerateDevices();
    if (FAILED(hr)) {
        printf("Failed to find webcam\n");
        capture.Cleanup();
        CoUninitialize();
        return -1;
    }

    // Setup video file
    hr = capture.SetupVideoFile(filename);
    if (FAILED(hr)) {
        printf("Failed to setup video file\n");
        capture.Cleanup();
        CoUninitialize();
        return -1;
    }


    // Start recording
    capture.Record(3);

    // Cleanup
    capture.Cleanup();
    CoUninitialize();

    printf("Recording completed.\n");
    return 1;
}
