#include "relay_peer.h"

#define RES_ID_CONNECTION_MSG 0
#define RES_ID_RELAY_MSG 1

using namespace godot;

WebSocketRelayPeer::WebSocketRelayPeer() {
    WebSocketRelayPeer::m_socket = std::make_unique<WebSocketPeer>();
}
WebSocketRelayPeer::~WebSocketRelayPeer() {}

void WebSocketRelayPeer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("create_client", "url", "options"), &WebSocketRelayPeer::create_client);
    ClassDB::bind_method(D_METHOD("create_host", "url", "options"), &WebSocketRelayPeer::create_host);
}

Error WebSocketRelayPeer::_get_packet(const uint8_t **r_buffer, int32_t *r_buffer_size) {
    if (m_incoming_packets.empty()) {
        return;
    }

    Packet packet = m_incoming_packets.pop();
    *r_buffer_size = packet.data.size();
    *r_buffer = packet.data.ptr();

    return
}


Error WebSocketRelayPeer::_put_packet(const uint8_t *p_buffer, int32_t p_buffer_size) {
    PackedByteArray buffer;
    for (int32_t i = 0; i < p_buffer_size; i++) {
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
    switch (state) {
        case WebSocketPeer::State::STATE_OPEN:
            PackedByteArray data = m_socket->get_packet();
            process_message(data);
            return;
        case WebSocketPeer::State::STATE_CLOSED:
        case WebSocketPeer::State::STATE_CLOSING:
        case WebSocketPeer::State::STATE_CONNECTING:
        default:
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

Error WebSocketRelayPeer::create_client(const String &p_url, const Ref<TLSOptions> &p_tls_client_options) {
    return m_socket->connect_to_url(p_url, p_tls_client_options);
}

Error WebSocketRelayPeer::create_host(const String &p_url, const Ref<TLSOptions> &p_tls_client_options) {
    return m_socket->connect_to_url(p_url, p_tls_client_options);
}