// Layer1.cpp: implementation of the Layer1 class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <iostream>

#include "TCPLayer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TCPLayer::TCPLayer()
{
    s_port = 0;
    d_port = 0;

    connectPending = false;
    isActive = false;

    connCount = 0;

    TCPHost = 0;
    enabled = true;

    memset(telegramBuf,0,sizeof(telegramBuf));

    TCP_SSock = INVALID_SOCKET;
    PartnerSock = INVALID_SOCKET;

    memset(hostname, 0, sizeof(hostname));

    FD_ZERO(&active_fd_set);
    FD_ZERO(&read_fd_set);
    FD_ZERO(&write_fd_set);
}

TCPLayer::~TCPLayer()
{
    shutdown();
}

void TCPLayer::setActiveConnect(const char *newHost)
{
    delete [] TCPHost;
    TCPHost = 0;

    if (newHost && strlen(newHost))
    {
        TCPHost = new char[strlen(newHost)+1];
        strcpy(TCPHost,newHost);
    }
}

void TCPLayer::setEnabled(bool en)
{ 
    // "changing connection settings on port %d to %s",(int)s_port,((en)?"enabled":"disabled"));

    enabled = en; 
    if (isConnected()) 
    {
        forceDisconnect(); 
    }
}

// activate server port
bool TCPLayer::activate()
{
    bool res = false;

    if (! enabled)
    {
        //  "layer1 on port %d is %s -  do not activate",(int)s_port,((enabled)?"enabled":"disabled"));
        return false;
    }

    FD_ZERO(&active_fd_set);

    // if we should start server 
    if (TCPHost == 0) 
    {
        //  "init TCP server port %d",(int)s_port);
        if (s_port)
        {
            TCP_SSock = makeTCPSocket(s_port);
            if (ISSOCKETINVALID(TCP_SSock))
            {
                // "cannot create socket for TCP server");
            }
            else
            {
                if (listen(TCP_SSock,1) < 0)
                {
                    // error on listen
                    // "error on listen port %d",(int)s_port);
                }
                else
                {
                    FD_SET(TCP_SSock,&active_fd_set);
                    std::cout << "TCP server running on port " << (int)s_port << std::endl;
                    res = true;
                }
            }
        }
        else
        {
            //"TCP server not started, invalid port %d",(int)s_port);
        }
    }
    else        // start client instead
    {
        if (s_port) 
        {
            // try connection
            res = TCPConnect();
            if (res) 
            {
                if (! connectPending)
                {
                    // "TCP client connected to host %s, port %d",TCPHost, (int)s_port);

                    // tell management layer
                    newConnectCB(TCPHost, s_port);
                }
                isActive = true;
            }
        }
        else
        {
            // "TCP server not started, invalid port %d",(int)s_port);
        }
    }
    return res;
}

// deactivate server ports
bool TCPLayer::shutdown()
{
    // shutdown partner
    if (! (ISSOCKETINVALID(PartnerSock)) )
    {
        forceDisconnect(PartnerSock);
    }

    // shutdown ssock
    if (! (ISSOCKETINVALID(TCP_SSock)) )
    {
        forceDisconnect(TCP_SSock);
        TCP_SSock = INVALID_SOCKET;
    }

    isActive = false;
    return true;
}

// disconnect all protocol partners
bool TCPLayer::forceDisconnect(SOCKET sock)
{
    if ( ISSOCKETINVALID(sock)  || sock == PartnerSock)
    {
        if (! (ISSOCKETINVALID(PartnerSock)) )
        {
            // remove socket from active list
            FD_CLR(PartnerSock, &active_fd_set);
            
            //"forced connection close to protocol partner");
            ::shutdown(PartnerSock,SD_BOTH);
            OS_closeSocket(PartnerSock);

            PartnerSock = INVALID_SOCKET;
            isActive = false;

            connectLostCB();

            // restart server
            activate();

            return true;
        }
    }
    else
    {
        // remove socket from active list
        FD_CLR(sock, &active_fd_set);
        
        // disconnect this sock
        // "forced connection close to socket %d",(int)sock);
        ::shutdown(sock,SD_BOTH);
        OS_closeSocket(sock);

        // do not restart server - this is a server shutdown
        // isActive = false;

        return true;
    }

    return false;
}

// telegram received - buffer belongs to layer 1
void TCPLayer::telegramCB(const char * /*buf*/, unsigned /* len */)
{
    // handle layer 2 fsm
}

