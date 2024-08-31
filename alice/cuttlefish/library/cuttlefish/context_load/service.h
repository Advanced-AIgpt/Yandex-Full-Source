#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>

#include <apphost/api/service/cpp/service.h>

#include <util/generic/strbuf.h>


namespace NAlice::NCuttlefish::NAppHostServices {


/**
 *  @brief prepares request for datasync and blackbox
 */
void ContextLoadPre(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);


/**
 *  @brief handles response from blackbox and prepares requests for datasync, memento, notificator and quasar iot
 */
void ContextLoadBlackboxSetdown(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);

/**
 *  @brief picks responses from blackbox and flags.json and make a request for contacts
 */
void ContextLoadMakeContactsRequest(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);


/**
 *  @brief handles response from blackbox and quasar iot and prepares request laas
 */
void ContextLoadPrepareLaas(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);


/**
 *  @brief handles response from laas and prepares request to flags.json
 */
void ContextLoadPrepareFlagsJson(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);


/**
 *  @brief aggregates responses from datasync, memento, notificator, cachalot and quasar iot into one big proto
 */
void ContextLoadPost(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);

void FakeContextLoad(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);


}  // namespace NAlice::NCuttlefish::NAppHostServices
