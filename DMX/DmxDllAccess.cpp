// #include "stdafx.h"

#include "DmxDllAccess.h"
#include <iostream>

#include <Windows.h>
HINSTANCE myDll = 0;


using namespace std;

// derfine the dll interface -----------------------
// typedefs
typedef const char * (__stdcall* DLL_GetDMXInterface_PTR)();
typedef bool         (__stdcall* DLL_SetLevel_PTR)(char *, int);
typedef int          (__stdcall* DLL_GetMaxChannels_PTR)();

// global pointers
DLL_GetDMXInterface_PTR pGetDMXInterface = 0;
DLL_SetLevel_PTR        pSetLevel = 0;
DLL_GetMaxChannels_PTR  pGetMaxChannels = 0;

// -------------------------------------------------

DmxDllAccess::DmxDllAccess() 
{
  isInit = false;
  myDll = 0;
  maxChannels = 0;
  myLevels = 0;
}

DmxDllAccess::~DmxDllAccess()
{
  delete myLevels;
}

// return true if dll/functions found
// exit on false
bool DmxDllAccess::Init()
{
  if (isInit) return true;

  myDll = LoadLibraryA("DMX510.dll");
  if (! myDll) 
  {
    cerr << "cannot locate dll DMX510.dll" << endl;
    return false;
  }

//  cout << "dll loaded" << endl;

  pGetDMXInterface = (DLL_GetDMXInterface_PTR) GetProcAddress(myDll, "GetDMXInterface");
  if (pGetDMXInterface == 0)
  {
    cerr << "cannot locate function GetDMXInterface" << endl;
    return false;
  }

//  cout << " successfully located function GetDMXInterface" << endl;

  pSetLevel = (DLL_SetLevel_PTR) GetProcAddress(myDll, "SetLevel");
  if (pSetLevel == 0)
  {
    cerr << "cannot locate function SetLevel" << endl;
    return false;
  }
  cout << " successfully located function SetLevel" << endl;

  pGetMaxChannels = (DLL_GetMaxChannels_PTR) GetProcAddress(myDll, "GetMaxChannels");
  if (pGetMaxChannels == 0)
  {
    cerr << "cannot locate function GetMaxChannels" << endl;
    return false;
  }
//  cout << " successfully located function GetMaxChannels" << endl;

  // check interface availability
  const char *txt = GetDMXInterface();
  if (! txt)
  {
    // no interface
    cerr << "no interface found" << endl;
    return false;
  }
//  cout << "found interface >" << txt << "<" << endl;

  // allocate space for levels
  maxChannels = GetMaxChannels();
  if (maxChannels == 0)
  {
    // no channels
    cerr << "no channels found" << endl;
    return false;
  }
//  cout << "found maxChannels >" << maxChannels << "<" << endl;
  
  myLevels = new unsigned char[maxChannels+10];   // +10 for safety

  // initialize all levels to 0
  memset(myLevels, 0, maxChannels+10);

  // & reset them
  UpdateChannels(); 

  isInit = true;
  return true;
}
  
// if loaded set all channels back to 0
bool DmxDllAccess::ShutDown()
{
  if (isInit)
  {
    // clear all channels
    // initialize all levels to 0
    memset(myLevels, 0, maxChannels+10);

    // & reset them
    UpdateChannels(); 
  }

  isInit = false;

  return true;
}

bool DmxDllAccess::UpdateChannels()
{
  bool res = false;

  if (myLevels && pSetLevel)
  {
    res = (*pSetLevel)((char *)myLevels, maxChannels-1);
  }

  return res;
}

// return value from interface
const char *DmxDllAccess::GetDMXInterface()
{
  const char *rval = 0;

  if (pGetDMXInterface)
  {
    rval = (*pGetDMXInterface)();
  }

  return rval;
}

// set a channel value, always provide id/value pairs
// arraylength = 2*count
bool DmxDllAccess::SetChannelValue( int *kvp, int count )
{
  for (int i=0; i<count; ++i)
  {
	// check channel id
    if (kvp[i*2] > 0 && kvp[2*i] < 511)
    {
      myLevels[kvp[2*i]-1] = (unsigned char)(kvp[2*i+1]);

//      cerr << "channel " << kvp[2*i] << " to value " << kvp[2*i+1] << endl;
    }
  }

  return UpdateChannels();
}

// return number of channels
int DmxDllAccess::GetMaxChannels()
{
  int channels = 0;

  if (pGetMaxChannels)
  {
    channels = (*pGetMaxChannels)();
  }

  return channels;
}
