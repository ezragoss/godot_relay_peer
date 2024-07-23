#ifndef RELAY_GODOT_MULTIPLAYER_PACKET_H
#define RELAY_GODOT_MULTIPLAYER_PACKET_H

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

/// Our packet protocol
class Packet : RefCounted {
public:
    PackedByteArray m_data;
};

#endif //RELAY_GODOT_MULTIPLAYER_PACKET_H
