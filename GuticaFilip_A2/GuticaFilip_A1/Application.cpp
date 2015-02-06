/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	Application.c - Simple, menu driven program to resolve user entered
--									Host, ip address, Service, or port info
--									This File represents the main UI implementation of
--									this application.
--
--	PROGRAM:		Network_Resolver
--
--	FUNCTIONS:		WinMain
--					WndProc
--					InstantiateWindow
--					InstantiateUI
--					UpdateUI
--					Resolve
--					ClearText
--
--	DATE:			January 14, 2015
--
--	REVISIONS:		(Date and Description)
--
--	DESIGNERS:		Filip Gutica
--
--	PROGRAMMER:		Filip Gutica
--
--	NOTES:
--     
---------------------------------------------------------------------------------------*/
#define STRICT
#define _CRT_SECURE_NO_WARNINGS


#include "Application.h"

#pragma warning (disable: 4096)

BOOL isConnected;
int clientWidth;
int clientHeight;
/*------------------------------------------------------------------------------
--	FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
--
--	PURPOSE: Main entry point for program. Start application, dispatch all window messages.
--
--	PARAMETERS:
--		hInst			- this program's instance
--		hPrevInstance	- previous instance of this application 
--		lspszCmdParam	- command line parameters
--		nCmdShow		- show the window
--
--	RETURN:
--		1 for success
--		0 for failure to start application
--
--	DESIGNERS:		Filip Gutica
--
--	PROGRAMMER:		Filip Gutica
--
/*-----------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance, LPSTR lspszCmdParam, int nCmdShow)
{
	MSG Msg;

	InstantiateWindow(hInst);

	ShowWindow(hwnd, nCmdShow);

	UpdateWindow(hwnd);
	
	PopulateUIElements();

	UIControl();

	while (GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	

	return Msg.wParam;
}

/*------------------------------------------------------------------------------
--	FUNCTION: DTWndProc(HWND, UINT, WPARAM, LPARAM)
--
--	PURPOSE: Process the messages generated by the window.
--
--	PARAMETERS:
--		hwnd	- window handle
--		Message	- window message
--		wParam	- window message parameter
--		lParam	- window message parameter
--
--	RETURN:
--		If the message was processed, the return value is 0
--		If the message was not processed and passed to DefWindowProc
--		and the return value depends on the value of that function.
--
--	DESIGNERS:		Filip Gutica
--
--	PROGRAMMER:		Filip Gutica
--
/*-----------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message,
	WPARAM wParam, LPARAM lParam)
{
	
	HMENU hMenu;
	PAINTSTRUCT paintstruct;
	
	switch (Message)
	{
		case WM_CREATE:
			hdc = GetDC(hwnd);
			
			ReleaseDC(hwnd, hdc);
			break;
		case WM_COMMAND:
			CheckMenu(wParam);

			break;
		case WM_CONNECTED:
			isConnected = TRUE;
			break;
		case WM_DISCONNECT:
			isConnected = FALSE;
			break;
		case WM_DESTROY:
			StopServer();
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}


/*------------------------------------------------------------------------------
--	FUNCTION: CheckMenu(WPARAM wP)
--
--	PURPOSE:	Check what menu items the user selected.
--				Set the mode, update the UI accordingly
--
--	PARAMETERS:
--		wP			- Windows message parameter.
--
--	DESIGNERS:		Filip Gutica
--
--	PROGRAMMER:		Filip Gutica
/*-----------------------------------------------------------------------------*/
void CheckMenu(WPARAM wP)
{
	HMENU hMenu = GetMenu(hwnd);

	switch (LOWORD(wP))
	{
	case IDM_OP1:
		mode = IDM_SERVER;
		StartServer(result);
		UpdateUI(mode);
		//SetWindowText(result, "");
		break;
	case IDM_OP2:
		mode = IDM_CLIENT;
		UpdateUI(mode);
		StopServer();
		//SetWindowText(result, "");
		break;
	case IDM_HELP:

			ShellExecute(
				NULL, 
				"open" , 
				"excel" , 
				"output.csv", 
				NULL,
				SW_SHOW);

			MessageBox(NULL, "Set your port configuration before connecting.",
								"Help", MB_OK);
		break;
	case IDM_BTN:

		Resolve(mode);
		
		break;	

	}
}

/*------------------------------------------------------------------------------
--	FUNCTION: UpdateUI(int m)
--
--	PURPOSE:	Update the UI according to what mode the application is in
--
--	PARAMETERS:
--		m			- the Mode.
--
--	DESIGNERS:		Filip Gutica
--
--	PROGRAMMER:		Filip Gutica
/*-----------------------------------------------------------------------------*/
void UpdateUI(int m)
{
	switch(m)
	{
	case IDM_SERVER:
		ServerUIstate();
		break;
	case IDM_CLIENT:
		ClientUIstate();
		break;
	}
}

