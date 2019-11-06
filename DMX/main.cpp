#include "dmxdllaccess.h"

#include <iostream>

#include <Windows.h>

using namespace std;

int main()
{
  DmxDllAccess myDll;

  myDll.Init();

  int values[8];

  for (int i= 0; i < 255; ++i)
  {
	values[0] = 1;
	values[1] = 255;

	values[2] = 3;
	values[3] = i;

	values[4] = 4;
	values[5] = i;

  values[4] = 5;
  values[5] = i;
  
  if (! myDll.SetChannelValue(values, 4))
		cerr << "error setting values" << endl;

  Sleep(10);
  }

  values[0] = 1;
  values[1] = 0;

  values[2] = 2;
  values[3] = 0;

  values[4] = 3;
  values[5] = 0;

  values[4] = 4;
  values[5] = 0;

  if (!myDll.SetChannelValue(values, 4))
    cerr << "error setting values" << endl;

  myDll.ShutDown();

  return 0;
}