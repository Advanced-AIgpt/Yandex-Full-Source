#include "convert.h"

#include <util/datetime/base.h>

namespace {

using namespace NSpeechkitProtocol;

void EncodeExApplicationInfo(TExApplicationInfo *pAppInfo, const Json::Value& jAppInfo) {
    if (jAppInfo.isMember("quasmodrom_group")) {
        pAppInfo->set_quasmodromgroup(jAppInfo["quasmodrom_group"].asString());;
    }
    if (jAppInfo.isMember("device_model")) {
        pAppInfo->set_devicemodel(jAppInfo["device_model"].asString());
    }
    if (jAppInfo.isMember("os_version")) {
        pAppInfo->set_osversion(jAppInfo["os_version"].asString());
    }
    if (jAppInfo.isMember("client_time")) {
        pAppInfo->set_clienttime(jAppInfo["client_time"].asString());
    }
    if (jAppInfo.isMember("timezone")) {
        pAppInfo->set_timezone(jAppInfo["timezone"].asString());
    }
    if (jAppInfo.isMember("quasmodrom_subgroup")) {
        pAppInfo->set_quasmodromsubgroup(jAppInfo["quasmodrom_subgroup"].asString());
    }
    if (jAppInfo.isMember("app_id")) {
        pAppInfo->set_appid(jAppInfo["app_id"].asString());
    }
    if (jAppInfo.isMember("platform")) {
        pAppInfo->set_platform(jAppInfo["platform"].asString());
    }
    if (jAppInfo.isMember("device_id")) {
        pAppInfo->set_deviceid(jAppInfo["device_id"].asString());
    }
    if (jAppInfo.isMember("app_version")) {
        pAppInfo->set_appversion(jAppInfo["app_version"].asString());
    }
    if (jAppInfo.isMember("lang")) {
        pAppInfo->set_lang(jAppInfo["lang"].asString());
    }
    if (jAppInfo.isMember("device_manufacturer")) {
        pAppInfo->set_devicemanufacturer(jAppInfo["device_manufacturer"].asString());
    }
    if (jAppInfo.isMember("uuid")) {
        pAppInfo->set_uuid(jAppInfo["uuid"].asString());
    }
    if (jAppInfo.isMember("timestamp")) {
        pAppInfo->set_timestamp(jAppInfo["timestamp"].asString());
    }
}

TSpeechkitMessage EncodeStreamControl(const Json::Value& jStreamControl) {
    TSpeechkitMessage msg;
    msg.set_timestamp(TInstant::Now().MicroSeconds());
    TStreamControl *pStreamControl = msg.mutable_streamcontrol();
    if (jStreamControl.isMember("messageId")) {
        pStreamControl->set_messageid(jStreamControl["messageId"].asString());
    }
    if (jStreamControl.isMember("streamId")) {
        pStreamControl->set_streamid(jStreamControl["streamId"].asInt64());
    }
    if (jStreamControl.isMember("action")) {
        pStreamControl->set_action((TStreamControl_EAction)jStreamControl["action"].asInt64());
    }
    return msg;
}

void EncodeMessageHeader(TMessageHeader *phdr, const Json::Value& jhdr) {
    phdr->set_messageid(jhdr["messageId"].asString());
    if (jhdr.isMember("refMessageId")) {
        phdr->set_refmessageid(jhdr["refMessageId"].asString());
    }
    if (jhdr.isMember("eventId")) {
        phdr->set_eventid(jhdr["eventId"].asString());
    }
    if (jhdr.isMember("rtLogId")) {
        phdr->set_rtlogid(jhdr["rtLogId"].asString());
    }
    if (jhdr.isMember("streamId")) {
        phdr->set_streamid(jhdr["streamId"].asInt64());
    }
    if (jhdr.isMember("ack")) {
        phdr->set_ack(jhdr["ack"].asInt64());
    }
    if (jhdr.isMember("sequenceNumber")) {
        phdr->set_sequencenumber(jhdr["sequenceNumber"].asInt64());
    }
    if (jhdr.isMember("refStreamId")) {
        phdr->set_refstreamid(jhdr["refStreamId"].asInt64());
    }
}

EFormat EncodeAsrFormat(const std::string& format) {
    if (format == "audio/opus") {
        return FORMAT_RAW_OPUS;
    } else if (format == "audio/ogg;codecs=opus") {
        return FORMAT_OPUS_IN_OGG;
    } else if (format == "audio/webm;codecs=opus") {
        return FORMAT_OPUS_IN_WEBM;
    } else if (format == "audio/x-pcm;bit=16;rate=16000") {
        return FORMAT_16000_PCM;
    } else if (format == "audio/x-pcm;bit=16;rate=8000") {
        return FORMAT_8000_PCM;
    } else {
        return FORMAT_UNKNOWN;
    }
}

void EncodeDesiredAsrOptions(TAsrOptions *asrOptions, const Json::Value& payload) {
    if (payload.isMember("advancedASROptions")) {
        const Json::Value& jAsrOptions = payload["advancedASROptions"];
        if (jAsrOptions.isMember("partial_results")) {
            asrOptions->set_enablepartials(jAsrOptions["partial_results"].asBool());
        }
        if (jAsrOptions.isMember("allow_multi_utt")) {
            asrOptions->set_multieou(jAsrOptions["allow_multi_utt"].asBool());
        }
    }
    if (payload.isMember("topic")) {
        asrOptions->set_topic(payload["topic"].asString());
    }
    if (payload.isMember("lang")) {
        asrOptions->set_lang(payload["lang"].asString());
    }
    if (payload.isMember("disableAntimatNormalizer")) {
        asrOptions->set_enableantimat(!payload["disableAntimatNormalizer"].asBool());
    }
    if (payload.isMember("punctutation")) {
        asrOptions->set_punctuation(payload["punctutation"].asBool());
    }
}

void EncodeApplicationInfo(TApplicationInfo *appInfo, const Json::Value& payload) {
    if (payload.isMember("application")) {
        appInfo->set_name(payload["application"].asString());
    }
    if (payload.isMember("uuid")) {
        appInfo->set_uuid(payload["uuid"].asString());
    }
}

void EncodeVinsXInput(const Json::Value& event, TXInput *xInput) {
    const Json::Value& header = event["header"];
    const Json::Value& payload = event["payload"];
    EncodeMessageHeader(xInput->mutable_header(), header);
    if (payload.isMember("biometry_classify")) {
        xInput->set_biometryclassify(payload["biometry_classify"].asString());
    }
    EncodeDesiredAsrOptions(xInput->mutable_desiredasroptions(), payload);
    if (payload.isMember("enable_spotter_validation")) {
        xInput->set_enablespottervalidation(payload["enable_spotter_validation"].asBool());
    }
    if (payload.isMember("biometry_group")) {
        xInput->set_biometrygroup(payload["biometry_group"].asString());
    }
    if (payload.isMember("format")) {
        xInput->set_format(EncodeAsrFormat(payload["format"].asString()));
    }
    EncodeExApplicationInfo(xInput->mutable_applicationinfo(), payload["application"]);

    if (payload.isMember("request")) {
        const Json::Value& request = payload["request"];
        if (request.isMember("experiments")) {
            const Json::Value& experiments = request["experiments"];
            for (auto it = experiments.begin(); it != experiments.end(); ++it) {
                xInput->add_experiments(it->asString());
            }
        }
    }
    if (payload.isMember("tags")) {
        xInput->set_tags(payload["tags"].asString());
    }

    TMusicOptions *musicOptions = xInput->mutable_musicoptions();
    if (payload.isMember("recognize_music_only")) {
        musicOptions->set_recognizemusiconly(payload["recognize_music_only"].asBool());
    }

}

TSpeechkitMessage EncodeVinsVoiceInput(const Json::Value& event) {
    TSpeechkitMessage msg;
    msg.set_timestamp(TInstant::Now().MicroSeconds());
    EncodeVinsXInput(event, msg.mutable_voiceinput());
    return msg;
}

TSpeechkitMessage EncodeVinsTextInput(const Json::Value& event) {
    TSpeechkitMessage msg;
    msg.set_timestamp(TInstant::Now().MicroSeconds());
    EncodeVinsXInput(event, msg.mutable_textinput());
    return msg;
}

TSpeechkitMessage EncodeVinsMusicInput(const Json::Value& event) {
    TSpeechkitMessage msg;
    msg.set_timestamp(TInstant::Now().MicroSeconds());
    EncodeVinsXInput(event, msg.mutable_musicinput());
    return msg;
}

TSpeechkitMessage EncodeAsrRecognize(const Json::Value& event) {
    const Json::Value& header = event["header"];
    const Json::Value& payload = event["payload"];
    TSpeechkitMessage msg;
    msg.set_timestamp(TInstant::Now().MicroSeconds());
    TAsrRecognize *asrRecognize = msg.mutable_asrrecognize();
    EncodeMessageHeader(asrRecognize->mutable_header(), header);
    if (payload.isMember("apiKey")) {
        asrRecognize->set_apikey(payload["apiKey"].asString());
    }
    TMusicOptions *musicOptions = asrRecognize->mutable_musicoptions();
    if (payload.isMember("music_request2")) {
        const Json::Value& music_req = payload["music_request2"];
        if (music_req.isMember("headers")) {
            const Json::Value& headers = music_req["headers"];
            const Json::Value::Members keys = headers.getMemberNames();
            for (auto it = keys.begin(); it != keys.end(); ++it) {
                THttpHeader* header = musicOptions->add_headers();
                header->set_name(TString(*it));
                header->set_value(headers[*it].asString());
            }
        }
    }
    if (payload.isMember("format")) {
        asrRecognize->set_format(EncodeAsrFormat(payload["format"].asString()));
    }
    if (payload.isMember("request")) {
        const Json::Value& request = payload["request"];
        if (request.isMember("experiments")) {
            const Json::Value& experiments = request["experiments"];
            for (auto it = experiments.begin(); it != experiments.end(); ++it) {
                asrRecognize->add_experiments(it->asString());
            }
        }
    }
    EncodeDesiredAsrOptions(asrRecognize->mutable_desiredasroptions(), payload);
    if (payload.isMember("tags")) {
        asrRecognize->set_tags(payload["tags"].asString());
    }
    if (payload.isMember("recognize_music_only")) {
        musicOptions->set_recognizemusiconly(payload["recognize_music_only"].asBool());
    }
    EncodeApplicationInfo(asrRecognize->mutable_applicationinfo(), payload);
    if (payload.isMember("yandexuid")) {
        asrRecognize->mutable_userinfo()->set_yandexuid(payload["yandexuid"].asString());
    }

    return msg;
}

}

