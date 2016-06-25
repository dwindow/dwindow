#pragma once
#include "stdafx.h"

class ITrayMenuCallback
{
public:
	virtual HRESULT Click(int id)=0;
	virtual HRESULT BeforeShow()=0;
};

class TrayMenu
{
public:
	CMenu m_Menu;	// use this thing to config...
	TrayMenu(ITrayMenuCallback *cb, LPCTSTR tooltip);
	~TrayMenu();
protected:
	ITrayMenuCallback * m_cb;
	TCHAR m_tooltip[MAX_PATH];
	HWND m_hwnd;
	HANDLE m_window_thread;
	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static DWORD WINAPI WindowThread(LPVOID lpParame);
};