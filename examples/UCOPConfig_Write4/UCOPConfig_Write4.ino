#include <Streaming.h>

#include <UCOPConfig.h>

const uint16_t c_EepromAddress = 0;

const bool      m_DeviceIdsUsed = true;
const bool      m_MessageIdUsed = true;
const bool      m_TimestampUsed = false;
const uint32_t  m_DeviceId      = 0x63691404;

void setup ()
{
  Serial.begin (115200);
  delay (2000);

  UCOPConfig* pUCOPConfig1 = nullptr;
  EResult result = UCOPConfig::Create ( m_DeviceIdsUsed,
                                        m_MessageIdUsed,
                                        m_TimestampUsed,
                                        m_DeviceId,
                                        pUCOPConfig1);
  Serial << "UCOPConfig.Create(..data..), Result: " << (int)result << " = " << UCOP::GetResultText (result) << endl;
  if (result != EResult::SUCCESS)
    return;

  pUCOPConfig1->Print ();

  result = pUCOPConfig1->WriteToEEPROM (c_EepromAddress);
  Serial << "UCOPConfig.WriteToEEPROM, Result: " << (int)result << " = " << UCOP::GetResultText (result) << endl;
  if (result != EResult::SUCCESS)
    return;

  UCOPConfig* pUCOPConfig2 = nullptr;
  result = UCOPConfig::Create (c_EepromAddress, pUCOPConfig2);
  Serial << "UCOPConfig.Create(..eepromAddress..), Result: " << (int)result << " = " << UCOP::GetResultText (result) << endl;
  if (result == EResult::SUCCESS)
    pUCOPConfig2->Print ();
}

void loop ()
{
}
