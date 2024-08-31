#pragma once

#include <alice/nlu/granet/lib/user_entity/dictionary.h>

#include <library/cpp/langs/langs.h>

#include <search/begemot/rules/alice/session/proto/alice_session.pb.h>

#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NBg {

NGranet::NUserEntity::TEntityDicts ReadDicts(ELanguage lang, IOutputStream* log, const TString& dictBase64,
                                             const NProto::TAliceSessionResult* session);

} // namespace NBg
