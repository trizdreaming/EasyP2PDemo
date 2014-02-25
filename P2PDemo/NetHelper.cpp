#include "stdafx.h"
#include "Config.h"
#include "NetHelper.h"

NetHelper* GNetHelper = NULL ;

NetHelper::NetHelper(bool serverMode, char* serverAddr) : m_PeerAddrLen(0), m_Socket(NULL), m_IsServerMode(serverMode), m_IsPeerLinked(false)
{
	strcpy_s(m_TargetAddr, serverAddr) ;
}

NetHelper::~NetHelper()
{
	closesocket(m_Socket) ;
	WSACleanup() ;
}


bool NetHelper::Initialize()
{
	/// Socket 초기화 
	// https://github.com/zrma/EasyGameServer/blob/master/EasyServer/EasyServer/EasyServer.cpp 참고
	WSADATA wsa ;
	if ( WSAStartup(MAKEWORD(2, 2), &wsa) != 0 )
		return false ;

	// SOCK_DGRAM : UDP
	m_Socket = socket(AF_INET, SOCK_DGRAM, 0) ;
	if (m_Socket == INVALID_SOCKET)
		return false ;

	return true ;
}

 
bool NetHelper::DoHandShake()
{
	char ioBuf[BUF_SIZE] = {0, } ;

	SOCKADDR_IN serveraddr ;
	ZeroMemory(&serveraddr, sizeof(serveraddr)) ;
	serveraddr.sin_family = AF_INET ;
	serveraddr.sin_port = htons(SERVER_PORT_NUM) ;

	// 서버일 때
	if (m_IsServerMode)
	{
		BOOL on = TRUE ;
		int retval = setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) ;
		//////////////////////////////////////////////////////////////////////////
		// https://github.com/zrma/EasyGameServer/blob/master/EasyServer/EasyServer/EasyServer.cpp 관련 주석 참고
		//////////////////////////////////////////////////////////////////////////

		if (retval == SOCKET_ERROR)
			return false ;
	
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY) ;
		// 이것도 관련 주석 참고 할 것

		retval = bind(m_Socket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) ;
		// (IP) 주소값 할당

		if (retval == SOCKET_ERROR)
			return false ;

		m_PeerAddrLen = sizeof(m_PeerAddrIn) ;

		//////////////////////////////////////////////////////////////////////////
		// http://mintnlatte.tistory.com/241 참고
		//
		// sendto(), recvfrom()은 UDP에서 사용. 주소를 명시 할 수 있다
		//////////////////////////////////////////////////////////////////////////
		retval = recvfrom(m_Socket, ioBuf, BUF_SIZE, 0, (SOCKADDR*)&m_PeerAddrIn, &m_PeerAddrLen) ;
		// 1. 데이터 받기

		if (retval == SOCKET_ERROR)
		{
			MessageBox(NULL, L"ERROR: first recvfrom()", L"ERROR", MB_OK) ;
			return false ;
		}

		// 2. CONNECT 라는 것을 받으면
		if (!strncmp(ioBuf, "CONNECT", 7))
		{
			sprintf_s(ioBuf, "SUCCESS") ;
			// 3. SUCCESS 라고 보내기

			retval = sendto(m_Socket, ioBuf, strlen(ioBuf), 0, (SOCKADDR*)&m_PeerAddrIn, sizeof(m_PeerAddrIn)) ;
			if (retval == SOCKET_ERROR)
			{
				MessageBox(NULL, L"ERROR: sendto(SUCCESS)", L"ERROR", MB_OK) ;
				return false ;
			}

			m_IsPeerLinked = true ;

			//////////////////////////////////////////////////////////////////////////
			// 1 -> 2 -> 3 악수(핸드쉐이크) 성공!
		}
		else
		{
			MessageBox(NULL, L"ERROR: INVALID CONNECT!!", L"ERROR", MB_OK) ;
			return false ;
		}

	}
	// 클라일 때
	else
	{
		serveraddr.sin_addr.s_addr = inet_addr(m_TargetAddr) ;

		// 1. CONNECT라고 보냄
		sprintf_s(ioBuf, "CONNECT");
		int retval = sendto(m_Socket, ioBuf, strlen(ioBuf), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) ;
		if (retval == SOCKET_ERROR)
		{
			MessageBox(NULL, L"ERROR: first sendto(CONNECT)", L"ERROR", MB_OK) ;
			return false ;
		}

		m_PeerAddrLen = sizeof(m_PeerAddrIn) ;

		// 2. 데이터 받기
		retval = recvfrom(m_Socket, ioBuf, BUF_SIZE, 0, (SOCKADDR*)&m_PeerAddrIn, &m_PeerAddrLen) ;
		if (retval == SOCKET_ERROR)
		{
			MessageBox(NULL, L"ERROR: recvfrom(SUCCESS)", L"ERROR", MB_OK) ;
			return false ;
		}

		// 3. SUCCESS 라고 받기
		if (!strncmp(ioBuf, "SUCCESS", 7))
		{
			m_IsPeerLinked = true ;
		}

		//////////////////////////////////////////////////////////////////////////
		// 1 -> 2 -> 3 악수 성공!

		else
		{
			MessageBox(NULL, L"ERROR: INVALID SERVER!!", L"ERROR", MB_OK) ;
			return false ;
		}
	}

	return true ;
}

bool NetHelper::SendKeyStatus(const PacketKeyStatus& sendKeys)
{
	/// SEND
	int retval = sendto(m_Socket, (char*)&sendKeys, sizeof(PacketKeyStatus), 0, (SOCKADDR*)&m_PeerAddrIn, sizeof(m_PeerAddrIn)) ;
	if (retval == SOCKET_ERROR)
	{
		MessageBox(NULL, L"ERROR: sendto() in LOOP!", L"ERROR", MB_OK) ;
		return false ;
	}
	
	return true ;
}

bool NetHelper::RecvKeyStatus(OUT PacketKeyStatus& recvKeys)
{
	/// RECEIVE
	int retval = recvfrom(m_Socket, (char*)&recvKeys, sizeof(PacketKeyStatus), 0, (SOCKADDR*)&m_PeerAddrIn, &m_PeerAddrLen) ;
	if (retval == SOCKET_ERROR)
	{
		MessageBox(NULL, L"ERROR: recvfrom() in LOOP!", L"ERROR", MB_OK) ;
		return false ;
	}

	return true ;
}
