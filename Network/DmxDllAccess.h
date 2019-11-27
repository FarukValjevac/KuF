#pragma once

class DmxDllAccess
{
public:
  DmxDllAccess();
  ~DmxDllAccess();

  // return true if dll/functions found
  // exit on false
  bool Init();
  
  // if loaded set all channels back to 0
  bool ShutDown();

  // return value from interface
  const char *GetDMXInterface();

  // set a channel value, always provide id/value pairs
  // arraylength = 2*count
  // return false if no interface is available
  bool SetChannelValue( int *kvp, int count );

  // return number of channels
  int GetMaxChannels();

private:

  bool UpdateChannels();

  bool isInit;

  unsigned char *myLevels;
  int maxChannels;
};