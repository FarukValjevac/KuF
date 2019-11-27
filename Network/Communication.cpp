#include "Communication.h"

#include <future>
#include <memory>
#include <iostream>
#include <string>
#include <exception>

using namespace std;

void WorkThreadWrapper(Communication *ptr)
{
  ptr->workFunc();
}

Communication::Communication(std::shared_ptr<CommCallbacks> cb) : m_cb(cb)
{
	for (int i = 1; i <= 4; i++) {
		DMXSpotlight sl(i);
		SLList.push_back(sl);
	}
	
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
		if (IsServer()) {
			WriteToPartner(WELCOMEMESSAGE, 0);
		}
      }
      else if (tel.m_flag == CONNECTIONLOST)
      {
        m_cb->ConnectionLost();
      }
      else
      {
        m_cb->DataReceived(tel.m_msg.c_str(), tel.m_msg.length());
		if (IsServer()) {
			//process the new Data
			string message = tel.m_msg.c_str();
			if (message.empty()) {
				WriteToPartner("No command received", 0);
				return true;
			}
			//delete the $ from the message
			if (message.back() != '$') {
				WriteToPartner(message.append("\nMissing end of command sign ($)").c_str(), 0);
				return true;
			}
			message.pop_back();

			string command = "";
			char* pMessage = (char*)tel.m_msg.c_str();
			cout << "message is: " << message << std::endl;

			command = strtok(pMessage, " $");
			cout << "command is: " << command << std::endl;

			switch (resolve(command)) {
			case SL: {
				//check if the parameters are valid
				try {
					string str_number = strtok(NULL, " ");
					int number = stoi(str_number);
					if (0 < number && number < 5) {
						string rgb = strtok(NULL, " ");
						int r = stoi(rgb);

						if (0 <= r && r <= 255) {
							rgb = strtok(NULL, " ");
							int g = stoi(rgb);

							if (0 <= g && g <= 255) {
								rgb = strtok(NULL, " ");
								int b = stoi(rgb);

								if (0 <= b && b <= 255) {
									string str_brt = strtok(NULL, "$");
									int brt = stoi(str_brt);

									if (0 <= brt && brt <= 100) {
										//write parameters into DMX class
										cout << number << r << g << b << brt << endl;
										executeCMD(number, r, g, b, brt);
										//activate all selected spotlights
										processCMD();
										WriteToPartner(message.append(" #103$").c_str(), 0);

										break;
									}
								}
							}
						}
					}
				}
				catch (exception & e) {
					cout << e.what() << endl;

				}
				WriteToPartner(message.append(" #212$").c_str(), 0);
				break;
			}
			case STATUS: {
					   string str_status = getStatus();
					   WriteToPartner(message.append(" #104$").c_str(), 0);
					   WriteToPartner(str_status.c_str(), 0);
					   break;
			}
			case QUIT:
				WriteToPartner(message.append(" #199$").c_str(), 0);
				break;
			default:
				WriteToPartner(message.append(" #211$").c_str(), 0);
				break;
			}

		}
		else {
			cout << "Answer of Server is: " << tel.m_msg << std::endl;
		}
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
  ProcessMessage();
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
  ProcessMessage();
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
  ProcessMessage();
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

void Communication::executeCMD(int nbr, int r, int g, int b, int brt) {
	SLList[nbr-1].setSLValues(r, g, b, brt);
}

void Communication::processCMD() {
	int channels[48];
	for (int i = 0; i < 4; i++) {
		if (this->SLList[i].selected == false) {
			continue;
		}
		channels[0] = 1 + i * 24;
		channels[1] = (uint8_t)(SLList[i].brightness * 2.25);
		channels[2] = 3 + i * 24;
		channels[3] = SLList[i].red;
		channels[4] = 4 + i * 24;
		channels[5] = SLList[i].green;
		channels[6] = 5 + i * 24;
		channels[7] = SLList[i].blue;

		if (!myDll.SetChannelValue(channels, 4)) {
			cerr << "error setting values" << endl;
		}
		Sleep(10);

	}
}


const char* Communication::getStatus() {
	char buf[300] = "Hallo";
	for (int i = 0; i < 4; i++) {
		if (this->SLList[i].selected == false) {
			continue;
		}
		snprintf(buf, sizeof(buf),"Spotlight %d color %d %d %d brightness %d\n",  SLList[i].SLNumber, SLList[i].red, SLList[i].green, SLList[i].blue, SLList[i].brightness);
		string buf1 = buf;
		status.append(buf1);
		
	}
	if (status == "STATUS: \n") {
		return "No Spotlights are selected";
	}
	return status.c_str() ;
}

void Communication::DLLInit() {
	myDll.Init();
	//cout << myDll.GetMaxChannels() << endl;
}

command resolve(string x) {
	if(x == "SL"){
		return SL;
	}
	if (x == "STATUS") {
		return STATUS;
	}
	if (x == "QUIT") {
		return QUIT;
	}
	return INVALID;
}

DMXSpotlight::DMXSpotlight(int nbr) {
	SLNumber = nbr;
	red = 255;
	green = 255;
	blue = 255;
	brightness = 50;
	selected = false;
}

DMXSpotlight::~DMXSpotlight() {

}

void DMXSpotlight::setSLValues(int r, int g, int b, int brt) {
	red = r;
	green = g;
	blue = b;
	brightness = brt;
	if (selected == false) {
		selected = true;
	}
}