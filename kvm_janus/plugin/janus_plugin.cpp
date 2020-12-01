// Copyright 2020 Christopher A. Taylor

/*
    Plugin for Meetecho's Janus Gateway WebRTC server

    Runs the kvm_pipeline video pipeline, which produces H.264 video in Annex B format.
    We packetize the video into RTP packets, which are then provided to Janus.
    Janus encrypts the RTP into SRTP/UDP datagrams that are transmitted to connected browsers.

    We also provide a WebRTC data-channel for receiving low-latency packets from the browsers.
    The browser uses the data-channel interface to transmit keystrokes to the input emulator.
*/

#include <janus/plugins/plugin.h>

#include <mutex>
#include <vector>
#include <sstream>

#include "kvm_pipeline.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("plugin");

#include <jansson.h>

static VideoPipeline m_Pipeline;

struct ClientData
{
    janus_plugin_session* handle = nullptr;
    bool transmit = true;
};

static std::mutex m_Lock;
static std::vector<ClientData> m_Clients;

static janus_callbacks* m_Callbacks = nullptr;

static RtpPayloader m_Payloader;

static PipelineNode m_WorkerNode;

extern janus_plugin m_Plugin;

/*! \brief Plugin initialization/constructor
    * @param[in] callback The callback instance the plugin can use to contact the Janus core
    * @param[in] config_path Path of the folder where the configuration for this plugin can be found
    * @returns 0 in case of success, a negative integer in case of error */
int plugin_init(janus_callbacks* callback, const char* /*config_path*/)
{
    m_Callbacks = callback;

    m_WorkerNode.Initialize("JanusWorker", 4/*max queue depth*/);

    m_Pipeline.Initialize([&](
        uint64_t /*frame_number*/,
        uint64_t shutter_usec,
        const uint8_t* data,
        int bytes
    ) {
        std::lock_guard<std::mutex> locker(m_Lock);

        m_Payloader.WrapH264Rtp(shutter_usec, data, bytes,
            [](const uint8_t* rtp_data, int rtp_bytes)
        {
            for (auto& client : m_Clients) {
                if (client.transmit) {
                    m_Callbacks->relay_rtp(client.handle, 1, (char*)rtp_data, rtp_bytes);
                }
            }
        });
    });

    return 0;
}

/*! \brief Plugin deinitialization/destructor */
void plugin_destroy(void)
{
    m_WorkerNode.Shutdown();
    m_Pipeline.Shutdown();
}

/*! \brief Informative method to request the API version this plugin was compiled against
    *  \note This was added in version 0.0.7 of Janus, to address changes
    * to the API that might break existing plugin or the core itself. All
    * plugins MUST implement this method and return JANUS_PLUGIN_API_VERSION
    * to make this work, or they will be rejected by the core. Do NOT try
    * to launch a <= 0.0.7 plugin on a >= 0.0.7 Janus or it will crash. */
int plugin_get_api_compatibility(void)
{
    return JANUS_PLUGIN_API_VERSION;
}

/*! \brief Informative method to request the numeric version of the plugin */
int plugin_get_version(void)
{
    return 100;
}

/*! \brief Informative method to request the string version of the plugin */
const char *plugin_get_version_string(void)
{
    return "1.0.0";
}

/*! \brief Informative method to request a description of the plugin */
const char *plugin_get_description(void)
{
    return "HDMI Video Pipeline";
}

/*! \brief Informative method to request the name of the plugin */
const char *plugin_get_name(void)
{
    return "kvm";
}

/*! \brief Informative method to request the author of the plugin */
const char *plugin_get_author(void)
{
    return "mrcatid@gmail.com";
}

/*! \brief Informative method to request the package name of the plugin (what will be used in web applications to refer to it) */
const char *plugin_get_package(void)
{
    return "kvm";
}

/*! \brief Method to create a new session/handle for a peer
    * @param[in] handle The plugin/gateway session that will be used for this peer
    * @param[out] error An integer that may contain information about any error */
void plugin_create_session(janus_plugin_session* handle, int* /*error*/)
{
    std::lock_guard<std::mutex> locker(m_Lock);

    ClientData data;
    data.handle = handle;

    m_Clients.push_back(data);
}

static void post_error(janus_plugin_session* handle, const std::string& transaction, int error_code, const std::string& cause)
{
    Logger.Error("Error: ", cause);

    json_t* event = json_object();
    json_object_set_new(event, "streaming", json_string("event"));
    json_object_set_new(event, "error_code", json_integer(error_code));
    json_object_set_new(event, "error", json_string(cause.c_str()));
    m_Callbacks->push_event(handle, &m_Plugin, transaction.c_str(), event, nullptr);
    json_decref(event);
}

