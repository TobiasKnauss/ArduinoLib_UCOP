#include <Arduino.h>
#include <Streaming.h>

#include <UCOP.h>
#include <UCOPConfig.h>
#include <UCOPData.h>
#include <MemoryTools_Memory.h>

using namespace MemoryTools;

const uint16_t c_EepromAddress = 0;
const uint8_t  c_BufferDefaultValue = 0xFF;

const uint8_t  c_PinNumber_SendEnable     = 10;
const uint8_t  c_PinNumber_ReceiveDisable = 22;

UCOP*       m_pUCOP       = nullptr;
UCOPConfig* m_pUCOPConfig = nullptr;

ByteBuffer* m_pReceiveBuffer;
ByteBuffer* m_pSendBuffer;
ByteBuffer* m_pPayloadSendBuffer;
ByteBuffer* m_pPayloadRecvBuffer;

uint32_t  m_WorkerDeviceId = 0x63691402;
uint16_t  m_CommandId      = 0x1042;

uint8_t   m_PayloadLength = 0;
bool      m_DataAvailable = false;
UCOPData  m_RequestData;

//--------------------------------------------------------------------
void setup ()
{
  Serial.begin (115200);
  Serial1.begin (9600);
  delay (2000);

  pinMode (c_PinNumber_SendEnable,     OUTPUT);
  pinMode (c_PinNumber_ReceiveDisable, OUTPUT);
  digitalWrite (c_PinNumber_SendEnable,     LOW);
  digitalWrite (c_PinNumber_ReceiveDisable, LOW);

  bool isSuccess = ByteBuffer::Create (80, 0xFF, true, m_pReceiveBuffer);
  Serial << "ByteBuffer.Create(80, m_pReceiveBuffer) Result: " << isSuccess << endl;
  isSuccess = ByteBuffer::Create (50, 0xFF, false, m_pSendBuffer);
  Serial << "ByteBuffer.Create(50, m_pSendBuffer) Result: " << isSuccess << endl;
  isSuccess = ByteBuffer::Create (20, 0xFF, false, m_pPayloadSendBuffer);
  Serial << "ByteBuffer.Create(20, m_PayloadSendBuffer) Result: " << isSuccess << endl;
  isSuccess = ByteBuffer::Create (20, 0xFF, false, m_pPayloadRecvBuffer);
  Serial << "ByteBuffer.Create(20, m_PayloadRecvBuffer) Result: " << isSuccess << endl;

  EResult result;

  Serial << F("SendBuffer Len=") << m_pSendBuffer->get_Length ()    << endl;
  m_pSendBuffer->Print (&Serial, true);
  Serial << F("RecvBuffer Len=") << m_pReceiveBuffer->get_Length () << endl;
  m_pReceiveBuffer->Print (&Serial, true);
  Serial << F("PayloadSendBuffer Len=") << m_pPayloadSendBuffer->get_Length () << endl;
  m_pPayloadSendBuffer->Print (&Serial, true);
  Serial << F("PayloadRecvBuffer Len=") << m_pPayloadRecvBuffer->get_Length () << endl;
  m_pPayloadRecvBuffer->Print (&Serial, true);

  result = UCOPConfig::Create (c_EepromAddress, m_pUCOPConfig);
  Serial << F("UCOPConfig.Create() Result: ") << (int)result << " = " << UCOP::GetResultText (result) << endl;
  result = UCOP::Create (m_pUCOPConfig, m_pUCOP);
  Serial << F("UCOP.Create() Result: ") << (int)result << " = " << UCOP::GetResultText (result) << endl;
}

//--------------------------------------------------------------------
void loop ()
{
  EResult result;

  if (Serial.available ())
  {
    Serial << "Serial available: " << Serial.available () << endl;

    // Receive all available data
    while (Serial.available () > 0
        && m_PayloadLength < m_pPayloadSendBuffer->get_Length ())
    {
      m_pPayloadSendBuffer->WriteValueAndMovePtr ((uint8_t)Serial.read ());
    }
  }

  if (m_PayloadLength > 0)
  {
    Serial << F("PayloadSendBuffer: bytes used = ") << m_PayloadLength << endl;
    m_pPayloadSendBuffer->Print (&Serial, true);

    m_RequestData = UCOPData (false,
                              m_WorkerDeviceId,
                              m_CommandId);
    m_RequestData.SetPayloadInfo (m_pPayloadSendBuffer,
                                  m_PayloadLength);
    uint16_t requestMessageLength = 0;

    result = m_pUCOP->ComposeRequest (m_RequestData,
                                      m_pSendBuffer,
                                      requestMessageLength);
    Serial << F("UCOP.ComposeRequest() result=") << UCOP::GetResultText (result) << endl;

    Serial << F("SendBuffer: bytes used = ") << requestMessageLength << endl;
    m_pSendBuffer->Print (&Serial, true);

    if (result == EResult::SUCCESS)
    {
      digitalWrite (c_PinNumber_ReceiveDisable, HIGH);
      digitalWrite (c_PinNumber_SendEnable,     HIGH);

      Serial << F("Sending data...") << endl;
      Serial1.write (m_pSendBuffer->get_pData (), requestMessageLength);
      Serial1.flush ();


      digitalWrite (c_PinNumber_SendEnable, LOW);
      digitalWrite (c_PinNumber_ReceiveDisable, LOW);
    }

    m_pPayloadSendBuffer->Clear ();
    Serial << F("PayloadSendBuffer:") << endl;
    m_pPayloadSendBuffer->Print (&Serial, true);
    m_PayloadLength = 0;

    m_pSendBuffer->Clear ();
    Serial << F("SendBuffer:") << endl;
    m_pSendBuffer->Print (&Serial, true);
  }

  if (Serial1.available ())
  {
    Serial << "Serial1 available: " << Serial1.available () << endl;

    // Receive all available data
    while (Serial1.available ())
    {
      m_pReceiveBuffer->WriteValueAndMovePtr ((uint8_t)Serial1.read ());
    }

    m_pReceiveBuffer->Print (&Serial, true);
    m_DataAvailable = true;
  }

  if (m_DataAvailable)
  {
    bool     receivedMessageTypeIsReply = false;
    uint16_t receivedMessageLength      = 0;
    UCOPData receivedData;
    receivedData.SetPayloadInfo (m_pPayloadRecvBuffer);

    // Search message in the received data
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

    if (result == EResult::SUCCESS)
    {
      m_pReceiveBuffer->Clear_To (m_pReceiveBuffer->get_CurrentReadAddress (), receivedMessageLength);
      Serial << F("ReceiveBuffer:") << endl;
      m_pReceiveBuffer->Print (&Serial, true);

      // Evaluate reply

      m_pPayloadRecvBuffer->Clear ();
      Serial << F("PayloadRecvBuffer:") << endl;
      m_pPayloadRecvBuffer->Print (&Serial, true);
    }
    else
      m_DataAvailable = false;
  }

  if (!m_DataAvailable)
  {
    Serial.println ("Idle.");
    delay (1000);
  }
}
