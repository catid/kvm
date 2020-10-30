/*
    TODO:
    Encoder Pipeline
    Video Parser
    RTP Payloader
*/

#include <janus/plugins/plugin.h>

static janus_callbacks *m_Callbacks = nullptr;

/*! \brief Plugin initialization/constructor
    * @param[in] callback The callback instance the plugin can use to contact the Janus core
    * @param[in] config_path Path of the folder where the configuration for this plugin can be found
    * @returns 0 in case of success, a negative integer in case of error */
int plugin_init(janus_callbacks *callback, const char *config_path)
{
    m_Callbacks = callback;
    return 0;
}

/*! \brief Plugin deinitialization/destructor */
void plugin_destroy(void)
{

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
    return "KVMHDMI ingest to video";
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
void plugin_create_session(janus_plugin_session *handle, int *error)
{

}

/*! \brief Method to handle an incoming message/request from a peer
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[in] transaction The transaction identifier for this message/request
    * @param[in] message The json_t object containing the message/request JSON
    * @param[in] jsep The json_t object containing the JSEP type/SDP, if available
    * @returns A janus_plugin_result instance that may contain a response (for immediate/synchronous replies), an ack
    * (for asynchronously managed requests) or an error */
struct janus_plugin_result *plugin_handle_message(janus_plugin_session *handle, char *transaction, json_t *message, json_t *jsep)
{

}

/*! \brief Callback to be notified when the associated PeerConnection is up and ready to be used
    * @param[in] handle The plugin/gateway session used for this peer */
void plugin_setup_media(janus_plugin_session *handle)
{

}

/*! \brief Method to handle an incoming RTP packet from a peer
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[in] video Whether this is an audio or a video frame
    * @param[in] buf The packet data (buffer)
    * @param[in] len The buffer lenght */
void plugin_incoming_rtp(janus_plugin_session *handle, int video, char *buf, int len)
{

}

/*! \brief Method to handle an incoming RTCP packet from a peer
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[in] video Whether this is related to an audio or a video stream
    * @param[in] buf The message data (buffer)
    * @param[in] len The buffer lenght */
void plugin_incoming_rtcp(janus_plugin_session *handle, int video, char *buf, int len)
{

}

/*! \brief Method to handle incoming SCTP/DataChannel data from a peer (text only, for the moment)
    * \note We currently only support text data, binary data will follow... please also notice that
    * DataChannels send unterminated strings, so you'll have to terminate them with a \0 yourself to
    * use them.
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[in] buf The message data (buffer)
    * @param[in] len The buffer lenght */
void plugin_incoming_data(janus_plugin_session *handle, char *buf, int len)
{

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
void plugin_slow_link(janus_plugin_session *handle, int uplink, int video)
{

}

/*! \brief Callback to be notified about DTLS alerts from a peer (i.e., the PeerConnection is not valid any more)
    * @param[in] handle The plugin/gateway session used for this peer */
void plugin_hangup_media(janus_plugin_session *handle)
{

}

/*! \brief Method to destroy a session/handle for a peer
    * @param[in] handle The plugin/gateway session used for this peer
    * @param[out] error An integer that may contain information about any error */
void plugin_destroy_session(janus_plugin_session *handle, int *error)
{

}

/*! \brief Method to get plugin-specific info of a session/handle
    *  \note This was added in version 0.0.7 of Janus. Janus assumes
    * the string is always allocated, so don't return constants here
    * @param[in] handle The plugin/gateway session used for this peer
    * @returns A json_t object with the requested info */
json_t *plugin_query_session(janus_plugin_session *handle)
{
    return nullptr;
}

static janus_plugin m_Plugin = {
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

janus_plugin* create()
{
    return &m_Plugin;
}
