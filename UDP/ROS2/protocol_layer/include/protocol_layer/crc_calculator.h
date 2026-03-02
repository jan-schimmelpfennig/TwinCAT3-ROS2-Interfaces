#ifndef PROTOCOL_LAYER__CRC_CALCULATOR_H
#define PROTOCOL_LAYER__CRC_CALCULATOR_H

#include <cstdint> // Include this for uint16_t and uint8_t


namespace protocol_layer
{
    uint8_t gen_crc8(uint8_t * data, uint16_t len);
}

#endif //PROTOCOL_LAYER__CRC_CALCULATOR_H
