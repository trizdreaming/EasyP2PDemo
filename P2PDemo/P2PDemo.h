#pragma once

#include "Config.h"

template <class DERIVED_TYPE> 
class BaseWindow
{
public:
	//////////////////////////////////////////////////////////////////////////
	// https://github.com/trizdreaming/oldboy/blob/master/oldboy/oldboy/RMmainLoop.h 참조
	//
	// 윈도우 메시지 핸들링을 위한 콜백함수는 static (정적) 함수
	//////////////////////////////////////////////////////////////////////////
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		DERIVED_TYPE *pThis = NULL;

		//////////////////////////////////////////////////////////////////////////
		// http://whiteme7.blog.me/110067048553
		// http://stackoverflow.com/questions/6289196/window-message-different-between-wm-create-and-wm-nccreate
		// http://msdn.microsoft.com/ko-kr/library/windows/desktop/ms632635(v=vs.85).aspx 참조
		//////////////////////////////////////////////////////////////////////////

		// 초기 생성 시
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
			// http://msdn.microsoft.com/ko-kr/library/windows/desktop/ms632603(v=vs.85).aspx

			pThis = (DERIVED_TYPE*)pCreate->lpCreateParams;

			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
			// 일단 윈도우와 관련 된 정보 저장해두기
			// http://blog.naver.com/jayjay8112/40157836437
			// http://diehard98.tistory.com/117 참조

			pThis->m_hwnd = hwnd;
		}
		else
		{
			pThis = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			// 가져와 본다
		}
		if (pThis)
		{
			// 창이 떠 있으면 메시지 핸들
			return pThis->HandleMessage(uMsg, wParam, lParam);
		}
		else
		{
			// 창이 안 떠 있으면 DefWindowProc()에 메시지 핸들링을 넘김
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	BaseWindow() : m_hwnd(NULL) { }

	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = WINDOW_WIDTH,
		int nHeight = WINDOW_HEIGHT,
		HWND hWndParent = 0,
		HMENU hMenu = 0
		)
	{
		WNDCLASS wc = {0};

		wc.lpfnWndProc   = DERIVED_TYPE::WindowProc;
		wc.hInstance     = GetModuleHandle(NULL);
		wc.lpszClassName = ClassName();

		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		// 요거 추가해둠

		RegisterClass(&wc);

		m_hwnd = CreateWindowEx(
			dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this
			);

		//////////////////////////////////////////////////////////////////////////
		// https://github.com/trizdreaming/oldboy/blob/master/oldboy/oldboy/RMmainLoop.cpp
		// 373번째 라인 - HRESULT CRMmainLoop::CreateMainLoopWindow() 구현부 참조
		//////////////////////////////////////////////////////////////////////////

		return (m_hwnd ? TRUE : FALSE);
	}

	HWND Window() const { return m_hwnd; }

protected:

	// 순수 가상 함수 설정 해 둠
	virtual PCWSTR  ClassName() const = 0;
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	HWND m_hwnd;
};


template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

