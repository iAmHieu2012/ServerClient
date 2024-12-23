#include "Tasks.h"

class webcamCapture {
public:
    webcamCapture()
        : pGraph(nullptr), pControl(nullptr), pEvent(nullptr),
        pBuilder(nullptr), pSource(nullptr), pRenderer(nullptr),
        pMux(nullptr), pSink(nullptr) {
    }

    ~webcamCapture() {
        if (pControl) pControl->Stop();
        if (pEvent) pEvent->Release();
        if (pSource) pSource->Release();
        if (pRenderer) pRenderer->Release();
        if (pMux) pMux->Release();
        if (pSink) pSink->Release();
        if (pBuilder) pBuilder->Release();
        if (pGraph) pGraph->Release();
        CoUninitialize();
    }

    bool initialize() {
        HRESULT hr = CoInitialize(nullptr);
        if (FAILED(hr)) {
            std::cerr << "Error initializing COM." << std::endl;
            return false;
        }

        // Create the Filter Graph Manager
        hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);
        if (FAILED(hr)) {
            std::cerr << "Error creating Filter Graph." << std::endl;
            return false;
        }

        // Create the Capture Graph Builder
        hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&pBuilder);
        if (FAILED(hr)) {
            std::cerr << "Error creating Capture Graph Builder." << std::endl;
            return false;
        }

        hr = pBuilder->SetFiltergraph(pGraph);
        if (FAILED(hr)) {
            std::cerr << "Error setting Filter Graph in Capture Graph Builder." << std::endl;
            return false;
        }

        hr = pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
        if (FAILED(hr)) {
            std::cerr << "Error querying MediaControl." << std::endl;
            return false;
        }

        hr = pGraph->QueryInterface(IID_IMediaEvent, (void**)&pEvent);
        if (FAILED(hr)) {
            std::cerr << "Error querying MediaEvent." << std::endl;
            return false;
        }

        return true;
    }

    bool setupGraph(const wchar_t* filename) {
        HRESULT hr;
        ICreateDevEnum* pDevEnum = nullptr;
        IEnumMoniker* pEnum = nullptr;

        // Delete the file if it already exists
        if (filename && GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES) {
            if (!DeleteFile(filename)) {
                std::cerr << "Error deleting existing file: " << filename << std::endl;
                return false;
            }
        }

        // Enumerate video capture devices
        hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);
        if (FAILED(hr)) {
            std::cerr << "Error creating Device Enumerator." << std::endl;
            return false;
        }

        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        if (hr != S_OK) {
            std::cerr << "No video capture devices found." << std::endl;
            pDevEnum->Release();
            return false;
        }

        IMoniker* pMoniker = nullptr;
        if (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
            hr = pMoniker->BindToObject(nullptr, nullptr, IID_IBaseFilter, (void**)&pSource);
            pMoniker->Release();
        }
        pEnum->Release();
        pDevEnum->Release();

        if (!pSource) {
            std::cerr << "Failed to bind video capture device." << std::endl;
            return false;
        }

        // Add the source filter (webcam) to the graph
        hr = pGraph->AddFilter(pSource, L"Video Capture");
        if (FAILED(hr)) {
            std::cerr << "Error adding video source to graph." << std::endl;
            return false;
        }

        // Add the Video Renderer for preview
        hr = CoCreateInstance(CLSID_VideoRenderer, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pRenderer);
        if (FAILED(hr)) {
            std::cerr << "Error creating Video Renderer." << std::endl;
            return false;
        }

        hr = pGraph->AddFilter(pRenderer, L"Video Renderer");
        if (FAILED(hr)) {
            std::cerr << "Error adding Video Renderer to graph." << std::endl;
            return false;
        }

        // Create the AVI Multiplexer
        hr = CoCreateInstance(CLSID_AviDest, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pMux);
        if (FAILED(hr)) {
            std::cerr << "Error creating AVI Multiplexer." << std::endl;
            return false;
        }

        hr = pGraph->AddFilter(pMux, L"AVI Multiplexer");
        if (FAILED(hr)) {
            std::cerr << "Error adding AVI Multiplexer to graph." << std::endl;
            return false;
        }

        // Create the File Writer
        hr = CoCreateInstance(CLSID_FileWriter, nullptr, CLSCTX_INPROC_SERVER, IID_IFileSinkFilter, (void**)&pSink);
        if (FAILED(hr)) {
            std::cerr << "Error creating File Writer." << std::endl;
            return false;
        }

        hr = pSink->SetFileName(filename, nullptr);
        if (FAILED(hr)) {
            std::cerr << "Error setting filename for File Writer." << std::endl;
            return false;
        }

        IBaseFilter* pSinkBase = nullptr;
        hr = pSink->QueryInterface(IID_IBaseFilter, (void**)&pSinkBase);
        if (FAILED(hr)) {
            std::cerr << "Error querying File Writer IBaseFilter." << std::endl;
            return false;
        }

        hr = pGraph->AddFilter(pSinkBase, L"File Writer");
        pSinkBase->Release();
        if (FAILED(hr)) {
            std::cerr << "Error adding File Writer to graph." << std::endl;
            return false;
        }

        // Render preview stream
        hr = pBuilder->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pSource, nullptr, pRenderer);
        if (FAILED(hr)) {
            std::cerr << "Error connecting preview stream." << std::endl;
            return false;
        }

        // Render capture stream to AVI file
        hr = pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pSource, pMux, pSinkBase);
        if (FAILED(hr)) {
            std::cerr << "Error connecting capture stream." << std::endl;
            return false;
        }

        return true;
    }

    void run(int sec) {
        HRESULT hr = pControl->Run();
        if (FAILED(hr)) {
            std::cerr << "Error starting the graph." << std::endl;
            return;
        }

        std::cout << "Recording for " << sec << " seconds..." << std::endl;

        long start_t = clock();
        MSG msg;
        while ((clock() - start_t) / CLOCKS_PER_SEC <= sec) {
            // Process Windows messages to keep the application responsive
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        pControl->Stop();
        std::cout << "Stopped recording." << std::endl;

        // Hide and release the preview window
        IVideoWindow* pVideoWindow = nullptr;
        hr = pGraph->QueryInterface(IID_IVideoWindow, (void**)&pVideoWindow);
        if (SUCCEEDED(hr)) {
            pVideoWindow->put_Visible(OAFALSE); // Hide the preview window
            pVideoWindow->put_Owner(NULL);  // Remove ownership of the window
            pVideoWindow->Release();
        }
    }



private:
    IGraphBuilder* pGraph;
    IMediaControl* pControl;
    IMediaEvent* pEvent;
    ICaptureGraphBuilder2* pBuilder;
    IBaseFilter* pSource;
    IBaseFilter* pRenderer;
    IBaseFilter* pMux;
    IFileSinkFilter* pSink;
};


int webcam(const wchar_t* filename) {
    webcamCapture camRec;

    if (!camRec.initialize()) {
        std::cerr << "Initialization failed." << std::endl;
        return 0;
    }
    if (!filename) return 0;
    if (!camRec.setupGraph(filename)) {
        std::cerr << "Failed to set up graph." << std::endl;
        return 0;
    }

    camRec.run(1);

    return 1;
}