namespace NSpeechkitProtocol {

TMaybe<Json::Value> DecodeMessage(const TSpeechkitMessage &proto) {
    if (proto.Payload_case() == TSpeechkitMessage::PayloadCase::kAudioStream) {
        return Nothing();
    } else if (proto.Payload_case() == TSpeechkitMessage::PayloadCase::kStreamControl) {
        Json::Value json;
        Json::Value jStreamControl;
        const TStreamControl &pStreamControl = proto.streamcontrol();
        if (pStreamControl.has_messageid()) {
            jStreamControl["messageId"] = pStreamControl.messageid();
        }
        if (pStreamControl.has_streamid()) {
            jStreamControl["streamId"] = static_cast<Json::Int64>(pStreamControl.streamid());
        }
        if (pStreamControl.has_action()) {
            jStreamControl["action"] = pStreamControl.action();
        }
        jStreamControl["reason"] = 0;
        json["streamcontrol"] = std::move(jStreamControl);
        return json;
    } else {
        Y_ENSURE(false, "malformed message");
    }
}

TMaybe<TSpeechkitMessage> EncodeMessage(const Json::Value &json) {
    if (json.isMember("streamcontrol")) {
        const Json::Value& jStreamControl = json["streamcontrol"];
        return EncodeStreamControl(jStreamControl);
    } else if (json.isMember("event")) {
        const Json::Value& event = json["event"];
        const Json::Value& header = event["header"];
        const std::string& name = header["name"].asString();
        const std::string& namespace_ = header["namespace"].asString();
        if (name == "Recognize" && namespace_ == "ASR") {
            return EncodeAsrRecognize(event);
        } else if (namespace_ == "Vins" && name == "VoiceInput") {
            return EncodeVinsVoiceInput(event);
        } else if (namespace_ == "Vins" && name == "TextInput") {
            return EncodeVinsTextInput(event);
        } else if (namespace_ == "Vins" && name == "MusicInput") {
            return EncodeVinsMusicInput(event);
        } else {
            return Nothing();
        }
    } else {
        return Nothing();
    }
}

}
