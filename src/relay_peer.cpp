#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/marshalls.hpp>

#include "relay_peer.h"

#define RES_ID_RELAY_MSG 0b00000000
#define RES_ID_COMMAND_RES 0b00000001
#define RES_ID_CONFIRMATION 0b00000010
#define RES_ID_PEER_CONNECTED 0b00000011
#define RES_ID_PEER_DISCONNECTED 0b00000100

#define CONF_JOIN_MATCH 0b00000000
#define CONF_FAILED_JOIN 0b00000001
#define CONF_HOSTED_MATCH 0b00000010
#define CONF_FAILED_HOST 0b00000011
#define CONF_CONNECTED 0b00000100

#define CMD_PREFIX 0b00000000
#define RELAY_PREFIX 0b00000001

using namespace godot;

WebSocketRelayPeer::WebSocketRelayPeer() {
    WebSocketRelayPeer::m_socket = std::make_unique<WebSocketPeer>();
}
WebSocketRelayPeer::~WebSocketRelayPeer() {}

void WebSocketRelayPeer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("join_match", "match_id"), &WebSocketRelayPeer::join_match);
    ClassDB::bind_method(D_METHOD("host_match", "name"), &WebSocketRelayPeer::host_match);
    ClassDB::bind_method(D_METHOD("connect_to_relay", "url", "options"), &WebSocketRelayPeer::connect_to_relay);
    ClassDB::bind_method(D_METHOD("refresh_match_list"), &WebSocketRelayPeer::refresh_match_list);
    ClassDB::bind_method(D_METHOD("get_client_desc", "peer_id"), &WebSocketRelayPeer::get_client_desc);
    ClassDB::bind_method(D_METHOD("peers"), &WebSocketRelayPeer::get_client_desc);

    ADD_SIGNAL(MethodInfo("match_list_refreshed", PropertyInfo(Variant::ARRAY, "match_list")));
    ADD_SIGNAL(MethodInfo("join_confirmed", PropertyInfo(Variant::BOOL, "succeeded")));
    ADD_SIGNAL(MethodInfo("host_confirmed", PropertyInfo(Variant::BOOL, "succeeded")));
    ADD_SIGNAL(MethodInfo("connection_confirmed", PropertyInfo(Variant::STRING, "network_id")));
}

Error WebSocketRelayPeer::_get_packet(const uint8_t **r_buffer, int32_t *r_buffer_size) {
    UtilityFunctions::print("Getting packet");


    if (m_incoming_packets.empty()) {
        return Error::OK;
    }

    Packet packet = m_incoming_packets.front();
    String sender_id = packet.m_data.slice(0, 24).get_string_from_ascii();

    // Using the prefix, assign the packet peer
    for (auto pair : clientDescByPeerID) {
        if (pair.second.get("uuid") == sender_id)
            UtilityFunctions::print("Got a match for sender id");
            m_packet_peer = pair.first;
    }

    PackedByteArray payload = packet.m_data.slice(24);

    *r_buffer_size = payload.size();
    *r_buffer = payload.ptr();
    uint8_t *vbuffer = payload.ptrw();
    uint8_t packet_type = vbuffer[0] & 0b00001111;
    char cbuffer [50];
    int n = std::sprintf(cbuffer, "RECEIVED CMD TYPE: %d", packet_type);
    UtilityFunctions::print(cbuffer);

    m_incoming_packets.pop();

    return Error::OK;
}


Error WebSocketRelayPeer::_put_packet(const uint8_t *p_buffer, int32_t p_buffer_size) {
    PackedByteArray buffer;
    // If it's going through put_packet directly then its a relay message
    buffer.append(RELAY_PREFIX);
    // Append the peer id data to the prefix
    int32_t peerId = m_target_peer;
    String networkPeerId = clientDescByPeerID[peerId].get("uuid");
    CharString peerIdAscii = networkPeerId.ascii();
    const char* peerIdBytes = peerIdAscii.ptr();

    for (int8_t i = 0; i < 24; i++) {
        // We are putting all 24 bytes of the peer ID
        buffer.append(peerIdBytes[i]);
    }

    uint8_t packet_type = p_buffer[0] & 0b00001111;
    char cbuffer [50];
    int n = std::sprintf(cbuffer, "CMD TYPE: %d", packet_type);
    UtilityFunctions::print(cbuffer);
    for (int32_t i = 0; i < p_buffer_size; i++) {
        // We are append the rest of the bytes that make up the packet
        buffer.append(p_buffer[i]);
    }
    return m_socket->put_packet(buffer);
}

int32_t WebSocketRelayPeer::_get_available_packet_count() const {
    return m_incoming_packets.size();
}

int32_t WebSocketRelayPeer::_get_max_packet_size() const {
    return m_socket->get_encode_buffer_max_size();
}

int32_t WebSocketRelayPeer::_get_packet_channel() const {
    return 0;
}

MultiplayerPeer::TransferMode WebSocketRelayPeer::_get_packet_mode() const {
    return MultiplayerPeer::TransferMode::TRANSFER_MODE_RELIABLE;
}

void WebSocketRelayPeer::_set_transfer_channel(int32_t p_channel) {
    // Do not allow channels to be set
    return;
}
int32_t WebSocketRelayPeer::_get_transfer_channel() const {
    return 0;
}

void WebSocketRelayPeer::_set_transfer_mode(MultiplayerPeer::TransferMode p_mode) {
    return;
}

MultiplayerPeer::TransferMode WebSocketRelayPeer::_get_transfer_mode() const {
    return MultiplayerPeer::TransferMode::TRANSFER_MODE_RELIABLE;
}

void WebSocketRelayPeer::_set_target_peer(int32_t p_peer) {
    m_target_peer = p_peer;
}

