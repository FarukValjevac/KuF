#include <iostream>
#include <time.h>

#include "Communication.h"

void server(unsigned short port);
void client(const char *hostname, unsigned short port);

using namespace std;

#define TIMEOUTMINUTES 5

void main(int argc, char *argv[])
{
  unsigned short serverPort = 6666;
  const char *clientname = nullptr;

  // expect client to connect to as argument
  // if no arg start as server
  if (argc > 1)
  {
    clientname = argv[1];
  }

  if (clientname)
    client(clientname, serverPort);
  else
    server(serverPort);
}

// ------------------------------------------------------------------

class CallbackHandler : public CommCallbacks
{
public:
  // got connection from host / port
  virtual void NewConnectionCB(const char *hostname)
  {
    std::cout << "got new connection from " << hostname << std::endl;
  }

  virtual void ConnectionLost()
  {
    std::cout << "connection lost - type anything to continue..." << std::endl;
  }

  virtual void DataReceived(const char *data, unsigned len)
  {
	 // string command = "";
	 // char* message = (char*)data;
	 // cout << "message is: " << message << std::endl;

	 //command = strtok(message, " ");
	 //cout << "command is: " << command << std::endl;

  //  // here we rely on the fact that the data is a string!
  //  cout << "got new data >>" << data << "<< len " << len << std::endl;
  }
};

// -------------------------------------------------------------------

void server(unsigned short port)
{
  std::shared_ptr<CommCallbacks> myCallbackHandler(new CallbackHandler());
  Communication myComm(myCallbackHandler);
  myComm.name = "server";

  myComm.Activate(port);

  std::cout << "server started at port " << port << std::endl;

  myComm.DLLInit();

  do {
	  while (myComm.ProcessMessage()) {

	  }
 
	  if (myComm.timeout != NULL) {
		  //cout << "timeout: " << time(0) - myComm.timeout << endl;
		  if ((time(0) - myComm.timeout) >= 60 * TIMEOUTMINUTES) {
			  myComm.writeToClient("Time is up, disconnecting!$", 28);
			  myComm.forceDisconnect(myComm.getPartnerSocket());
			  myComm.resetSL();
			  myComm.client_connection = false;
		  }
	  }

  } while (myComm.getRunningStatus());

  myComm.Deactivate();
}

void client(const char *hostname, unsigned short port)
{
  std::shared_ptr<CommCallbacks> myCallbackHandler(new CallbackHandler());
  Communication myComm(myCallbackHandler);
  myComm.name = "client";

  std::cout << "Welcome! \nType connect to start and disconnect to finish.\nType ENDE to close the program." << std::endl;

  char inputstr[100];

  while (1) {
	  memset(inputstr, 0, sizeof(inputstr));
	  while (strcmp(inputstr, "connect")) {
		  std::cout << "Not connected! " << std::endl;
		  memset(inputstr, 0, sizeof(inputstr));
		  std::cin.getline(inputstr, 100);
		  if (!strcmp(inputstr, "ENDE")) {
			  myComm.Deactivate();
			  return;
		  }
	  }


	  bool res = myComm.Connect(hostname, port);

	  std::cout << "client started for server " << hostname << " at port " << port << "result " << res << std::endl;
	  if (res)
	  {
		  do {
			  if (!myComm.IsConnected()) {
				  break;
			  }
			  memset(inputstr, 0, sizeof(inputstr));
			  std::cin.getline(inputstr, 100);

			  if (!strcmp(inputstr, "disconnect")) {
				  break;
			  }

			  if (myComm.IsConnected()) {
				  std::cout << "sending \n>> " << inputstr << std::endl;
			  }

			  myComm.WriteToPartner(inputstr, strlen(inputstr) + 1);

			  myComm.ProcessMessage();
			  if (!strcmp(inputstr, "ENDE")) {
				  myComm.Deactivate();
				  return;
			  }


		  } while (1);
		  res = myComm.Disconnect();
		  std::cout << "client disconnected from server " << hostname << " at port " << port << " result " << res << std::endl;
	  }
  }
  myComm.Deactivate();
}

