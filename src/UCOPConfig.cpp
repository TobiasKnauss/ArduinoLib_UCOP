#include <EEPROM.h>

#include "UCOPConfig.h"

//--------------------------------------------------------------------
::EResult UCOPConfig::Create (bool                i_DeviceIdsUsed,
                              bool                i_MessageIdUsed,
                              bool                i_TimestampUsed,
                              uint32_t            i_DeviceId,
                              UCOP::EChecksumType i_ChecksumType,
                              UCOPConfig*&        o_pConfig)
{
  o_pConfig = nullptr;
  UCOPConfig* pConfig = new UCOPConfig (i_DeviceIdsUsed,
                                        i_MessageIdUsed,
                                        i_TimestampUsed,
                                        i_DeviceId,
                                        i_ChecksumType);

  ::EResult result = pConfig->Verify_exec ();
  if (result != ::EResult::SUCCESS)
  {
    delete pConfig;
    return result;
  }

  o_pConfig = pConfig;
  return ::EResult::SUCCESS;
}

//--------------------------------------------------------------------
::EResult UCOPConfig::Create (uint16_t      i_EepromAddress,
                              UCOPConfig*&  o_pConfig)
{
  if (o_pConfig != nullptr)
    return ::EResult::FAIL_Pointer_IsNotZero;

  UCOPConfig* pConfig = new UCOPConfig ();

  uint16_t address = i_EepromAddress;
  ::EResult result = pConfig->ReadFromEEPROM_exec (address);
  if (result != ::EResult::SUCCESS)
  {
    delete pConfig;
    return result;
  }

  result = pConfig->Verify_exec ();
  if (result != ::EResult::SUCCESS)
  {
    delete pConfig;
    return result;
  }

  o_pConfig = pConfig;
  return ::EResult::SUCCESS;
}

//--------------------------------------------------------------------
UCOPConfig::UCOPConfig ()
{
}

//--------------------------------------------------------------------
UCOPConfig::~UCOPConfig ()
{
}

//--------------------------------------------------------------------
UCOPConfig::UCOPConfig (bool                i_DeviceIdsUsed,
                        bool                i_MessageIdUsed,
                        bool                i_TimestampUsed,
                        uint32_t            i_DeviceId,
                        UCOP::EChecksumType i_ChecksumType)
{
  m_DeviceIdsUsed = i_DeviceIdsUsed;
  m_MessageIdUsed = i_MessageIdUsed;
  m_TimestampUsed = i_TimestampUsed;
  m_DeviceId      = i_DeviceId;
  m_ChecksumType  = i_ChecksumType;

}

//--------------------------------------------------------------------
uint8_t UCOPConfig::get_EepromConfigDataSize ()
{
  return 8;
}

//--------------------------------------------------------------------
uint8_t UCOPConfig::get_EepromConfigChecksumSize ()
{
  return 2;
}

//--------------------------------------------------------------------
UCOP::EChecksumType UCOPConfig::get_ChecksumType ()
{
  return m_ChecksumType;
}

//--------------------------------------------------------------------
uint32_t UCOPConfig::get_DeviceId ()
{
  return m_DeviceId;
}

//--------------------------------------------------------------------
bool UCOPConfig::get_DeviceIdsUsed ()
{
  return m_DeviceIdsUsed;
}

//--------------------------------------------------------------------
bool UCOPConfig::get_MessageIdUsed ()
{
  return m_MessageIdUsed;
}

//--------------------------------------------------------------------
bool UCOPConfig::get_TimestampUsed ()
{
  return m_TimestampUsed;
}

//--------------------------------------------------------------------
void UCOPConfig::Print ()
{
  Serial << F("DeviceIdsUsed = ") << m_DeviceIdsUsed << endl;
  Serial << F("MessageIdUsed = ") << m_MessageIdUsed << endl;
  Serial << F("TimestampUsed = ") << m_TimestampUsed << endl;
  Serial << F("DeviceId      = ") << m_DeviceId << " = 0x" << _HEX8 (m_DeviceId) << endl;
  Serial << F("ChecksumType  = ") << (uint8_t)m_ChecksumType << endl;
}

