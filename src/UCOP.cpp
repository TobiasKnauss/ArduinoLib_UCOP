#include "UCOP.h"
#include "UCOPConfig.h"
#include "UCOPData.h"

//--------------------------------------------------------------------
#define X(name) const char UCOP::_EResult_##name[] PROGMEM = #name;
#include "UCOP_EResult_failures.h"
#undef X

//--------------------------------------------------------------------
#define X(name) _EResult_##name,
const char* const UCOP::c_EResult_ClassFailures_Names[] PROGMEM =
{
  #include "UCOP_EResult_failures.h"
};
#undef X

//--------------------------------------------------------------------
#define X(name) const char UCOP::_EMessageResult_##name[] PROGMEM = #name;
#include "UCOP_EMessageResult_results.h"
#include "UCOP_EMessageResult_failures.h"
#undef X

//--------------------------------------------------------------------
#define X(name) _EMessageResult_##name,
const char* const UCOP::c_EMessageResult_Results_Names[] PROGMEM =
{
#include "UCOP_EMessageResult_results.h"
};
const char* const UCOP::c_EMessageResult_Failures_Names[] PROGMEM =
{
#include "UCOP_EMessageResult_failures.h"
};
#undef X

//--------------------------------------------------------------------
::EResult UCOP::Create (UCOPConfig* i_pConfig,
                        UCOP*&      o_pUCOP)
{
  o_pUCOP = nullptr;
  UCOP* pUCOP = new UCOP (i_pConfig);

  ::EResult result = pUCOP->Verify_exec ();
  if (result != ::EResult::SUCCESS)
  {
    delete pUCOP;
    return result;
  }

  o_pUCOP = pUCOP;
  return ::EResult::SUCCESS;
}

//--------------------------------------------------------------------
UCOP::~UCOP ()
{
}

//--------------------------------------------------------------------
UCOP::UCOP (UCOPConfig* i_pConfig)
{
  m_pConfig = i_pConfig;
}

//--------------------------------------------------------------------
UCOPConfig* UCOP::get_Config ()
{
  return m_pConfig;
}

//--------------------------------------------------------------------
UCOP::EMessageResult UCOP::GetMessageResultForFunctionResult (::EResult i_Result)
{
  switch ((EResult)i_Result)
  {
  case EResult::FAIL_UCOP_Message_NotFound:                 return EMessageResult::None;
  case EResult::FAIL_UCOP_Message_ReceiverDeviceIdMismatch: return EMessageResult::None;
  case EResult::FAIL_UCOP_Message_SenderDeviceIdInvalid:    return EMessageResult::FAIL_DeviceIdInvalid;
  case EResult::FAIL_UCOP_Message_MessageIdInvalid:         return EMessageResult::FAIL_MessageIdInvalid;
  case EResult::FAIL_UCOP_Message_TimestampInvalid:         return EMessageResult::FAIL_TimestampInvalid;
  case EResult::FAIL_UCOP_Message_CommandIdInvalid:         return EMessageResult::FAIL_CommandIdInvalid;
  case EResult::FAIL_UCOP_Message_ResultWrong:              return EMessageResult::FAIL_ResultWrong;
  default:                                                  return EMessageResult::FAIL_InternalFailure;
  }
}

//--------------------------------------------------------------------
const __FlashStringHelper* UCOP::GetMessageResultText (EMessageResult i_MessageResult)
{
  if ((uint16_t)i_MessageResult < (uint16_t)EMessageResult::Dummy_FirstFailure)
    return (const __FlashStringHelper*)pgm_read_ptr(&c_EMessageResult_Results_Names[(uint16_t)i_MessageResult]);

  return (const __FlashStringHelper*)pgm_read_ptr(&c_EMessageResult_Failures_Names[(uint16_t)i_MessageResult - (uint16_t)EMessageResult::Dummy_FirstFailure - 1]);
}

