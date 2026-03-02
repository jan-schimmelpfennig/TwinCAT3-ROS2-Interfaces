//
// Created by biromed on 12/19/25.
//

#ifndef PROTOCOL_LAYER__MESSAGE_SERIALIZER_H
#define PROTOCOL_LAYER__MESSAGE_SERIALIZER_H
namespace protocol_layer {
    std::size_t serialize(
		toTwinCAT& msg,
		uint8_t* buffer,
		std::size_t buffer_size
	);

	bool deserialize(
		uint8_t* buffer,
		std::size_t buffer_size,
		fromTwinCAT& out
	);
}

#endif //PROTOCOL_LAYER__MESSAGE_SERIALIZER_H