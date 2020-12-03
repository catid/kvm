// Copyright 2020 Christopher A. Taylor

// Select HTTPS backend if HTTPS is used for front-end
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
        var video = $("#remotevideo").get(0);
        var w = video.videoWidth;
        var h = video.videoHeight;
        $('#resolution-text').text(w + "x" + h);
    }, 1000);
}

function watchStream() {
    console.log("watchStream");
    var body = { "request": "watch" };
    handle.send({"message": body});
}

const kLeftCtrl = 1 << 0;
const kLeftShift = 1 << 1;
const kLeftAlt = 1 << 2;
const kLeftSuper = 1 << 3;
const kRightCtrl = 1 << 4;
const kRightShift = 1 << 5;
const kRightAlt = 1 << 6;
const kRightSuper = 1 << 7;

// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf#page=53
// https://wiki.osdev.org/USB_Human_Interface_Devices
// http://www.javascriptkeycode.com/
// https://www.asciitable.com/
// https://api.jquery.com/keyup/
function convertKey(event) {
    // Modifiers
    var modifier_keys = 0;
    if (event.ctrlKey) {
        modifier_keys |= kLeftCtrl;
    }
    if (event.shiftKey) {
        modifier_keys |= kLeftShift;
    }
    if (event.altKey) {
        modifier_keys |= kLeftAlt;
    }

    // Scan code
    var code = 0;
    if (event.which >= 65 && event.which <= 90) {
        code = event.which - 65 + 4; // Letter keys
        if (event.key.charCodeAt(0) < 97) {
            modifier_keys |= kLeftShift;
        }
    }
    else if (event.which >= 48 && event.which <= 57) {
        // 0-9
        switch (event.key) {
        case ')': // fall-thru
        case '0': code = 39; break;
        case '!': // fall-thru
        case '1': code = 30; break;
        case '@': // fall-thru
        case '2': code = 31; break;
        case '#': // fall-thru
        case '3': code = 32; break;
        case '$': // fall-thru
        case '4': code = 33; break;
        case '%': // fall-thru
        case '5': code = 34; break;
        case '^': // fall-thru
        case '6': code = 35; break;
        case '&': // fall-thru
        case '7': code = 36; break;
        case '*': // fall-thru
        case '8': code = 37; break;
        case '(': // fall-thru
        case '9': code = 38; break;
        }
    }
    else if (event.which >= 96 && event.which <= 111) {
        // NumPad
        switch (event.key) {
        case 96: code = 98; break; // NumPad 0
        case 97: code = 89; break; // NumPad 1
        case 98: code = 90; break; // NumPad 2
        case 99: code = 91; break; // NumPad 3
        case 100: code = 92; break; // NumPad 4
        case 101: code = 93; break; // NumPad 5
        case 102: code = 94; break; // NumPad 6
        case 103: code = 95; break; // NumPad 7
        case 104: code = 96; break; // NumPad 8
        case 105: code = 97; break; // NumPad 9
        case 106: code = 85; break; // NumPad *
        case 107: code = 87; break; // NumPad +
        case 108: code = 99; break; // NumPad . (period)
        case 109: code = 86; break; // NumPad -
        case 110: code = 99; break; // NumPad . (decimal point)
        case 111: code = 84; break; // NumPad /
        }
    }
    else if (event.which >= 112 && event.which <= 123) {
        code = event.which - 112 + 58; // F1 - F12
    }
    else if (event.which >= 186 && event.which <= 222) {
        // ; to '
        switch (event.key) {
        case '-': // fall-thru
        case '_': code = 45; break;
        case '=': // fall-thru
        case '+': code = 46; break;
        case '[': // fall-thru
        case '{': code = 47; break;
        case ']': // fall-thru
        case '}': code = 48; break;
        case '\\': // fall-thru
        case '|': code = 49; break;
        case ';': // fall-thru
        case ':': code = 51; break;
        case '\'': // fall-thru
        case '"': code = 52; break;
        case '`': // fall-thru
        case '~': code = 53; break;
        case ',': // fall-thru
        case '<': code = 54; break;
        case '.': // fall-thru
        case '>': code = 55; break;
        case '/': // fall-thru
        case '?': code = 56; break;
        }
    }
    else if (event.which == 16) {
        if (event.code == "ShiftRight") {
            code = 229; // RightShift
        } else {
            code = 225; // LeftShift
        }
    }
    else if (event.which == 17) {
        if (event.code == "ControlRight") {
            code = 228; // RightControl
        } else {
            code = 224; // LeftControl
        }
    }
    else if (event.which == 18) {
        if (event.code == "AltRight") {
            code = 230; // RightAlt
        } else {
            code = 226; // LeftAlt
        }
    }
    else {
        switch (event.which) {
        case 13: code = 40; break; // Return
        case 27: code = 41; break; // Escape
        case 8: code = 42; break; // BackSpace
        case 9: code = 43; break; // Tab
        case 32: code = 44; break; // Space
        case 91: code = 227; break; // Left Super
        case 92: code = 231; break; // Right Super
        case 20: code = 57; break; // Caps Lock
        case 44: code = 70; break; // PrintScreen
        case 144: code = 83; break; // Num Lock
        case 145: code = 71; break; // Scroll Lock
        case 19: code = 72; break; // Pause
        case 45: code = 73; break; // Insert
        case 36: code = 74; break; // Home
        case 33: code = 75; break; // PageUp
        case 46: code = 76; break; // Delete
        case 35: code = 77; break; // End
        case 34: code = 78; break; // PageDown
        case 39: code = 79; break; // RightArrow
        case 37: code = 80; break; // LeftArrow
        case 40: code = 81; break; // DownArrow
        case 38: code = 82; break; // UpArrow
        case 173: code = 127; break; // Mute
        case 174: code = 129; break; // VolumeDown
        case 175: code = 128; break; // VolumeUp
        case 93: code = 119; break; // Select
        case 177: code = 182; break; // PrevTrack
        case 176: code = 181; break; // NextTrack
        case 179: code = 205; break; // PlayPause
        }
    }

    return [modifier_keys, code];
}

