#ifndef UCOP_h
#define UCOP_h

#include <Arduino.h>
#include <FastCRC.h>
#include <Streaming.h>

#include <MemoryTools.h>
#include <MemoryTools_RingBuffer.h>
#include <Result.h>

class UCOPConfig;
class UCOPData;

// UCOP: Universal Communication Protocol
//
// The UCOP protocol defines the structure and content of data packets that are exchanged between microcontrollers and/or computers.
class UCOP
{
//--------------------------------------------------------------------
//          Offset        Length    Data
//           0               1.0    STX char
//           1               1.0    Version (= 1)
//           2               1.0    Flags (bit field)
//           2.0             0.1    Flags: Message Type     (0=Request, 1=Reply)
//           2.1             0.1    Flags: Action           (0=Read, 1=Write)
//           2.2             0.1    Flags: Device IDs used  (0=Off, 1=On)
//           2.3             0.1    Flags: Message ID used  (0=Off, 1=On)
//           2.4             0.1    Flags: Timestamp used   (0=Off, 1=On)
//           2.5             0.2    Flags: Checksum type    (0=None, 1=CRC8, 2=CRC16, 3=CRC32)
//           2.7             0.1    Flags: reserved
//           3           0.0/4.0    Sender Device ID, Length ("DIL"): 0/4
//           7           0.0/4.0    Receiver Device ID, Length ("DIL"): 0/4
//  + 2xDIL  3 - 11      0.0/4.0    Message ID, Length ("MIL"): 0/4
//  + MIL    3 - 15      0.0/4.0    Timestamp, Length ("TSL"): 0/4
//  + TSL    3 - 19          2.0    Command ID
//           5 - 21          1.0    Result
//           6 - 22          1.0    Payload data length ("PDL")
//           7 - 21+PDL      PDL    Payload data, Length ("PDL"): 0-255
//  + PDL    7 - 21+PDL  0.0-4.0    Checksum, Length ("CL"): 0/1/2/4
//  + CL     7 - 25+PDL      1.0    ETX char
//--------------------------------------------------------------------

//==================== Enums ====================
public:
  enum class EResult : uint16_t
  {
    Dummy_FirstClassFailure = (uint16_t)::EResult::Dummy_FirstClassFailure,
    #define X(name) name,
    #include "UCOP_EResult_failures.h"
    #undef X
    Dummy_LastClassFailure
  };

  enum class EChecksumType : uint8_t
  {
    None  = 0,
    CRC8  = 1,
    CRC16 = 2,
    CRC32 = 3
  };

  enum class EMessageResult : uint8_t
  {
    #define X(name) name,
    #include "UCOP_EMessageResult_results.h"
    Dummy_FirstFailure = 0x10,
    #include "UCOP_EMessageResult_failures.h"
    #undef X
  };

//==================== Fields ====================
private:
  //-------------------- static --------------------

  static const uint8_t c_MessageStartID = 0x02;  // STX
  static const uint8_t c_MessageEndID   = 0x03;  // ETX
  static const uint8_t c_Version = 0x01;
  static const uint8_t c_HeaderMinLength  = 7;   // STX:1, Version:1, Flags:1, CommandID:2, Result:1, PayloadLength:1
  static const uint8_t c_TrailerMinLength = 1;   // ETX:1
  static const uint8_t c_MessageMinLength = c_HeaderMinLength + c_TrailerMinLength;
  static const uint8_t c_FlagIndex_MessageType   = 0;
  static const uint8_t c_FlagIndex_Action        = 1;
  static const uint8_t c_FlagIndex_DeviceIdsUsed = 2;
  static const uint8_t c_FlagIndex_MessageIdUsed = 3;
  static const uint8_t c_FlagIndex_TimestampUsed = 4;
  static const uint8_t c_FlagIndex_ChecksumType  = 5;

  static const char* const c_EResult_ClassFailures_Names[] PROGMEM;
  static const char* const c_EMessageResult_Results_Names[] PROGMEM;
  static const char* const c_EMessageResult_Failures_Names[] PROGMEM;

  #define X(name) static const char _EResult_##name[] PROGMEM;
  #include "UCOP_EResult_failures.h"
  #undef X

  #define X(name) static const char _EMessageResult_##name[] PROGMEM;
  #include "UCOP_EMessageResult_results.h"
  #include "UCOP_EMessageResult_failures.h"
  #undef X

  //-------------------- instance --------------------

  UCOPConfig* m_pConfig   = nullptr;
  uint32_t    m_MessageId = 0;

  FastCRC8  m_Crc8;
  FastCRC16 m_Crc16;
  FastCRC32 m_Crc32;

//==================== Constructors ====================
public:
  //-------------------- static --------------------

  static ::EResult Create ( UCOPConfig* i_pConfig,
                            UCOP*&      o_pUCOP);

  //-------------------- instance --------------------

  ~UCOP ();

protected:
  //-------------------- instance --------------------

  UCOP (UCOPConfig* i_pConfig);

private:
  //-------------------- instance --------------------

  ::EResult CommonConstructor ( bool          i_DeviceIdsUsed,
                                bool          i_MessageIdUsed,
                                bool          i_TimestampUsed,
                                uint32_t      i_DeviceId,
                                EChecksumType i_ChecksumType);

//==================== Properties ====================
public:
  //-------------------- instance --------------------

  UCOPConfig* get_Config ();

//==================== Public Methods ====================
public:
  //-------------------- static --------------------

  static uint8_t GetChecksumLength (EChecksumType i_ChecksumType);

  static EMessageResult GetMessageResultForFunctionResult (::EResult i_Result);

  static const __FlashStringHelper* GetMessageResultText (EMessageResult i_MessageResult);

  static const __FlashStringHelper* GetResultText (::EResult i_Result);
  
  static bool IsChecksumValid (EChecksumType i_ChecksumType);

  //-------------------- instance --------------------

  uint8_t CalcHeaderSize ();

  uint8_t CalcTrailerSize ();

  ::EResult ComposeReply (UCOPData& i_Data,
                          uint8_t*  i_pMessageBuffer,
                          uint8_t   i_MessageBufferLength,
                          uint16_t& o_MessageLength);

  ::EResult ComposeRequest (UCOPData& i_Data,
                            uint8_t*  i_pMessageBuffer,
                            uint8_t   i_MessageBufferLength,
                            uint16_t& o_MessageLength);

  ::EResult SearchMessage (uint8_t*  i_pRingBuffer,
                           uint16_t  i_RingBufferLength,
                           uint16_t& io_RingBufferStartIndex,
                           UCOPData& io_Data,
                           bool&     o_MessageTypeIsReply,
                           uint16_t& o_MessageLength);

//==================== Private Methods ====================
private:
  //-------------------- instance --------------------

  ::EResult ComposeMessage (UCOPData& i_Data,
                            uint8_t*  i_pMessageBuffer,
                            uint8_t   i_MessageBufferLength,
                            uint16_t& o_MessageLength,
                            bool      i_MessageIsReply);

  ::EResult Verify_exec ();
};

#endif
