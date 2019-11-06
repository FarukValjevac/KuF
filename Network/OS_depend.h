#pragma once

// this file covers OS dependencies

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#define FD_SETSIZE  1024
#include <winsock2.h>
#include <windows.h>

// SOCKET ist definiert
// INVALID_SOCKET ist definiert

typedef int socklen_t;

#define ISSOCKETINVALID(sock)  (sock == INVALID_SOCKET)

// threading
#include <process.h>

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <netdb.h>

#include <pthread.h>

#define SOCKET int 
#define INVALID_SOCKET (-1)

#define ISSOCKETINVALID(sock)  (sock < 0)
#define SOCKET_ERROR (-1)

// shutdown constants
#define SD_RECEIVE  SHUT_RD
#define SD_SEND     SHUT_WR
#define SD_BOTH     SHUT_RDWR

// socket errors
#define WSAEWOULDBLOCK  EINPROGRESS
#define WSAEALREADY     EALREADY
#define WSACONNREFUSED  ECONNREFUSED
#define WSATIMEDOUT     ETIMEDOUT
#define WSAEINTR        EINTR
#define WSAENOTSOCK     EBADF

#define CDECL

#define _MAX_PATH 1024

// windows comatible defines
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD)(b)) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)(((DWORD)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD)(w)) >> 8) & 0xff))

#define THREAD_FUNC void *
void _beginthread(void * (*start_routine)(void *), unsigned /*stacksize*/, void *args);

#define _endthread() return 0;

#define INFINITE -1
#define WAIT_OBJECT_0 EBUSY

int WaitForSingleObject(pthread_mutex_t &mutex, int time);
int ReleaseMutex(pthread_mutex_t &mutex);
int ReleaseSemaphore(pthread_mutex_t &mutex, int, void*);

//--------------------------------------------------------------------------------

#define WINAPI
#define LPTSTR char*
#define MAX_PATH PATH_MAX
#define S_OK 0

#endif

// start OS specific communication layer
// return true on success
bool OS_comm_startup();

// shut down OS specific communication layer
// return true on success
bool OS_comm_shutdown();

// return last error
int OS_returnLastError();

// return description for last error
const char *OS_getErrorText(int error);

// sleep function
void OS_Sleep(int msecs);

// close socket descriptor
int OS_closeSocket(SOCKET s);

// ioctl operation
int OS_ioctl(SOCKET s, long cmd, void *argp);

// get exact time
void OS_getExactTime(time_t &secs, unsigned short &msecs);