//--------------------------------------------------------------------
::EResult UCOPConfig::WriteToEEPROM (uint16_t i_Address)
{
  uint8_t eepromConfigDataSize = get_EepromConfigDataSize ();
  uint8_t eepromConfigTotalSize = eepromConfigDataSize + get_EepromConfigChecksumSize ();
  if (eepromConfigTotalSize + i_Address > EEPROM.length ())
    return ::EResult::FAIL_EEPROM_IndexOutsideRange;

  uint16_t address = i_Address;
  ::EResult result = WriteToEEPROM_exec (address);
  if (result != ::EResult::SUCCESS)
    return result;

  uint16_t checksum = EEPROM_CalcChecksumCRC16 (i_Address, eepromConfigDataSize);
  EEPROM_SetValueAndMovePtr (address, checksum);

  return ::EResult::SUCCESS;
}

//--------------------------------------------------------------------
::EResult UCOPConfig::ReadFromEEPROM (uint16_t i_Address)
{
  uint8_t eepromConfigDataSize = get_EepromConfigDataSize ();
  uint8_t eepromConfigTotalSize = eepromConfigDataSize + get_EepromConfigChecksumSize ();
  if (eepromConfigTotalSize + i_Address > EEPROM.length ())
    return ::EResult::FAIL_EEPROM_IndexOutsideRange;

  uint16_t address = i_Address;
  ::EResult result = ReadFromEEPROM_exec (address);
  if (result != ::EResult::SUCCESS)
    return result;

  uint16_t checksumFromEEPROM = 0;
  EEPROM_GetValueAndMovePtr (address, checksumFromEEPROM);

  uint16_t checksum = EEPROM_CalcChecksumCRC16 (i_Address, eepromConfigDataSize);
  if (checksum != checksumFromEEPROM)
    return ::EResult::FAIL_Device_ConfigChecksumWrong;

  return ::EResult::SUCCESS;
}

//--------------------------------------------------------------------
::EResult UCOPConfig::ReadFromEEPROM_exec (uint16_t& io_Address)
{
  bool isOK = true;
  isOK &= EEPROM_GetValueAndMovePtr (io_Address, m_DeviceIdsUsed);
  isOK &= EEPROM_GetValueAndMovePtr (io_Address, m_MessageIdUsed);
  isOK &= EEPROM_GetValueAndMovePtr (io_Address, m_TimestampUsed);
  isOK &= EEPROM_GetValueAndMovePtr (io_Address, m_DeviceId);
  uint8_t checksumType = 0;
  isOK &= EEPROM_GetValueAndMovePtr (io_Address, checksumType);
  m_ChecksumType = (UCOP::EChecksumType)checksumType;
  if (!isOK)
    return ::EResult::FAIL_EEPROM_GetValue;

  return ::EResult::SUCCESS;
}

//--------------------------------------------------------------------
::EResult UCOPConfig::Verify_exec ()
{
  if (m_DeviceId == 0)
    return ::EResult::FAIL_Device_IdInvalid;
  if (!UCOP::IsChecksumValid (m_ChecksumType))
    return ::EResult::FAIL_Device_ConfigInvalid;

  return ::EResult::SUCCESS;
}

//--------------------------------------------------------------------
::EResult UCOPConfig::WriteToEEPROM_exec (uint16_t& io_Address)
{
  bool isOK = true;
  isOK &= EEPROM_SetValueAndMovePtr (io_Address, m_DeviceIdsUsed);
  isOK &= EEPROM_SetValueAndMovePtr (io_Address, m_MessageIdUsed);
  isOK &= EEPROM_SetValueAndMovePtr (io_Address, m_TimestampUsed);
  isOK &= EEPROM_SetValueAndMovePtr (io_Address, m_DeviceId);
  isOK &= EEPROM_SetValueAndMovePtr (io_Address, (uint8_t)m_ChecksumType);
  if (!isOK)
    return ::EResult::FAIL_EEPROM_SetValue;
  
  return ::EResult::SUCCESS;
}

