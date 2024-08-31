#include "flags_json.h"
#include "utils.h"

#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/experiments/flags_json.h>
#include <alice/cuttlefish/library/logging/log_context.h>

#include <voicetech/library/uniproxy2/dns/dns.h>

#include <laas/lib/ip_properties/proto/ip_properties.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/singleton.h>


namespace {
    class TCountFlagsHelper {
    public:
        TCountFlagsHelper(
            const NAliceProtocol::TFlagsInfo& flagsInfo,
            const TMaybe<NAliceProtocol::TAbFlagsProviderOptions>& options,
            NAlice::NCuttlefish::TSourceMetrics* metrics,
            NAlice::NCuttlefish::TLogContext* logContext
        )
            : FlagsInfo(flagsInfo)
            , Options(options)
            , Metrics(metrics)
            , LogContext(logContext)
            , ProblemFound(false)
        {
        }

        ~TCountFlagsHelper() {
            if (!ProblemFound) {
                Metrics->PushRate("response", "ok", "flags_json");
            }
        }

        void OnError(const TString& sensor) {
            ProblemFound = true;
            Metrics->PushRate(sensor, "error", "flags_json");
            LogContext->LogEventInfoCombo<NEvClass::CountFlagsError>(
                sensor,
                TStringBuilder() << FlagsInfo,
                (Options ? (TStringBuilder() << *Options) : TString("no_options"))
            );
        }

    private:
        const NAliceProtocol::TFlagsInfo& FlagsInfo;
        const TMaybe<NAliceProtocol::TAbFlagsProviderOptions>& Options;
        NAlice::NCuttlefish::TSourceMetrics* Metrics;
        NAlice::NCuttlefish::TLogContext* LogContext;

        bool ProblemFound = false;
    };

    struct TLocalIpAddressHolder {
        inline TLocalIpAddressHolder() : LocalIpAddress(NVoicetech::NUniproxy2::NDns::GetLocalIpAddress()) {
            Y_ENSURE(!LocalIpAddress.Empty());
        }

        const TString LocalIpAddress;
    };

    const TString& GetLocalIpAddress() {
        static const TString STUB = "";

        try {
            return Singleton<TLocalIpAddressHolder>()->LocalIpAddress;
        } catch (...) {}

        return STUB;
    }
}