//--------------------------------------------------------------------
const __FlashStringHelper* UCOP::GetResultText (::EResult i_Result)
{
  if ((uint16_t)i_Result < (uint16_t)EResult::Dummy_FirstClassFailure)
    return Result::GetText (i_Result);
  return (const __FlashStringHelper*)pgm_read_ptr(&c_EResult_ClassFailures_Names[(uint16_t)i_Result - (uint16_t)EResult::Dummy_FirstClassFailure - 1]);
}

//--------------------------------------------------------------------
uint8_t UCOP::CalcHeaderSize ()
{
  return c_HeaderMinLength
       + (m_pConfig->get_DeviceIdsUsed () ? 8 : 0)
       + (m_pConfig->get_MessageIdUsed () ? 4 : 0)
       + (m_pConfig->get_TimestampUsed () ? 4 : 0);
}

//--------------------------------------------------------------------
uint8_t UCOP::CalcTrailerSize ()
{
  return c_TrailerMinLength;
}

//--------------------------------------------------------------------
EResult UCOP::ComposeReply (UCOPData&   i_Data,
                            ByteBuffer* i_pMessageBuffer,
                            uint16_t&   o_MessageLength)
{
  return ComposeMessage (i_Data,
                         i_pMessageBuffer,
                         o_MessageLength,
                         true);
}

//--------------------------------------------------------------------
EResult UCOP::ComposeRequest (UCOPData&   i_Data,
                              ByteBuffer* i_pMessageBuffer,
                              uint16_t&   o_MessageLength)
{
  return ComposeMessage (i_Data,
                         i_pMessageBuffer,
                         o_MessageLength,
                         false);
}

