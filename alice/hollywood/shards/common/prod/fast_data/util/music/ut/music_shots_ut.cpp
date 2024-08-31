#include <alice/hollywood/library/phrases/testing/testing.h>
#include <alice/hollywood/library/scenarios/music/proto/fast_data.pb.h>
#include <alice/hollywood/library/tags/testing/testing.h>

#include <library/cpp/protobuf/util/pb_io.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>
#include <util/stream/file.h>

namespace NAlice::NHollywood {

Y_UNIT_TEST_SUITE(MusicShots) {
    Y_UNIT_TEST(SanityCheck) {
        TMusicShotsFastDataProto proto;

        auto file = TFsPath(BuildRoot()) / "alice/hollywood/shards/common/prod/fast_data/music/music_shots.pb";

        Y_ENSURE(proto.ParseFromString(TUnbufferedFileInput{file}.ReadAll()));

        const auto tags = NTesting::CheckTagConditionsCorpus(proto.GetTagConditionsCorpus());
        const auto checkTag = [&](const auto& tag) {
            return tags.IsValidTag(tag);
        };

        NTesting::CheckPhrasesCorpus(proto.GetPhrasesCorpus(), checkTag);
    }
}

} // namespace NAlice::NHollywood
