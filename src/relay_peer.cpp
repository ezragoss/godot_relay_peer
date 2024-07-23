#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "relay_peer.h"

#define RES_ID_CONNECTION_MSG 0b00000000
#define RES_ID_RELAY_MSG 0b00000001
#define RES_ID_COMMAND_RES 0b00000010
#define RES_ID_CONFIRMATION 0b00000011

#define CONF_JOIN_MATCH = 0b00000000
#define CONF_FAILED_JOIN = 0b00000001
#define CONF_HOSTED_MATCH = 0b00000010
#define CONF_FAILED_HOST = 0b00000011

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

    ADD_SIGNAL(MethodInfo("match_list_refreshed", PropertyInfo(Variant::ARRAY, "match_list")));
    ADD_SIGNAL(MethodInfo("join_confirmed", PropertyInfo(Variant::BOOL, "succeeded")));
    ADD_SIGNAL(MethodInfo("host_confirmed", PropertyInfo(Variant::BOOL, "succeeded")));
}

Error WebSocketRelayPeer::_get_packet(const uint8_t **r_buffer, int32_t *r_buffer_size) {
    if (m_incoming_packets.empty()) {
        return Error::OK;
    }

    Packet packet = m_incoming_packets.back();
    *r_buffer_size = packet.m_data.size();
    *r_buffer = packet.m_data.ptr();

    return Error::OK;
}


Error WebSocketRelayPeer::_put_packet(const uint8_t *p_buffer, int32_t p_buffer_size) {
    PackedByteArray buffer;
    // If it's going through put_packet directly then its a relay message
    buffer.append(RELAY_PREFIX);
    // Append the peer id data to the prefix
    int32_t peerId = _get_packet_peer();
    char* peerIdBytes = reinterpret_cast<char*>(&peerId);
    for (int8_t i = 0; i < 4; i++) {
        // We are putting all 32 bytes of the peer ID
        buffer.append(peerIdBytes[i]);
    }
    for (int32_t i = 0; i < p_buffer_size; i++) {
        // We are append the rest of the bytes that make up the packet
        buffer.append(p_buffer[i]);
    }
    return m_socket->put_packet(buffer);
}

int32_t WebSocketRelayPeer::_get_available_packet_count() const {
    return m_socket->get_available_packet_count();
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
    m_packet_peer = p_peer;
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
        case WebSocketPeer::State::STATE_CLOSING:
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
    else if (classifyingByte == RES_ID_CONNECTION_MSG) {}
    else if (classifyingByte == RES_ID_CONFIRMATION) {
        PackedByteArray json_
    }
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
        case WebSocketPeer::State::STATE_CLOSING:
        default:
            return MultiplayerPeer::ConnectionStatus::CONNECTION_DISCONNECTED;
    }
}

Error WebSocketRelayPeer::join_match(String match_id) {
    return Error::OK;
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