// new connection detected
void TCPLayer::newConnectCB(const char * /*hostname*/, unsigned short port)
{
    // init connect proc
}

// lost connection detected
void TCPLayer::connectLostCB()
{
}

// the working thread
void TCPLayer::workProc()
{
    static struct timeval tmv = {0, 5000};      // 5 ms

    connCount ++;

    // if we have no socket skip this
    if (ISSOCKETINVALID(TCP_SSock) && ISSOCKETINVALID(PartnerSock)) return;

    if (connectPending)
    {
        FD_ZERO(&read_fd_set);
        write_fd_set = active_fd_set;
//      error_fd_set = active_fd_set;

        if (connCount > 100)
        {
            connCount = 0;
            //"connect timeout");
            forceDisconnect(PartnerSock);
            connectPending = false;
        }
    }
    else
    {
        read_fd_set = active_fd_set;
        FD_ZERO(&write_fd_set);
//      error_fd_set = active_fd_set;
    }
    
	if (select(FD_SETSIZE,&read_fd_set, &write_fd_set, 0, &tmv) >= 0)
    {
        // check pending connect
        if (connectPending)
        {
            if (FD_ISSET(PartnerSock, &write_fd_set))
            {
                // check for error
                int sockopt;
                int len = sizeof(sockopt);
                if (getsockopt(PartnerSock, SOL_SOCKET, SO_ERROR, (char *)&sockopt, (socklen_t *)&len) == SOCKET_ERROR)
                {
                    // abort connect
                    int status = getLastSocketError();
                    //"getsockopt socket error %d", status);
                    forceDisconnect(PartnerSock);
                    connectPending = false;
                }
                else
                {
                    if (sockopt == 0)  // ok
                    {
                        FD_SET(PartnerSock, &active_fd_set);
                        connectPending = false;

                        //"TCP client connected to host %s",getConnectedHost());
                        // tell management layer
                        newConnectCB(getConnectedHost(), s_port);
                    }
                    else
                    {
                        // error
                        int status = getLastSocketError();
                        if (status != WSAENOTSOCK) // this happens when connect is aborted
                        {
                            //"write socket error %d", status);
                            forceDisconnect(PartnerSock);
                            connectPending = false;        // might also happen during connectPending
                        }
                    }
                }
            }
        }

        // check the set
        if (FD_ISSET(TCP_SSock, &read_fd_set))          // new protocol connection
        {
            //"new connection on TCP server");

            struct sockaddr_in client;
            size_t size = sizeof(client);
            SOCKET partner;

            partner = accept(TCP_SSock, (struct sockaddr *)&client, (socklen_t *)&size);
            if (ISSOCKETINVALID(partner))
            {
                int status = OS_returnLastError();
                std::cout << "accept error " << status << std::endl;
            }
            else
            {
                memset(hostname, 0, sizeof(hostname));
                sprintf(hostname, "%s:%u", inet_ntoa(client.sin_addr), ntohs(client.sin_port) );
                
                std::cout << "protocol connection requested from " << hostname << std::endl;

                if (ISSOCKETINVALID(PartnerSock))
                {
                    // accept new connection
                    PartnerSock = partner;
                    FD_SET(PartnerSock, &active_fd_set);

                    // shutdown server socket
                    forceDisconnect(TCP_SSock);
                    TCP_SSock = INVALID_SOCKET;

                    isActive = true;

                    // tell management layer
                    newConnectCB(hostname, ntohs(client.sin_port));
                }
                else
                {
                    // refuse due to other existing conn
                    OS_closeSocket(partner);
                    //"connection refused due to existing client connection");
                }
            }
        }

        else if (FD_ISSET(PartnerSock, &read_fd_set))
        {
           //"data avail on protocol line");

            readFromClient();
        }
    }
    else // select error
    {
        // socket error
        int status = getLastSocketError();
        if (status != WSAENOTSOCK)    // this happens when connect is aborted
        {
            //"select error %d", status);
            forceDisconnect(PartnerSock);
            connectPending = false;        // might also happen during connectPending
        }
    }
}

