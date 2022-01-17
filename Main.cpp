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


BOOL RunNotepad(HWND hWnd) { // создаём процесс
	char processName[50]; // идёт вместо "Notepad.exe"
	int processNameLength = sizeof(processName);
	if (!GetDlgItemText(hWnd, IDC_COMMANDLINE, processName, processNameLength)) // читает что мы написали в строку и открывает прогу
		return 0;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si)); // избегаем любых нежелательных эффектов
	si.cb = sizeof(si);


	if (!CreateProcess(
		NULL,
		processName, // командная строка
		NULL,
		NULL,
		FALSE,
		0,   // флажки создание
		NULL,
		NULL,
		&si, // информация предустановки
		&pi) // информация процесса
		)return 0;

	if (handle != 0) CloseHandle(handle);
	CloseHandle(pi.hThread);
	handle = pi.hProcess;
	return 1; // return true



}


bool BrowseFileName(HWND Wnd, char* FileName) {
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn); // определяет строку в БАЙТАХ !!! 
	ofn.hwndOwner = Wnd;
	ofn.lpstrFilter = "Executable Files (*.exe)\0*.exe\0" // информационная строка, которая описывает фильтр
		"All Files(*.*)\0 * .*\0"; //  определяет модель фильтра
	ofn.lpstrFile = FileName; // Указатель на буфер, который содержит имя файла
	ofn.nMaxFile = MAX_PATH; // Размер буфера
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt = "exe";

	return GetOpenFileName(&ofn); // возращаем диалоговое окно в котором находится открываемый диск, имя файла...
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
	SendDlgItemMessage(hMainWnd, IDC_TIME, DTM_GETSYSTEMTIME, 0, (LPARAM)&localtime); // // Получает текущее выбранное время из элемента управления выбора даты и времени (DTP) и помещает его в указанную структуру СИСТЕМНОГО времени.
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
		SendDlgItemMessage(hMainWnd, IDC_TIMELIST, LB_DELETESTRING, 0, 0); // удаляю строки которые были выбранны
	}


	return 0;
}


// LB_FINDSTRINGEXACT - Находит первую строку списка, которая точно соответствует указанной строке, за исключением того, что поиск не чувствителен к регистру.


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
				SuspendThread(hClock); // приостонавливает время заданного потока
				ClockPaused = true;
				return TRUE;
			}
			if (ClockPaused == true) {
				ResumeThread(hClock); // уменьшает счет времени приостановки работы потока
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
				SendDlgItemMessage(hMainWnd, IDC_TIMELIST, LB_ADDSTRING, 0, (LPARAM)&timestr); // Добавляет строку в поле списка.
				CreateThread(NULL, 0, ScheduleThread, NULL, 0, NULL);
			}
			return TRUE;

		case IDC_DELETE:
			double position;
			position = SendDlgItemMessage(hMainWnd, IDC_TIMELIST, LB_GETCURSEL, 0, 0);//Выбирает строку и при необходимости прокручивает ее в поле зрения.
			SendDlgItemMessage(hMainWnd, IDC_TIMELIST, LB_DELETESTRING, 0, 0); //  Когда выбрана новая строка, в списке удаляется выделение из ранее выбранной строки.
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