namespace NAlice::NCuttlefish::NAppHostServices {

NAppHostHttp::THttpRequest TFlagsJson::GetExperiments(
    const NAliceProtocol::TSessionContext& ctx,
    const NAliceProtocol::TAbFlagsProviderOptions& options
) {
    const NAliceProtocol::TUserInfo& user = ctx.GetUserInfo();
    Y_ENSURE(user.HasUuid());
    Y_ASSERT(!(options.HasDisregardUaas() && options.GetDisregardUaas()));

    NAppHostHttp::THttpRequest req;
    req.SetScheme(NAppHostHttp::THttpRequest::Http);
    req.SetMethod(NAppHostHttp::THttpRequest::Get);

    TStringBuilder path;
    path << "?uuid=" << user.GetUuid();
    if (ctx.GetDeviceInfo().HasDeviceId()) {
        path << "&deviceid=" << ctx.GetDeviceInfo().GetDeviceId();
    }

    if (options.HasOnly100PercentFlags() && options.GetOnly100PercentFlags()) {
        path << "&no-tests=1";
    }

    AddHeader(req, "User-Agent",
        ctx.GetConnectionInfo().HasUserAgent() ? ctx.GetConnectionInfo().GetUserAgent() : "uniproxy");

    if (user.HasCookie()) {
        AddHeader(req, "Cookie", user.GetCookie());
    }

    if (user.HasICookie()) {
        AddHeader(req, "X-Yandex-ICookie", user.GetICookie());
    }

    if (AppendTestIds(&path, ctx, options)) {
        AddHeader(req, "X-Yandex-UAAS", "testing");
    }

    if (ctx.GetExperiments().GetFlagsJsonData().HasAppInfo()) {
        AddHeader(req, "X-Yandex-AppInfo", ctx.GetExperiments().GetFlagsJsonData().GetAppInfo());
    }

    if (options.HasIsYandexStaff() && options.GetIsYandexStaff()) {
        NLaas::TIpProperties props;
        props.SetIsYandexStaff(true);

        const TString value = Base64Encode(props.SerializeAsString());
        AddHeader(req, "X-Ip-Properties", value);
    }

    if (options.HasPuid()) {
        AddHeader(req, "X-Yandex-Puid", options.GetPuid());
    }

    if (options.HasRegionId()) {
        AddHeader(req, "X-Region-City-Id", ToString(options.GetRegionId()));
    }

    req.SetPath(path);
    return req;
}

NJson::TJsonValue TFlagsJson::GetExperimentsJson(
    const NAliceProtocol::TSessionContext& ctx,
    const NAliceProtocol::TAbFlagsProviderOptions& options,
    NAlice::NCuttlefish::TSourceMetrics& metrics,
    NAlice::NCuttlefish::TLogContext& logContext
) {
    const NAliceProtocol::TUserInfo& user = ctx.GetUserInfo();
    Y_ENSURE(user.HasUuid());
    Y_ASSERT(!(options.HasDisregardUaas() && options.GetDisregardUaas()));

    TStringBuilder path;
    path << "/uniproxy?uuid=" << user.GetUuid();
    if (ctx.GetDeviceInfo().HasDeviceId()) {
        path << "&deviceid=" << ctx.GetDeviceInfo().GetDeviceId();
    }

    if (options.HasOnly100PercentFlags() && options.GetOnly100PercentFlags()) {
        path << "&no-tests=1";
    }

    NJson::TJsonValue req;

    auto addHeader = [](
        NJson::TJsonValue& req,
        const TString& headerName,
        const TString& headerValue
    ) {
        NJson::TJsonValue jsonHeaderNameValue;
        jsonHeaderNameValue.AppendValue(headerName);
        jsonHeaderNameValue.AppendValue(headerValue);
        req["headers"].AppendValue(std::move(jsonHeaderNameValue));
    };

    addHeader(req, "User-Agent",
        ctx.GetConnectionInfo().HasUserAgent() ? ctx.GetConnectionInfo().GetUserAgent() : "uniproxy");

    if (user.HasCookie()) {
        addHeader(req, "Cookie", user.GetCookie());
    }

    if (user.HasICookie()) {
        addHeader(req, "X-Yandex-ICookie", user.GetICookie());
    }

    if (AppendTestIds(&path, ctx, options)) {
        addHeader(req, "X-Yandex-UAAS", "testing");
    }

    if (ctx.GetExperiments().GetFlagsJsonData().HasAppInfo()) {
        addHeader(req, "X-Yandex-AppInfo", ctx.GetExperiments().GetFlagsJsonData().GetAppInfo());
    }

    if (options.HasIsYandexStaff() && options.GetIsYandexStaff()) {
        NLaas::TIpProperties props;
        props.SetIsYandexStaff(true);

        const TString value = Base64Encode(props.SerializeAsString());
        addHeader(req, "X-Ip-Properties", value);
    } else if (options.HasIsBetaTester() && options.GetIsBetaTester()) {
        // https://st.yandex-team.ru/VOICESERV-4396#62bd560fc4b58748041e1756
        const TString& localIp = GetLocalIpAddress();
        if (!localIp.Empty()) {
            addHeader(req, "X-Forwarded-For-Y", localIp);
        } else {
            metrics.PushRate("local_ip", "empty");
            logContext.LogEventErrorCombo<NEvClass::GetExperimentsJsonEmptyLocalIpError>();
        }
    } else if (user.HasUuidKind() && user.GetUuidKind() != NAliceProtocol::TUserInfo_EUuidKind_USER) {
        // https://st.yandex-team.ru/VOICESERV-4315#62cefa6d3d21a77efc6d4b18
        if (ctx.HasConnectionInfo() && ctx.GetConnectionInfo().HasIpAddress()) {
            addHeader(req, "X-Forwarded-For-Y", ctx.GetConnectionInfo().GetIpAddress());
        } else {
            metrics.PushRate("not_user_conn_info", "has_no_origin");
            logContext.LogEventErrorCombo<NEvClass::GetExperimentsJsonNoIpAddressInConnInfoError>();
        }
    }

    if (options.HasPuid()) {
        addHeader(req, "X-Yandex-Puid", options.GetPuid());
    }

    if (options.HasRegionId()) {
        addHeader(req, "X-Region-City-Id", ToString(options.GetRegionId()));
    }

    req["uri"] = std::move(path);
    req["method"] = "GET";

    return req;
}

void TFlagsJson::ParseResponse(NAliceProtocol::TExperimentsContext* dst, TString resp) {
    NVoice::NExperiments::ParseFlagsInfoFromRawResponse(dst->MutableFlagsJsonData()->MutableFlagsInfo(), resp);
    dst->MutableFlagsJsonData()->SetData(std::move(resp));
}

void TFlagsJson::CountFlags(
    const NAliceProtocol::TFlagsInfo& flagsInfo,
    const TMaybe<NAliceProtocol::TAbFlagsProviderOptions>& options,
    NAlice::NCuttlefish::TSourceMetrics* metrics,
    NAlice::NCuttlefish::TLogContext* logContext
) {
    // For explanation see VOICESERV-4171 and related tickets.

    // This flag is released via flags.json, must appear in every flags.json response (real users + ue2e,evo,etc).
    static constexpr TStringBuf EternalFlag = "eternal_100_percent_flag_for_uniproxy_correctness_alert";

    // This flag is enabled for every user, must appear in every non-testing flags.json response (only real users).
    static constexpr TStringBuf ExperimentFlag = "experiment_flag_for_uniproxy_correctness_alert";

    // This flag is enabled for all yandexoids, must appear only for yandex staff users.
    static constexpr TStringBuf StaffExperimentFlag = "staff_experiment_flag_for_uniproxy_correctness_alert";

    Y_ENSURE(metrics != nullptr);
    Y_ENSURE(logContext != nullptr);

    if (const int64_t flagsCount = flagsInfo.GetVoiceFlags().GetStorage().size(); flagsCount > 0) {
        metrics->PushRate("nonempty", "ok", "flags_json");
        metrics->PushRate(flagsCount, "total_count", "ok", "flags_json");
    } else {
        metrics->PushRate("empty", "ok", "flags_json");
    }

    TCountFlagsHelper helper(flagsInfo, options, metrics, logContext);
    if (options) {
        NVoice::NExperiments::TFlagsJsonFlagsConstRef flagsInfoRef(&flagsInfo);

        if (options->HasDisregardUaas() && options->GetDisregardUaas()) {
            helper.OnError("disregard_uaas_ignored");
        } else {
            if (!flagsInfoRef.ConductingExperiment(EternalFlag)) {
                helper.OnError("no_eternal_flag");
            }

            if (options->HasIsYandexStaff() && options->GetIsYandexStaff()) {
                if (!flagsInfoRef.ConductingExperiment(StaffExperimentFlag)) {
                    helper.OnError("staff_experiment_flag_not_found");
                }
            } else {
                if (flagsInfoRef.ConductingExperiment(StaffExperimentFlag)) {
                    helper.OnError("staff_experiment_flag_found");
                }
            }

            if (options->HasOnly100PercentFlags() && options->GetOnly100PercentFlags()) {
                if (flagsInfoRef.ConductingExperiment(ExperimentFlag)) {
                    helper.OnError("experiment_flag_found");
                }
            } else {
                if (!flagsInfoRef.ConductingExperiment(ExperimentFlag)) {
                    helper.OnError("experiment_flag_not_found");
                }
            }

            // Validating test-ids
            if (!options->GetTestIds().empty()) {
                const THashSet<TString> testIdsInResponse(
                    flagsInfo.GetExperimentalTestIds().begin(),
                    flagsInfo.GetExperimentalTestIds().end()
                );
                const THashSet<TString> testIdsInOpts(
                    options->GetTestIds().begin(),
                    options->GetTestIds().end()
                );
                if (testIdsInResponse.size() == testIdsInOpts.size()) {
                    for (const TString& testIdInOpts : testIdsInOpts) {
                        if (!testIdsInResponse.contains(testIdInOpts)) {
                            helper.OnError("test_ids_differ");
                            break;
                        }
                    }
                } else {
                    helper.OnError("test_ids_differ");
                }
            }
            if (flagsInfo.GetAllTestIds().size() <= flagsInfo.GetExperimentalTestIds().size()) {
                // Here we assume that we have at least one 100%-released flag (e.g. EternalFlag).
                helper.OnError("invalid_test_id_sets");
            }
        }
    } else {
        // Unable to validate constraints without request options.
        // This error cam occur if graph is misconfigured or apphost has lost item.
        helper.OnError("no_options");
    }
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
