#include "fast_data.h"

#include <google/protobuf/text_format.h>

#include <library/cpp/protobuf/util/pb_io.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/string/cast.h>
#include <util/stream/file.h>

namespace NAlice::NHollywood {

constexpr TStringBuf FAST_DATA_VERSION = "fast_data_version: ";

TFastData::TFastData(const TString& dirPath)
    : DirPath_(dirPath)
{
}

void TFastData::Register(const TVector<TFastDataInfo>& fastDataInfo) {
    for (const auto& [fileName, producers] : fastDataInfo) {
        const auto [_, inserted] = FastDataProducers_.insert({fileName, producers});
        Y_ENSURE(inserted, TStringBuilder() << "Duplicate fast_data file name: " << fileName);
    }
}

void TFastData::Reload() {
    TVector<TScenarioFastDataPtr> newFastData;
    for (const auto& [fileName, producers] : FastDataProducers_) {
        const auto& [protoProducer, fastDataProducer] = producers;
        TScenarioFastDataProtoPtr msg = protoProducer();
        if (DirPath_.IsDefined()) {
            if ((DirPath_ / fileName).Exists()) {
                TFileInput fi(DirPath_ / fileName);
                Y_ENSURE(msg->ParseFromArcadiaStream(&fi), "Failed to parse protobuf from file " << fileName);
                Cerr << TInstant::Now().ToString() << " Reloaded " << fileName << Endl;
            } else {
                ythrow yexception() << "Can't find " << fileName;
            }
        }
        newFastData.push_back(fastDataProducer(msg));
    }

    if (!newFastData.empty()) {
        TMaybe<int> newVersion;
        if (DirPath_.IsDefined()) {
            TFileInput fi(DirPath_ /  ".svninfo");
            TString line;
            while (fi.ReadLine(line) != 0) {
                if (line.StartsWith(FAST_DATA_VERSION)) {
                    newVersion = FromString<int>(line.substr(FAST_DATA_VERSION.Size(), line.Size()));
                    break;
                }
            }
        } else {
            newVersion = -1;
        }
        Y_ENSURE(newVersion.Defined(), "Missing fast_data_version in .svninfo");
        {
            TWriteGuard lock(Mutex_);
            std::swap(newFastData, FastData_);
            Version_ = *newVersion;
        }
    }
}

int TFastData::GetVersion() const {
    TReadGuard lock(Mutex_);
    return Version_;
}

} // namespace NAlice::NHollywood
