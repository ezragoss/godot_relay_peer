class_name TestController
extends Control


const URL = "localhost:1234"
const HOST_COLOR = "842835"
const CLIENT_COLOR = "2b4598"
const BASE_COLOR = "626368"

var peer := WebSocketRelayPeer.new()
var new_peer = WebSocketPeer.new()

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

# Called when the node enters the scene tree for the first time.
func _ready():
	self.get_window().title = "%s" % randf_range(0, 1000)
	peer.match_list_refreshed.connect(_handle_match_list_refreshed)
	#multiplayer.multiplayer_peer = peer
	


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
	BackgroundColor.color = CLIENT_COLOR
	IsHost.text = "Is Client"
	IsInMatch.text = "Is In Match"
	pass
	
func _leave_game():
	BackgroundColor.color = BASE_COLOR
	IsHost.text = "Is Not Host"
	IsInMatch.text = "Not In Match"

func _connect_to_server():
	_is_connected = true
	var formatted = "ws://%s" % URL
	print("Connecting to %s" % formatted)
	#var err = new_peer.connect_to_url(formatted)
	var err = peer.connect_to_relay(URL, null)
	if err:
		print(err)

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
		_increment_peer_ctr.rpc(select)
	
@rpc("any_peer", "call_local", "reliable")
func _increment_broadcast_ctr():
	var current := int(AllCtr.text)
	current += 1
	AllCtr.text = "%s" % current
	
func increment_broadcast_ctr():
	_increment_broadcast_ctr.rpc()
	
	
func refresh_matches():
	peer.refresh_match_list()
