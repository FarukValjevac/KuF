#include "Communication.h"

#include <future>
#include <memory>
#include <iostream>

void WorkThreadWrapper(Communication *ptr)
{
  ptr->workFunc();
}


Communication::Communication(std::shared_ptr<CommCallbacks> cb) : m_cb(cb)
{
  OS_comm_startup();
}

Communication::~Communication()
{
  m_cb = nullptr;
  OS_comm_shutdown();
}

bool Communication::Activate(unsigned short port)
{
  this->setServerPort(port);

  this->activate();

  // start worker
  keepRunning = true;
  workerThread = std::async(WorkThreadWrapper, this);

  return true;
}

bool Communication::Deactivate()
{
  keepRunning = false;
  workerThread.wait();

  this->shutdown();
  return true;
}

bool Communication::Connect(const char *host, unsigned short port) 
{
  this->setServerPort(port);
  this->setActiveConnect(host);

  bool res = this->activate();

  // start worker
  keepRunning = true;
  workerThread = std::async(WorkThreadWrapper, this);

  return res;
}

bool Communication::Disconnect() 
{
  keepRunning = false;
  workerThread.wait();

  this->shutdown();
  return true;
}

bool Communication::IsConnected() 
{
  return this->isConnected();
}

bool Communication::IsServer() 
{
  return !(this->isActiveConnect());
}

bool Communication::WriteToPartner(const char *buf, unsigned len)
{
  Telegram tel;
  if (buf)
  {
    tel.m_flag = DATA;
    tel.m_msg = buf;
  }

#ifdef SHOWMESSAGE
  std::cout << "sending >" << tel.m_msg << "<, flag " << tel.m_flag << std::endl;
#endif

  {
    std::lock_guard<std::mutex> guard(m_telegramListMutex);
    m_telegramList_Out.push_back(tel);
  }

  return true;
}

bool Communication::ProcessMessage() 
{
  // take one msg out of input queue & dispatch to cb interface
  Telegram tel;

  do {

    tel.m_flag = EMPTY;

    {
      std::lock_guard<std::mutex> guard(m_telegramListMutex);
      if (!m_telegramList_In.empty())
      {
        tel = m_telegramList_In.front();
        m_telegramList_In.pop_front();
      }
    }

    if (tel.m_flag == EMPTY) return false;

    if (m_cb)
    {
      if (tel.m_flag == NEWCONNECTION)
      {
        m_cb->NewConnectionCB(tel.m_msg.c_str());
      }
      else if (tel.m_flag == CONNECTIONLOST)
      {
        m_cb->ConnectionLost();
      }
      else
      {
        m_cb->DataReceived(tel.m_msg.c_str(), tel.m_msg.length());
      }
    }
  } while (tel.m_flag != EMPTY);

  return true;
}

void Communication::telegramCB(const char *buf, unsigned len) 
{
  Telegram tel;

  if (buf)
  {
    tel.m_flag = DATA;
    tel.m_msg = buf;
  }

#ifdef SHOWMESSAGE
  std::cout << "receiving >" << tel.m_msg << "<, flag " << tel.m_flag << std::endl;
#endif

  {
    std::lock_guard<std::mutex> guard(m_telegramListMutex);
    m_telegramList_In.push_back(tel);
  }
}

void Communication::newConnectCB(const char *hostname, unsigned short port)
{
  Telegram tel;

  tel.m_flag = NEWCONNECTION;
  tel.m_msg = hostname;

#ifdef SHOWMESSAGE
  std::cout << "new connection >" << tel.m_msg << "<, flag " << tel.m_flag << std::endl;
#endif

  {
    std::lock_guard<std::mutex> guard(m_telegramListMutex);
    m_telegramList_In.push_back(tel);
  }
}

void Communication::connectLostCB() 
{
  Telegram tel;
  tel.m_flag = CONNECTIONLOST;

#ifdef SHOWMESSAGE
  std::cout << "connection lost >" << tel.m_msg << "<, flag " << tel.m_flag << std::endl;
#endif
  {
    std::lock_guard<std::mutex> guard(m_telegramListMutex);
    m_telegramList_In.push_back(tel);
  }
}

void Communication::workFunc() 
{
  while (keepRunning)
  {
    // wait some time
    OS_Sleep(10);

    {
      // take one msg out of input queue & dispatch to cb interface
      Telegram tel;

      {
        std::lock_guard<std::mutex> guard(m_telegramListMutex);
        if (!m_telegramList_Out.empty())
        {
          tel = m_telegramList_Out.front();
          m_telegramList_Out.pop_front();
        }
      }

      if (isConnected())
      {
        if (tel.m_flag != EMPTY)
        {
          this->writeToClient(tel.m_msg.c_str(), tel.m_msg.length()+1);
        }

      }

      // check partner connection
      this->workProc();
    }
  }
}