// open server socket
SOCKET TCPLayer::makeTCPSocket(unsigned short int port)
{
    SOCKET sock;
    struct sockaddr_in name;
    struct linger lin;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ISSOCKETINVALID(sock))
    {
        //"cannot create socket for TCP server");
        return INVALID_SOCKET;
    }

    // turn off SO_LINGER
    lin.l_onoff = 0;        
    lin.l_linger = 60;
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &lin, sizeof(lin));

    // enable reuse of socket
    int use = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&use, sizeof(use));

    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    if (0 > bind(sock, (struct sockaddr *)&name, sizeof(name)))
    {
        //"cannot bind to socket %d for TCP server", port);
        OS_closeSocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

bool TCPLayer::TCPConnect()
{
    if (TCPHost == 0)
    {
        //"invalid hostname for %s active connect",(alternate)?"alternate":"primary");
        return false;
    }

    // search for host address
    struct hostent *hostinfo = gethostbyname(TCPHost);

    if (hostinfo == 0)
    {
        //"unknown host %s",TCPHost);
        return false;
    }

    PartnerSock = socket(AF_INET, SOCK_STREAM, 0);
    if (ISSOCKETINVALID(PartnerSock)) 
    {
        //"cannot create socket for %s active connect");
        return false;
    }

    // use async sockets for connect
    // setBlocking(false);

    struct sockaddr_in partnerinfo;
    memset (&partnerinfo, 0, sizeof(partnerinfo));

    partnerinfo.sin_family = AF_INET;
    partnerinfo.sin_port = htons(s_port);

    partnerinfo.sin_addr = *(struct in_addr *)hostinfo->h_addr;

    // store for later use
    memset(hostname, 0, sizeof(hostname));
    sprintf(hostname, "%s:%u", inet_ntoa(partnerinfo.sin_addr), ntohs(partnerinfo.sin_port) );

    if (0 > connect(PartnerSock, (struct sockaddr *)&partnerinfo, sizeof(partnerinfo)))
    {
        int error = getLastSocketError();
        if (error == WSAEWOULDBLOCK || error == WSAEALREADY )
        {
            //"connect on socket %s still pending",hostname);
            connectPending = true;
            connCount = 0;    // reset connect counter
            FD_SET(PartnerSock, &active_fd_set);
            return true;
        }
        else
        {
          std::cout << "cannot connect to host " << TCPHost << " socket " << s_port << std::endl;
            OS_closeSocket(PartnerSock);
            PartnerSock = INVALID_SOCKET;
            return false;
        }
    }

    FD_SET(PartnerSock, &active_fd_set);
    return true;
}

void TCPLayer::readFromClient()
{
    SOCKET sock = PartnerSock;
    unsigned long telegramLen = 0;

    if (ISSOCKETINVALID(sock)) return;

    // get 1 telegram and send it to layer 2

    // clear buf
    memset(telegramBuf,0,sizeof(telegramBuf));

    int rc = OS_ioctl(sock, FIONREAD, &telegramLen);        // check for pending data on socket
    if (rc != 0)        // socket error
    {
        int error = getLastSocketError();
        if (error == WSAEWOULDBLOCK) return;  // this code is ok since we use async sockets

        // receive error - disconnect & report error
        //%d: telegram layer socket error %d", sock, error);
        forceDisconnect(sock);
        return;
    }

    memset(telegramBuf, 0, TELEGRAMBUFLEN);
    int rlen = recv(sock,telegramBuf, TELEGRAMBUFLEN,0);

    if (rlen <= 0)      // connection closed
    { 
      int err = getLastSocketError();
      if (err == WSAEWOULDBLOCK) return;

      //"%d: telegram layer 1 partner disconnect", sock);
      forceDisconnect(sock);
      return;
    }

    telegramCB(telegramBuf, rlen);
}

bool TCPLayer::writeToClient(const char *buf, unsigned len)
{
    bool res = false;

    SOCKET sock = PartnerSock;

    if (! (ISSOCKETINVALID(sock)) )
    {
        int slen = send(sock, buf, len, 0);
        if (slen == (int)len)
        {
            // ok
            res = true;
        }
        else
        {
            //"%d: send problem to protocol partner",(int)sock);
            forceDisconnect(sock);
        }
    }

    return res;
}

int TCPLayer::getLastSocketError()
{
  return OS_returnLastError();

  // errno on unix
}

bool TCPLayer::setBlocking(bool doBlock)
{
  if (! (ISSOCKETINVALID(PartnerSock)) )
  {
    u_long newMode = doBlock ? 0 : 1;
    if (OS_ioctl(PartnerSock, FIONBIO, &newMode) == SOCKET_ERROR) return false;
    // ioctl on unix

    return true;
  }
  else
    return false;
}
