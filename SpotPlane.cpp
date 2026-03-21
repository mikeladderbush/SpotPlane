// SpotPlane.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "SpotPlane.h"
#include "threadpool.h"
#include "SBSObjects.h"
#include <atomic>

#define MAX_LOADSTRING 100
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define WM_CLIENT_UPDATE (WM_APP + 1)

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib") 
#pragma comment (lib, "AdvApi32.lib")

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HWND gMainWnd = NULL;
std::atomic<int> gManagerCount = 0;
std::atomic<int> gjob_id = 0;

std::vector<AircraftUpdate> gUpdateList;
std::mutex gUpdateMutex;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
bool                InitializeWSA(WSADATA*);
DWORD WINAPI        ThreadManager(LPVOID);


struct ManagerContext {
    SharedQueue* queue;
    Thread_pool* pool;
};

int CALLBACK wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    WSADATA data;
    if (!InitializeWSA(&data)) {
        return 1;
    }

    ManagerContext* manager_ctx = new ManagerContext();
    SharedQueue* queue = new SharedQueue();
    Thread_pool* pool = new Thread_pool(*queue, 10, [](const JobMessage& job) {
        std::string payload(job.payload);
        AircraftUpdate* h_update = new AircraftUpdate(ParseSBS(payload));
        PostMessage(gMainWnd, WM_CLIENT_UPDATE, 0, (LPARAM)h_update);
        });
    manager_ctx->pool = pool;
    manager_ctx->queue = queue;

    HANDLE Manager = CreateThread(NULL, 0, ThreadManager, manager_ctx, 0, NULL);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SPOTPLANE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SPOTPLANE));

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

    WSACleanup();

    delete pool;
    delete queue;
    delete manager_ctx;

    CloseHandle(Manager);

    return (int)msg.wParam;
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

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPOTPLANE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDI_SPOTPLANE);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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

    gMainWnd = hWnd;

    if (!hWnd)
    {
        return FALSE;
    }

    // TODO: ListView table 
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
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
        AircraftUpdate h_update;
        {
            std::lock_guard<std::mutex> lock(gUpdateMutex);
            if (gUpdateList.size() > 0) {
                h_update = gUpdateList.front();
            }
        }
        int size = MultiByteToWideChar(CP_UTF8, 0, h_update.AircraftID.c_str(), -1, NULL, 0);
        std::wstring wide(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, h_update.AircraftID.c_str(), -1, &wide[0], size);
        TextOut(hdc, 10, 30, wide.c_str(), wide.size());


        if (gManagerCount > 0) {
            TextOut(hdc, 10, 10, L"Server started", 15);
        }
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLIENT_UPDATE:
    {
        AircraftUpdate* h_update = (AircraftUpdate*)lParam;
        {
            std::lock_guard<std::mutex> lock(gUpdateMutex);
            gUpdateList.push_back(*h_update);
        }
        delete h_update;
        InvalidateRect(hWnd, NULL, 0);
        return 0;
    }
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

bool InitializeWSA(WSADATA* data) {

    return WSAStartup(MAKEWORD(2, 2), data) == 0;

}

DWORD WINAPI ThreadManager(LPVOID lpParam)
{
    ManagerContext* manager_ctx = (ManagerContext*)lpParam;
    gManagerCount++;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(30003);
    inet_pton(AF_INET, "192.168.x.x", &addr.sin_addr);

    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    int iResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    std::string line_buffer;

    do {
        iResult = recv(sock, recvbuf, recvbuflen, 0);

        if (iResult > 0) {
            line_buffer.append(recvbuf, iResult);
            size_t newline;
            while ((newline = line_buffer.find('\n')) != std::string::npos) {
                std::string line = line_buffer.substr(0, newline);
                line_buffer.erase(0, newline + 1);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                JobMessage job;
                job.payload = line;
                job.timestamp = std::chrono::steady_clock::now();
                job.job_id = gjob_id++;
                manager_ctx->queue->enqueue_job(job);
            }
        }
    } while (iResult > 0);

    closesocket(sock);
    gManagerCount--;
    return 0;
}