#include <Arduino.h>
#include <Streaming.h>

#include <UCOP.h>
#include <UCOPConfig.h>
#include <UCOPData.h>

const uint8_t c_BufferDefaultValue = 0xDD;

const uint16_t c_EepromAddress = 0;

UCOPConfig* m_pUCOPConfig = nullptr;
UCOP*       m_pUCOP       = nullptr;

uint8_t m_PayloadSendBuffer[20];
uint8_t m_PayloadRecvBuffer[20];
uint8_t m_SendBuffer[50];
uint8_t m_ReceiveBuffer[80];

UCOP::EChecksumType m_ChecksumType   = UCOP::EChecksumType::CRC16;

uint16_t  m_ReceiveBufferWriteIndex = 0;
uint16_t  m_ReceiveBufferReadIndex  = 0;
bool      m_DataAvailable           = false;
bool      m_IsWorking               = false;
UCOPData  m_RequestData;

//--------------------------------------------------------------------
void setup ()
{
  EResult result;

  memset (m_PayloadSendBuffer, 0xFF, sizeof (m_PayloadSendBuffer));
  memset (m_PayloadRecvBuffer, 0xFF, sizeof (m_PayloadRecvBuffer));
  memset (m_SendBuffer       , 0xFF, sizeof (m_SendBuffer));
  memset (m_ReceiveBuffer    , 0xFF, sizeof (m_ReceiveBuffer));

  Serial.begin (9600);
  Serial1.begin (9600);
  delay (2000);

  Serial << F("PayloadSendBuffer Addr=") << _HEX4((uint16_t)m_PayloadSendBuffer) << " Len=" << sizeof (m_PayloadSendBuffer) << endl;
  Memory_PrintLn (m_PayloadSendBuffer, sizeof (m_PayloadSendBuffer));
  Serial << F("PayloadRecvBuffer Addr=") << _HEX4((uint16_t)m_PayloadRecvBuffer) << " Len=" << sizeof (m_PayloadRecvBuffer) << endl;
  Memory_PrintLn (m_PayloadRecvBuffer, sizeof (m_PayloadRecvBuffer));
  Serial << F("SendBuffer        Addr=") << _HEX4((uint16_t)m_SendBuffer)        << " Len=" << sizeof (m_SendBuffer) << endl;
  Memory_PrintLn (m_SendBuffer, sizeof (m_SendBuffer));
  Serial << F("ReceiveBuffer     Addr=") << _HEX4((uint16_t)m_ReceiveBuffer)     << " Len=" << sizeof (m_ReceiveBuffer) << endl;
  Memory_PrintLn (m_ReceiveBuffer, sizeof (m_ReceiveBuffer));

  result = UCOPConfig::Create (c_EepromAddress, m_pUCOPConfig);
  Serial << F("UCOPConfig.Create() result=") << UCOP::GetResultText (result) << endl;

  result = UCOP::Create (m_pUCOPConfig, m_pUCOP);
  Serial << F("UCOP.Create() result=") << UCOP::GetResultText (result) << endl;
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
      m_ReceiveBuffer[m_ReceiveBufferWriteIndex++] = Serial1.read ();
      if (m_ReceiveBufferWriteIndex >= sizeof (m_ReceiveBuffer))
        m_ReceiveBufferWriteIndex = 0;
    }

    Serial << F("ReceiveBuffer: position = ") << m_ReceiveBufferWriteIndex << endl;
    Memory_PrintLn (m_ReceiveBuffer, sizeof (m_ReceiveBuffer));

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
    result = m_pUCOP->SearchMessage (m_ReceiveBuffer,
                                     sizeof (m_ReceiveBuffer),
                                     m_ReceiveBufferReadIndex,
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
    Memory_PrintLn (m_PayloadRecvBuffer, sizeof (m_PayloadRecvBuffer));

    if (result == EResult::SUCCESS)
    {
      RingBuffer_SetValue_FromEnd (m_ReceiveBuffer,
                                   sizeof (m_ReceiveBuffer),
                                   m_ReceiveBufferReadIndex,
                                   receivedMessageLength,
                                   c_BufferDefaultValue);
      Serial << F("ReceiveBuffer:") << endl;
      Memory_PrintLn (m_ReceiveBuffer, sizeof (m_ReceiveBuffer));

      if (receivedMessageTypeIsReply)
      {
        Serial << F("Message is a request. Start working...");
        m_IsWorking = true;
      }
      else
      {
        Serial << F("Message is no request. Nothing to do.");
      }
    }
    else
      m_DataAvailable = false;
  }

  if (m_IsWorking)
  {
    for (int index = 0; index < m_RequestData.PayloadLength; index++)
      m_PayloadSendBuffer[index] = m_PayloadRecvBuffer[m_RequestData.PayloadLength - index - 1];

    memset (m_PayloadRecvBuffer, c_BufferDefaultValue, m_RequestData.PayloadLength);
    Serial << F("PayloadRecvBuffer:") << endl;
    Memory_PrintLn (m_PayloadRecvBuffer, sizeof (m_PayloadRecvBuffer));
    Serial << F("PayloadSendBuffer:") << endl;
    Memory_PrintLn (m_PayloadSendBuffer, sizeof (m_PayloadSendBuffer));

    UCOPData replyData = UCOPData::CreateReplyData (m_RequestData,
                                                    m_RequestData.Timestamp,
                                                  UCOP::EMessageResult::SUCCESS);
    replyData.SetPayloadInfo (m_PayloadSendBuffer, sizeof (m_PayloadSendBuffer), m_RequestData.PayloadLength);

    uint16_t replyMessageLength = 0;
    result = m_pUCOP->ComposeReply (replyData,
                                    m_SendBuffer,
                                    sizeof (m_SendBuffer),
                                    replyMessageLength);
    Serial << F("UCOP.ComposeReply() result=") << UCOP::GetResultText (result) << endl;

    Serial << F("SendBuffer: bytes used = ") << replyMessageLength << endl;
    Memory_PrintLn (m_SendBuffer, sizeof (m_SendBuffer));

    if (result == EResult::SUCCESS)
    {
      Serial1.write (m_SendBuffer, replyMessageLength);
      Serial1.flush ();
    }

    memset (m_PayloadSendBuffer, c_BufferDefaultValue, replyData.PayloadLength);
    memset (m_SendBuffer,        c_BufferDefaultValue, replyMessageLength);

    m_IsWorking = false;
  }

  if (!m_DataAvailable
  &&  !m_IsWorking)
  {
    Serial.println ("Idle.");
    delay (1000);
  }
}
