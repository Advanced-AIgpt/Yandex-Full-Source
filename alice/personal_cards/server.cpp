#include "server.h"
#include "application.h"

#include <alice/personal_cards/card/card_request.h>
#include <alice/personal_cards/card/utils.h>
#include <alice/personal_cards/protos/model.pb.h>

#include <infra/libs/sensors/sensor_registry.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/json/json2proto.h>

namespace NPersonalCards {

namespace {

constexpr TStringBuf SEARCH_APP = "search_app";
constexpr TStringBuf RES_MSG = "Operation success";

void WriteError(const TString& msg, NJson::TJsonMap* result) {
    LOG(ERR) << msg << Endl;
    (*result)["error"] = msg;
}

} // namespace

TServer::TServer(TPushCardsStoragePtr pushCardsStorage)
    : IsShutdown_(false)
    , PushCardsStorage_(pushCardsStorage)
{
    LOG(INFO) << "CardsServer started" << Endl;
}

void TServer::Shutdown() {
    IsShutdown_.store(true, std::memory_order_release);
}

static TVector<TString> GetUserIds(const TDismissCardRequest& request) {
    TVector<TString> ids(Reserve(5));
    if (request.has_auth() && request.auth().uid()) {
        ids.push_back(TStringBuilder() << "uid/" << NormalizeUUID(request.auth().uid()));
    }
    if (request.has_app_metrika()) {
        const auto appMetrika = request.app_metrika();
        if (appMetrika.has_device_id()) {
            ids.push_back(TStringBuilder() << "device_id/" << NormalizeUUID(appMetrika.device_id()));
        }
        if (appMetrika.has_did()) {
            ids.push_back(TStringBuilder() << "device_id/" << NormalizeUUID(appMetrika.did()));
        }
        if (appMetrika.has_uuid()) {
            ids.push_back(TStringBuilder() << "uuid/" << NormalizeUUID(appMetrika.uuid()));
        }
    }
    if (request.has_yandexuid()) {
        ids.push_back(TStringBuilder() << "yandexuid/" << request.yandexuid());
    }
    return ids;
}

bool TServer::GetCards(const TRequestContext& context, NJson::TJsonMap* result) {
    const auto& params = context.Request();
    TCardRequest request(params);

    LOG(INFO) << "card request: " << context << Endl;

    const auto uidFromTvm = context.UidFromTvm();
    const auto uid = uidFromTvm.Defined() ? TString(TStringBuilder() << "uid/" << *uidFromTvm) : request.GetBestUserId();

    auto rsp = params;
    // Idk: From legacy - https://a.yandex-team.ru/arc/trunk/arcadia/bass/personal_cards/card/card_request.cpp?rev=r7839080#L143
    rsp.EraseValue("bigb");

    const auto clientId = request.GetClientId();
    const auto now = request.GetTimeNow();
    if (clientId == SEARCH_APP) {
        auto blocks = PushCardsStorage_->GetPushCards(uid, now);
        if (blocks.GetArray().empty()) {
            static NInfra::TRateSensor rateSensor(SENSOR_GROUP, "push_cards_storage_get_cards_empty_responses");
            rateSensor.Inc();
        }

        rsp["blocks"] = std::move(blocks);
    } else {
        rsp.EraseValue("blocks");
        LOG(WARNING) << "Unknown 'client_id': " << clientId << Endl;
    }

    LOG(INFO) << "card response: "
        << NJson::WriteJson(
            rsp,
            false, // formatOutput
            false, // sortkeys
            false  // validateUtf8
        )
        << Endl;

    *result = std::move(rsp);

    return true;
}

bool TServer::DismissCard(const TRequestContext& context, NJson::TJsonMap* result) {
    LOG(INFO) << "params to remove: " << context << Endl;

    TDismissCardRequest request;
    try {
        NProtobufJson::Json2Proto(context.Request(), request);
    } catch (...) {
        static NInfra::TRateSensor rateSensor(SENSOR_GROUP, "dismiss_card", {{
            "error", "proto_parse"
        }});
        rateSensor.Inc();
        WriteError(TStringBuf("bad request: ") + CurrentExceptionMessage(), result);
        return false;
    }

    const auto uidFromTvm = context.UidFromTvm();
    TVector<TString> ids = uidFromTvm.Defined() ? TVector<TString>({TStringBuilder() << "uid/" << *uidFromTvm}) : GetUserIds(request);
    if (ids.empty()) {
        WriteError("request has no valid user ids", result);
        return false;
    } else if (ids.size() > 1) {
        static NInfra::TRateSensor rateSensor(SENSOR_GROUP, "dismiss_card" ,{{
            "error", "more_than_one_user_id"
        }});
        rateSensor.Inc();
        LOG(WARNING) << "dismiss request has more than one user id: " << context << Endl;
    }

    PushCardsStorage_->DismissPushCard(ids.at(0), TString(request.card_id()));

    (*result)["result"] = RES_MSG;
    return true;
}

static void AddToArrayIds(const NJson::TJsonMap& req, TStringBuf id, TVector<TString>& ids) {
    auto incIdCounter = [&id] {
        NInfra::TRateSensor(SENSOR_GROUP, "id_in_request", {{
            "type", id
        }}).Inc();
    };

    const auto& user = req[id];
    if (user.IsString()) {
        incIdCounter();
        ids.push_back(TStringBuilder() << id << '/' << NormalizeUUID(user.GetString()));
    } else if (user.IsArray()) {
        const auto& users = user.GetArray();
        if (!users.empty()) {
            incIdCounter();
        }
        for (const auto& item : users) {
            if (item.IsString()) {
                ids.push_back(TStringBuilder() << id << '/' << NormalizeUUID(item.GetString()));
            }
        }
    }
}

bool TServer::AddPushCard(const TRequestContext& context, NJson::TJsonMap* result) {
    const auto& req = context.Request();
    LOG(INFO) << "add push cards request: " << context << Endl;

    TVector<TString> ids;
    if (const auto uidFromTvm = context.UidFromTvm(); uidFromTvm.Defined()) {
        ids = {TStringBuilder() << "uid/" << *uidFromTvm};
    } else {
        AddToArrayIds(req, "uid", ids);
        AddToArrayIds(req, "device_id", ids);
        AddToArrayIds(req, "did", ids);
        AddToArrayIds(req, "uuid", ids);
        if (ids.empty()) {
            WriteError("Not found users ids", result);
            return false;
        } else if (ids.size() > 1) {
            static NInfra::TRateSensor rateSensor(SENSOR_GROUP, "add_push_card", {{
                "error", "more_than_one_user_id"
            }});
            rateSensor.Inc();
            LOG(WARNING) << "add push card request has more than one user id: " << context << Endl;
        }
    }

    if (const auto push = req["card"]; push.IsMap()) {
        TInstant sentDateTime = TInstant::Now();
        if (const auto dateMsPtr = push.GetValueByPath("sent_date_time.$date"); dateMsPtr && dateMsPtr->IsUInteger()) {
            sentDateTime = TInstant::MilliSeconds(dateMsPtr->GetUInteger());
        }

        TPushCard pushCard;
        try {
            NProtobufJson::Json2Proto(push["card"], pushCard);
        } catch (...) {
            static NInfra::TRateSensor rateSensor(SENSOR_GROUP, "add_push_card", {{
                "error", "proto_parse"
            }});
            rateSensor.Inc();
            WriteError(TStringBuf("bad request: ") + CurrentExceptionMessage(), result);
            return false;
        }

        PushCardsStorage_->AddPushCard(ids.at(0), pushCard, sentDateTime, push["card"]["data"]);
    }

    LOG(INFO) << RES_MSG << Endl;
    (*result)["result"] = RES_MSG;
    return true;
}

bool TServer::Sensors(const TRequestContext&, NJson::TJsonMap* result) {
    TApplication::GetInstance()->BeforeSensors();

    TStringStream out;
    NInfra::SensorRegistryPrint(out);
    NJson::ReadJsonFastTree(out.Str(), result);

    return true;
}

bool TServer::LogDeprecatedRequestAndReturnNothing(const TRequestContext& context, NJson::TJsonMap*) {
    LOG(WARNING) << "deprecated request: " << context << Endl;
    return true;
}

} // namespace NPersonalCards
