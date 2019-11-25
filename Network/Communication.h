#pragma once

#include <memory>
#include <atomic>
#include <mutex>
#include <future>

#include <list>
#include <vector>
#include <string>

#include "TCPLayer.h"

class CommCallbacks
{
public:
  // got connection from host:port
  virtual void NewConnectionCB(const char *hostname) = 0;
  virtual void ConnectionLost() = 0;

  virtual void DataReceived(const char *data, unsigned len) = 0;
};

class DMXSpotlight
{
public:
	DMXSpotlight(int nbr);
	~DMXSpotlight();

	void setSLValues(int r, int g, int b, int brt);

private:
	int SLNumber;
	int red;
	int green;
	int blue;
	int brightness;
	bool selected;
};

// ------------------------------------------------------

#undef SHOWMESSAGE
#define WELCOMEMESSAGE "DMX-Remote for Spotlights.\nCurrent Version is 1.0.\nChoose your SL #101$"
// special cases coded in message
#define NEWCONNECTION (unsigned(-1))
#define CONNECTIONLOST (unsigned(-2))
#define EMPTY (unsigned(0))
#define DATA (unsigned(1))

struct Telegram
{
  Telegram() { m_flag = EMPTY; }
  ~Telegram() {}

  std::string m_msg;
  unsigned m_flag;
};

class Communication : private TCPLayer
{
public:
  Communication(std::shared_ptr<CommCallbacks> cb);
  ~Communication();

  bool Activate(unsigned short port);
  bool Deactivate();

  bool Connect(const char *host, unsigned short port);
  bool Disconnect();

  bool IsConnected();
  bool IsServer();

  bool WriteToPartner(const char *buf, unsigned len);
  bool ProcessMessage();

  void workFunc();

  void executeCMD(int nbr, int r, int g, int b, int brt);

protected:
  // telegram received - buffer belongs to layer 1
  virtual void telegramCB(const char *buf, unsigned len);

  // new connection detected
  virtual void newConnectCB(const char *hostname, unsigned short port);

  // lost connection detected
  virtual void connectLostCB();

private:
  std::shared_ptr<CommCallbacks> m_cb;

  std::atomic_bool keepRunning;

  std::list<Telegram> m_telegramList_In;
  std::list<Telegram> m_telegramList_Out;
  std::mutex m_telegramListMutex;

  std::future<void> workerThread;

  std::vector<DMXSpotlight> SLList;

};




enum command {
	INVALID,
	SL,
	STATUS,
	QUIT
};

command resolve(std::string x);
