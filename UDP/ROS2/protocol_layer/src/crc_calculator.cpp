//
// Created by jan on 12/19/25.
//

#include "protocol_layer/crc_calculator.h"
#define CRC16 0x8005

namespace protocol_layer {
    uint8_t gen_crc8(uint8_t * data, uint16_t len)
    {
        uint8_t crc = 0x00;  // Start with CRC value 0
        uint16_t i, j;
        for (i = 2; i < len; i++) {  //ATTENTION: Start from the 3rd byte (skip first two)
            crc ^= data[i];  // XOR the current byte with CRC
            for (j = 0; j < 8; j++) {  // Loop through each bit in the byte
                if ((crc & 0x80) != 0)  // Check if MSB is 1
                    crc = (crc << 1) ^ 0x07;  // If MSB is 1 XOR with polynomial 0x07 (0b0111)
                else
                    crc <<= 1;  // otherwise just shift left
            }
        }
        return crc;
    }
}