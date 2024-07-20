#ifndef SRC_RELAY_PEER_H
#define SRC_RELAY_PEER_H

#include <godot_cpp/classes/multiplayer_peer_extension.hpp>
#include <godot_cpp/classes/multiplayer_peer.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

    /// Manages connections to and from a lightweight relay
    class WebSocketRelayPeer : public MultiplayerPeerExtension {
        GDCLASS(WebSocketRelayPeer, MultiplayerPeerExtension)

    private:
        std::unique_ptr<WebSocketPeer> _socket;
        bool _is_refusing_connections = false;
        int32_t _packet_peer = MultiplayerPeer::TARGET_PEER_BROADCAST;
        bool m_is_server = false;

    public:
        // Overrides
        Error _get_packet(const uint8_t **r_buffer, int32_t *r_buffer_size) override;
        Error _put_packet(const uint8_t *p_buffer, int32_t p_buffer_size) override;
        int32_t _get_available_packet_count() const override;
        int32_t _get_max_packet_size() const override;
//        PackedByteArray _get_packet_script() override;
//        Error _put_packet_script(const PackedByteArray &p_buffer) override;
        int32_t _get_packet_channel() const override;
        MultiplayerPeer::TransferMode _get_packet_mode() const override;
        void _set_transfer_channel(int32_t p_channel) override;
        int32_t _get_transfer_channel() const override;
        void _set_transfer_mode(MultiplayerPeer::TransferMode p_mode) override;
        MultiplayerPeer::TransferMode _get_transfer_mode() const override;
        void _set_target_peer(int32_t p_peer) override;
        int32_t _get_packet_peer() const override;
        bool _is_server() const override;
        void _poll() override;
        void _close() override;
        void _disconnect_peer(int32_t p_peer, bool p_force) override;
        int32_t _get_unique_id() const override;
        void _set_refuse_new_connections(bool p_enable) override;
        bool _is_refusing_new_connections() const override;
        bool _is_server_relay_supported() const override;
        MultiplayerPeer::ConnectionStatus _get_connection_status() const override;

    protected:
        static void _bind_methods();

    public:
        WebSocketRelayPeer();
        ~WebSocketRelayPeer();

        Error create_client(const String &p_url, const Ref<TLSOptions> &p_tls_client_options = nullptr);
        Error create_host(const String &p_url, const Ref<TLSOptions> &p_tls_client_options = nullptr);
    };

}


#endif //SRC_RELAY_PEER_H