int32_t WebSocketRelayPeer::_get_packet_peer() const {
    return m_packet_peer;
}

bool WebSocketRelayPeer::_is_server() const {
    return m_is_server;
}

void WebSocketRelayPeer::_poll() {
    m_socket->poll();
    WebSocketPeer::State state = m_socket->get_ready_state();
    PackedByteArray data;
    switch (state) {
        case WebSocketPeer::State::STATE_OPEN:
            data = m_socket->get_packet();
            process_message(data);
            return;
        case WebSocketPeer::State::STATE_CLOSED:
            UtilityFunctions::print("State closed");
        case WebSocketPeer::State::STATE_CLOSING:
            UtilityFunctions::print("State closing");
        case WebSocketPeer::State::STATE_CONNECTING:
        default:
            return;
    }
}

void WebSocketRelayPeer::process_message(PackedByteArray message) {
    /// Read the classifying byte
    if (message.is_empty()) {
        return;
    }

    uint8_t classifyingByte = message[0];
    if (classifyingByte == RES_ID_RELAY_MSG) {
        Packet packet;
        /// We need everything after the classifying byte
        packet.m_data = message.slice(1);
        m_incoming_packets.push(packet);
        return;
    }
    else if (classifyingByte == RES_ID_COMMAND_RES) {
        PackedByteArray json_message = message.slice(1);
        auto obj = JSON::parse_string(json_message.get_string_from_ascii());
        emit_signal("match_list_refreshed", obj);
        return;
    }
    else if (classifyingByte == RES_ID_CONFIRMATION) {
        switch (message[1]) {
            case CONF_CONNECTED:
                UtilityFunctions::print("Confirming connection!");
                emit_signal("connection_confirmed", message.slice(2).get_string_from_ascii());
            case CONF_JOIN_MATCH:
                UtilityFunctions::print("Confirming joined!");
            default:
                return;
        }
    }
    else if (classifyingByte == RES_ID_PEER_CONNECTED) {
        PackedByteArray client_desc = message.slice(1);
        auto data = client_desc.get_string_from_ascii();
        int32_t peer_id = generate_unique_id();
        clientDescByPeerID[peer_id] = JSON::parse_string(client_desc.get_string_from_ascii());
        emit_signal("peer_connected", peer_id);
    }
    else if (classifyingByte == RES_ID_PEER_DISCONNECTED) {
        UtilityFunctions::print("Disconnection message");
    }
//    else if (classifyingByte == RES_ID_CONFIRMATION) {
//        PackedByteArray json_
//    }
}

Variant WebSocketRelayPeer::get_client_desc(int32_t peer_id) {
    return clientDescByPeerID[peer_id];
}

Array WebSocketRelayPeer::peers() {
    Array keys;
    for (auto pair : clientDescByPeerID) {
        keys.append(pair.first);
    }

    return keys;
}

void WebSocketRelayPeer::_close() {
    m_socket->close();
}

void WebSocketRelayPeer::_disconnect_peer(int32_t p_peer, bool p_force) {
    // Send message to disconnect the given peer
}

int32_t WebSocketRelayPeer::_get_unique_id() const {
    return WebSocketRelayPeer::generate_unique_id();
}

void WebSocketRelayPeer::_set_refuse_new_connections(bool p_enable) {
    m_is_refusing_connections = p_enable;
}

bool WebSocketRelayPeer::_is_refusing_new_connections() const {
    return m_is_refusing_connections;
}

bool WebSocketRelayPeer::_is_server_relay_supported() const {
    return false;
}

MultiplayerPeer::ConnectionStatus WebSocketRelayPeer::_get_connection_status() const {
    WebSocketPeer::State state = m_socket->get_ready_state();
    switch (state) {
        case WebSocketPeer::State::STATE_CONNECTING:
            return MultiplayerPeer::ConnectionStatus::CONNECTION_CONNECTING;
        case WebSocketPeer::State::STATE_OPEN:
            return MultiplayerPeer::ConnectionStatus::CONNECTION_CONNECTED;
        case WebSocketPeer::State::STATE_CLOSED:
            UtilityFunctions::print("State closed");
        case WebSocketPeer::State::STATE_CLOSING:
            UtilityFunctions::print("State closing");
        default:
            return MultiplayerPeer::ConnectionStatus::CONNECTION_DISCONNECTED;
    }
}

Error WebSocketRelayPeer::join_match(PackedByteArray match_id) {
    UtilityFunctions::print("Joining match!");
    PackedByteArray packet;
    packet.append(CMD_PREFIX);

    std::string command = "{\"action\": \"join_match\", \"uuid\": \"" + std::string(Marshalls::get_singleton()->raw_to_base64(match_id).ascii().ptrw()) + "\"}";

    for (auto ch: command) {
        packet.append(ch);
    }

    return m_socket->put_packet(packet);
}

Error WebSocketRelayPeer::host_match(String name) {
    PackedByteArray packet;
    packet.append(CMD_PREFIX);

    std::string command = "{\"action\": \"host_match\", \"name\": \"" + std::string(name.ascii().ptrw()) + "\"}";

    for (auto ch: command) {
        packet.append(ch);
    }

    return m_socket->put_packet(packet);
}

Error WebSocketRelayPeer::connect_to_relay(const String &p_url, const Ref<TLSOptions> &p_tls_client_options) {
    return m_socket->connect_to_url(p_url, p_tls_client_options);
}

Error WebSocketRelayPeer::refresh_match_list() {
    PackedByteArray packet;
    packet.append(CMD_PREFIX);

    std::string command = "{\"action\": \"list_matches\"}";

    for (auto ch: command) {
        packet.append(ch);
    }
    return m_socket->put_packet(packet);
}