static void background_handle_message(janus_plugin_session* handle, const std::string& transaction, json_t* message, json_t* /*jsep*/)
{
    json_t* request_obj = json_object_get(message, "request");
    if (!request_obj) {
        post_error(handle, transaction, 1, "request missing");
        return;
    }

    const char* request_str = json_string_value(request_obj);
    if (!request_str) {
        post_error(handle, transaction, 2, "request not string");
        return;
    }

    Logger.Info("Message: ", request_str);

    if (0 == strcmp(request_str, "stop"))
    {
        json_t* event = json_object();
        json_object_set_new(event, "streaming", json_string("event"));
        json_t* result = json_object();
        json_object_set_new(result, "status", json_string("stopped"));
        json_object_set_new(event, "result", result);
        m_Callbacks->push_event(handle, &m_Plugin, transaction.c_str(), event, nullptr);
        json_decref(event);
        return;
    }

    if (0 == strcmp(request_str, "start"))
    {
        json_t* event = json_object();
        json_object_set_new(event, "streaming", json_string("event"));
        json_t* result = json_object();
        json_object_set_new(result, "status", json_string("started"));
        json_object_set_new(event, "result", result);
        m_Callbacks->push_event(handle, &m_Plugin, transaction.c_str(), event, nullptr);
        json_decref(event);
        return;
    }

    if (0 == strcmp(request_str, "watch"))
    {
        std::string sdp = m_Payloader.GenerateSDP();

        if (sdp.empty()) {
            post_error(handle, transaction, 10, "video source not ready");
            return;
        }

        std::ostringstream oss;
        oss << "m=application 1 UDP/DTLS/SCTP webrtc-datachannel\r\n";
        oss << "c=IN IP4 1.1.1.1\r\n";
        oss << "a=sctp-port:5000\r\n";
        sdp += oss.str();

        Logger.Info("SDP: ", sdp);

        json_t* jsep = json_pack("{ssss}", "type", "offer", "sdp", sdp.c_str());

        json_t* result = json_object();
        json_object_set_new(result, "status", json_string("started"));

        json_t* event = json_object();
        json_object_set_new(event, "streaming", json_string("event"));
        json_object_set_new(event, "result", result);

        m_Callbacks->push_event(handle, &m_Plugin, transaction.c_str(), event, jsep);
        json_decref(event);
        json_decref(jsep);
        return;
    }

    post_error(handle, transaction, 100, std::string("unhandled request: ") + request_str);
}

/*! \brief Method to handle an incoming message/request from a peer
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[in] transaction The transaction identifier for this message/request
    * @param[in] message The json_t object containing the message/request JSON
    * @param[in] jsep The json_t object containing the JSEP type/SDP, if available
    * @returns A janus_plugin_result instance that may contain a response (for immediate/synchronous replies), an ack
    * (for asynchronously managed requests) or an error */
struct janus_plugin_result* plugin_handle_message(janus_plugin_session* handle, char* transaction, json_t* message, json_t* jsep)
{
    ScopedFunction ref_scope([&]() {
        if (message) {
            json_decref(message);
        }
        if (jsep) {
            json_decref(jsep);
        }
    });

    janus_plugin_result* result = (janus_plugin_result*)malloc(sizeof(janus_plugin_result));
    result->type = JANUS_PLUGIN_ERROR;
    result->text = nullptr;
    result->content = nullptr;
    if (!handle) {
        result->text = "No session";
        return result;
    }
    if (!message) {
        result->text = "No message";
        return result;
    }

    std::string transaction_str = transaction;
    m_WorkerNode.Queue([handle, transaction_str, message, jsep]() {
        ScopedFunction ref_scope([&]() {
            if (message) {
                json_decref(message);
            }
            if (jsep) {
                json_decref(jsep);
            }
        });
        background_handle_message(handle, transaction_str.c_str(), message, jsep);
    });

    ref_scope.Cancel();
    result->type = JANUS_PLUGIN_OK_WAIT;
    return result;
}

/*! \brief Callback to be notified when the associated PeerConnection is up and ready to be used
    * @param[in] handle The plugin/gateway session used for this peer */
void plugin_setup_media(janus_plugin_session* handle)
{
    std::lock_guard<std::mutex> locker(m_Lock);

    for (size_t i = 0; i < m_Clients.size(); ++i) {
        if (m_Clients[i].handle == handle) {
            Logger.Info("plugin_setup_media: Session unmuted");
            m_Clients[i].transmit = true;
            return;
        }
    }
    Logger.Error("plugin_setup_media: Session not found");
}

