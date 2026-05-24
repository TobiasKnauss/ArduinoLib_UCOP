#ifndef UCOPData_h
#define UCOPData_h

#include <Arduino.h>

#include "UCOP.h"

class UCOPData
{
//==================== Fields ====================
public:
  //-------------------- instance --------------------

  bool                  ActionIsWrite       = false;
  uint16_t              CommandId           = 0;      // in valid data this may NEVER be zero.
  uint32_t              MessageId           = 0;      // may be zero if not used.
  UCOP::EMessageResult  MessageResult       = UCOP::EMessageResult::None; // may be zero (=None) in requests.
  uint32_t              RemoteDeviceId      = 0;      // may be zero if not used.
  uint32_t              Timestamp           = 0;      // may be zero if not used.
  uint8_t*              pPayloadBuffer      = nullptr;
  uint16_t              PayloadBufferLength = 0;
  uint16_t              PayloadLength       = 0;

//==================== Constructors ====================
public:
  //-------------------- static --------------------

  static UCOPData CreateReplyData ( UCOPData&             i_RequestData,
                                    uint32_t              i_Timestamp,
                                    UCOP::EMessageResult  i_MessageResult);

  //-------------------- instance --------------------

  UCOPData ();

  UCOPData (bool      i_ActionIsWrite,
            uint32_t  i_RemoteDeviceId,
            uint16_t  i_CommandId);

  UCOPData (bool                  i_ActionIsWrite,
            uint32_t              i_RemoteDeviceId,
            uint32_t              i_MessageId,
            uint32_t              i_Timestamp,
            uint16_t              i_CommandId,
            UCOP::EMessageResult  i_MessageResult);

//==================== Public Methods ====================
public:
  //-------------------- instance --------------------

  void Clear ();

  bool IsEmpty ();

  void SetPayloadInfo (uint8_t* i_pPayloadBuffer,
                       uint16_t i_PayloadBufferLength,
                       uint16_t i_PayloadLength = 0);
};

#endif
