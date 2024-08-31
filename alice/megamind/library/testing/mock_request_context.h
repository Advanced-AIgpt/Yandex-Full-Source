#pragma once

#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/requestctx/requestctx.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NMegamind {

class TMockInitializer : public TRequestCtx::IInitializer {
public:
    TMockInitializer();

    MOCK_METHOD(TRTLogger&, Logger, (), (override));

    MOCK_METHOD(TCgiParameters, StealCgi, (), (override));
    MOCK_METHOD(THttpHeaders, StealHeaders, (), (override));
    MOCK_METHOD(NUri::TUri, StealUri, (), (override));

    NUri::TUri Uri;
    TCgiParameters Cgi;
    THttpHeaders Headers;
};

class TMockRequestCtx : public TRequestCtx {
public:
    static testing::StrictMock<TMockRequestCtx> CreateStrict(IGlobalCtx& globalCtx) {
        return CreateStrict(globalCtx, TMockInitializer{});
    }

    static testing::StrictMock<TMockRequestCtx> CreateStrict(IGlobalCtx& globalCtx, TMockInitializer&& initializer) {
        return testing::StrictMock<TMockRequestCtx>(&globalCtx, &initializer);
    }

public:
    TMockRequestCtx(IGlobalCtx& globalCtx, TMockInitializer&& initializer)
        : TRequestCtx(globalCtx, std::move(initializer))
    {
        DefaultInit();
    }
    TMockRequestCtx(IGlobalCtx& globalCtx, TRequestCtx::IInitializer&& initializer)
        : TRequestCtx(globalCtx, std::move(initializer))
    {
        DefaultInit();
    }

    MOCK_METHOD(THolder<IHttpResponse>, CreateHttpResponse, (), (const, override));
    MOCK_METHOD(const TString&, Body, (), (const, override));
    MOCK_METHOD(TStringBuf, NodeLocation, (), (const, override));

private:
    friend class testing::StrictMock<TMockRequestCtx>;
    friend class testing::NiceMock<TMockRequestCtx>;

    // Workaround for StrictMock wrapper.
    TMockRequestCtx(IGlobalCtx* globalCtx, TMockInitializer* initializer)
        : TRequestCtx(*globalCtx, std::move(*initializer))
    {
        DefaultInit();
    }

    void DefaultInit();
};

} // namespace NAlice::NMegamind
