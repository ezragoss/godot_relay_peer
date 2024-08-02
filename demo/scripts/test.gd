class_name TestController
extends Control


const URL = "localhost:1234"
const HOST_COLOR = "842835"
const CLIENT_COLOR = "2b4598"
const BASE_COLOR = "626368"

var peer := WebSocketRelayPeer.new()
var connected_peers_list: Array;

var network_id = null

var session_uid = null

var match_list: Array = []:
	set(new):
		match_list = new
		if not is_node_ready():
			return
		
		MatchList.clear()
		for entry in new:
			MatchList.add_item(entry["name"])
			

var is_in_match: bool:
	set(new):
		is_in_match = new
		if not is_node_ready():
			return
			
		JoinBtn.disabled = new


@onready var BackgroundColor : ColorRect = %BackgroundColor
@onready var MatchList := %MatchList
@onready var ConnectedPeersList := %ConnectedPeersList
@onready var IsHost := %IsHost
@onready var IsInMatch := %IsInMatch
@onready var HostCtr := %IncrementHostCtr
@onready var MyCtr := %MyCtr
@onready var AllCtr := %IncrementAllCtr
@onready var GameName := %GameName
@onready var Username := %Username
@onready var ConnectedLabel := %ConnectedLabel
@onready var ConnectBtn := %ConnectBtn
@onready var JoinBtn := %Join

var _is_connected := false:
	set(new):
		ConnectedLabel.text = "Connected" if new else "Not Connected"
		_is_connected = new


func _handle_match_list_refreshed(list: Array):
	match_list = list
	
func _handle_peer_connected(peer_id: int):
	print("Connected %s" % peer.get_client_desc(peer_id))
	var description = peer.get_client_desc(peer_id)
	if description["uuid"] == network_id:
		return
	ConnectedPeersList.add_item(description["username"])
	connected_peers_list.append(peer_id)
	
func _handle_connection_confirmed(network_id: String):
	_is_connected = true
	self.network_id = network_id
	

# Called when the node enters the scene tree for the first time.
func _ready():
	session_uid = "%s" % randi_range(0, 1000)
	
	self.get_window().title = session_uid
	peer.match_list_refreshed.connect(_handle_match_list_refreshed)
	peer.peer_connected.connect(_handle_peer_connected)
	peer.connection_confirmed.connect(_handle_connection_confirmed)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	peer.poll()


func _host_game():
	BackgroundColor.color = HOST_COLOR
	IsHost.text = "Is Host" 
	IsInMatch.text = "In Match"
	var current_name = GameName.text
	peer.host_match(current_name)
	
func _join_game():
	var selected = MatchList.get_selected_items()
	if not selected:
		print("No match selected")
		return
		
	var index = selected[0]
	var data = match_list[index]
	
	peer.join_match(Marshalls.base64_to_raw(data["guid"]))
	
	BackgroundColor.color = CLIENT_COLOR
	IsHost.text = "Is Client"
	IsInMatch.text = "Is In Match"
	pass
	
func _leave_game():
	BackgroundColor.color = BASE_COLOR
	IsHost.text = "Is Not Host"
	IsInMatch.text = "Not In Match"

func _connect_to_server():
	var formatted = "ws://%s" % URL
	print("Connecting to %s" % formatted)
	var err = peer.connect_to_relay(URL, null)
	if err:
		print(err)
		return
		
	multiplayer.multiplayer_peer = peer

@rpc("any_peer", "call_local", "reliable")
func _increment_host_ctr():
	var current := int(HostCtr.text)
	current += 1
	HostCtr.text = "%s" % current
	
func increment_host_ctr():
	_increment_host_ctr.rpc(1)
	
@rpc("any_peer", "call_remote", "reliable")
func _increment_peer_ctr():
	var current := int(MyCtr.text)
	current += 1
	MyCtr.text = "%s" % current
	
func increment_peer_ctr():
	var selected : Array = ConnectedPeersList.get_selected_items()
	if not selected:
		return
	
	for select in selected:
		var peer_id = connected_peers_list[select]
		_increment_peer_ctr.rpc_id(peer_id)
	
@rpc("any_peer", "call_local", "reliable")
func _increment_broadcast_ctr():
	var current := int(AllCtr.text)
	current += 1
	AllCtr.text = "%s" % current
	
func increment_broadcast_ctr():
	_increment_broadcast_ctr.rpc()
	
	
func refresh_matches():
	peer.refresh_match_list()
