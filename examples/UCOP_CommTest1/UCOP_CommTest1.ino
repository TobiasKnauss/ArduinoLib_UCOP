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
ByteBuffer* m_pPayloadSendBuffer;
ByteBuffer* m_pPayloadRecvBuffer;

uint16_t messageLength = 0;

FastCRC16 m_Crc16;

//--------------------------------------------------------------------
void setup ()
{
  Serial.begin (115200);
  delay (2000);

  bool isSuccess = ByteBuffer::Create (80, 0xFF, true, m_pReceiveBuffer);
  Serial << "ByteBuffer.Create(80, m_pReceiveBuffer) Result: " << isSuccess << endl;
  isSuccess = ByteBuffer::Create (50, 0xFF, false, m_pSendBuffer);
  Serial << "ByteBuffer.Create(50, m_pSendBuffer) Result: " << isSuccess << endl;
  isSuccess = ByteBuffer::Create (20, 0xFF, false, m_pPayloadSendBuffer);
  Serial << "ByteBuffer.Create(20, m_PayloadSendBuffer) Result: " << isSuccess << endl;
  isSuccess = ByteBuffer::Create (20, 0xFF, false, m_pPayloadRecvBuffer);
  Serial << "ByteBuffer.Create(20, m_PayloadRecvBuffer) Result: " << isSuccess << endl;

  EResult result;
  byte payloadLength = 6;
  char text[10] = "Hello!";
  byte textLength = 10;
  m_pPayloadSendBuffer->WriteBytesAndMovePtr (textLength, (uint8_t*)text, false);

  Serial << F("SendBuffer Len=") << m_pSendBuffer->get_Length ()    << endl;
  m_pSendBuffer->Print (&Serial, true);
  Serial << F("RecvBuffer Len=") << m_pReceiveBuffer->get_Length () << endl;
  m_pReceiveBuffer->Print (&Serial, true);
  Serial << F("PayloadSendBuffer Len=") << m_pPayloadSendBuffer->get_Length () << endl;
  m_pPayloadSendBuffer->Print (&Serial, true);
  Serial << F("PayloadRecvBuffer Len=") << m_pPayloadRecvBuffer->get_Length () << endl;
  m_pPayloadRecvBuffer->Print (&Serial, true);

  result = UCOPConfig::Create (true, true, false, 101, m_pUCOPConfig);
  Serial << F("UCOPConfig.Create() Result: ") << (int)result << " = " << UCOP::GetResultText (result) << endl;
  result = UCOP::Create (m_pUCOPConfig, m_pUCOP);
  Serial << F("UCOP.Create() Result: ") << (int)result << " = " << UCOP::GetResultText (result) << endl;

  uint16_t messageLength = 0;
  UCOPData ucopData = UCOPData (UCOP::ACTION_Read, 258, 42);
  ucopData.SetPayloadInfo (m_pPayloadSendBuffer, payloadLength);
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

  UCOPData receivedData;
  receivedData.SetPayloadInfo (m_pPayloadRecvBuffer);
  bool     receivedMessageTypeIsReply  = false;
  uint16_t receivedMessageLength       = 0;

  do
  {
    result = m_pUCOP->SearchMessage (m_pReceiveBuffer,
                                     receivedData,
                                     receivedMessageTypeIsReply,
                                     receivedMessageLength);
    Serial << F("UCOP.SearchMessage() result = ") << (int)result << " = " << UCOP::GetResultText (result) << endl;

    Serial << F("Message Type is REPLY: ") << receivedMessageTypeIsReply          << endl;
    Serial << F("Action is WRITE:       ") << receivedData.ActionIsWrite          << endl;
    Serial << F("Remote Device Id:      ") << _HEX8 (receivedData.RemoteDeviceId) << endl;
    Serial << F("Message Id:            ") << receivedData.MessageId              << endl;
    Serial << F("Timestamp:             ") << receivedData.Timestamp              << endl;
    Serial << F("CommandId:             ") << receivedData.CommandId              << endl;
    Serial << F("Result:                ") << (uint8_t)receivedData.MessageResult << endl;
    Serial << F("Payload Data Length:   ") << receivedData.PayloadLength          << endl;
    Serial << F("Message Length:        ") << receivedMessageLength               << endl;

    Serial << F("Payload Data: ") << endl;
    m_pPayloadRecvBuffer->Print (&Serial, true);
    m_pReceiveBuffer->Print (&Serial, true);
  }
  while (result == ::EResult::SUCCESS);
}

void loop ()
{
}
