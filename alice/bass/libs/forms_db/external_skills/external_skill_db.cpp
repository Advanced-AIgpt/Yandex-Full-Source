#include "external_skill_db.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/scheduler/scheduler.h>

#include <library/cpp/scheme/scheme.h>

namespace NBASS {

TExternalSkillsDb::TExternalSkillsDb(IFormsDbDelegate& delegate)
    : Delegate(delegate)
    , UpdatePeriod(delegate.Config().CacheUpdatePeriod().Get())
{
}

void TExternalSkillsDb::Init() {
    Delegate.Scheduler().Schedule([this]() { return Update(this); });
}

const NSc::TValue TExternalSkillsDb::SkillNotFound = NSc::TValue::FromJson("{\"error\":{\"code\":1, \"message\":\"Skill not found\"}}");

const NSc::TValue& TExternalSkillsDb::SkillInfo(const TStringBuf id) const {
    with_lock (Mutex) {
        if (auto skillInfo = ExternalSkillsDb_.FindPtr(id); skillInfo) {
            Y_STATS_INC_COUNTER("forms_db_external_skills_success");
            return *skillInfo;
        }
        Y_STATS_INC_COUNTER("forms_db_external_skills_fail");
    }

    return SkillNotFound;
}

TDuration TExternalSkillsDb::DoUpdate() {
    auto req = Delegate.ExternalSkillDbRequest();
    if (!req) {
        return {};
    }
    auto resp = req->Fetch()->Wait();
    if (!resp || resp->IsError()) {
        LOG(ERR) << "Failed to update ExternalSkillsDb, error: "
                 << (resp ? resp->GetErrorText() : "no response") << Endl;
        return UpdatePeriod;
    }

    NSc::TValue skills;
    if (!NSc::TValue::FromJson(skills, resp->Data)) {
        LOG(ERR) << "Failed to parse SkillsDb" << Endl;
        return UpdatePeriod;
    }

    THashMap<TString, NSc::TValue> skillDb;
    for (const auto& skill : skills.GetDict()) {
        NSc::TValue skillInfo;
        skillInfo["result"] = skill.second;
        skillDb.emplace(TString{skill.first}, skillInfo);
    }

    with_lock (Mutex) {
        std::swap(skillDb, ExternalSkillsDb_);
    }

    LOG(INFO) << "Updated ExternalSkillsDb, total: " << ExternalSkillsDb_.size() << " skills" << Endl;
    return UpdatePeriod;
}

} // namespace NBASS
