#pragma once

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/nlu/libs/entity_searcher/entity_searcher_types.h>

#include <kernel/lemmer/core/language.h>

namespace NAlice::NContacts {

constexpr TStringBuf EXTERNAL_SOURCE_CONTACTS = "contacts";
constexpr size_t MAX_CONSIDERED_VARIANTS = 25000;
constexpr size_t MAX_CONSIDERED_TOKENS_SIZE = 10;

TVector<NNlu::TEntityString> ParseContacts(const NAlice::TClientEntity& contacts, ELanguage lang);

} // namespace NAlice::NContacts