//--------------------------------------------------------------------
EResult UCOP::SearchMessage ( ByteBuffer* i_pRingBuffer,
                              UCOPData&   io_Data,
                              bool&       o_MessageTypeIsReply,
                              uint16_t&   o_MessageLength)
{
  if (i_pRingBuffer == nullptr)
    return ::EResult::FAIL_Pointer_IsZero;
  uint16_t bufferLength = i_pRingBuffer->get_Length ();
  if (bufferLength < c_MessageMinLength)
    return ::EResult::FAIL_Buffer_TooSmall;

  io_Data.Clear ();
  io_Data.PayloadLength = 0;
  if (io_Data.pPayloadBuffer != nullptr)
    io_Data.pPayloadBuffer->Clear ();
  o_MessageTypeIsReply = false;
  o_MessageLength      = 0;

  // Execute multiple attempts to search the start of the message:
  // - In each attempt, the message start ID is searched by looping over the buffer.
  // - When the message start ID is found, it is assumed that a complete message follows.
  // - The found possible message is analyzed. If it is a valid message, it is evaluated. If not, the next search attempt is started one byte after the previously found message start ID.

  uint16_t searchStartAddress = i_pRingBuffer->get_CurrentReadAddress ();

  for (uint16_t searchIndex = 0; searchIndex < bufferLength; searchIndex++)
  {
    if (!i_pRingBuffer->SetReadPointer (searchStartAddress + searchIndex, true))
      return ::EResult::FAIL_Buffer_SetPointer;

    ::EResult result = ::EResult::InProgress;
    uint16_t messageStartAddress = i_pRingBuffer->get_CurrentReadAddress ();

    //========== Header ==========
    // STX
    uint8_t messageStartId;
    if (!i_pRingBuffer->ReadValueAndMovePtr (messageStartId))
      return ::EResult::FAIL_Buffer_ReadValue;
    if (messageStartId != c_MessageStartId)
      continue;

    uint16_t checksumCalcStart = i_pRingBuffer->get_CurrentReadAddress ();

    // Version
    uint8_t version;
    if (!i_pRingBuffer->ReadValueAndMovePtr (version))
      return ::EResult::FAIL_Buffer_ReadValue;
    if (version != c_Version)
      continue;

    // Flags
    uint8_t flags;
    if (!i_pRingBuffer->ReadValueAndMovePtr (flags))
      return ::EResult::FAIL_Buffer_ReadValue;

    // Flag: MessageType
    o_MessageTypeIsReply = (flags >> c_FlagIndex_MessageType) & 0x01;

    // Flag: Action
    io_Data.ActionIsWrite = (flags >> c_FlagIndex_Action) & 0x01;

    // Flag: DeviceIdsUsed
    if (flags & 1 << c_FlagIndex_DeviceIdsUsed)
    {
      if (!i_pRingBuffer->ReadValueAndMovePtr (io_Data.RemoteDeviceId, c_InvertByteOrder))
        return ::EResult::FAIL_Buffer_ReadValue;
      if (io_Data.RemoteDeviceId == 0
      &&  result == ::EResult::InProgress)
        result = (::EResult)EResult::FAIL_UCOP_Message_SenderDeviceIdInvalid;

      uint32_t ownDeviceId;
      if (!i_pRingBuffer->ReadValueAndMovePtr (ownDeviceId, c_InvertByteOrder))
        return ::EResult::FAIL_Buffer_ReadValue;
      if (ownDeviceId != m_pConfig->get_DeviceId ()
      &&  result == ::EResult::InProgress)
        result = (::EResult)EResult::FAIL_UCOP_Message_ReceiverDeviceIdMismatch;
    }

    // Flag: MessageIdUsed
    if (flags & 1 << c_FlagIndex_MessageIdUsed)
    {
      if (!i_pRingBuffer->ReadValueAndMovePtr (io_Data.MessageId, c_InvertByteOrder))
        return ::EResult::FAIL_Buffer_ReadValue;
      if (io_Data.MessageId == 0
      &&  result == ::EResult::InProgress)
        result = (::EResult)EResult::FAIL_UCOP_Message_MessageIdInvalid;
    }

    // Flag: TimestampUsed
    if (flags & 1 << c_FlagIndex_TimestampUsed)
    {
      if (!i_pRingBuffer->ReadValueAndMovePtr (io_Data.Timestamp, c_InvertByteOrder))
        return ::EResult::FAIL_Buffer_ReadValue;
      if (io_Data.Timestamp == 0
      &&  result == ::EResult::InProgress)
        result = (::EResult)EResult::FAIL_UCOP_Message_TimestampInvalid;
    }

    // Command ID
    if (!i_pRingBuffer->ReadValueAndMovePtr (io_Data.CommandId, c_InvertByteOrder))
      return ::EResult::FAIL_Buffer_ReadValue;
    if (io_Data.CommandId == 0
    &&  result == ::EResult::InProgress)
      result = (::EResult)EResult::FAIL_UCOP_Message_CommandIdInvalid;

    // Result
    uint8_t messageResult;
    if (!i_pRingBuffer->ReadValueAndMovePtr (messageResult))
      return ::EResult::FAIL_Buffer_ReadValue;
    io_Data.MessageResult = (EMessageResult)messageResult;
    if ((io_Data.MessageResult == EMessageResult::None) == o_MessageTypeIsReply  // failure if (reply and no result) or (request and result)
    &&  result == ::EResult::InProgress)
      result = (::EResult)EResult::FAIL_UCOP_Message_ResultWrong;

    // Payload data length
    if (!i_pRingBuffer->ReadValueAndMovePtr (io_Data.PayloadLength, c_InvertByteOrder))
      return ::EResult::FAIL_Buffer_ReadValue;
    if (io_Data.PayloadLength > 0)
    {
      if (io_Data.pPayloadBuffer == nullptr)
        return ::EResult::FAIL_Pointer_IsZero;
      if (io_Data.pPayloadBuffer->get_Length () < io_Data.PayloadLength)
        return ::EResult::FAIL_Buffer_TooSmall;
      io_Data.pPayloadBuffer->SetWritePointer (0);
    }

    //========== Payload ==========
    // Payload data length
    if (!i_pRingBuffer->ReadBytesAndMovePtr (io_Data.PayloadLength, io_Data.pPayloadBuffer, false))
      return ::EResult::FAIL_Buffer_ReadBytes;

    //========== Trailer ==========
    // Checksum:  header without STX, payload data
    uint16_t checksumCalculated = 0;
    uint16_t checksumFromMessage = 0;
    if (!i_pRingBuffer->CalcChecksumCRC16_FromTo (checksumCalcStart, i_pRingBuffer->get_CurrentReadAddress (), checksumCalculated))
      return (::EResult)EResult::FAIL_UCOP_CalcChecksum;
    if (!i_pRingBuffer->ReadValueAndMovePtr (checksumFromMessage, c_InvertByteOrder))
      return ::EResult::FAIL_Buffer_ReadValue;

    // ETX
    uint8_t messageEndID;
    if (!i_pRingBuffer->ReadValueAndMovePtr (messageEndID))
      return ::EResult::FAIL_Buffer_ReadValue;
    if (messageEndID == c_MessageEndID // message was found
    &&  checksumCalculated == checksumFromMessage) // message is valid
    {
      uint16_t messageEndAddress = i_pRingBuffer->get_CurrentReadAddress ();
      o_MessageLength = messageEndAddress > messageStartAddress
                      ? messageEndAddress - messageStartAddress
                      : (bufferLength - messageStartAddress) + messageEndAddress;

      // clear the analyzed buffer part
      i_pRingBuffer->Clear_FromTo (messageStartAddress, messageEndAddress);

      return result == ::EResult::InProgress ? ::EResult::SUCCESS : result;
    }
    // else: Message End ID missing -> This was no message. Previously identified failures are invalid.
    //       or Checksum wrong -> The message content is incorrect. Previously identified failures are invalid.
    // else
    //   Serial << "checksumCalculated: " << _HEX4(checksumCalculated) << "  checksumFromMessage: " << _HEX4 (checksumFromMessage) << endl;

    io_Data.Clear ();
    io_Data.PayloadLength = 0;
    o_MessageTypeIsReply  = false;
    o_MessageLength       = 0;
  }

  return (::EResult)EResult::FAIL_UCOP_Message_NotFound;
}