/*! \brief Method to handle an incoming RTP packet from a peer
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[in] video Whether this is an audio or a video frame
    * @param[in] buf The packet data (buffer)
    * @param[in] len The buffer lenght */
void plugin_incoming_rtp(janus_plugin_session* /*handle*/, int /*video*/, char* /*buf*/, int /*len*/)
{
    Logger.Info("plugin_incoming_rtp: Ignored");
}

/*! \brief Method to handle an incoming RTCP packet from a peer
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[in] video Whether this is related to an audio or a video stream
    * @param[in] buf The message data (buffer)
    * @param[in] len The buffer lenght */
void plugin_incoming_rtcp(janus_plugin_session* /*handle*/, int /*video*/, char* /*buf*/, int /*len*/)
{
    //Logger.Info("plugin_incoming_rtcp: Ignored");
}

/*! \brief Method to handle incoming SCTP/DataChannel data from a peer (text only, for the moment)
    * \note We currently only support text data, binary data will follow... please also notice that
    * DataChannels send unterminated strings, so you'll have to terminate them with a \0 yourself to
    * use them.
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[in] buf The message data (buffer)
    * @param[in] len The buffer lenght */
void plugin_incoming_data(janus_plugin_session* /*handle*/, char* buf, int len)
{
    Logger.Info("plugin_incoming_data: Ignored len=", len, " data=", buf);
}

/*! \brief Method to be notified by the core when too many NACKs have
    * been received or sent by Janus, and so a slow or potentially
    * unreliable network is to be expected for this peer
    * \note Beware that this callback may be called more than once in a row,
    * (even though never more than once per second), until things go better for that
    * PeerConnection. You may or may not want to handle this callback and
    * act on it, considering you can get bandwidth information from REMB
    * feedback sent by the peer if the browser supports it. Besides, your
    * plugin may not have access to encoder related settings to slow down
    * or decreae the bitrate if required after the callback is called.
    * Nevertheless, it can be useful for debugging, or for informing your
    * users about potential issues that may be happening media-wise.
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[in] uplink Whether this is related to the uplink (Janus to peer)
    * or downlink (peer to Janus)
    * @param[in] video Whether this is related to an audio or a video stream */
void plugin_slow_link(janus_plugin_session* /*handle*/, int uplink, int video)
{
    Logger.Info("plugin_slow_link: uplink=", uplink, " video=", video);
}

/*! \brief Callback to be notified about DTLS alerts from a peer (i.e., the PeerConnection is not valid any more)
    * @param[in] handle The plugin/gateway session used for this peer */
void plugin_hangup_media(janus_plugin_session* handle)
{
    std::lock_guard<std::mutex> locker(m_Lock);

    for (size_t i = 0; i < m_Clients.size(); ++i) {
        if (m_Clients[i].handle == handle) {
            Logger.Info("plugin_hangup_media: Session muted");
            m_Clients[i].transmit = false;
            return;
        }
    }
    Logger.Error("plugin_hangup_media: Session not found");
}

/*! \brief Method to destroy a session/handle for a peer
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[out] error An integer that may contain information about any error */
void plugin_destroy_session(janus_plugin_session* handle, int* /*error*/)
{
    std::lock_guard<std::mutex> locker(m_Lock);

    for (size_t i = 0; i < m_Clients.size(); ++i) {
        if (m_Clients[i].handle == handle) {
            Logger.Info("plugin_destroy_session: Session removed");
            m_Clients[i] = m_Clients[m_Clients.size() - 1];
            m_Clients.resize(m_Clients.size() - 1);
            return;
        }
    }
    Logger.Error("plugin_destroy_session: Session not found");
}

/*! \brief Method to get plugin-specific info of a session/handle
    *  \note This was added in version 0.0.7 of Janus. Janus assumes
    * the string is always allocated, so don't return constants here
    * @param[in] handle The plugin/gateway session used for this peer
    * @returns A json_t object with the requested info */
json_t* plugin_query_session(janus_plugin_session* /*handle*/)
{
    return json_string("SessionInfoHere");
}

janus_plugin m_Plugin = {
    &plugin_init,
    &plugin_destroy,
    &plugin_get_api_compatibility,
    &plugin_get_version,
    &plugin_get_version_string,
    &plugin_get_description,
    &plugin_get_name,
    &plugin_get_author,
    &plugin_get_package,
    &plugin_create_session,
    &plugin_handle_message,
    &plugin_setup_media,
    &plugin_incoming_rtp,
    &plugin_incoming_rtcp,
    &plugin_incoming_data,
    &plugin_slow_link,
    &plugin_hangup_media,
    &plugin_destroy_session,
    &plugin_query_session
};

extern "C" {

janus_plugin* create()
{
    return &m_Plugin;
}

} // extern "C"