/*------------------------------------------------------------------------------
--	FUNCTION: InstantiateWindow(HINSTANCE hInst)
--
--	PURPOSE: Create and display window. 
--
--	PARAMETERS:
--		hInst		- HINSTANCE for this program
--
--	DESIGNERS:		Filip Gutica
--
--	PROGRAMMER:		Filip Gutica
/*-----------------------------------------------------------------------------*/
void InstantiateWindow(HINSTANCE hInst)
{
	WNDCLASSEX Wcl;
	char Name[] = "Windows Network Resolver";

	Wcl.cbSize = sizeof(WNDCLASSEX);
	Wcl.style = CS_HREDRAW | CS_VREDRAW;
	Wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION);	// large icon 
	Wcl.hIconSm = NULL;								// use small version of large icon
	Wcl.hCursor = LoadCursor(NULL, IDC_ARROW);		// cursor style

	Wcl.lpfnWndProc = WndProc;
	Wcl.hInstance = hInst;
	Wcl.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH); //white background
	Wcl.lpszClassName = Name;

	Wcl.lpszMenuName = "MYMENU";	// The menu Class
	Wcl.cbClsExtra = 0;				// no extra memory needed
	Wcl.cbWndExtra = 0;

	if (!RegisterClassEx(&Wcl))
		return;

	hwnd = CreateWindow(Name, Name, WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU ,
		50, 50, 700, 500, NULL, NULL, hInst, NULL);

	InstantiateUI();

	//memset(buffer, 0, sizeof(buffer));
}

/*------------------------------------------------------------------------------
--	FUNCTION: InstantiateUI()
--
--	PURPOSE: Create UI elements
--
--	PARAMETERS:
--		void		
--
--	DESIGNERS:		Filip Gutica
--
--	PROGRAMMER:		Filip Gutica
/*-----------------------------------------------------------------------------*/
void InstantiateUI()
{
	RECT clientArea;

	int labelWidth = 95;
	int labelHeight = 20;

	int comboHeight = 100;

	int btnWidth = 120;
	int btnHeight = 40;

	int editCtrl_X = 110;

	int radio_X = 260;
	
	GetClientRect(hwnd, &clientArea);
	clientWidth = clientArea.right - clientArea.left;
	clientHeight = clientArea.bottom - clientArea.top;

	result = CreateTextBox(clientWidth/10, clientHeight * 3/5, 
		clientWidth * 4/5, clientHeight / 3, hwnd);

	labelIP = CreateLabel("IP Address ", 10, 10, labelWidth, labelHeight, hwnd); 

	editCtlIP = CreateEditCtrl("127.0.0.1", editCtrl_X, 10, 140, 25, hwnd);

	labelPort = CreateLabel("Port ", 10, 50, labelWidth, labelHeight, hwnd);

	editCtlPort = CreateEditCtrl("7000", editCtrl_X, 50, 140, 25, hwnd);

	radioTCP = CreateRadioBtn("TCP ", radio_X, 10, 75, 25,(HMENU) IDM_TCP, hwnd);

	radioUDP = CreateRadioBtn("UDP ", radio_X, 50, 75, 25, (HMENU) IDM_UDP, hwnd);

	labelSize = CreateLabel("Size (Bytes) ", 10, 90, labelWidth, labelHeight, hwnd);

	labaelNumTimes = CreateLabel("Times to send", 10, 130, labelWidth, labelHeight, hwnd); 

	dropDown1 = CreateDropeDownList(editCtrl_X, 90, 140, comboHeight, hwnd);

	dropDown2 = CreateDropeDownList(editCtrl_X, 130, 140, comboHeight, hwnd);

	btn = CreateBtn("Send ", 40, 180,
		btnWidth, btnHeight, (HMENU) IDM_BTN, hwnd);



	CommandUIstate();
}

