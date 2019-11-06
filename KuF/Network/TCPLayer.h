// TCPLayer.h: interface for the TCPLayer class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "OS_depend.h"

class TCPLayer  
{
  public:
    TCPLayer();
    virtual ~TCPLayer();

    void setServerPort(unsigned short portnum) { s_port = portnum; }
    unsigned short getServerPort() { return s_port; }

    // helper function for connection test
    void setActiveConnect(const char *TCPHost);
    bool isActiveConnect() { return (TCPHost != 0); }

    // activate server ports
    bool activate();

    // deactivate server ports
    bool shutdown();

    // disconnect all protocol partners
    virtual bool forceDisconnect(SOCKET sock = INVALID_SOCKET);

    // telegram received - buffer belongs to layer 1
    virtual void telegramCB(const char *buf, unsigned len);

    // new connection detected
    virtual void newConnectCB(const char *hostname, unsigned short port);
    
    // lost connection detected
    virtual void connectLostCB();

    // the working thread
    void workProc();

    bool isConnectPending() { return connectPending; } 
    bool isConnected()    { return isActive; } 
    bool isServerActive() { return (! ISSOCKETINVALID(TCP_SSock)); }

    void setEnabled(bool enabled);
    bool isEnabled() { return enabled; }

    const char *const getConnectedHost() { return hostname; }

    static int getLastSocketError();
    // set PartnerSock blocking mode
    bool setBlocking(bool doBlock);

  protected:

    SOCKET makeTCPSocket(unsigned short int port);
    bool TCPConnect();

    void readFromClient();
    bool writeToClient(const char *buf, unsigned len);
    
  private:
    unsigned short s_port;
    unsigned short d_port;

    fd_set active_fd_set;
    fd_set read_fd_set;
  
    bool connectPending;
    fd_set write_fd_set;  // for async connect
    //  fd_set error_fd_set;
    
    bool isActive;

    bool enabled;

    unsigned long connCount;    // for connect timeout

    char *TCPHost;

    // the telegram buffer
#define TELEGRAMBUFLEN 256
    char telegramBuf[TELEGRAMBUFLEN];

    SOCKET TCP_SSock;
    SOCKET PartnerSock;
    char hostname[256];
};

