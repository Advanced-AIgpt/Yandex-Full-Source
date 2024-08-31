#pragma once

#include <alice/bass/libs/forms_db/external_skills/external_skill_db.h>

class IScheduler;
class TSourcesRegistry;

namespace NBASS {

class TFormsDb {
public:
    explicit TFormsDb(IFormsDbDelegate& delegate);

    const TExternalSkillsDb& ExternalSkillsDb() const;

    void InitExternalSkillsDb() {
        ExternalSkillsDb_.Init();
    }

private:
    TExternalSkillsDb ExternalSkillsDb_;
};

} // namespace NBASS
