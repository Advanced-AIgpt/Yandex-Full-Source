#include "common_resources.h"

#include <library/cpp/testing/gtest/gtest.h>

using namespace NAlice::NHollywood;
using testing::Throws;

struct TFoo final : public IResourceContainer {
    void LoadFromPath(const TFsPath& /* dirPath */) override {
    }
};

struct TBar final : public IResourceContainer {
    void LoadFromPath(const TFsPath& /* dirPath */) override {
    }
};

TEST(TCommonResources, Empty) {
    TCommonResources resources;

    EXPECT_THAT(
        [&]() { resources.Resource<TFoo>(); },
        Throws<TCommonResourceNotFoundError>()
    );

    EXPECT_THAT(
        [&]() { resources.Resource<TBar>(); },
        Throws<TCommonResourceNotFoundError>()
    );
}

TEST(TCommonResources, NonEmpty) {
    auto foo = std::make_unique<TFoo>();
    auto* fooAddr = foo.get();

    TCommonResources resources;
    resources.AddResource(std::move(foo));

    EXPECT_EQ(fooAddr, &resources.Resource<TFoo>());

    EXPECT_THAT(
        [&]() { resources.Resource<TBar>(); },
        Throws<TCommonResourceNotFoundError>()
    );
}
