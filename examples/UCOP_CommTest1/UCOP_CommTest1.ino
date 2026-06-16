#include <Arduino.h>
#include <Streaming.h>

#include <UCOP.h>
#include <UCOPConfig.h>
#include <UCOPData.h>
#include <MemoryTools_Memory.h>

using namespace MemoryTools;

UCOP*       m_pUCOP       = nullptr;
UCOPConfig* m_pUCOPConfig = nullptr;

ByteBuffer* m_pReceiveBuffer;
ByteBuffer* m_pSendBuffer;
uint8_t m_PayloadSendBuffer[20];
uint8_t m_PayloadRecvBuffer[20];

uint16_t messageLength = 0;

FastCRC16 m_Crc16;

//--------------------------------------------------------------------
void setup ()
{
  Serial.begin (115200);
  delay (2000);

  bool isSuccess = ByteBuffer::Create (80, 0xFF, true, m_pReceiveBuffer);
  Serial << "ByteBuffer.Create(80, m_pReceiveBuffer) Result: " << isSuccess << endl;
  isSuccess = ByteBuffer::Create (80, 0xFF, false, m_pSendBuffer);
  Serial << "ByteBuffer.Create(80, m_pSendBuffer) Result: " << isSuccess << endl;

  memset (m_PayloadSendBuffer, 0xFF, sizeof (m_PayloadSendBuffer));
  memset (m_PayloadRecvBuffer, 0xFF, sizeof (m_PayloadRecvBuffer));

  EResult result;
  byte payloadLength = 6;
  char text[10] = "Hello!";
  byte textLength = 10;
  memcpy (&m_PayloadSendBuffer, &text, textLength);

  Serial << F("SendBuffer Len=") << m_pSendBuffer->get_Length ()    << endl;
  m_pSendBuffer->Print (&Serial, true);
  Serial << F("RecvBuffer Len=") << m_pReceiveBuffer->get_Length () << endl;
  m_pReceiveBuffer->Print (&Serial, true);

  result = UCOPConfig::Create (true, true, false, 101, m_pUCOPConfig);
  Serial << F("UCOPConfig.Create() Result: ") << (int)result << " = " << UCOP::GetResultText (result) << endl;
  result = UCOP::Create (m_pUCOPConfig, m_pUCOP);
  Serial << F("UCOP.Create() Result: ") << (int)result << " = " << UCOP::GetResultText (result) << endl;

  uint16_t messageLength = 0;
  UCOPData ucopData = UCOPData (UCOP::ACTION_Read, 258, 42);
  ucopData.SetPayloadInfo (m_PayloadSendBuffer, sizeof (m_PayloadSendBuffer), payloadLength);
  result = m_pUCOP->ComposeRequest (ucopData, m_pSendBuffer, messageLength);
  Serial << "UCOP.ComposeRequest() Result: " << (int)result << " = " << UCOP::GetResultText (result) << ", MsgLen=" << messageLength << endl;
  m_pSendBuffer->Print (&Serial, true);

  Serial.println ("create data for receiver...");
  m_pReceiveBuffer->WriteBytesAndMovePtr (3, m_pSendBuffer->get_pData () + 0, false);
  m_pReceiveBuffer->WriteBytesAndMovePtr (4, m_pSendBuffer->get_pData () + 7, false);
  m_pReceiveBuffer->WriteBytesAndMovePtr (4, m_pSendBuffer->get_pData () + 3, false);
  m_pReceiveBuffer->WriteBytesAndMovePtr (messageLength - 14, m_pSendBuffer->get_pData () + 11, false);
  m_pReceiveBuffer->Print (&Serial, true);

  Serial.println ("Recalc checksum...");
  uint16_t checksum = m_Crc16.modbus (m_pReceiveBuffer->get_pData () + 1, messageLength - 1 - 2 - 1);
  m_pReceiveBuffer->WriteValueAndMovePtr (checksum, true);
  m_pReceiveBuffer->WriteBytesAndMovePtr (1, m_pSendBuffer->get_pData () + messageLength - 1, false);
  m_pReceiveBuffer->Print (&Serial, true);

  Serial.println ("move and duplicate...");
  memmove (m_pReceiveBuffer->get_pData () + 20, m_pReceiveBuffer->get_pData (), messageLength);
  memset (m_pReceiveBuffer->get_pData (), 0xFF, 20);
  byte msgWrap = 15;
  memcpy (m_pReceiveBuffer->get_pData () + m_pReceiveBuffer->get_Length () - msgWrap, m_pReceiveBuffer->get_pData () + 20, msgWrap);
  memcpy (m_pReceiveBuffer->get_pData (), m_pReceiveBuffer->get_pData () + 35, messageLength - msgWrap);
  m_pReceiveBuffer->Print (&Serial, true);

  UCOPData recvData;
  recvData.SetPayloadInfo (m_PayloadRecvBuffer, sizeof (m_PayloadRecvBuffer));
  bool     recvMessageTypeIsReply  = false;
  uint16_t recvMessageLength       = 0;

  do
  {
    result = m_pUCOP->SearchMessage (m_pReceiveBuffer,
                                     recvData,
                                     recvMessageTypeIsReply,
                                     recvMessageLength);
    Serial << "SearchMessage() Result: " << (int)result << " = " << UCOP::GetResultText (result) << endl;
    Serial << F("Message Type is REPLY: ") << (uint8_t)recvMessageTypeIsReply << endl;
    Serial << F("Action is WRITE:       ") << recvData.ActionIsWrite          << endl;
    Serial << F("Remote Device Id:      ") << recvData.RemoteDeviceId         << endl;
    Serial << F("Message Id:            ") << recvData.MessageId              << endl;
    Serial << F("Timestamp:             ") << recvData.Timestamp              << endl;
    Serial << F("CommandId:             ") << recvData.CommandId              << endl;
    Serial << F("Result:                ") << (uint8_t)recvData.MessageResult << endl;
    Serial << F("Payload Data Length:   ") << recvData.PayloadLength          << endl;
    Serial << F("Payload Data: ");
    Memory::PrintLn (m_PayloadRecvBuffer, recvData.PayloadLength);
    Serial << F("Message Length: ") << recvMessageLength      << endl;

    m_pReceiveBuffer->Print (&Serial, true);
  }
  while (result == ::EResult::SUCCESS);
}

void loop ()
{
}