/*------------------------------------------------------------------------------
--	FUNCTION: Resolve(int m)
--
--	PURPOSE:		Perform the appropriate resolution based on what mode we are in
--
--	PARAMETERS:
--		m			- The mode we are in		
--
--	DESIGNERS:		Filip Gutica
--
--	PROGRAMMER:		Filip Gutica
/*-----------------------------------------------------------------------------*/
void Resolve(int m)
{
	char strIP[BUFFER_SIZE];
	char strPort[BUFFER_SIZE];
	char strSize[BUFFER_SIZE];
	char strNumTimes[BUFFER_SIZE];
	char protocol[BUFFER_SIZE];

	switch(m)
	{
	case IDM_SERVER:
		GetWindowText(editCtlIP, strIP, sizeof(strIP));
		GetWindowText(editCtlPort, strPort, sizeof(strPort));
		break;
	case IDM_CLIENT:
		GetWindowText(editCtlIP, strIP, sizeof(strIP));
		GetWindowText(editCtlPort, strPort, sizeof(strPort));
		GetWindowText(dropDown1,strSize, sizeof(strSize));
		GetWindowText(dropDown2, strNumTimes, sizeof(strNumTimes));
		if (IsDlgButtonChecked(hwnd, IDM_TCP))
			sprintf(protocol, "tcp");
		else if (IsDlgButtonChecked(hwnd, IDM_UDP))
			sprintf(protocol, "udp");	


		int size = atoi(strSize);
		int times = atoi(strNumTimes);

	
		StartClient(strIP, strPort, size, times, protocol, hwnd, result);
		
		
		break;

	}
}

/*------------------------------------------------------------------------------
--	FUNCTION: ClearText()
--
--	PURPOSE:		Simple helper function to clear the text in the UI elements
--
--	PARAMETERS:
--		void					
--
--	DESIGNERS:		Filip Gutica
--
--	PROGRAMMER:		Filip Gutica
/*-----------------------------------------------------------------------------*/
void ClearText()
{
	SetWindowText(editCtlIP, "");
	SetWindowText(editCtlPort, "");
	SetWindowText(result, "");
}

void CommandUIstate()
{
	ShowWindow(result, FALSE);
	ShowWindow(editCtlIP, FALSE);
	ShowWindow(editCtlPort, FALSE);
	ShowWindow(btn, FALSE);
	ShowWindow(labelIP, FALSE);
	ShowWindow(labelPort, FALSE);
	ShowWindow(radioTCP, FALSE);
	ShowWindow(radioUDP, FALSE);
	ShowWindow(labelSize, FALSE);
	ShowWindow(labaelNumTimes, FALSE);
	ShowWindow(dropDown1, FALSE);
	ShowWindow(dropDown2, FALSE);

}

void ServerUIstate()
{
	MoveWindow(hwnd, 50, 50, 400, 200, 1);
	MoveWindow(result, 0, 0, 400, 200, 1);
	ShowWindow(result, TRUE);
	ShowWindow(editCtlIP, FALSE);
	ShowWindow(editCtlPort, FALSE);
	ShowWindow(btn, FALSE);
	ShowWindow(labelIP, FALSE);
	ShowWindow(labelPort, FALSE);
	ShowWindow(radioTCP, FALSE);
	ShowWindow(radioUDP, FALSE);
	ShowWindow(labelSize, FALSE);
	ShowWindow(labaelNumTimes, FALSE);
	ShowWindow(dropDown1, FALSE);
	ShowWindow(dropDown2, FALSE);
	
}

void ClientUIstate()
{	
	MoveWindow(hwnd, 50, 50, 700, 500, 1);
	MoveWindow(result, clientWidth/10, clientHeight * 3/5, clientWidth * 4/5, clientHeight / 3, 1);
	ShowWindow(editCtlIP, TRUE);
	ShowWindow(btn, TRUE);
	ShowWindow(labelIP, TRUE);
	ShowWindow(result, TRUE);
	ShowWindow(labelPort, TRUE);
	ShowWindow(editCtlPort, TRUE);
	ShowWindow(radioTCP, TRUE);
	ShowWindow(radioUDP, TRUE);
	ShowWindow(labelSize, TRUE);
	ShowWindow(labaelNumTimes, TRUE);
	ShowWindow(dropDown1, TRUE);
	ShowWindow(dropDown2, TRUE);
}


void UIControl()
{
	SendMessage(editCtlIP, EM_SETLIMITTEXT, (WPARAM)BUFFER_SIZE - 1, (LPARAM)0);
	SendMessage(editCtlPort, EM_SETLIMITTEXT, (WPARAM)BUFFER_SIZE - 1, (LPARAM)0);
}

void PopulateUIElements()
{
	SendMessage(dropDown1,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) TEXT("1024"));
	SendMessage(dropDown1,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) TEXT("4096"));
	SendMessage(dropDown1,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) TEXT("20480"));
	SendMessage(dropDown1,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) TEXT("65535"));

	SendMessage(dropDown2,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) TEXT("1"));
	SendMessage(dropDown2,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) TEXT("10"));
	SendMessage(dropDown2,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) TEXT("100"));
	SendMessage(dropDown2,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) TEXT("1000"));

	SendMessage(dropDown1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	SendMessage(dropDown2, CB_SETCURSEL, (WPARAM)1, (LPARAM)0);
}