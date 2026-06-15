#ifndef UCOPConfig_h
#define UCOPConfig_h

#include <Result.h>

#include "UCOP.h"

//--------------------------------------------------------------------
class UCOPConfig
{
//==================== Fields ====================
private:
  //-------------------- static --------------------

  static const bool c_InvertByteOrder = false;

  //-------------------- instance --------------------

  uint32_t  m_DeviceId      = 0;
  bool      m_DeviceIdsUsed = false;
  bool      m_MessageIdUsed = false;
  bool      m_TimestampUsed = false;

//==================== Constructors ====================
public:
  //-------------------- static --------------------

  static ::EResult Create ( bool          i_DeviceIdsUsed,
                            bool          i_MessageIdUsed,
                            bool          i_TimestampUsed,
                            uint32_t      i_DeviceId,
                            UCOPConfig*&  o_pConfig);

  static ::EResult Create ( uint16_t      i_EepromAddress,
                            UCOPConfig*&  o_pConfig);

  //-------------------- instance --------------------

  UCOPConfig ();

  ~UCOPConfig ();

protected:
  //-------------------- instance --------------------

  UCOPConfig (bool      i_DeviceIdsUsed,
              bool      i_MessageIdUsed,
              bool      i_TimestampUsed,
              uint32_t  i_DeviceId);

//==================== Properties ====================
public:
  //-------------------- instance --------------------

  uint8_t get_EepromConfigDataSize ();
  uint8_t get_EepromConfigChecksumSize ();

  uint32_t  get_DeviceId ();
  bool      get_DeviceIdsUsed ();
  bool      get_MessageIdUsed ();
  bool      get_TimestampUsed ();

//==================== Public Methods ====================
public:
  //-------------------- instance --------------------

  void Print ();

  ::EResult WriteToEEPROM (uint16_t i_Address);

//==================== Protected Methods ====================
private:
  //-------------------- instance --------------------

  ::EResult ReadFromEEPROM (uint16_t i_Address);

  ::EResult ReadFromEEPROM_exec (uint16_t& io_Address);

  ::EResult Verify_exec ();

  ::EResult WriteToEEPROM_exec (uint16_t& io_Address);

};

#endif