var NextIdentifier = 0;
var KeysDown = [];
var RecentReports = [];
var SendTimer = null;

// We need to send 8 bits per byte in a string, because Janus only supports sending strings.
// But JavaScript cannot convert values with the high bit set to a single byte string AFAIK,
// so this is encoding the high bits for every 7 bytes in one additional byte.
function convertUint8ArrayToBinaryString(u8Array) {
    var extra_bits = 0;
    var extra_count = 0;

    var b_str = "";
    var len = u8Array.length;

    for (var i = 0; i < len; i++) {
        var x = u8Array[i];

        // Clear high bit because otherwise it trigger weird Unicode conversions
        b_str += String.fromCodePoint(x & 0x7f);

        if (x & 0x80) {
            extra_bits |= 1 << extra_count;
        }
        ++extra_count;

        // Every 7 bytes, output an extra byte to store the missing high bits
        if (extra_count >= 7) {
            b_str += String.fromCodePoint(extra_bits);
            extra_count = 0;
            extra_bits = 0;
        }
    }

    // Add any extra bits
    if (extra_count >= 1) {
        b_str += String.fromCodePoint(extra_bits);
    }

    return b_str;
}

function addReportWithKeysDown(modifier_keys) {
    NextIdentifier++;
    if (NextIdentifier >= 256) {
        NextIdentifier = 0;
    }

    // Make a copy of the current keys that are down
    var keys = KeysDown.slice();

    // Check if there are any non-modifier keys
    var found_non_modifier = false;
    for (var i = 0; i < keys.length; ++i) {
        // If this key is a non-modifier:
        var key = keys[i];
        if (key < 224 || key > 231) {
            found_non_modifier = true;
            break;
        }
    }

    // If there are non-modifier keys:
    if (found_non_modifier) {
        // Eliminate any modifier keys that are already captured by the bitfield
        for (var i = 0; i < keys.length; ++i) {
            // If this key is a modifier:
            var key = keys[i];
            if (key >= 224 && key <= 231) {
                switch (key) {
                case 224: // fall-thru: Left Control
                case 228: // Right Control
                    if ((modifier_keys & (kLeftCtrl | kRightCtrl)) == 0) {
                        // If modifier key does not capture this, then keep it.
                        continue;
                    }
                    break;

                case 225: // fall-thru: Left Shift
                case 229: // Right Shift
                    if ((modifier_keys & (kLeftShift | kRightShift)) == 0) {
                        // If modifier key does not capture this, then keep it.
                        continue;
                    }
                    break;

                case 226: // fall-thru: Left Alt
                case 230: // Right Alt
                    if ((modifier_keys & (kLeftAlt | kRightAlt)) == 0) {
                        // If modifier key does not capture this, then keep it.
                        continue;
                    }
                    break;

                case 227: // fall-thru: Left Super
                case 231: // Right Super
                    if ((modifier_keys & (kLeftSuper | kRightSuper)) == 0) {
                        // If modifier key does not capture this, then keep it.
                        continue;
                    }
                    break;
                }
                keys.splice(i, 1);
                // Fix array index
                --i;
            }
        }
    }
    // Note: Bitfield flags for modifier keys should be retained or e.g. ALT key will not activate menus.

    var count = keys.length;
    if (count > 6) {
        count = 6;
    }

    var report = [NextIdentifier, 1 + count, modifier_keys];
    for (var i = 0; i < count; ++i) {
        report.push(keys[i]);
    }

    RecentReports.push(report);
    if (RecentReports.length > 8) {
        RecentReports.shift();
    }
}

