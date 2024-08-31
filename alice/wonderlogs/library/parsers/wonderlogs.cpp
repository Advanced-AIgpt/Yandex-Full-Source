#include "wonderlogs.h"

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/wonderlogs/protos/request_stat.pb.h>

namespace NAlice::NWonderlogs {

void ParseUniproxyPrepared(TWonderlog& wonderlog, const TUniproxyPrepared& uniproxyPrepared) {
    if (!uniproxyPrepared.GetRealMessageId()) {
        wonderlog.SetRealMessageId(false);
    }
    if (uniproxyPrepared.HasRequestStat()) {
        *wonderlog.MutableRequestStat() = uniproxyPrepared.GetRequestStat();
    }
    {
        const auto& spotterValidation = uniproxyPrepared.GetSpotterValidation();
        if (spotterValidation.HasValid() && spotterValidation.HasResult() &&
            !uniproxyPrepared.GetSpotterValidation().GetResult()) {
            wonderlog.MutableSpotter()->SetFalseActivation(true);
        }
        if (spotterValidation.HasModelResult()) {
            wonderlog.MutableSpotter()->MutableValidation()->SetModelResult(spotterValidation.GetModelResult());
        }
        if (spotterValidation.HasFinalResult()) {
            wonderlog.MutableSpotter()->MutableValidation()->SetFinalResult(spotterValidation.GetFinalResult());
        }
    }
    if (uniproxyPrepared.HasAsrStream()) {
        *wonderlog.MutableAsr()->MutableVoiceByUniproxy() = uniproxyPrepared.GetAsrStream();
    }
    if (!uniproxyPrepared.GetSpotterStreams().empty()) {
        *wonderlog.MutableSpotter()->MutableStreams() = uniproxyPrepared.GetSpotterStreams();
    }
    if (uniproxyPrepared.GetSpotterStream().HasMds()) {
        wonderlog.MutableSpotter()->SetMdsUrl(uniproxyPrepared.GetSpotterStream().GetMds());
        wonderlog.MutableSpotter()->SetFormat(uniproxyPrepared.GetSpotterStream().GetFormat());
    }
    wonderlog.MutablePresence()->SetUniproxy(true);
    *wonderlog.MutablePresence()->MutableUniproxyPresence() = uniproxyPrepared.GetPresence();
    if (uniproxyPrepared.HasTimestampLogMs()) {
        wonderlog.SetServerTimeMs(uniproxyPrepared.GetTimestampLogMs());
    }
    *wonderlog.MutableEnvironment()->MutableUniproxyEnvironment() = uniproxyPrepared.GetEnvironment();
    if (uniproxyPrepared.HasClientIp()) {
        wonderlog.MutableDownloadingInfo()->MutableUniproxy()->SetClientIp(uniproxyPrepared.GetClientIp());
    }
    if (uniproxyPrepared.HasSpotterValidation() && uniproxyPrepared.GetSpotterValidation().HasMultiactivation()) {
        const auto& multiactivationFrom = uniproxyPrepared.GetSpotterValidation().GetMultiactivation();
        auto& multiactivationTo = *wonderlog.MutableSpotter()->MutableMultiactivation();
        if (multiactivationFrom.HasCanceled()) {
            multiactivationTo.SetCanceled(multiactivationFrom.GetCanceled());
        }
        if (multiactivationFrom.HasId()) {
            multiactivationTo.SetId(multiactivationFrom.GetId());
        }
    }
    if (uniproxyPrepared.HasVoiceInput()) {
        const auto& voiceInput = uniproxyPrepared.GetVoiceInput();
        if (voiceInput.HasActivationType()) {
            wonderlog.MutableAsr()->SetActivationType(voiceInput.GetActivationType());
        }
        if (voiceInput.HasTopic()) {
            wonderlog.MutableAsr()->MutableTopics()->SetRequest(voiceInput.GetTopic());
        }
    }
    if (uniproxyPrepared.HasAsrRecognize()) {
        const auto& asrRecognize = uniproxyPrepared.GetAsrRecognize();
        if (asrRecognize.HasTopic()) {
            wonderlog.MutableAsr()->MutableTopics()->SetRequest(asrRecognize.GetTopic());
        }
    }
    if (uniproxyPrepared.HasAsrResult()) {
        const auto& asrResult = uniproxyPrepared.GetAsrResult();
        if (asrResult.HasTopic()) {
            wonderlog.MutableAsr()->MutableTopics()->SetModel(asrResult.GetTopic());
        }
        if (asrResult.HasLingwareVersion()) {
            wonderlog.MutableAsr()->SetLingwareVersion(asrResult.GetLingwareVersion());
        }
    }
    if (uniproxyPrepared.HasAsrDebug()) {
        *wonderlog.MutableAsr()->MutableOnlineValidationDebug() = uniproxyPrepared.GetAsrDebug();
    }
    if (uniproxyPrepared.HasSynchronizeState()) {
        const auto& synchronizeState = uniproxyPrepared.GetSynchronizeState();
        if (synchronizeState.HasAuthToken()) {
            wonderlog.MutableClient()->SetAuthToken(synchronizeState.GetAuthToken());
        }
        if (synchronizeState.HasApplication()) {
            *wonderlog.MutableClient()->MutableApplication() = synchronizeState.GetApplication();
        }
    }
    if (uniproxyPrepared.HasLogSpotter()) {
        const auto& logSpotter = uniproxyPrepared.GetLogSpotter();
        if (logSpotter.HasTranscript()) {
            wonderlog.MutableSpotter()->SetTranscript(logSpotter.GetTranscript());
        }
        if (logSpotter.HasTopic()) {
            wonderlog.MutableSpotter()->SetTopic(logSpotter.GetTopic());
        }
        if (logSpotter.HasSource()) {
            wonderlog.MutableSpotter()->SetSpotterSource(logSpotter.GetSource());
        }
        if (logSpotter.HasFirmware()) {
            wonderlog.MutableClient()->MutableApplication()->SetDeviceFirmware(logSpotter.GetFirmware());
        }
        if (logSpotter.HasSpotterActivationInfo()) {
            const auto& spotterActivationInfo = logSpotter.GetSpotterActivationInfo();

            if (spotterActivationInfo.HasQuasmodromGroup()) {
                wonderlog.MutableClient()->MutableApplication()->SetQuasmodromGroup(
                    spotterActivationInfo.GetQuasmodromGroup());
            }

            if (spotterActivationInfo.HasQuasmodromSubgroup()) {
                wonderlog.MutableClient()->MutableApplication()->SetQuasmodromSubgroup(
                    spotterActivationInfo.GetQuasmodromSubgroup());
            }

            if (spotterActivationInfo.HasSpotterStats()) {
                *wonderlog.MutableSpotter()->MutableActivation()->MutableTechStats() =
                    spotterActivationInfo.GetSpotterStats();
            }

            if (spotterActivationInfo.HasContext()) {
                wonderlog.MutableSpotter()->MutableActivation()->SetContext(spotterActivationInfo.GetContext());
            }

            TWonderlog::TSpotter::TCommonStats commonStats;
            if (ParseSpotterCommonStats(commonStats, spotterActivationInfo)) {
                *wonderlog.MutableSpotter()->MutableActivation()->MutableCommonStats() = commonStats;
            }
        }
    }
    if (uniproxyPrepared.HasMegamindTimings()) {
        *wonderlog.MutableTimings()->MutableUniproxy()->MutableMegamind() = uniproxyPrepared.GetMegamindTimings();
    }
    if (uniproxyPrepared.HasTtsTimings()) {
        *wonderlog.MutableTimings()->MutableUniproxy()->MutableTts() = uniproxyPrepared.GetTtsTimings();
    }
    if (uniproxyPrepared.HasTtsGenerate()) {
        const auto& ttsGenerate = uniproxyPrepared.GetTtsGenerate();
        auto& wonderlogTts = *wonderlog.MutableTts();

        if (ttsGenerate.HasText()) {
            wonderlogTts.SetGeneratedText(ttsGenerate.GetText());
        }
        if (ttsGenerate.HasEmotion()) {
            wonderlogTts.SetEmotion(ttsGenerate.GetEmotion());
        }
        if (ttsGenerate.HasFormat()) {
            wonderlogTts.SetFormat(ttsGenerate.GetFormat());
        }
        if (ttsGenerate.HasLang()) {
            wonderlogTts.SetLang(ttsGenerate.GetLang());
        }
        if (ttsGenerate.HasQuality()) {
            wonderlogTts.SetQuality(ttsGenerate.GetQuality());
        }
        if (ttsGenerate.HasVoice()) {
            wonderlogTts.SetVoice(ttsGenerate.GetVoice());
        }
    }
    if (uniproxyPrepared.GetVoiceInput().HasSpeechKitRequest()) {
        *wonderlog.MutableSpeechkitRequest() = uniproxyPrepared.GetVoiceInput().GetSpeechKitRequest();
    }
    if (wonderlog.GetSpeechkitRequest().GetRequest().GetTestIDs().empty() && !uniproxyPrepared.GetTestIds().empty()) {
        for (const auto testId : uniproxyPrepared.GetTestIds()) {
            wonderlog.MutableSpeechkitRequest()->MutableRequest()->MutableTestIDs()->Add(testId);
        }
    }
}

bool ParseSpotterCommonStats(TWonderlog::TSpotter::TCommonStats& commonStats,
                             const TLogSpotter::TSpotterActivationInfo& spotterActivationInfo) {
    bool isInitialised = false;

    if (spotterActivationInfo.HasActualSoundAfterTriggerMs()) {
        commonStats.SetActualSoundAfterTriggerMs(spotterActivationInfo.GetActualSoundAfterTriggerMs());
        isInitialised = true;
    }
    if (spotterActivationInfo.HasActualSoundBeforeTriggerMs()) {
        commonStats.SetActualSoundBeforeTriggerMs(spotterActivationInfo.GetActualSoundBeforeTriggerMs());
        isInitialised = true;
    }
    if (spotterActivationInfo.HasRequestSoundAfterTriggerMs()) {
        commonStats.SetRequestSoundAfterTriggerMs(spotterActivationInfo.GetRequestSoundAfterTriggerMs());
        isInitialised = true;
    }
    if (spotterActivationInfo.HasRequestSoundBeforeTriggerMs()) {
        commonStats.SetRequestSoundBeforeTriggerMs(spotterActivationInfo.GetRequestSoundBeforeTriggerMs());
        isInitialised = true;
    }
    if (spotterActivationInfo.HasUnhandledDataBytes()) {
        commonStats.SetUnhandledBytes(spotterActivationInfo.GetUnhandledDataBytes());
        isInitialised = true;
    }
    if (spotterActivationInfo.HasDurationDataSubmitted()) {
        commonStats.SetDurationSubmitted(spotterActivationInfo.GetDurationDataSubmitted());
        isInitialised = true;
    }

    return isInitialised;
}

} // namespace NAlice::NWonderlogs
