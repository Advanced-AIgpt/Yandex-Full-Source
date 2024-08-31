#include "answers.h"
#include "catalog.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS::NMusic {


// TMusicCatalog ---------------------------------------------------------------
TMusicCatalog::TMusicCatalog(const TCommonHeaders& commonHeaders,
                             const NAlice::TClientFeatures& clientFeatures,
                             bool autoplay,
                             bool shouldLogResponse)
    : CommonHeaders(commonHeaders)
    , ClientFeatures(clientFeatures)
    , Autoplay(autoplay)
    , ShouldLogResponse{shouldLogResponse}
{
}

bool TMusicCatalog::ProcessRequest(NSc::TValue* out) {
    if (!Handler) {
        return false;
    }
    NHttpFetcher::TResponse::TRef resp = Handler->Wait();

    HttpCode = resp->Code;

    if (resp->IsError()) {
        return false;
    }

    const TString& data = resp->Data;
    if (data.empty()) {
        return false;
    }

    NSc::TValue tmp;
    if (!NSc::TValue::FromJson(tmp, data) || !tmp.Has(RESULT_FIELD)) {
        return false;
    }

    return ValidateCatalogAnswer(tmp[RESULT_FIELD], out);
}

bool TMusicCatalog::ValidateSingleCatalogAnswer(const NSc::TValue& musicAnswer, TStringBuf answerType,
                                                NSc::TValue* out) const {
    if (musicAnswer.IsNull() || (musicAnswer.IsArray() && musicAnswer.ArrayEmpty())) {
        return false;
    }

    NSc::TValue value = musicAnswer.IsArray() ? musicAnswer[0] : musicAnswer;

    // FIXME (a-sidorin@): Workaround for https://st.yandex-team.ru/MUSICBACKEND-5686.
    // Change 'while' to 'if' after the task is done.
    TMaybe<NSc::TValue> allPartsContainer;
    while (value.Has(answerType)) {
        if (value.Has("allPartsContainer")) {
            allPartsContainer = value.Delete("allPartsContainer");
        }
        value = value[answerType];
    }
    if (allPartsContainer.Defined()) {
        value["allPartsContainer"] = allPartsContainer.GetRef();
    }

    if (answerType == "track") {
        if (!value["available"].GetBool(false)) {
            return false;
        }

        TString realId(value["realId"].ForceString());
        if (realId != value["id"].ForceString()) {
            value["id"].SetString(realId);
        }
    } else {
        const NSc::TValue& deprecatedId =
            ((answerType == "album")
                 ? value["deprecation"]["targetAlbumId"]
                 : ((answerType == "artist") ? value["deprecation"]["targetArtistId"] : NSc::Null()));

        if (!deprecatedId.IsNull()) {
            value["id"] = deprecatedId;
        } else if (!value["available"].GetBool(false)) {
            return false;
        }
    }

    TYandexMusicAnswer answer(ClientFeatures);
    answer.InitWithRelatedAnswer(TString{answerType}, value, Autoplay);

    return answer.ConvertAnswerToOutputFormat(out);
}

// TMusicCatalogSingle ---------------------------------------------------------
TMusicCatalogSingle::TMusicCatalogSingle(const TCommonHeaders& commonHeaders,
                                         const NAlice::TClientFeatures& clientFeatures,
                                         const TStringBuf type,
                                         bool autoplay,
                                         bool shouldLogResponse)
    : TMusicCatalog(commonHeaders, clientFeatures, autoplay, shouldLogResponse)
    , AnswerType(type)
{
}

void TMusicCatalogSingle::CreateRequestHandler(NHttpFetcher::TRequestPtr request, const TCgiParameters& cgi,
                                               NSc::TValue* data) {
    request->AddCgiParams(cgi);
    AddHeaders(CommonHeaders, request.Get());
    if (data) {
        (*data)["catalogurl"].SetString(request->Url());
    }
    Handler = request->Fetch();
}

bool TMusicCatalogSingle::ValidateCatalogAnswer(const NSc::TValue& musicAnswer, NSc::TValue* out) const {
    if (ShouldLogResponse) {
        LOG(INFO) << "MusicCatalog response: " << musicAnswer << Endl;
    }
    return ValidateSingleCatalogAnswer(musicAnswer, AnswerType, out);
}

// TMusicCatalogBulk -----------------------------------------------------------
bool TMusicCatalogBulk::ValidateCatalogAnswer(const NSc::TValue& musicAnswer, NSc::TValue* out) const {
    if (ShouldLogResponse) {
        LOG(INFO) << "MusicCatalog response: " << musicAnswer << Endl;
    }

    if (musicAnswer.IsNull() || !musicAnswer.IsArray() || musicAnswer.ArrayEmpty()) {
        return false;
    }

    bool hasValidAnswers = false;

    for (auto&& elem : musicAnswer.GetArray()) {
        auto& request = elem[REQUEST_FIELD];
        TStringBuf answerType = request[TYPE_FIELD];
        if (answerType.empty()) {
            continue;
        }

        NSc::TValue response;
        if (ValidateSingleCatalogAnswer(elem[RESPONSE_FIELD], answerType, &response)) {
            hasValidAnswers = true;
            auto& value = out->Push();
            value[REQUEST_FIELD] = std::move(request);
            value[RESPONSE_FIELD] = std::move(response);
        }
    }

    return hasValidAnswers;
}

void TMusicCatalogBulk::CreateRequestHandler(NHttpFetcher::TRequestPtr request, const NSc::TValue& body) {
    const auto json = body.ToJson();
    LOG(INFO) << "MusicCatalog request body: " << json << Endl;
    request->SetBody(json, NAlice::NHttpMethods::POST);
    request->SetContentType(NAlice::NContentTypes::APPLICATION_JSON);
    AddHeaders(CommonHeaders, request.Get());
    Handler = request->Fetch();
}

} // namespace NBASS::NMusic
