#include "aggregation.h"


#include <util/stream/output.h>


template <class T>
void OutImpl(IOutputStream& stream, const NVoice::NMetrics::TLabelsBase<T>& data) {
    stream << "TLabels<T>("
        << "'Device='" << data.DeviceName
        << "', Surface='" << data.GroupName
        << "', SubgroupName='" << data.SubgroupName
        << "', AppId='" << data.AppId
        << "', ClientType='" << (data.ClientType == NVoice::NMetrics::EClientType::User ? "user" : (data.ClientType == NVoice::NMetrics::EClientType::User ? "robot" : ""))
        << "')";
}


template <>
void Out<NVoice::NMetrics::TLabelsBase<TStringBuf>>(IOutputStream& stream, const NVoice::NMetrics::TLabelsBase<TStringBuf>& data) {
    OutImpl(stream, data);
}


template <>
void Out<NVoice::NMetrics::TLabelsBase<TString>>(IOutputStream& stream, const NVoice::NMetrics::TLabelsBase<TString>& data) {
    OutImpl(stream, data);
}
