#include "Server.h"



DWORD EventTotal = 0;
WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];
LPSOCKET_INFORMATION SocketArray[WSA_MAXIMUM_WAIT_EVENTS];
CRITICAL_SECTION CriticalSection;   
SOCKET ListenSocket, AcceptSocket, ControlSocket;
HANDLE hThrdIO, hThrdListen;

int TotalBytes;


void StartServer(HWND h)
{
   WSADATA wsaData;
   SOCKADDR_IN InternetAddr;
   DWORD Flags;
   DWORD IOThreadId;
   DWORD ListenThreadId;
   DWORD RecvBytes;
   INT Ret;

   InitializeCriticalSection(&CriticalSection);

   if ((Ret = WSAStartup(0x0202,&wsaData)) != 0)
   {
      printf("WSAStartup failed with error %d\n", Ret);
      WSACleanup();
      return;
   }

   if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 
      WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) 
   {
      printf("Failed to get a socket %d\n", WSAGetLastError());
      return;
   }

   InternetAddr.sin_family = AF_INET;
   InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   InternetAddr.sin_port = htons(PORT);

   if (bind(ListenSocket, (PSOCKADDR) &InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
   {
      printf("bind() failed with error %d\n", WSAGetLastError());
      return;
   }

   if (listen(ListenSocket, 5))
   {
      printf("listen() failed with error %d\n", WSAGetLastError());
      return;
   }

   // Setup the listening socket for connections.

   if ((AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,
      WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) 
   {
      printf("Failed to get a socket %d\n", WSAGetLastError());
      return;
   }

   if ((EventArray[0] = WSACreateEvent()) == WSA_INVALID_EVENT)
   {
      printf("WSACreateEvent failed with error %d\n", WSAGetLastError());
      return;
   }

   // Create a thread to service overlapped requests

   if ((hThrdIO = CreateThread(NULL, 0, ProcessIO, (LPVOID)h, 0, &IOThreadId)) == NULL)
   {
      printf("CreateThread failed with error %d\n", GetLastError());
      return;
   } 

   //Create a thread to Listen for incomming connections

    if ((hThrdListen = CreateThread(NULL, 0, ListenThread, (LPVOID)h, 0, &ListenThreadId)) == NULL)
   {
      printf("CreateThread failed with error %d\n", GetLastError());
      return;
   } 

   EventTotal = 1;
}

void StopServer()
{
	if (closesocket(ListenSocket) == SOCKET_ERROR)
		return;
	if (closesocket(AcceptSocket) == SOCKET_ERROR)
		return;

	
	TerminateThread(hThrdListen, 0);
	TerminateThread(hThrdIO, 0);
}

DWORD WINAPI ListenThread(LPVOID lpParameter)
{
	DWORD Flags;
	DWORD RecvBytes;
	HWND hwnd = (HWND)lpParameter;
	char temp[64];

	 while(TRUE)
   {
       // Accept inbound connections

      if ((AcceptSocket = accept(ListenSocket, NULL, NULL)) == INVALID_SOCKET)
      {
          printf("accept failed with error %d\n", WSAGetLastError());
          return 0;
      }

      EnterCriticalSection(&CriticalSection);

      // Create a socket information structure to associate with the accepted socket.

      if ((SocketArray[EventTotal] = (LPSOCKET_INFORMATION) GlobalAlloc(GPTR,
         sizeof(SOCKET_INFORMATION))) == NULL)
      {
         printf("GlobalAlloc() failed with error %d\n", GetLastError());
         return 0;
      } 

	   sprintf(temp, "Accepted connection: %d", AcceptSocket);
	   SetWindowText(hwnd,temp);

      // Fill in the details of our accepted socket.
      SocketArray[EventTotal]->Socket = AcceptSocket;
      ZeroMemory(&(SocketArray[EventTotal]->Overlapped), sizeof(OVERLAPPED));
      SocketArray[EventTotal]->BytesSEND = 0;
      SocketArray[EventTotal]->BytesRECV = 0;
      SocketArray[EventTotal]->DataBuf.len = DATA_BUFSIZE;
      SocketArray[EventTotal]->DataBuf.buf = SocketArray[EventTotal]->Buffer;

      if ((SocketArray[EventTotal]->Overlapped.hEvent = EventArray[EventTotal] = 
          WSACreateEvent()) == WSA_INVALID_EVENT)
      {
         printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
         return 0;
      }

      // Post a WSARecv request to to begin receiving data on the socket

      Flags = 0;
      if (WSARecv(SocketArray[EventTotal]->Socket, 
         &(SocketArray[EventTotal]->DataBuf), 1, &RecvBytes, &Flags,
         &(SocketArray[EventTotal]->Overlapped), NULL) == SOCKET_ERROR)
      {
         if (WSAGetLastError() != ERROR_IO_PENDING)
         {
            printf("WSARecv() failed with error %d\n", WSAGetLastError());
            return 0;
         }
      }


      EventTotal++;

      LeaveCriticalSection(&CriticalSection);

      //
      // Signal the first event in the event array to tell the worker thread to
      // service an additional event in the event array
      //
      if (WSASetEvent(EventArray[0]) == FALSE)
      {
         printf("WSASetEvent failed with error %d\n", WSAGetLastError());
         return 0;
      }
   }

}


DWORD WINAPI ProcessIO(LPVOID lpParameter)
{
	DWORD Index;
	DWORD Flags;
	LPSOCKET_INFORMATION SI;
	DWORD BytesTransferred;
	DWORD i;
	int count = 1;
	DWORD RecvBytes = 0;
	DWORD SendBytes;
	char temp[64];
	char bytes_to_receive[64];
	std::string bigBuff;
	std::string received;
	std::string result;
	HWND hwnd = (HWND)lpParameter;
	std::vector<std::string> infoVector;
	BOOL lock = false;
  
   // Process asynchronous WSASend, WSARecv requests.

	while(TRUE)
	{

		if ((Index = WSAWaitForMultipleEvents(EventTotal, EventArray, FALSE,
			WSA_INFINITE, FALSE)) == WSA_WAIT_FAILED)
		{
			printf("WSAWaitForMultipleEvents failed %d\n", WSAGetLastError());
			return 0;
		} 

		// If the event triggered was zero then a connection attempt was made
		// on our listening socket.
 
		if ((Index - WSA_WAIT_EVENT_0) == 0)
		{
			WSAResetEvent(EventArray[0]);
			continue;
		}

		SI = SocketArray[Index - WSA_WAIT_EVENT_0];
		ControlSocket = SocketArray[1]->Socket;
	
		WSAResetEvent(EventArray[Index - WSA_WAIT_EVENT_0]);
		
		if (WSAGetOverlappedResult(SI->Socket, &(SI->Overlapped), &BytesTransferred,
			FALSE, &Flags) == FALSE || BytesTransferred == 0)
		{
			sprintf(temp, "\nClosing socket %d\n", SI->Socket);
			SetWindowText(hwnd, temp);
			
			TotalBytes = 0;

			if (closesocket(SI->Socket) == SOCKET_ERROR)
			{
				printf("closesocket() failed with error %d\n", WSAGetLastError());
			}

			GlobalFree(SI);
			WSACloseEvent(EventArray[Index - WSA_WAIT_EVENT_0]);

			// Cleanup SocketArray and EventArray by removing the socket event handle
			// and socket information structure if they are not at the end of the
			// arrays.

			EnterCriticalSection(&CriticalSection);

			if ((Index - WSA_WAIT_EVENT_0) + 1 != EventTotal)
				for (i = Index - WSA_WAIT_EVENT_0; i < EventTotal; i++)
				{
					EventArray[i] = EventArray[i + 1];
					SocketArray[i] = SocketArray[i + 1];
				}

			EventTotal--;

			LeaveCriticalSection(&CriticalSection);

			continue;
		}

		// Check to see if the BytesRECV field equals zero. If this is so, then
		// this means a WSARecv call just completed so update the BytesRECV field
		// with the BytesTransferred value from the completed WSARecv() call.

		
		if (SI->BytesRECV == 0)
		{
			SI->BytesRECV = BytesTransferred;
	
			SI->BytesSEND = 0;

			//Don't cont bytes sent on the control channel
			if (SI->Socket != ControlSocket)
				TotalBytes += (SI->BytesRECV);

			sprintf(temp, "Received bytes: %d", TotalBytes);
			infoVector.push_back(temp);

		
			sprintf(temp, "\nReceived: %s", SI->Buffer);
			infoVector.push_back(temp);

			sprintf(bytes_to_receive, "%s", SI->Buffer);
			
			
			bigBuff += SI->Buffer;
			

			// Post another WSASend() request.
			// Since WSASend() is not gauranteed to send all of the bytes requested,
			// continue posting WSASend() calls until all received bytes are sent.

			ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));
			SI->Overlapped.hEvent = EventArray[Index - WSA_WAIT_EVENT_0];
			
		

			if (strcmp(SI->Buffer, "FIN") == 0)
			{
				char c[64] = "\0";
				sprintf(c, "%d", TotalBytes);
				SI->DataBuf.buf = c;
				SI->DataBuf.len = strlen(c);

				sprintf(temp, "\nSending: %s\n", SI->DataBuf.buf);
				infoVector.push_back(temp);

				if (WSASend(SI->Socket, &(SI->DataBuf), 1, &SendBytes, 0,
					&(SI->Overlapped), NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != ERROR_IO_PENDING)
					{
						sprintf(temp, "WSASend() failed with error %d\n", WSAGetLastError());
						SetWindowText(hwnd, temp);
						return 0;
					}
				}
			}
			else
			{
				char *c = ACK;
				SI->DataBuf.buf = c;
				SI->DataBuf.len = strlen(c);

				sprintf(temp, "\nSending: %s\n", SI->DataBuf.buf);
				infoVector.push_back(temp);

				if (WSASend(SI->Socket, &(SI->DataBuf), 1, &SendBytes, 0,
					&(SI->Overlapped), NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != ERROR_IO_PENDING)
					{
						sprintf(temp, "WSASend() failed with error %d\n", WSAGetLastError());
						SetWindowText(hwnd, temp);
						return 0;
					}
				}
			}
			

		}
		else
		{
			
			SI->BytesSEND += BytesTransferred;
		}

		SI->BytesRECV = 0;

		// Now that there are no more bytes to send post another WSARecv() request.

		Flags = 0;
		ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));
		SI->Overlapped.hEvent = EventArray[Index - WSA_WAIT_EVENT_0];

		SI->DataBuf.len = DATA_BUFSIZE;
		SI->DataBuf.buf = SI->Buffer;

		if (WSARecv(SI->Socket, &(SI->DataBuf), 1, &RecvBytes, &Flags,
			&(SI->Overlapped), NULL) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{

				sprintf(temp, "WSARecv() failed with error %d\n", WSAGetLastError());
				SetWindowText(hwnd, temp);
				return 0;
			}
			
		}

		

		if (!infoVector.empty())
		{
			for (int j = 0; j < infoVector.size(); j++)
			{
			
				result += infoVector[j];

				SetWindowText(hwnd, result.c_str());
			}
		}

		
		infoVector.clear();
		result = "";
	}
	
}

void WriteToSocket()
{

}