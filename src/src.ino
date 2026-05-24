#include "UCOP.h"
#include "UCOPConfig.h"
#include "UCOPData.h"

UCOP*       m_pUCOP       = nullptr;
UCOPConfig* m_pUCOPConfig = nullptr;
UCOPData    m_RequestData;
UCOPData    m_ReplyData;
uint8_t   m_MessageBuffer[80];

void setup ()
{
  Serial.begin (9600);
  EResult result;
  result = UCOPConfig::Create (true, true, false, 0x63691401, UCOP::EChecksumType::CRC8, m_pUCOPConfig);
  result = UCOPConfig::Create (0, m_pUCOPConfig);

  result = UCOP::Create (m_pUCOPConfig, m_pUCOP);
  Serial.println (UCOP::GetResultText (result));
}

void loop ()
{
  EResult result;

  uint16_t messageLength;
  bool messageTypeIsReply;
  uint16_t bufferStartIndex = 0;
  result = m_pUCOP->ComposeRequest (m_RequestData, m_MessageBuffer, sizeof (m_MessageBuffer), messageLength);
  result = m_pUCOP->ComposeReply   (m_ReplyData,   m_MessageBuffer, sizeof (m_MessageBuffer), messageLength);
  result = m_pUCOP->SearchMessage (m_MessageBuffer, sizeof (m_MessageBuffer), bufferStartIndex, m_ReplyData, messageTypeIsReply, messageLength);
  Serial.println (UCOP::GetResultText (result));
}
