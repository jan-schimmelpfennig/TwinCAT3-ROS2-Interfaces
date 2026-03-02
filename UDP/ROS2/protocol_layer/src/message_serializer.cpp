//
// Created by biromed on 12/19/25.
//



#include <cstdint>
#include "protocol_layer/crc_calculator.h"
#include "protocol_layer/message_definitions.h"
#include "protocol_layer/message_serializer.h"
#include <rclcpp/rclcpp.hpp>
#include <cstring>

namespace protocol_layer
{
    std::size_t serialize(toTwinCAT& msg, uint8_t* buffer, std::size_t buffer_size) {
        if (buffer_size < sizeof(toTwinCAT))
        {
            RCLCPP_ERROR(rclcpp::get_logger("protocol_layer"),
               "Buffer size too small for serialization. Expected size: %lu, got: %zu",
               sizeof(toTwinCAT), buffer_size);
            return 0; // buffer is too small
        }
        //set keyword
        msg.keyword = FrameConstants::KEYWORD;
        //calculate the checksum
        msg.crc8 = gen_crc8(reinterpret_cast<uint8_t*>(&msg), sizeof(toTwinCAT));
        // Copy the entire struct to the buffer at once
        std::memcpy(buffer, &msg, sizeof(toTwinCAT));
        // Return the number of bytes written
        return sizeof(toTwinCAT); // the total bytes that were written
    }

    bool deserialize(uint8_t* buffer, std::size_t buffer_size, fromTwinCAT& out)
    {
        if (buffer_size < sizeof(fromTwinCAT))
        {
            RCLCPP_ERROR(rclcpp::get_logger("protocol_layer"),
                "Buffer size too small for deserilaization. Expected size: %lu, got: %zu",
                sizeof(fromTwinCAT), buffer_size);
            return false; // buffer is too small
        }

        // Copy the entire struct from the buffer into the out object
        std::memcpy(&out, buffer, sizeof(fromTwinCAT));

        //check checksum
        uint8_t expected_crc8 = gen_crc8(buffer, buffer_size);
        if (expected_crc8 != out.crc8)
        {
            RCLCPP_ERROR(rclcpp::get_logger("protocol_layer"),
                "CRC mismatch. Expected CRC: 0x%02x, got: 0x%02x", expected_crc8, out.crc8
                );
            return false;
        }

        //check keyword: the keyword of the outgoing and ingoing messages should be the same
        if (out.keyword != FrameConstants::KEYWORD)
        {
            RCLCPP_ERROR(rclcpp::get_logger("protocol_layer"),
                "Keyword mismatch. Expected: 0x%02x, got: 0x%02x", FrameConstants::KEYWORD, out.keyword);
            return false;
        }

        return true;
    }
}