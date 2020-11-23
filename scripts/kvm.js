var server = null;
if(window.location.protocol === 'http:') {
    server = "http://" + window.location.hostname + ":8088/janus";
} else {
    server = "https://" + window.location.hostname + ":8089/janus";
}

var janus = null;
var handle = null;
var opaqueId = "oid-"+Janus.randomString(12);
var bitrateTimer = null;


function sendData(text) {
    handle.data({
        text: text
    });
}

function stopBitrateTimer() {
    if (bitrateTimer) {
        clearInterval(bitrateTimer);
    }
    bitrateTimer = null;
}

function startBitrateTimer() {
    stopBitrateTimer();
    bitrateTimer = setInterval(function() {
        var bitrate = handle.getBitrate();
        $('#bitrate-text').text(bitrate);
        $('#resolution-text').text($("#remotevideo").get(0).videoWidth + "x" + $("#remotevideo").get(0).videoHeight);
    }, 1000);
}

function watchStream() {
    console.log("watchStream");
    var body = { "request": "watch" };
    handle.send({"message": body});
}

// http://www.javascriptkeycode.com/
// https://www.asciitable.com/
// https://api.jquery.com/keyup/
function serializeKey(event) {
    var s = String.fromCharCode(event.which);
    if (event.altKey) {
        s += "a"; // ALT flag
    }
    if (event.ctrlKey) {
        s += "c"; // CTRL flag
    }
    if (event.which >= 65 && event.which <= 90) {
        var code = event.key.charCodeAt(0);
        if (code >= 97) {
            s += "l"; // Lower-case flag
        }
    }
    else if (event.which == 16) {
        if (event.code == "ShiftRight") {
            s += "r"; // Right flag
        }
    }
    else if (event.which == 17) {
        if (event.code == "ControlRight") {
            s += "r"; // Right flag
        }
    }
    else if (event.which == 18) {
        if (event.code == "AltRight") {
            s += "r"; // Right flag
        }
    }
    else if (event.which >= 48 && event.which <= 57) {
        // 0-9
        if (event.shiftKey) {
            s += "s"; // Shift key flag
        }
    }
    else if (event.which >= 186 && event.which <= 222) {
        // ; to '
        if (event.shiftKey) {
            s += "s"; // Shift key flag
        }
    }
    return s;
}

var LastKeyDown;

function startCapture() {
    $(document).keydown(function(event){
        var data = "D" + serializeKey(event);
        if (data == LastKeyDown) {
            return;
        }
        sendData(data);
        LastKeyDown = data;
    });
    $(document).keyup(function(event){
        sendData("U" + serializeKey(event));
        LastKeyDown = null;
    });
}

function stopCapture() {
    $(document).off("keydown");
    $(document).off("keyup");
}

function startStream(jsep) {
    console.log("startStream");
    var body = { request: "start" };
    handle.send({ message: body, jsep: jsep });
}

function stopStream() {
    stopBitrateTimer();
    stopCapture();
    console.log("stopStream");
    var body = { request: "stop" };
    handle.send({ message: body });
    handle.hangup();
}

function attachStream() {
    // Attach to Streaming plugin
    janus.attach({
        plugin: "kvm",
        opaqueId: opaqueId,
        success: function(pluginHandle) {
            handle = pluginHandle;
            Janus.log("Plugin attached! (" + handle.getPlugin() + ", id=" + handle.getId() + ")");
            watchStream();
        },
        error: function(error) {
            Janus.error("  -- Error attaching plugin... ", error);
        },
        iceState: function(state) {
            Janus.log("ICE state changed to " + state);
        },
        webrtcState: function(on) {
            Janus.log("Janus says our WebRTC PeerConnection is " + (on ? "up" : "down") + " now");
        },
        onmessage: function(msg, jsep) {
            Janus.debug(" ::: Got a message :::", msg);
            var result = msg["result"];
            if (result) {
                if (result["status"]) {
                    var status = result["status"];
                    if(status === 'starting') {
                        $("#status-text").text("starting");
                    }
                    else if(status === 'started') {
                        $("#status-text").text("started");
                    }
                    else if(status === 'stopped') {
                        $("#status-text").text("stopped");
                        stopStream();
                    }
                }
            } else if (msg["error"]) {
                console.error(msg["error"]);
                stopStream();
                return;
            }

            if(jsep) {
                Janus.debug("Handling SDP as well...", jsep);
                // Offer from the plugin, let's answer
                handle.createAnswer({
                    jsep: jsep,
                    // We want recvonly audio/video and, if negotiated, datachannels
                    media: { audioSend: false, videoSend: false, data: true },
                    customizeSdp: function(jsep) {
                        // Modify jsep.sdp here
                    },
                    success: function(jsep) {
                        Janus.debug("Got SDP!", jsep);
                        startStream(jsep);
                    },
                    error: function(error) {
                        Janus.error("WebRTC error:", error);
                    }
                });
            }
        },
        ondataopen: function(data) {
            Janus.log("The DataChannel is available!");
            startCapture();
        },
        ondata: function(data) {
            Janus.debug("We got data from the DataChannel: ", data);
        },
        onremotestream: function(stream) {
            Janus.debug(" ::: Got a remote stream :::", stream);
            Janus.attachMediaStream($('#remotevideo').get(0), stream);
            startBitrateTimer();
        },
        oncleanup: function() {
            Janus.log(" ::: Got a cleanup notification :::");
            stopBitrateTimer();
            stopCapture();
        }
    });
}

$(document).ready(function() {
    // Initialize the library (all console debuggers enabled)
    Janus.init({
        debug: "all",
        callback: function() {
            if(!Janus.isWebrtcSupported()) {
                console.error("No WebRTC support");
                return;
            }

            janus = new Janus({
                longPollTimeout: 0,
                server: server,
                success: function() {
                    attachStream();
                },
                error: function(error) {
                    Janus.error(error);
                },
                destroyed: function() {
                    window.location.reload();
                }
            });
        }
    });
});
