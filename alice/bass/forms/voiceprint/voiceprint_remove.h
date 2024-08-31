#pragma once

#include <alice/bass/forms/common/blackbox_api.h>
#include <alice/bass/forms/common/data_sync_api.h>

#include <alice/bass/forms/vins.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>

namespace NVoiceprintTesting {
class TVoiceprintRemoveWrapper;
class TVoiceprintRemoveConfirmWrapper;
} // namespace NVoiceprintTesting

namespace NBASS {

class TContext;

class TVoiceprintRemoveHandler : public IHandler {
public:
    TVoiceprintRemoveHandler(THolder<TBlackBoxAPI> blackBoxAPI, THolder<TDataSyncAPI> dataSyncAPI);

    TResultValue Do(TRequestHandler& r) override {
        return DoImpl(r.Ctx());
    }

private:
    friend class NVoiceprintTesting::TVoiceprintRemoveWrapper;
    TResultValue DoImpl(TContext& ctx);

private:
    THolder<TBlackBoxAPI> BlackBoxAPI;
    THolder<TDataSyncAPI> DataSyncAPI;
};

class TVoiceprintRemoveConfirmHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override {
        return Do(r.Ctx());
    }

private:
    friend class NVoiceprintTesting::TVoiceprintRemoveConfirmWrapper;
    TResultValue Do(TContext& ctx);
    TResultValue DoImpl(TContext& ctx);

private:
    THolder<TBlackBoxAPI> BlackBoxAPI;
    THolder<TDataSyncAPI> DataSyncAPI;
};

void RegisterVoiceprintRemoveHandlers(THandlersMap& handlers);

} // namespace NBASS
