#pragma once

#include <alice/megamind/library/registry/registry.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

#include <util/generic/hash.h>
#include <util/generic/map.h>

namespace NAlice::NMegamind {

class TFakeRegistry final : public TRegistry {
public:
    TFakeRegistry();

    void Add(const TString& path, NAppHost::TAsyncAppService handler) override;
    void Add(const TString& path, NAppHost::TAppService handler) override;
    void Add(const TString& path, NNeh::TServiceFunction handler) override;

public:
    THashSet<TString> AppHostHandlers;
};

} // namespace NAlice::NMegamind
