#include <alice/hollywood/library/phrases/testing/testing.h>
#include <alice/hollywood/library/scenarios/alice_show/proto/config.pb.h>
#include <alice/hollywood/library/scenarios/alice_show/proto/fast_data.pb.h>
#include <alice/hollywood/library/tags/testing/testing.h>

#include <library/cpp/protobuf/util/pb_io.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>
#include <util/stream/file.h>

namespace NAlice::NHollywood {

Y_UNIT_TEST_SUITE(AliceShow) {
    Y_UNIT_TEST(SanityCheck) {
        TAliceShowFastDataProto proto;

        auto file = TFsPath(BuildRoot()) / "alice/hollywood/shards/common/prod/fast_data/alice_show/alice_show.pb";

        Y_ENSURE(proto.ParseFromString(TUnbufferedFileInput{file}.ReadAll()));

        const auto tags = NTesting::CheckTagConditionsCorpus(proto.GetTagConditionsCorpus());
        const auto checkTag = [&](const auto& tag) {
            return tags.IsValidTag(tag);
        };

        TSet<TString> images;

        UNIT_ASSERT(!proto.GetImageBaseUrl().empty());
        for (const auto& img : proto.GetImages()) {
            UNIT_ASSERT(!img.GetId().empty());
            UNIT_ASSERT(!img.GetUris().empty());
            auto [it, inserted] = images.emplace(img.GetId());
            UNIT_ASSERT_C(inserted, "duplicate Image " << img.GetId());
        }

        const auto& config = proto.GetConfig();
        UNIT_ASSERT(!config.GetInitialVertex().empty());
        UNIT_ASSERT(config.GetGraph().contains(config.GetInitialVertex()));
        TSet<TString> referencedVertices{config.GetInitialVertex()};
        TSet<TString> referencedActions;
        for (const auto& [vertex, graphNode] : config.GetGraph()) {
            UNIT_ASSERT(!vertex.empty());
            for (const auto& actionName : graphNode.GetEntryActions().GetActions()) {
                UNIT_ASSERT_C(config.GetActions().contains(actionName), "unknown Action " << actionName << " in EntryActions for vertex " << vertex);
                referencedActions.emplace(actionName);
            }
            for (const auto& transition : graphNode.GetTransitions()) {
                referencedVertices.emplace(transition.GetTargetVertex());
                UNIT_ASSERT_C(config.GetGraph().contains(transition.GetTargetVertex()),
                    "unknown TargetVertex " << transition.GetTargetVertex());
                for (const auto& tag : transition.GetTags()) {
                    UNIT_ASSERT_C(checkTag(tag), "unknown tag " << tag << " for transition " << vertex << " => " << transition.GetTargetVertex());
                }
            }
        }
        for (const auto& [vertex, _] : config.GetGraph()) {
            UNIT_ASSERT_C(referencedVertices.contains(vertex), "dangling vertex " << vertex);
        }

        THashSet<TString> appendedPhrases;
        const auto checkActionPart = [&](const NAliceShow::TActionPart& actionPart, const TString& obj, const TString& name) {
            for (const auto& appendedPhraseId : actionPart.GetAppendPhrases()) {
                UNIT_ASSERT(!appendedPhraseId.empty());
                appendedPhrases.emplace(appendedPhraseId);
            }
            if (actionPart.GetImageId()) {
                UNIT_ASSERT_C(!actionPart.GetPhraseId().empty(), "ImageId should come with PhraseId in " << obj << " " << name);
            }
        };

        TSet<TString> referencedParts;
        for (const auto& [actionName, action] : config.GetActions()) {
            UNIT_ASSERT(!actionName.empty());
            UNIT_ASSERT_C(referencedActions.contains(actionName), "unreferenced Action " << actionName);
            for (const auto& tag : action.GetTags()) {
                UNIT_ASSERT_C(checkTag(tag), "unknown tag " << tag << " in action " << actionName);
            }
            for (const auto& actionPart : action.GetActionParts()) {
                if (const auto& usePart = actionPart.GetUsePart()) {
                    UNIT_ASSERT_C(config.GetParts().contains(usePart),
                        "unknown Part " << usePart << " referenced in Action " << actionName);
                    referencedParts.emplace(usePart);
                }
                checkActionPart(actionPart, "Action", actionName);
            }
        }
        for (const auto& [partName, actionPart] : config.GetParts()) {
            UNIT_ASSERT(!partName.empty());
            UNIT_ASSERT_C(referencedParts.contains(partName), "unreferenced Part " << partName);
            checkActionPart(actionPart, "Part", partName);
        }

        const auto groups = NTesting::CheckPhrasesCorpus(proto.GetPhrasesCorpus(), checkTag, true, appendedPhrases);

        for (const auto& appendedPhrase : appendedPhrases) {
            UNIT_ASSERT_C(groups.contains(appendedPhrase), "unknown group " << appendedPhrase << " referenced from AppendPhrases");
        }
    }
}

} // namespace NAlice::NHollywood
