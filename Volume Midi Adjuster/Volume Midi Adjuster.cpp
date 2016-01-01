// Volume Midi Adjuster.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Volume Midi Adjuster.h"
#include <sstream>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include "RtMidi.h"
#include <signal.h>
#include <windows.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

bool done;
static void finish(int ignore) { done = true; }

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VOLUMEMIDIADJUSTER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VOLUMEMIDIADJUSTER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VOLUMEMIDIADJUSTER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_VOLUMEMIDIADJUSTER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // Get ready to read midi
   RtMidiIn  *midiin = 0;
   try {
	   midiin = new RtMidiIn();
   }
   catch (RtMidiError &error) {
	   error.printMessage();
	   exit(EXIT_FAILURE);
   }
   // Check inputs.
   unsigned int nPorts = midiin->getPortCount();
   std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
   std::string portName;
   for (unsigned int i = 0; i<nPorts; i++) {
	   try {
		   portName = midiin->getPortName(i);
	   }
	   catch (RtMidiError &error) {
		   error.printMessage();
	   }
	   std::cout << "  Input Port #" << i + 1 << ": " << portName << '\n';
   }


   // Listen to midi port
   midiin->openPort(0);
   midiin->ignoreTypes(false, false, false);
   done = false;
   (void) signal(SIGINT, finish);

   double stamp;
   int nBytes, i;
   std::vector<unsigned char> message;

   // Volume Adjustmnet Initizliazation
   HRESULT hr = NULL;
   bool decibels = false;
   double newVolume = 0.5;

   CoInitialize(NULL);
   IMMDeviceEnumerator *deviceEnumerator = NULL;
   hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
	   __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);

   IMMDeviceCollection *deviceCollection = NULL;
   hr = deviceEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &deviceCollection);
   UINT deviceCount = 0;
   hr = deviceCollection->GetCount(&deviceCount);

   deviceEnumerator->Release();
   deviceEnumerator = NULL;

   IMMDevice *device = NULL;
   IAudioEndpointVolume *endpointVolume = NULL;

   // Looping for midi values
   while (!done) {
	   stamp = midiin->getMessage(&message);
	   nBytes = message.size();
	   if (nBytes > 0) {
		   std::wstringstream ws;
		   ws << L"Message size: " << nBytes;
		   OutputDebugStringW(LPCWSTR(L"\n"));
		   OutputDebugStringW(ws.str().c_str());
	   }
	   for (i = 0; i < nBytes; i++) {
		   if (message[0] == 176 && message[1] == 7) {
			   hr = deviceCollection->Item(2, &device);
		   }
		   else if (message[0] == 177 && message[1] == 7) {
			   hr = deviceCollection->Item(1, &device);
		   }
		   else {
			   break;
		   }
		   hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (LPVOID *)&endpointVolume);
		   float currentVolume = 0;
		   endpointVolume->GetMasterVolumeLevel(&currentVolume);

		   hr = endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);

		   newVolume = message[2] / (float)127.0;

		   hr = endpointVolume->SetMasterVolumeLevelScalar((float)newVolume, NULL);
	   }

	   Sleep(10);
   }


   device->Release();
   device = NULL;

   deviceCollection->Release();
   deviceCollection = NULL;
   endpointVolume->Release();

   CoUninitialize();
   /* 
   IPropertyStore *pProps = NULL;
   hr = device->OpenPropertyStore(STGM_READ, &pProps);
   PROPVARIANT varName;

   hr = pProps->GetValue(PKEY_DeviceInterface_FriendlyName, &varName);

   LPWSTR deviceId = NULL;
   hr = device->GetId(&deviceId);
   */

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...

			//TextOut(hdc, 70, 50, szMessage, 10);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