//--------------------------------------------------------------------
::EResult UCOP::ComposeMessage (UCOPData&   i_Data,
                                ByteBuffer* i_pMessageBuffer,
                                uint16_t&   o_MessageLength,
                                bool        i_MessageIsReply)
{
  if (i_pMessageBuffer == nullptr)
    return ::EResult::FAIL_Pointer_IsZero;

  uint8_t headerSize  = CalcHeaderSize ();
  uint8_t trailerSize = CalcTrailerSize ();
  if (i_pMessageBuffer->get_Length () < headerSize + i_Data.PayloadLength + trailerSize)
    return ::EResult::FAIL_Buffer_TooSmall;

  if (i_Data.PayloadLength > 0)
  {
    if (i_Data.pPayloadBuffer == nullptr)
      return ::EResult::FAIL_Pointer_IsZero;
    if (i_Data.pPayloadBuffer->get_Length () < i_Data.PayloadLength)
      return ::EResult::FAIL_Buffer_TooSmall;
  }

  i_pMessageBuffer->SetWritePointer (0);
  if (!i_pMessageBuffer->Clear_From (0, headerSize))
    return ::EResult::FAIL_Buffer_Clear;

  //========== Header ==========

  // STX
  if (!i_pMessageBuffer->WriteValueAndMovePtr (c_MessageStartId))
    return ::EResult::FAIL_Buffer_WriteValue;

  // Version
  if (!i_pMessageBuffer->WriteValueAndMovePtr (c_Version))
    return ::EResult::FAIL_Buffer_WriteValue;

  // Flags
  uint8_t flags = 0;
  if (i_MessageIsReply               ) flags |= 1 << c_FlagIndex_MessageType;
  if (i_Data.ActionIsWrite           ) flags |= 1 << c_FlagIndex_Action;
  if (m_pConfig->get_DeviceIdsUsed ()) flags |= 1 << c_FlagIndex_DeviceIdsUsed;
  if (m_pConfig->get_MessageIdUsed ()) flags |= 1 << c_FlagIndex_MessageIdUsed;
  if (m_pConfig->get_TimestampUsed ()) flags |= 1 << c_FlagIndex_TimestampUsed;
  if (!i_pMessageBuffer->WriteValueAndMovePtr (flags))
    return ::EResult::FAIL_Buffer_WriteValue;

  // Sender and Receiver Device IDs
  if (m_pConfig->get_DeviceIdsUsed ())
  {
    if (!i_pMessageBuffer->WriteValueAndMovePtr (m_pConfig->get_DeviceId (), c_InvertByteOrder))
      return ::EResult::FAIL_Buffer_WriteValue;
    if (!i_pMessageBuffer->WriteValueAndMovePtr (i_Data.RemoteDeviceId, c_InvertByteOrder))
      return ::EResult::FAIL_Buffer_WriteValue;
  }

  // Message ID
  if (m_pConfig->get_MessageIdUsed ())
  {
    if (i_MessageIsReply)
    {
      if (!i_pMessageBuffer->WriteValueAndMovePtr (i_Data.MessageId, c_InvertByteOrder))
        return ::EResult::FAIL_Buffer_WriteValue;
    }
    else
    {
      m_MessageId++;
      if (m_MessageId == 0)
        m_MessageId = 1;
      if (!i_pMessageBuffer->WriteValueAndMovePtr (m_MessageId, c_InvertByteOrder))
        return ::EResult::FAIL_Buffer_WriteValue;
    }
  }

  // Timestamp
  if (m_pConfig->get_TimestampUsed ())
  {
    if (!i_pMessageBuffer->WriteValueAndMovePtr (i_Data.Timestamp, c_InvertByteOrder))
      return ::EResult::FAIL_Buffer_WriteValue;
  }

  // Command ID
  if (!i_pMessageBuffer->WriteValueAndMovePtr (i_Data.CommandId, c_InvertByteOrder))
    return ::EResult::FAIL_Buffer_WriteValue;

  // Message Result
  if (!i_pMessageBuffer->WriteValueAndMovePtr (i_MessageIsReply ? (uint8_t)i_Data.MessageResult : (uint8_t)0))
    return ::EResult::FAIL_Buffer_WriteValue;

  // Payload data length
  if (!i_pMessageBuffer->WriteValueAndMovePtr (i_Data.PayloadLength, c_InvertByteOrder))
    return ::EResult::FAIL_Buffer_WriteValue;

  //========== Payload ==========
  // Payload data
  i_Data.pPayloadBuffer->SetReadPointer (0);
  if (!i_pMessageBuffer->WriteBytesAndMovePtr (i_Data.PayloadLength, i_Data.pPayloadBuffer, false))
    return ::EResult::FAIL_Buffer_WriteBytes;

  //========== Trailer ==========
  if (!i_pMessageBuffer->Clear_From (i_pMessageBuffer->get_CurrentWriteAddress (), trailerSize))
    return ::EResult::FAIL_Buffer_Clear;

  // Checksum
  uint16_t crc16;
  if (!i_pMessageBuffer->CalcChecksumCRC16_From (1, headerSize + i_Data.PayloadLength - 1, crc16)) // header without STX, payload data
    return ::EResult::FAIL_Buffer_CalcChecksum;
  if (!i_pMessageBuffer->WriteValueAndMovePtr (crc16, c_InvertByteOrder))
    return ::EResult::FAIL_Buffer_WriteValue;

  // ETX
  if (!i_pMessageBuffer->WriteValueAndMovePtr (c_MessageEndID))
    return ::EResult::FAIL_Buffer_WriteValue;

  o_MessageLength = i_pMessageBuffer->get_CurrentWriteAddress ();

  return ::EResult::SUCCESS;
}

//--------------------------------------------------------------------
::EResult UCOP::Verify_exec ()
{
  if (m_pConfig == nullptr)
    return ::EResult::FAIL_Pointer_IsZero;

  return ::EResult::SUCCESS;
}
