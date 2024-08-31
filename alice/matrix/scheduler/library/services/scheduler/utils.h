#pragma once

#include <alice/protos/api/matrix/scheduled_action.pb.h>

#include <infra/libs/outcome/result.h>

namespace NMatrix::NScheduler {

TExpected<NMatrix::NApi::TScheduledAction, TString> CreateScheduledActionFromMetaAndSpec(
    const NMatrix::NApi::TScheduledActionMeta& meta,
    const NMatrix::NApi::TScheduledActionSpec& spec
);

} // namespace NMatrix::NScheduler
