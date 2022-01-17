#undef UNICODE
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include "resource.h"
#include <iostream>
#include <commctrl.h>

bool Terminate = false;
HWND hMainWnd = 0;
HANDLE hClock = 0;
bool ClockPaused = false;
HANDLE handle;

BOOL CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, DLGPROC(MainWndProc));
	return 0;

}


DWORD WINAPI ClockThread(LPVOID lpParameter) {
	while (!Terminate) {
		char timestr[9];
		SYSTEMTIME time;
		GetLocalTime(&time);
		sprintf(timestr, "%.2d:%.2d:%.2d",
			time.wHour, time.wMinute, time.wSecond);
		SetDlgItemText(hMainWnd, IDC_CLOCK, timestr);
		Sleep(250);
	}
	return 0;
}


BOOL RunNotepad(HWND hWnd) { // ������ �������
	char processName[50]; // ��� ������ "Notepad.exe"
	int processNameLength = sizeof(processName);
	if (!GetDlgItemText(hWnd, IDC_COMMANDLINE, processName, processNameLength)) // ������ ��� �� �������� � ������ � ��������� �����
		return 0;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si)); // �������� ����� ������������� ��������
	si.cb = sizeof(si);


	if (!CreateProcess(
		NULL,
		processName, // ��������� ������
		NULL,
		NULL,
		FALSE,
		0,   // ������ ��������
		NULL,
		NULL,
		&si, // ���������� �������������
		&pi) // ���������� ��������
		)return 0;

	if (handle != 0) CloseHandle(handle);
	CloseHandle(pi.hThread);
	handle = pi.hProcess;
	return 1; // return true



}


bool BrowseFileName(HWND Wnd, char* FileName) {
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn); // ���������� ������ � ������ !!! 
	ofn.hwndOwner = Wnd;
	ofn.lpstrFilter = "Executable Files (*.exe)\0*.exe\0" // �������������� ������, ������� ��������� ������
		"All Files(*.*)\0 * .*\0"; //  ���������� ������ �������
	ofn.lpstrFile = FileName; // ��������� �� �����, ������� �������� ��� �����
	ofn.nMaxFile = MAX_PATH; // ������ ������
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt = "exe";

	return GetOpenFileName(&ofn); // ��������� ���������� ���� � ������� ��������� ����������� ����, ��� �����...
}

unsigned __int64 SystemTimeToInt(SYSTEMTIME Time) {
	FILETIME ft;
	SystemTimeToFileTime(&Time, &ft);
	ULARGE_INTEGER fti;
	fti.HighPart = ft.dwHighDateTime;
	fti.LowPart = ft.dwLowDateTime;
	return fti.QuadPart;
}




DWORD WINAPI ScheduleThread(LPVOID lpParameter) {
	SYSTEMTIME currentTime;
	SYSTEMTIME localtime;
	SendDlgItemMessage(hMainWnd, IDC_TIME, DTM_GETSYSTEMTIME, 0, (LPARAM)&localtime); // // �������� ������� ��������� ����� �� �������� ���������� ������ ���� � ������� (DTP) � �������� ��� � ��������� ��������� ���������� �������.
	while (true) {
		GetLocalTime(&currentTime);
		if (SystemTimeToInt(currentTime) >= SystemTimeToInt(localtime) || Terminate) {
			break;
		}
		Sleep(100);
	}
	char timestr[10];
	sprintf(timestr, "%.2d:%.2d:%.2d", localtime.wHour, localtime.wMinute, localtime.wSecond);
	LRESULT result = SendDlgItemMessage(hMainWnd, IDC_TIMELIST, LB_FINDSTRINGEXACT, 0, (LPARAM)timestr);

	if (result >= 0 && !Terminate) {
		RunNotepad(hMainWnd);
		SendDlgItemMessage(hMainWnd, IDC_TIMELIST, LB_DELETESTRING, 0, 0); // ������ ������ ������� ���� ��������
	}


	return 0;
}


// LB_FINDSTRINGEXACT - ������� ������ ������ ������, ������� ����� ������������� ��������� ������, �� ����������� ����, ��� ����� �� ������������ � ��������.


BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	switch (Msg) {
	case WM_INITDIALOG:
		hMainWnd = hWnd;
		hClock = CreateThread(NULL, 0, ClockThread, NULL, 0, NULL);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			DestroyWindow(hWnd);
			return TRUE;
		case IDC_PAUSE: {
			if (ClockPaused == false) {
				SuspendThread(hClock); // ���������������� ����� ��������� ������
				ClockPaused = true;
				return TRUE;
			}
			if (ClockPaused == true) {
				ResumeThread(hClock); // ��������� ���� ������� ������������ ������ ������
				ClockPaused = false;
				return TRUE;
			}
		}
					  return TRUE;

		case IDC_ADD:
			SYSTEMTIME localtime;
			SendDlgItemMessage(hMainWnd, IDC_TIME, DTM_GETSYSTEMTIME, 0, (LPARAM)&localtime);
			SYSTEMTIME currentTime;
			GetLocalTime(&currentTime);
			if (SystemTimeToInt(localtime) > SystemTimeToInt(currentTime)) {
				char timestr[9];
				sprintf(timestr, "%.2d:%.2d:%.2d", localtime.wHour, localtime.wMinute, localtime.wSecond);
				SendDlgItemMessage(hMainWnd, IDC_TIMELIST, LB_ADDSTRING, 0, (LPARAM)&timestr); // ��������� ������ � ���� ������.
				CreateThread(NULL, 0, ScheduleThread, NULL, 0, NULL);
			}
			return TRUE;

		case IDC_DELETE:
			double position;
			position = SendDlgItemMessage(hMainWnd, IDC_TIMELIST, LB_GETCURSEL, 0, 0);//�������� ������ � ��� ������������� ������������ �� � ���� ������.
			SendDlgItemMessage(hMainWnd, IDC_TIMELIST, LB_DELETESTRING, 0, 0); //  ����� ������� ����� ������, � ������ ��������� ��������� �� ����� ��������� ������.
			return TRUE;

		case IDC_BROWSE:
			char filename[MAX_PATH] = "notepad.exe";
			if (BrowseFileName(hWnd, filename)) SetDlgItemText(hWnd, IDC_COMMANDLINE, filename);
			return TRUE;
		}
		return FALSE;


	case WM_DESTROY:
		Terminate = true;
		Sleep(500);
		CloseHandle(hClock);
		PostQuitMessage(0);
		return TRUE;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return TRUE;

	}
	return FALSE;
}
