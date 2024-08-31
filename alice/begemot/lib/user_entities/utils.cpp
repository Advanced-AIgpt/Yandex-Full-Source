#include "utils.h"

#include <alice/nlu/granet/lib/sample/entity_utils.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>

#define TRACE_LINE(log, arg) if (log) {*log << arg << Endl;}
#define TRACE_TEXT(log, arg) if (log) {*log << arg;}

using namespace NGranet;

namespace NBg {

NUserEntity::TEntityDicts ReadDicts(ELanguage lang, IOutputStream* log, const TString& dictBase64, const NProto::TAliceSessionResult* session) {
    NUserEntity::TEntityDicts dicts;

    if (!dictBase64.empty()) {
        dicts.FromBase64(dictBase64);
    }

    if (session) {
        ::google::protobuf::RepeatedPtrField<NAlice::TClientEntity> entities = session->GetEntities();
        if (entities.empty()) {
            return dicts;
        }

        THashMap<TString, size_t> nameToDictIndex;

        TRACE_LINE(log, "Entities from session:");
        for (const NAlice::TClientEntity& entity : entities) {
            const TString& deprecatedEntityName = entity.GetName();
            const TString entityName = NGranet::NEntityTypePrefixes::SCENARIO + deprecatedEntityName;
            for (const TString& name : {entityName, deprecatedEntityName}) {
                const auto [it, isNew] = nameToDictIndex.try_emplace(name, dicts.Dicts.size());
                if (isNew) {
                    dicts.AddDict(name);
                }
                for (const auto& [value, nluHint] : entity.GetItems()) {
                    for (const auto& nluPhrase : nluHint.GetInstances()) {
                        TRACE_TEXT(log, " lang " << static_cast<int>(nluPhrase.GetLanguage()));
                        TRACE_TEXT(log, ", name: " << name);
                        TRACE_TEXT(log, ", value: " << value);
                        TRACE_TEXT(log, ", text: " << nluPhrase.GetPhrase());
                        TRACE_LINE(log, "");

                        if (static_cast<ELanguage>(nluPhrase.GetLanguage()) != lang) {
                            continue;
                        }
                        NUserEntity::TEntityDict& dict = *dicts.Dicts[it->second];
                        NUserEntity::TEntityDictItem& dictItem = dict.Items.emplace_back();
                        dictItem.Value = TString(value);
                        dictItem.Text = nluPhrase.GetPhrase();
                    }
                }
            }
        }
    }
    return dicts;
}

} // namespace NBg
