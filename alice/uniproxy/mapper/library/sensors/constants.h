#pragma once

#include <library/cpp/monlib/metrics/labels.h>
#include <library/cpp/monlib/metrics/metric.h>

namespace NAlice::NUniproxy {
    struct TSensor {
        const NMonitoring::TLabels Labels;

        NMonitoring::TGauge* Gauge = nullptr;
        NMonitoring::TIntGauge* IntGauge = nullptr;
        NMonitoring::TRate* Rate = nullptr;
        NMonitoring::TCounter* Counter = nullptr;
        NMonitoring::THistogram* Histogram = nullptr;
    };

    /* Bins for histograms */

    const NMonitoring::TBucketBounds TimingsCounter{0.005, 0.010, 0.015, 0.020, 0.025, 0.030, 0.035,
                                                    0.040, 0.045, 0.050, 0.06, 0.07, 0.08, 0.09, 0.1, 0.2, 0.3,
                                                    0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 2.0, 3.0, 5.0, 7.5,
                                                    10.0};
    const NMonitoring::TBucketBounds SizeCounter{100, 200, 300, 400, 500, 750, 1500, 3000, 5000, 10000,
                                                 50000, 100000, 500000, 1000000, 2000000};

    struct TSensorContainer {
        /* Uniproxy sensors */

        TSensor UniproxySyncStateTimeMs;
        TSensor UniproxyVoiceResponseTimeMs;
        TSensor UniproxyTextResponseTimeMs;

        TSensor UniproxyTextRequestPayloadSizeBytes;
        TSensor UniproxyTextResponsePayloadSizeBytes;
        TSensor UniproxyVoiceRequestPayloadSizeBytes;
        TSensor UniproxyVoiceResponsePayloadSizeBytes;

        TSensor UniproxyVoiceRequestVoiceSizeBytes;
        TSensor UniproxyVoiceResponseVoiceSizeBytes;

        TSensor UniproxyRequestSuccessRate;
        TSensor UniproxyRequestFailRate;
        TSensor UniproxyRequestWithRetriesRate;

        /* Binary holder sensors */

        TSensor BinaryHolderStartCommandTimeMs;
        TSensor BinaryHolderContinueCommandTimeMs;

        TSensor BinaryHolderStartCommandFailRate;
        TSensor BinaryHolderContinueCommandFailRate;

        TSensorContainer()
            : UniproxySyncStateTimeMs{{{"sensor", "alice_downloader.uniproxy_sync_state_response_time_ms"}}}
            , UniproxyVoiceResponseTimeMs{{{"sensor", "alice_downloader.uniproxy_voice_response_time_ms"}}}
            , UniproxyTextResponseTimeMs{{{"sensor", "alice_downloader.uniproxy_text_response_time_ms"}}}
            , UniproxyTextRequestPayloadSizeBytes{{{"sensor", "alice_downloader.uniproxy_request_payload_size_bytes"},
                                                   {"type", "text"}}}
            , UniproxyTextResponsePayloadSizeBytes{{{"sensor", "alice_downloader.uniproxy_response_payload_size_bytes"},
                                                    {"type", "text"}}}
            , UniproxyVoiceRequestPayloadSizeBytes{{{"sensor", "alice_downloader.uniproxy_request_payload_size_bytes"},
                                                    {"type", "voice"}}}
            , UniproxyVoiceResponsePayloadSizeBytes{{{"sensor", "alice_downloader.uniproxy_response_payload_size_bytes"},
                                                     {"type", "voice"}}}
            , UniproxyVoiceRequestVoiceSizeBytes{{{"sensor", "alice_downloader.uniproxy_request_voice_size_bytes"},
                                                  {"type", "voice"}}}
            , UniproxyVoiceResponseVoiceSizeBytes{{{"sensor", "alice_downloader.uniproxy_response_voice_size_bytes"},
                                                   {"type", "voice"}}}
            , UniproxyRequestSuccessRate{{{"sensor", "alice_downloader.uniproxy_request_rate"},
                                          {"status", "success"}}}
            , UniproxyRequestFailRate{{{"sensor", "alice_downloader.uniproxy_request_rate"},
                                       {"status", "fail"}}}
            , UniproxyRequestWithRetriesRate{{{"sensor", "alice_downloader.uniproxy_request_with_retries_rate"}}}
            , BinaryHolderStartCommandTimeMs{{{"sensor", "alice_downloader.binary_holder_time_ms"},
                                              {"command", "start"}}}
            , BinaryHolderContinueCommandTimeMs{{{"sensor", "alice_downloader.binary_holder_time_ms"},
                                                 {"command", "continue"}}}

            , BinaryHolderStartCommandFailRate{{{"sensor", "alice_downloader.binary_holder_rate"},
                                                {"status", "fail"},
                                                {"command", "start"}}}
            , BinaryHolderContinueCommandFailRate{{{"sensor", "alice_downloader.binary_holder_rate"},
                                                   {"status", "fail"},
                                                   {"command", "continue"}}}
        {
        }
    };
}
