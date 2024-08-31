#pragma once

#include <map>

#include <grpcpp/support/string_ref.h>

#include <library/cpp/json/json_value.h>

#include <alice/cuttlefish/library/logging/event_log.h>
#include <alice/cuttlefish/library/logging/log_context.h>

#include <alice/gproxy/library/protos/metadata.pb.h>

namespace NGProxy {

using TClientMetadata = std::multimap<grpc::string_ref, grpc::string_ref>;

/**
 *  @brief fills ::NGProxy::TMetadata message with grpc's client metadata
 *  See details in https://wiki.yandex-team.ru/users/pazus/fetch-zaprosy/
 *  @retval true    if `to` contains all required fields and 0 or more optional
 *  @retval false   if `to` does not contain all required fields
 */
bool FillMetadata(const google::protobuf::RepeatedPtrField<NAppHostHttp::THeader>& from, TMetadata& to, TMaybe<NAlice::NCuttlefish::TLogContext> logContext);

NJson::TJsonValue CreateAppHostParams(const TClientMetadata& from, bool allowSrcrwr = false, bool allowDumpReqResp = false, TString rtLogToken = "");

bool LoggingIsAllowedForHeader(TString header);

} // namespace NGProxy
