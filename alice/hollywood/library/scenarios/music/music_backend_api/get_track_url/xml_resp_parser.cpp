#include "xml_resp_parser.h"

#include <contrib/libs/expat/expat.h>

#include <memory>

namespace NAlice::NHollywood::NMusic {

namespace {

/*
Parse xml with download info returned by mds handle. Example:
<?xml version="1.0" encoding="utf-8"?>
<download-info>
    <s>df4dd51823e5bec643e62a3e2b1ca91acd622d849e71a52cd658a757c1f4be97</s>
    <region>-1</region>
    <ts>0005a3f1f0b59585</ts>
    <host>s174sas.storage.yandex.net</host>
    <path>/rmusic/U2FsdGVkX19LO</path>
</download-info>
*/

class TXmlRespParser {
public:
    [[nodiscard]] TXmlRespParseResult&& Result() && {
        return std::move(Result_);
    }

    const TXmlRespParseResult& Result() const & {
        return Result_;
    }

    [[nodiscard]] bool Parse(TStringBuf xmlStr);

private:
    void OnStartElement(TStringBuf name) {
        if (name == TStringBuf("s")) {
            NextItem_ = NextSignature;
        } else if (name == TStringBuf("ts")) {
            NextItem_ = NextTs;
        } else if (name == TStringBuf("host")) {
            NextItem_ = NextHost;
        } else if (name == TStringBuf("path")) {
            NextItem_ = NextPath;
        } else if (name == TStringBuf("region")) {
            NextItem_ = NextRegion;
        } else {
            NextItem_ = NextUnknown;
        }
    }

    void OnText(TStringBuf data) {
        switch (NextItem_) {
            case NextHost:
                Result_.Host = data;
                break;
            case NextPath:
                Result_.Path = data;
                break;
            case NextTs:
                Result_.Ts = data;
                break;
            case NextRegion:
                Result_.Region = data;
                break;
            case NextSignature:
                Result_.Signature = data;
                break;
            case NextUnknown:
                break;
        }
        NextItem_ = NextUnknown;
    }

    static void OnStartElementCb(void* userData, const XML_Char* name, const XML_Char** atts) {
        Y_UNUSED(atts);
        static_cast<TXmlRespParser*>(userData)->OnStartElement(name);
    }

    static void OnTextCb(void* userData, const XML_Char* s, int len) {
        static_cast<TXmlRespParser*>(userData)->OnText(TStringBuf(s, len));
    }

private:
    enum ENextItem {
        NextUnknown,
        NextHost,
        NextPath,
        NextTs,
        NextRegion,
        NextSignature
    };

    ENextItem NextItem_ = NextUnknown;
    TXmlRespParseResult Result_;
};

bool TXmlRespParser::Parse(TStringBuf xmlStr) {
    using XML_ParserType = std::remove_pointer_t<XML_Parser>;
    auto parser = std::unique_ptr<XML_ParserType, decltype(&XML_ParserFree)>(XML_ParserCreate(nullptr),
                                                                             &XML_ParserFree);
    XML_SetUserData(parser.get(), this);
    XML_SetStartElementHandler(parser.get(), &OnStartElementCb);
    XML_SetCharacterDataHandler(parser.get(), &OnTextCb);
    auto status = XML_Parse(parser.get(), xmlStr.data(), xmlStr.length(), 1);
    return status == XML_STATUS_OK;
}

} // namespace

TMaybe<TXmlRespParseResult> ParseDlInfoXmlResp(const TStringBuf xmlStr) {
    TXmlRespParser parser;
    if (!parser.Parse(xmlStr)) {
        return Nothing();
    }

    const auto& result = parser.Result();
    if (result.Host.empty() || result.Path.empty() || result.Signature.empty() || result.Ts.empty()) {
        return Nothing();
    }

    return std::move(parser).Result();
}

} // namespace NAlice::NHollywood::NMusic
