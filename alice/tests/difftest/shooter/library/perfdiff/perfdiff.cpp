#include "perfdiff.h"

#include <alice/joker/library/log/log.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/hash_set.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/system/shellcommand.h>
#include <util/system/tempfile.h>

namespace NAlice::NShooter {

TPerfDiff::TPerfDiff(const TFsPath& oldResponsesPath, const TFsPath& newResponsesPath, const TFsPath& outputPath)
    : OldResponsesPath_{oldResponsesPath}
    , NewResponsesPath_{newResponsesPath}
    , OutputPath_{outputPath}
{
}

void TPerfDiff::ConstructDiff() {
    LOG(INFO) << "Constructing diff" << Endl;
    LOG(INFO) << "Old responses path " << OldResponsesPath_ << Endl;
    LOG(INFO) << "New responses path " << NewResponsesPath_ << Endl;
    LOG(INFO) << "Output path " << OutputPath_ << Endl;
}

} // namespace NAlice::NShooter
