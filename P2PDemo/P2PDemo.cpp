// P2PDemo.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "P2PDemo.h"
#include "Player.h"
#include "PacketType.h"
#include "Scene.h"
#include "NetHelper.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib,"ws2_32.lib")

HANDLE g_hTimer = NULL;
BOOL InitializeTimer();

//////////////////////////////////////////////////////////////////////////
// https://github.com/LeeInJae/meteor/blob/master/src/Meteor/MainWindow.h
// https://github.com/LeeInJae/meteor/blob/master/src/Meteor/MainWindow.cpp 참조
//////////////////////////////////////////////////////////////////////////
class MainWindow : public BaseWindow<MainWindow>
// 템플릿으로 구현 되어 있는 BaseWindow를 상속 받아서 MainWindow 구현
{
	Scene   m_scene;

public:
	// 아래 두 개는 순수 가상 함수였으므로 반드시 구현 해야 됨
	PCWSTR  ClassName() const { return L"P2P Demo"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

const WCHAR WINDOW_NAME[] = L"P2P Test";


INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow)
{
	//////////////////////////////////////////////////////////////////////////
	// https://github.com/trizdreaming/oldboy/wiki/3주차-학습 참조
	//
	// COM 호출 순서 참고할 것
	//////////////////////////////////////////////////////////////////////////

	// COM 라이브러리 호출
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
		return 0;

	char* cmdLine = GetCommandLineA() ;
	//////////////////////////////////////////////////////////////////////////
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms683156(v=vs.85).aspx 참조
	//
	// CreateProcess( ,xxx, ); <- 이렇게 호출한 경우 command line을 얻어오기 위해서는 GetCommandLine()을 사용
	// WinMain()으로 들어오는 lpCmdLine을 통해서 접근을 하게 되면 마지막 인자만 얻을 수 있음
	//////////////////////////////////////////////////////////////////////////

	char seps[]   = " ,\t\n" ;
	char* serverIpAddr = NULL ;
	bool serverMode = true ;

	strtok_s(cmdLine, seps, &serverIpAddr) ;
	//////////////////////////////////////////////////////////////////////////
	// 문자열 분리 함수
	// http://mongyang.tistory.com/76
	// http://harmonize84.tistory.com/112 참조
	//////////////////////////////////////////////////////////////////////////

	// 콘솔 출력용
	// AllocConsole();
	// FILE* pFile; 
	// freopen_s(&pFile, "CONOUT$", "wb", stdout);

	if ( strlen(serverIpAddr) > 0 )
	{	
		/// 대상 서버 주소가 있으면 클라 모드
		serverMode = false ;

		//////////////////////////////////////////////////////////////////////////
		// 커맨드 창에서 입력 할 경우 앞에 스페이스 바 때문에 공백 하나 더 포함 됨
		// 그 공백을 제거 하기 위해서 첫 칸이 공백인지 확인해서 memmove() 실행
		//////////////////////////////////////////////////////////////////////////
		if ( ' ' == serverIpAddr[0])
		{
			printf_s("[%s] Delete first character \n", serverIpAddr);
			memmove(serverIpAddr, serverIpAddr+1, strlen(serverIpAddr)-1) ;
			serverIpAddr[strlen(serverIpAddr)-1] = '\0';
		}

		// printf_s("Client Mode - Target IP : [%s] \n", serverIpAddr);
	}
	else
	{
		// printf_s("Server Mode - Target IP : [%s] \n", serverIpAddr);
	}

	GNetHelper = new NetHelper(serverMode, serverIpAddr) ;
	
	if ( !GNetHelper->Initialize() )
	{
		MessageBox(NULL, L"NetHelper::Initialize()", L"ERROR", MB_OK) ;
		return -1 ;
	}


	if (!InitializeTimer())
	{
		CoUninitialize();
		// COM 사용 종료
		return 0;
	}

	MainWindow win;

	if (!win.Create(WINDOW_NAME, WS_OVERLAPPEDWINDOW))
	{
		return 0;
	}

	ShowWindow(win.Window(), nCmdShow);

	// Run the message loop.

	MSG msg = { };
	//////////////////////////////////////////////////////////////////////////
	//	typedef struct tagMSG {
	//		HWND        hwnd;
	//		UINT        message;
	//		WPARAM      wParam;
	//		LPARAM      lParam;
	//		DWORD       time;
	//		POINT       pt;
	//	#ifdef _MAC
	//		DWORD       lPrivate;
	//	#endif
	//	} MSG, *PMSG, NEAR *NPMSG, FAR *LPMSG;


	while (msg.message != WM_QUIT)
	{
		// 메세지가 들어오면 처리
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		// Wait until the timer expires or any message is posted.
		//
		// http://process3.blog.me/20032377719 참조

		// 메세지가 안 들어와도 일정 시간마다 발생하는 타이머 이벤트를 받아서 화면 갱신
		if (MsgWaitForMultipleObjects(1, &g_hTimer, FALSE, INFINITE, QS_ALLINPUT) == WAIT_OBJECT_0)
		{
			InvalidateRect(win.Window(), NULL, FALSE);
			//////////////////////////////////////////////////////////////////////////
			// http://msdn.microsoft.com/en-us/library/windows/desktop/dd145002(v=vs.85).aspx
			// http://blog.naver.com/pok_jadan/186208331 참조
			//////////////////////////////////////////////////////////////////////////
		}
	}

	CloseHandle(g_hTimer);
	CoUninitialize();
	// COM 사용 종료


	delete GNetHelper ;

	return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwnd = m_hwnd;
	static bool handshaked = false ;

	switch (uMsg)
	{
	case WM_CREATE:
		if (FAILED(m_scene.Initialize()))
		{
			return -1;
		}
		return 0;

	case WM_DESTROY:
		m_scene.CleanUp();
		PostQuitMessage(0);
		return 0;

	case WM_KEYDOWN:
		// 키가 눌렸을 때 핸들링

		// 엔터 눌렀을 때, 연결이 맺어져 있지 않을 경우 연결(악수) 시도
		if ( wParam == VK_RETURN && !handshaked )
		{
			handshaked = true ;
			if ( !GNetHelper->DoHandShake() )
			{
				ExitProcess(1) ;
			}
		}

		return 0 ;

	case WM_PAINT:
	case WM_DISPLAYCHANGE:
		//////////////////////////////////////////////////////////////////////////
		// {}로 한 뎁스 생성 - 스택을 이용해서 PAINTSTRUCT 생명주기 관리
		//////////////////////////////////////////////////////////////////////////
		{
			PAINTSTRUCT ps;
			BeginPaint(m_hwnd, &ps);
			m_scene.Render(m_hwnd);
			EndPaint(m_hwnd, &ps);
		}
		return 0;

	case WM_SIZE:
		{
			int x = (int)(short)LOWORD(lParam);
			int y = (int)(short)HIWORD(lParam);
			m_scene.Resize(x,y);
			InvalidateRect(m_hwnd, NULL, FALSE);
		}
		return 0;

	case WM_ERASEBKGND:
		return 1;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}


BOOL InitializeTimer()
{
	//////////////////////////////////////////////////////////////////////////
	// https://github.com/zrma/EasyGameServer/blob/master/EasyServer/EasyServer/EasyServer.cpp 참조
	//
	// ClientHandlingThread() 함수 내부의 CreateWaitableTimer 관련 주석 참고할 것
	//////////////////////////////////////////////////////////////////////////
	g_hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	if (g_hTimer == NULL)
	{
		return FALSE;
	}

	LARGE_INTEGER li = {0};

	if (!SetWaitableTimer(g_hTimer, &li, (1000/30), NULL, NULL,FALSE))
	{
		CloseHandle(g_hTimer);
		g_hTimer = NULL;
		return FALSE;
	}

	return TRUE;
}

