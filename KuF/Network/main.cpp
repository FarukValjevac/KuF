#include <iostream>

#include "Communication.h"

void server(unsigned short port);
void client(const char *hostname, unsigned short port);

void main(int argc, char *argv[])
{
  unsigned short serverPort = 8000;
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
    std::cout << "connection lost" << std::endl;
  }

  virtual void DataReceived(const char *data, unsigned len)
  {
    // here we rely on the fact that the data is a string!
    std::cout << "got new data >>" << data << "<< len " << len << std::endl;
  }
};

// -------------------------------------------------------------------

void server(unsigned short port)
{
  std::shared_ptr<CommCallbacks> myCallbackHandler(new CallbackHandler());
  Communication myComm(myCallbackHandler);

  myComm.Activate(port);

  std::cout << "server started at port " << port << std::endl;

  char inputstr[100];

  do {
    memset(inputstr, 0, sizeof(inputstr));
    std::cin >> inputstr;

    std::cout << "sending >>" << inputstr << std::endl;

    myComm.WriteToPartner(inputstr, strlen(inputstr) + 1);

    myComm.ProcessMessage();
    myComm.ProcessMessage();

  } while (strcmp(inputstr, "ENDE"));

  myComm.Deactivate();
}

void client(const char *hostname, unsigned short port)
{
  std::shared_ptr<CommCallbacks> myCallbackHandler(new CallbackHandler());
  Communication myComm(myCallbackHandler);

  bool res = myComm.Connect(hostname, port);

  std::cout << "client started for server " << hostname << " at port " << port << "result " << res << std::endl;
  if (res)
  {
    char inputstr[100];

    do {
      memset(inputstr, 0, sizeof(inputstr));
      std::cin >> inputstr;

      std::cout << "sending >>" << inputstr << std::endl;

      myComm.WriteToPartner(inputstr, strlen(inputstr) + 1);

      myComm.ProcessMessage();
      myComm.ProcessMessage();

    } while (strcmp(inputstr, "ENDE"));
  }
  myComm.Disconnect();
}

