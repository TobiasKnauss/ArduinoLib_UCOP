#include <Streaming.h>

#include <UCOPConfig.h>

const uint16_t c_EepromOffset = 0;

void setup ()
{
  Serial.begin (9600);
  delay (2000);

  UCOPConfig* pUCOPConfig = nullptr;
  EResult result = UCOPConfig::Create (c_EepromOffset, pUCOPConfig);
  Serial << "UCOPConfig.Create(..eepromAddress..), Result: " << (int)result << " = " << UCOP::GetResultText (result) << endl;
  if (result == EResult::SUCCESS)
    pUCOPConfig->Print ();
}

void loop ()
{
}
