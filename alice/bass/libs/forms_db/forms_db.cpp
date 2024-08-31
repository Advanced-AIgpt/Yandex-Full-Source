#include "forms_db.h"

#include <alice/bass/libs/forms_db/delegate/delagate.h>

namespace NBASS {

TFormsDb::TFormsDb(IFormsDbDelegate& delegate)
    : ExternalSkillsDb_(delegate)
{
}

const TExternalSkillsDb& TFormsDb::ExternalSkillsDb() const {
    return ExternalSkillsDb_;
}

} // namespace NBASS
