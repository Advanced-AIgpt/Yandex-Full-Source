#include "asr_result.h"

#include <util/system/hostname.h>
#include <voicetech/library/uniproxy2/unistat.h>
#include <map>

using namespace NAlice::NAsr;
using namespace NJson;
using namespace NVoicetech::NUniproxy2;

void NAlice::NCuttlefish::NAppHostServices::AsrResponseToJson(const NProtobuf::TResponse& response, NJson::TJsonValue& payload) {
    static const TString ok = "OK";
    static const TString internalError = "InternalError";
    if (response.HasInitResponse()) {
        auto& initResponse = response.GetInitResponse();
        if (!initResponse.GetIsOk()) {
            payload[TStringBuf("responseCode")] = internalError;
            if (initResponse.HasErrMsg()) {
                payload[TStringBuf("error")] = initResponse.GetErrMsg();
            }
            return;
        }

        payload[TStringBuf("responseCode")] = ok;
        payload[TStringBuf("serverHostname")] = initResponse.GetHostname();
        payload[TStringBuf("topic")] = initResponse.GetTopic();
        payload[TStringBuf("topicVersion")] = initResponse.GetTopicVersion();
        payload[TStringBuf("serverVersion")] = initResponse.GetServerVersion();
    } else if (response.HasAddDataResponse()) {
        auto& addDataResponse = response.GetAddDataResponse();
        if (!addDataResponse.GetIsOk()) {
            payload[TStringBuf("responseCode")] = internalError;
            if (addDataResponse.HasErrMsg()) {
                payload[TStringBuf("error")] = addDataResponse.GetErrMsg();
            }
            return;
        }

        payload[TStringBuf("responseCode")] = ok;
        if (addDataResponse.GetResponseStatus() == NProtobuf::ValidationFailed) {
            ythrow yexception() << "can not convert spotter ValidationFailed result";
        } else if (addDataResponse.GetResponseStatus() == NProtobuf::EndOfUtterance) {
            payload[TStringBuf("endOfUtt")] = true;
        } else if (addDataResponse.GetResponseStatus() == NProtobuf::Active) {
            payload[TStringBuf("endOfUtt")] = false;
        }
        if (addDataResponse.HasCacheKey()) {
            payload[TStringBuf("cacheKey")] = addDataResponse.GetCacheKey();
        }
        if (addDataResponse.HasDoNotSendToClient() && addDataResponse.GetDoNotSendToClient()) {
            payload[TStringBuf("doNotSendToClient")] = true;
        }

        bool isWhisper = false;
        if (addDataResponse.HasIsWhisper()) {
            isWhisper = addDataResponse.GetIsWhisper();
        }
        payload[TStringBuf("whisperInfo")] = NJson::TJsonMap();
        payload[TStringBuf("whisperInfo")][TStringBuf("isWhisper")] = isWhisper;


        payload[TStringBuf("recognition")] = NJson::JSON_ARRAY;  // ensure recognition field exist (for ftest asr_empty_stream)
        if (addDataResponse.HasRecognition()) {
            auto& recognition = addDataResponse.GetRecognition();
            for (auto& hypo : recognition.GetHypos()) {
                auto& jRecognition = payload[TStringBuf("recognition")].AppendValue(NJson::JSON_ARRAY);
                jRecognition[TStringBuf("confidence")] = hypo.GetTotalScore();
                auto& words = jRecognition[TStringBuf("words")];
                words = NJson::JSON_ARRAY;

                for (auto& word : hypo.GetWords()) {
                    auto& jWord = words.AppendValue(NJson::JSON_MAP);
                    jWord["value"] = word;
                    jWord["confidence"] = 1.;
                    //v2 has not this data: j["align_info"] = ?;
                }
                //v2 has not this data: jRecognition[TStringBuf("align_info")] = ?;
                if (hypo.HasNormalized()) {
                    jRecognition[TStringBuf("normalized")] = hypo.GetNormalized();
                } else {
                    jRecognition[TStringBuf("normalized")] = "";  // emulate old version behaviour (proto_to_json generate empty normalized results)
                }
                //v2 has not this data: jRecognition[TStringBuf("end_of_phrase")] = ?;
                if (hypo.HasParentModel()) {
                    jRecognition[TStringBuf("parentModel")] = hypo.GetParentModel();
                }
            }
            for (auto& cr : recognition.GetContextRef()) {
                auto& jCR = payload[TStringBuf("contextRef")].AppendValue(NJson::JSON_MAP);
                jCR[TStringBuf("index")] = cr.GetIndex();
                jCR[TStringBuf("contentIndex")] = cr.GetContentIndex();
                jCR[TStringBuf("confidence")] = cr.GetConfidence();
            }
            if (recognition.HasThrownPartialsFraction()) {
                 payload[TStringBuf("thrownPartialsFraction")] = recognition.GetThrownPartialsFraction();
            }
        }
        payload[TStringBuf("messagesCount")] = addDataResponse.GetMessagesCount();
        //TODO payload[TStringBuf("bioResult)] = ?;
        payload["bioResult"] = NJson::JSON_ARRAY;  // field _MUST_ exist in result (at least empty list/array)
        //v2 has not data for this field:  payload[TStringBuf("`metainfo)] = ?;
        //v2 has not data for this field: payload[TStringBuf("earlyEndOfUtt")] = ?;
        payload[TStringBuf("durationProcessedAudio")] = ToString(addDataResponse.GetDurationProcessedAudio());
        //v2 has not data for this field: payload[TStringBuf("is_trash)] = ?;
        //v2 has not data for this field: payload[TStringBuf("trash_score)] = ?;
        if (addDataResponse.HasCoreDebug()) {
            payload[TStringBuf("coreDebug")] = addDataResponse.GetCoreDebug();
        }
        if (addDataResponse.HasMetaInfo()) {
            auto &metainfo = addDataResponse.GetMetaInfo();
            if (metainfo.HasHostname()) {
                payload[TStringBuf("metainfo")]["serverHostname"] = metainfo.GetHostname();
            }
            if (metainfo.HasTopic()) {
                payload[TStringBuf("metainfo")]["topic"] = metainfo.GetTopic();
            }
            if (metainfo.HasTopicVersion()) {
                payload[TStringBuf("metainfo")]["version"] = metainfo.GetTopicVersion();
            }
            if (metainfo.HasServerVersion()) {
                payload[TStringBuf("metainfo")]["serverVersion"] = metainfo.GetServerVersion();
            }
        }
    } else {
        ythrow yexception() << "not convertable asr response protobuf type";
    }
}

bool NAlice::NCuttlefish::NAppHostServices::HasEmptyText(const NAlice::NAsr::NProtobuf::TResponse& response) {
    if (response.HasAddDataResponse()) {
        auto& addDataResponse = response.GetAddDataResponse();
        if (addDataResponse.HasRecognition()) {
            auto& recognition = addDataResponse.GetRecognition();
            for (auto& hypo : recognition.GetHypos()) {
                for (auto& word : hypo.GetWords()) {
                    if (word.size()) {
                        return false; // found not empty word
                    }
                }
                break;  // check only first hypotesis
            }
        }
    }
    return true;
}
