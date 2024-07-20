extends Node

var peer = WebSocketRelayPeer.new()

func _ready():
	peer.create
