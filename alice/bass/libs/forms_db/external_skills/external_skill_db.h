#pragma once

#include <alice/bass/libs/config/config.sc.h>
#include <alice/bass/libs/forms_db/delegate/delagate.h>

#include <library/cpp/scheme/scheme.h>

#include <util/datetime/base.h>
#include <util/system/spinlock.h>

class IScheduler;

namespace NBASS {

/*
 * This is a backup for ExternalSkillsApi
 * SkillInfo returns:
 *     1. Null -- same as no answer from ExternalSkillsApi (not implemented here)
 *     2. {"error":{"code":1, "message":"Skill not found"}} -- skill was not found
 *     3. real skill info -- good answer from ExternalSkillsApi
 */
class TExternalSkillsDb {
public:
    TExternalSkillsDb(IFormsDbDelegate& delegate);

    const NSc::TValue& SkillInfo(const TStringBuf id) const;

    void Init();

private:
    static TDuration Update(TExternalSkillsDb* skillDb) {
        return skillDb->DoUpdate();
    }

    TDuration DoUpdate();

private:
    static const NSc::TValue SkillNotFound;
    IFormsDbDelegate& Delegate;
    TDuration UpdatePeriod;

    TAdaptiveLock Mutex;
    THashMap<TString, NSc::TValue> ExternalSkillsDb_;
};

} // namespace NBASS
