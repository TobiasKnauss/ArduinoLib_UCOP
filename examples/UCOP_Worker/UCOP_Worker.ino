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
uint8_t m_PayloadSendBuffer[20];
uint8_t m_PayloadRecvBuffer[20];

bool      m_DataAvailable           = false;
bool      m_IsWorking               = false;
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

  memset (m_PayloadSendBuffer, 0xFF, sizeof (m_PayloadSendBuffer));
  memset (m_PayloadRecvBuffer, 0xFF, sizeof (m_PayloadRecvBuffer));

  EResult result;

  Serial << F("SendBuffer Len=") << m_pSendBuffer->get_Length ()    << endl;
  m_pSendBuffer->Print (&Serial, true);
  Serial << F("RecvBuffer Len=") << m_pReceiveBuffer->get_Length () << endl;
  m_pReceiveBuffer->Print (&Serial, true);
  Serial << F("PayloadSendBuffer Addr=") << _HEX4((uint16_t)m_PayloadSendBuffer) << " Len=" << sizeof (m_PayloadSendBuffer) << endl;
  Memory::PrintLn (m_PayloadSendBuffer, sizeof (m_PayloadSendBuffer));
  Serial << F("PayloadRecvBuffer Addr=") << _HEX4((uint16_t)m_PayloadRecvBuffer) << " Len=" << sizeof (m_PayloadRecvBuffer) << endl;
  Memory::PrintLn (m_PayloadRecvBuffer, sizeof (m_PayloadRecvBuffer));

  result = UCOPConfig::Create (c_EepromAddress, m_pUCOPConfig);
  Serial << F("UCOPConfig.Create() Result: ") << (int)result << " = " << UCOP::GetResultText (result) << endl;
  result = UCOP::Create (m_pUCOPConfig, m_pUCOP);
  Serial << F("UCOP.Create() Result: ") << (int)result << " = " << UCOP::GetResultText (result) << endl;
}

//--------------------------------------------------------------------
void loop ()
{
  EResult result;

  if (Serial1.available ()
  &&  !m_IsWorking)
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

  if (m_DataAvailable
  &&  !m_IsWorking)
  {
    bool     receivedMessageTypeIsReply = false;
    uint16_t receivedMessageLength      = 0;
    UCOPData receivedData;
    receivedData.SetPayloadInfo (m_PayloadRecvBuffer,
                                 sizeof (m_PayloadRecvBuffer));

    // Search message in the received data
    result = m_pUCOP->SearchMessage (m_pReceiveBuffer,
                                     receivedData,
                                     receivedMessageTypeIsReply,
                                     receivedMessageLength);
    Serial << F("UCOP.SearchMessage() result=") << UCOP::GetResultText (result) << endl;

    Serial << F("Message Type is Reply: ") << receivedMessageTypeIsReply          << endl;
    Serial << F("Action is Write:       ") << receivedData.ActionIsWrite          << endl;
    Serial << F("Remote Device Id:      ") << _HEX8 (receivedData.RemoteDeviceId) << endl;
    Serial << F("Message Id:            ") << receivedData.MessageId              << endl;
    Serial << F("Timestamp:             ") << receivedData.Timestamp              << endl;
    Serial << F("CommandId:             ") << receivedData.CommandId              << endl;
    Serial << F("Result:                ") << (uint8_t)receivedData.MessageResult << endl;
    Serial << F("Payload Data Length:   ") << receivedData.PayloadLength          << endl;
    Serial << F("Message Length:        ") << receivedMessageLength               << endl;

    Serial << F("PayloadRecvBuffer: bytes used = ") << receivedData.PayloadLength << endl;
    Memory::PrintLn (m_PayloadRecvBuffer, sizeof (m_PayloadRecvBuffer));

    if (result == EResult::SUCCESS)
    {
      m_pReceiveBuffer->Clear_To (m_pReceiveBuffer->get_CurrentReadAddress (), receivedMessageLength);
      Serial << F("ReceiveBuffer:") << endl;
      m_pReceiveBuffer->Print (&Serial, true);

      if (receivedMessageTypeIsReply)
      {
        Serial << F("Message is no request. Nothing to do.") << endl;
      }
      else
      {
        Serial << F("Message is a request. Start working...") << endl;
        m_RequestData = receivedData;
        m_IsWorking = true;
      }
    }
    else
      m_DataAvailable = false;
  }

  if (m_IsWorking)
  {
    for (uint16_t index = 0; index < m_RequestData.PayloadLength; index++)
      m_PayloadSendBuffer[index] = m_PayloadRecvBuffer[m_RequestData.PayloadLength - index - 1];

    memset (m_PayloadRecvBuffer, c_BufferDefaultValue, m_RequestData.PayloadLength);
    Serial << F("PayloadRecvBuffer:") << endl;
    Memory::PrintLn (m_PayloadRecvBuffer, sizeof (m_PayloadRecvBuffer));
    Serial << F("PayloadSendBuffer:") << endl;
    Memory::PrintLn (m_PayloadSendBuffer, sizeof (m_PayloadSendBuffer));

    UCOPData replyData = UCOPData::CreateReplyData (m_RequestData,
                                                    m_RequestData.Timestamp,
                                                    UCOP::EMessageResult::SUCCESS);
    replyData.SetPayloadInfo (m_PayloadSendBuffer, sizeof (m_PayloadSendBuffer), m_RequestData.PayloadLength);

    uint16_t replyMessageLength = 0;
    result = m_pUCOP->ComposeReply (replyData,
                                    m_pSendBuffer,
                                    replyMessageLength);
    Serial << F("UCOP.ComposeReply() result=") << UCOP::GetResultText (result) << endl;

    Serial << F("SendBuffer: bytes used = ") << replyMessageLength << endl;
    m_pSendBuffer->Print (&Serial, true);

    if (result == EResult::SUCCESS)
    {
      digitalWrite (c_PinNumber_ReceiveDisable, HIGH);
      digitalWrite (c_PinNumber_SendEnable,     HIGH);

      Serial << F("Sending data...") << endl;
      Serial1.write (m_pSendBuffer->get_pData (), replyMessageLength);
      Serial1.flush ();

      digitalWrite (c_PinNumber_SendEnable, LOW);
      digitalWrite (c_PinNumber_ReceiveDisable, LOW);
    }

    memset (m_PayloadSendBuffer, c_BufferDefaultValue, replyData.PayloadLength);
    Serial << F("PayloadSendBuffer:") << endl;
    Memory::PrintLn (m_PayloadSendBuffer, sizeof (m_PayloadSendBuffer));

    m_pSendBuffer->Clear_From (m_pSendBuffer->get_CurrentReadAddress (), replyMessageLength);
    Serial << F("SendBuffer:") << endl;
    m_pSendBuffer->Print (&Serial, true);

    m_IsWorking = false;
  }

  if (!m_DataAvailable
  &&  !m_IsWorking)
  {
    Serial.println ("Idle.");
    delay (1000);
  }
}
