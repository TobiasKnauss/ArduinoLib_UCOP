#include "UCOP.h"
#include "UCOPConfig.h"
#include "UCOPData.h"

UCOP*       m_pUCOP       = nullptr;
UCOPConfig* m_pUCOPConfig = nullptr;
UCOPData    m_RequestData;
UCOPData    m_ReplyData;
ByteBuffer* m_pMessageBuffer = nullptr;
ByteBuffer* m_pPayloadBuffer = nullptr;

void setup ()
{
  Serial.begin (9600);
  EResult result;

  ByteBuffer::Create (80, 0xFF, false, m_pMessageBuffer);

  result = UCOPConfig::Create (true, true, false, 0x63691401, m_pUCOPConfig);
  result = UCOPConfig::Create (0, m_pUCOPConfig);

  result = UCOP::Create (m_pUCOPConfig, m_pUCOP);
  Serial.println (UCOP::GetResultText (result));

  m_RequestData.SetPayloadInfo (m_pPayloadBuffer);
}

void loop ()
{
  EResult result;

  uint16_t messageLength;
  bool messageTypeIsReply;
  result = m_pUCOP->ComposeRequest (m_RequestData, m_pMessageBuffer, messageLength);
  result = m_pUCOP->ComposeReply   (m_ReplyData,   m_pMessageBuffer, messageLength);
  result = m_pUCOP->SearchMessage (m_pMessageBuffer, m_ReplyData, messageTypeIsReply, messageLength);
  Serial.println (UCOP::GetResultText (result));
}
