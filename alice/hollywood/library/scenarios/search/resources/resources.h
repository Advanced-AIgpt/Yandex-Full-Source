#pragma once

#include "logadapters.h"

#include <alice/hollywood/library/resources/resources.h>

#include <alice/library/app_navigation/bno_apps.h>
#include <alice/library/app_navigation/fixlist.h>


namespace NAlice::NHollywood::NSearch {

class TSearchScenarioResources final : public IResourceContainer {
public:
    void LoadFromPath(const TFsPath& dirPath) override;

    const TBnoApps* GetBnoApps() const;
    const TNavigationFixList* GetNavigationFixList() const;
private:
    THolder<TBnoApps> BnoApps;
    THolder<TNavigationFixList> NavigationFixList;
};

} // namespace NAlice::NHollywood::NSearch
