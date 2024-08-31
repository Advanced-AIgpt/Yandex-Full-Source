#pragma once

#include <alice/nlu/granet/lib/compiler/source_text_collection.h>

#include <alice/megamind/protos/common/frame.pb.h>

#include <kernel/lemmer/core/language.h>

#include <util/generic/maybe.h>

namespace NAlice::NContacts {

constexpr TStringBuf EXTERNAL_SOURCE_CONTACTS = "contacts";

TMaybe<NGranet::NCompiler::TSourceTextCollection> ParseContacts(const NAlice::TClientEntity& contacts, ELanguage lang, bool skipExact = false);


} // namespace NAlice::NContacts
