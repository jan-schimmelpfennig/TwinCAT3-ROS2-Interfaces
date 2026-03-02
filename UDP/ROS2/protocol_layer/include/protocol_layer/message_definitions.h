#ifndef PROTOCOL_LAYER__DEFINITIONS_H
#define PROTOCOL_LAYER__DEFINITIONS_H

#include <cstdint>

namespace protocol_layer{
    /*
    @brief your message structure
    */
    #pragma pack(push, 1)
    struct toTwinCAT
    {
        uint8_t keyword;
        uint8_t crc8;
        uint64_t timestamp;
        uint8_t a_byte;
        uint16_t an_integer;
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct fromTwinCAT
    {
        uint8_t keyword;
        uint8_t crc8;
        uint64_t timestamp;
        uint16_t an_integer;
    };
    #pragma pack(pop)


    /*
    @brief constants that are used to construct the application frame
    */
    struct FrameConstants{
        static constexpr uint8_t KEYWORD = 42;
        //static constexpr uint8_t HEADER_BYTE = 0xAA;
        //static constexpr uint8_t FOOTER_BYTE = 0x55;
        //static constexpr uint16_t PAYLOAD_SIZE = sizeof(toTwinCAT); //how big in byte your payload is
        //static constexpr uint16_t TOTAL_SIZE = PAYLOAD_SIZE;//(PAYLOAD_SIZE +7) & ~7;//(1+2+ PAYLOAD_SIZE+1)+3 & ~3;
        //With crc: footer (1byte) + payload size (2byte) + payload (variable) + checksum (2byte)+ footer(1byte)
        //without crc: footer (1byte) + payload size (2byte) + payload  (variable) + footer(1byte)
    };
}
#endif //PROTOCOL_LAYER__DEFINITIONS_H