function sendReports() {
    if (RecentReports.length > 0) {
        var arr = [];
        for (var i = 0; i < RecentReports.length; ++i) {
            arr = arr.concat(RecentReports[i]);
        }

        sendData(convertUint8ArrayToBinaryString(arr));
    }
}

function pressKey(data) {
    // If key is already down:
    if (KeysDown.find(code => code == data[1])) {
        return;
    }

    // Add key
    KeysDown.push(data[1]);

    addReportWithKeysDown(data[0]);
    sendReports();
}
function releaseKey(data) {
    // If key is already up:
    if (!KeysDown.find(code => code == data[1])) {
        return;
    }

    // Remove code from KeysDown array
    for (var i = 0; i < KeysDown.length; i++) {
        if (KeysDown[i] === data[1]) {
            KeysDown.splice(i, 1);
            break;
        }
    }

    addReportWithKeysDown(data[0]);
    sendReports();
}

function startCapture() {
    $(document).keydown(function(event){
        event.preventDefault();
        pressKey(convertKey(event));
    });
    $(document).keyup(function(event){
        event.preventDefault();
        releaseKey(convertKey(event));
    });

    $(document).mouseup(function(event){
        event.preventDefault();
    });
    $(document).mousedown(function(event){
        event.preventDefault();
    });
    $(document).mouseover(function(event){
        event.preventDefault();
    });
    $(document).click(function(event){
        // Disable left click
        event.preventDefault();
    });
    $(document).contextmenu(function(event){
        // Disable right-click (causes video to show properties menu)
        event.preventDefault();
    });

    $("#af4_key").click(function(event){
        // Scan codes from https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf#page=53
        pressKey([kLeftAlt, 61]);
        releaseKey([0, 61]);
    });
    $("#cad_key").click(function(event){
        pressKey([kLeftAlt|kLeftCtrl, 76]);
        releaseKey([0, 76]);
    });
    $("#super_key").click(function(event){
        pressKey([kLeftSuper, 227]);
        releaseKey([0, 227]);
    });

    // Send reports every 60 milliseconds -> ~500 bytes/second.
    // This is done to fill in for lost packets.
    SendTimer = setInterval(function() {
        sendReports();
    }, 60);
}

function stopCapture() {
    $(document).off("keydown");
    $(document).off("keyup");
    if (SendTimer) {
        clearInterval(SendTimer);
        SendTimer = null;
    }
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
                    }
                }
            } else if (msg["error"]) {
                console.error(msg["error"]);
                stopStream();
                watchStream();
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
