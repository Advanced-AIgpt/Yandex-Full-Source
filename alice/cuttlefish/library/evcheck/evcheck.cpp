#include "evcheck.h"
#include "builder.h"

namespace NVoice {

namespace {

TParserBuilder BuildSynchronizeState()
{
    TParserBuilder builder;

    builder.AddNewProfile()
        .AddField("event", NODE_MAP).BeginSubMap()
            .AddField("header", NODE_MAP)
            .AddField("payload", NODE_MAP).BeginSubMap()
                // about voice
                .AddField("speed", NODE_STRING|NODE_NUMBER)
                .AddField("format", NODE_STRING)
                .AddField("lang", NODE_STRING)
                .AddField("volume", NODE_INTEGER)
                .AddField("emotion", NODE_STRING)
                .AddField("quality", NODE_STRING)
                .AddField("voice", NODE_STRING)
                // biometry
                .AddField("biometry_classify", NODE_STRING)
                .AddField("biometry_group", NODE_STRING)
                .AddField("biometry_request", NODE_STRING)
                .AddField("biometry_score", NODE_BOOLEAN)
                // device info
                .AddField("device", NODE_STRING)
                .AddField("device_model", NODE_STRING)
                .AddField("device_manufacturer", NODE_STRING)
                .AddField("platform_info", NODE_STRING)
                .AddField("network_type", NODE_STRING)
                // software info
                .AddField("app_version", NODE_STRING)
                .AddField("speechkitVersion", NODE_STRING)
                .AddField("sdkVersion", NODE_INTEGER)
                .AddField("app_id", NODE_STRING)
                // ps*
                .AddField("ps_interruption_model", NODE_STRING)
                .AddField("ps_activation_model", NODE_STRING)
                .AddField("ps_additional_model", NODE_STRING)
                // user info
                .AddField("auth_token", NODE_STRING)
                .AddField("yandexuid", NODE_STRING)
                .AddField("oauth_token", NODE_STRING)
                .AddField("uuid", NODE_STRING)
                // messenger
                .AddField("Messenger")
                    .BeginSubMap()
                    .AddField("version", NODE_INTEGER)
                    .EndSubMap()
                // flags and so
                .AddField("accept_invalid_auth", NODE_STRING|NODE_BOOLEAN)
                .AddField("enable_realtime_streamer", NODE_STRING|NODE_BOOLEAN)
                .AddField("disable_local_experiments", NODE_STRING|NODE_BOOLEAN)
                .AddField("vins_partials", NODE_STRING|NODE_BOOLEAN)  // mistype?
                .AddField("vins_partial", NODE_STRING|NODE_BOOLEAN)
                .AddField("punctuation", NODE_STRING|NODE_BOOLEAN)
                .AddField("enable_goaway", NODE_STRING|NODE_BOOLEAN)
                // vins
                .AddField("vins", NODE_MAP)
                    .BeginSubMap()
                    .AddField("application", NODE_MAP)
                        .BeginSubMap()
                        .AddField("device_manufacturer", NODE_STRING)
                        .AddField("lang", NODE_STRING)
                        .AddField("enable_realtime_streamer", NODE_STRING|NODE_BOOLEAN)
                        .AddField("platform", NODE_STRING)
                        .AddField("device_model", NODE_STRING)
                        .AddField("device_id", NODE_STRING)
                        .AddField("X-UPRX-AUTH-TOKEN", NODE_STRING)
                        .AddField("X-UPRX-UUID", NODE_STRING)
                        .AddField("uuid", NODE_STRING)
                        .AddField("app_version", NODE_STRING)
                        .AddField("os_version", NODE_STRING)
                        .AddField("client_time", NODE_STRING|NODE_NUMBER)
                        .AddField("timestamp", NODE_STRING|NODE_NUMBER)
                        .AddField("timezone", NODE_STRING|NODE_NUMBER)
                        .AddField("app_id", NODE_STRING)
                        .EndSubMap()
                    .EndSubMap()
                // misc
                .AddField("seamlessBufferDurationMs", NODE_STRING)
                .AddField("uaas_tests", NODE_ARRAY)
                .AddField("url", NODE_STRING)
                .AddField("__auth_token", NODE_STRING)
                .AddField("vinsUrl", NODE_STRING)
                .AddField("advanced_options", NODE_MAP)
                .AddField("request", NODE_MAP);

    return builder;
}

} // anonymous namespace


TParser ConstructSynchronizeStateParser()
{
    return BuildSynchronizeState().CreateParser();
}

THolder<TParser> ConstructSynchronizeStateParserInHeap() noexcept
{
    try {
        return BuildSynchronizeState().CreateParserInHeap();
    } catch (const std::exception& exc) {
        Cerr << "ConstructSynchronizeStateParserInHeap failed: " << exc.what() << Endl;
    } catch (...) {
        Cerr << "ConstructSynchronizeStateParserInHeap failed: unknown exception" << Endl;
    }
    return nullptr;
}

} // namespace NVoice

