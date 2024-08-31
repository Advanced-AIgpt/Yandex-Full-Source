#include "uniproxy.h"

#include <mapreduce/yt/common/config.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/json_reader.h>

#include <util/generic/string.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/strip.h>
#include <util/string/split.h>
#include <util/string/cast.h>

namespace NAlice {

bool TryParseLogMessage(TStringBuf messageBuf, NYT::TNode& row) {
    auto logType = messageBuf.NextTok(" ");
    if (logType == TStringBuf("ACCESSLOG:")) {
        return TryParseAccessLogMessage(messageBuf, row);
    }
    if (logType == TStringBuf("SESSIONLOG:")) {
        return TryParseSessionLogMessage(messageBuf, row);
    }
    return false;
}

bool TryParseAccessLogMessage(TStringBuf messageBuf, NYT::TNode& row) {
    messageBuf.NextTok("\"");
    if (messageBuf.NextTok("\" ") != TStringBuf("POST /vins 1/1")) {
        return false;
    }
    auto httpCode = FromString<ui64>(messageBuf.NextTok(" "));
    if (httpCode != HttpCodes::HTTP_OK && httpCode != HttpCodes::HTTP_RESET_CONTENT) {
        return false;
    }
    messageBuf.NextTok("\"");
    auto eventId = ToString(messageBuf.NextTok("\" "));
    messageBuf.NextTok(" ");
    messageBuf.NextTok(" ");
    messageBuf.NextTok(" ");
    messageBuf.NextTok("\"");
    auto duration = FromString<float>(messageBuf.NextTok("\""));
    row["eventId"] = eventId;
    row["duration"] = duration;
    row["type"] = "ACCESS";
    return true;
}

bool TryParseSessionLogMessage(TStringBuf messageBuf, NYT::TNode& row) {
    if (!messageBuf.Contains("VinsRequest")) {
        return false;
    }
    NJson::TJsonValue message;
    if (!NJson::ReadJsonTree(messageBuf, &message, /*throw on error*/false)) {
        return false;
    }

    row["reqId"] = message["Directive"]["Body"]["header"]["request_id"].GetString();
    row["eventId"] = message["Directive"]["ForEvent"].GetString();
    row["text"] = message["Directive"]["Body"]["request"]["event"]["text"].GetString();
    row["app_id"] = message["Directive"]["Body"]["application"]["app_id"].GetString();
    row["app_version"] = message["Directive"]["Body"]["application"]["app_version"].GetString();
    row["platform"] = message["Directive"]["Body"]["application"]["platform"].GetString();
    row["os_version"] = message["Directive"]["Body"]["application"]["os_version"].GetString();
    if (row["text"].Empty()) {
        for (const auto& record : message["Directive"]["Body"]["request"]["event"]["asr_result"].GetArray()) {
            if (!record["utterance"].GetString().empty()) {
                row["text"] = record["utterance"].GetString();
            }
        }
     }

     if (row["text"].Empty()) {
         return false;
     }

     row["type"] = "SESSION";
     return true;
}

}//namespace NAlice

