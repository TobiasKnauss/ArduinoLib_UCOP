#include "UCOPData.h"

//--------------------------------------------------------------------
UCOPData UCOPData::CreateReplyData (UCOPData&             i_RequestData,
                                    uint32_t              i_Timestamp,
                                    UCOP::EMessageResult  i_MessageResult)
{
  return UCOPData ( i_RequestData.ActionIsWrite,
                    i_RequestData.RemoteDeviceId,
                    i_RequestData.MessageId,
                    i_Timestamp,
                    i_RequestData.CommandId,
                    i_MessageResult);
}

//--------------------------------------------------------------------
UCOPData::UCOPData ()
{
}

//--------------------------------------------------------------------
UCOPData::UCOPData (bool      i_ActionIsWrite,
                    uint32_t  i_RemoteDeviceId,
                    uint16_t  i_CommandId)
{
  ActionIsWrite  = i_ActionIsWrite;
  CommandId      = i_CommandId;
  RemoteDeviceId = i_RemoteDeviceId;
}

//--------------------------------------------------------------------
UCOPData::UCOPData (bool                  i_ActionIsWrite,
                    uint32_t              i_RemoteDeviceId,
                    uint32_t              i_MessageId,
                    uint32_t              i_Timestamp,
                    uint16_t              i_CommandId,
                    UCOP::EMessageResult  i_MessageResult)
{
  ActionIsWrite  = i_ActionIsWrite;
  CommandId      = i_CommandId;
  MessageId      = i_MessageId;
  MessageResult  = i_MessageResult;
  RemoteDeviceId = i_RemoteDeviceId;
  Timestamp      = i_Timestamp;
}

//--------------------------------------------------------------------
void UCOPData::Clear ()
{
  ActionIsWrite  = false;
  CommandId      = 0;
  MessageId      = 0;
  MessageResult  = UCOP::EMessageResult::None;
  RemoteDeviceId = 0;
  Timestamp      = 0;
}

//--------------------------------------------------------------------
bool UCOPData::IsEmpty ()
{
  return CommandId > 0;  // The command ID must never be zero if data exists. All other values may be zero.
}

//--------------------------------------------------------------------
void UCOPData::SetPayloadInfo (uint8_t* i_pPayloadBuffer,
                               uint16_t i_PayloadBufferLength,
                               uint16_t i_PayloadLength)
{
  pPayloadBuffer      = i_pPayloadBuffer;
  PayloadBufferLength = i_PayloadBufferLength;
  PayloadLength       = i_PayloadLength;
}
