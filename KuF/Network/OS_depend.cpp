#include "OS_depend.h"

#ifdef _WIN32

#include <sys/types.h>
#include <sys/timeb.h>

// start OS specific communication layer
bool OS_comm_startup()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
     
    wVersionRequested = MAKEWORD( 2, 0 );
     
    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 ) 
    {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        return false;
    }

  return true;
}

// shut down OS specific communication layer
bool OS_comm_shutdown()
{
    WSACleanup( );

    return true;
}

int OS_returnLastError()
{
    return WSAGetLastError();
}

const char *OS_getErrorText(int error)
{
    const char *text = 0;
    text = strerror(error);
    return text;
}

void OS_Sleep(int msecs)
{
    Sleep(msecs);
}

int OS_closeSocket(SOCKET s)
{
    return closesocket(s);
}

// ioctl operation
int OS_ioctl(SOCKET s, long cmd, void *argp)
{
    return ioctlsocket(s, cmd, (u_long *)argp);
}

// get exact time
void OS_getExactTime(time_t &secs, unsigned short &msecs)
{
  struct _timeb ttime;

  _ftime(&ttime);

  secs  = ttime.time;
  msecs = ttime.millitm;
}

#else

#include <string.h>
#include <sys/time.h>

// start OS specific communication layer
bool OS_comm_startup(logger &mylog)
{
    // do nothing on linux
    return true;
}

// shut down OS specific communication layer
bool OS_comm_shutdown(logger &mylog)
{
    // do nothing on linux
    return true;
}

int OS_returnLastError()
{
    return errno;
}

const char *OS_getErrorText(int error)
{
    const char *text = 0;
    text = strerror(error);
    return text;
}

void OS_Sleep(int msecs)
{
    usleep(msecs*1000);
}

int OS_closeSocket(SOCKET s)
{
    return close(s);
}

// ioctl operation
int OS_ioctl(SOCKET s, long cmd, void *argp)
{
    return ioctl(s, cmd, argp);
}

void _beginthread(void * (*start_routine)(void *), unsigned /*stacksize*/, void *args)
{
    pthread_t thread;
    pthread_create(&thread, 0 /*attr*/, start_routine, args);
}

int WaitForSingleObject(pthread_mutex_t &mutex, int time)
{
    if ( time == INFINITE )
        return pthread_mutex_lock(&mutex);
    else
        return pthread_mutex_trylock(&mutex);
}

int ReleaseMutex(pthread_mutex_t &mutex)
{
    return pthread_mutex_unlock(&mutex);
}

int ReleaseSemaphore(pthread_mutex_t &mutex, int, void*)
{
    return pthread_mutex_unlock(&mutex);
}

// get exact time
void OS_getExactTime(time_t &secs, unsigned short &msecs)
{
  struct timeval ctime;
  struct timezone tzone;

  gettimeofday(&ctime,&tzone);

  secs  = ctime.tv_sec;
  msecs = ctime.tv_usec / 1000;
}

#endif