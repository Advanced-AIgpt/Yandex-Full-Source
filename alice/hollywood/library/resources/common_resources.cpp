#include "common_resources.h"
#include "geobase.h"
#include "nlg_translations.h"

#include <alice/library/logger/logger.h>

#include <util/generic/set.h>

namespace NAlice::NHollywood {

namespace {

std::unique_ptr<IResourceContainer> CreateResourceFromTag(const TConfig::ECommonResource tag, const TConfig& config) {
    switch (tag) {
        // NOTE(a-square): this is the extension point for adding new common resources
        case TConfig::ECommonResource::TConfig_ECommonResource_Geobase:
            return std::make_unique<TGeobaseResource>(
                config.GetLockMemory() != TConfig::ELockMemory::TConfig_ELockMemory_None
            );
        case TConfig::ECommonResource::TConfig_ECommonResource_NlgTranslations:
            return std::make_unique<TNlgTranslationsResource>();
    }
}

} // namespace

TCommonResources LoadCommonResources(TRTLogger& logger, const TConfig& config) {
    TCommonResources resources;

    TSet<int> enabledCommonResourceTags{
        config.GetEnabledCommonResources().begin(),
        config.GetEnabledCommonResources().end(),
    };

    for (const auto enabledResourceTag : enabledCommonResourceTags) {
        const auto tagEnum = static_cast<TConfig::ECommonResource>(enabledResourceTag);
        LOG_INFO(logger) << "Loading common resource: " << TConfig_ECommonResource_Name(tagEnum);
        auto resource = CreateResourceFromTag(tagEnum, config);
        resource->LoadFromPath(config.GetCommonResourcesPath());
        resources.AddResource(std::move(resource));
    }

    return resources;
}

} // namespace NAlice::NHollywood
