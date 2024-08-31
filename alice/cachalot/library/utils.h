#pragma once

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cachalot/library/debug.h>

#include <library/cpp/threading/future/core/future.h>
#include <library/cpp/threading/future/subscription/subscription.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>

#include <atomic>


namespace NCachalot {

double MillisecondsSince(const TInstant startTime);


template <typename T>
NThreading::TFuture<std::pair<T, T>> WaitBoth(NThreading::TFuture<T> lhs, NThreading::TFuture<T> rhs) {
    auto counter = MakeAtomicShared<std::atomic<int>>(0);
    auto result = NThreading::NewPromise<std::pair<T, T>>();
    auto values = MakeAtomicShared<std::pair<T, T>>();

    auto mngr = NThreading::TSubscriptionManager::NewInstance();

    mngr->Subscribe(
        lhs,
        [counter, result, values](const auto& future) mutable {
            values->first = future.GetValueSync();
            if (counter->fetch_add(1) == 1) {
                result.SetValue(*values);
            }
        }
    );

    mngr->Subscribe(
        rhs,
        [counter, result, values](const auto& future) mutable {
            values->second = future.GetValueSync();
            if (counter->fetch_add(1) == 1) {
                result.SetValue(*values);
            }
        }
    );

    return result;
}

template <typename TEvent, typename T, typename TFunc>
void SubscribeWithTryCatchWrapper(NThreading::TFuture<T> future, TFunc&& func, TChroniclerPtr chronicler) {
    auto mngr = NThreading::TSubscriptionManager::NewInstance();

    mngr->Subscribe(
        future,
        [func=std::forward<TFunc>(func), chronicler](NThreading::TFuture<T> result) mutable {
            try {
                func(result);
            } catch (...) {
                const TString msg = "Caught exception in callback: " + CurrentExceptionMessage();
                chronicler->LogEvent(TEvent(msg));
            }
        }
    );
}

namespace {
    using TProtobufItem = NAppHost::NService::TProtobufItem;
}

/// Applies the given NON-THROWABLE THandler function to every iterator in the range [first, last), in order.
/// \tparam TProto          Protobuf item type
/// \tparam TItemStorageIt  Iterator type
/// \tparam THandler        Handler type
/// \param begin            Start of apply range
/// \param end              End of apply range
/// \param handler          Should NEVER throw exception
/// \return
template <typename TProto, typename TItemStorageIt, typename THandler, typename = std::enable_if<std::is_nothrow_invocable<THandler, TStringBuf, TProto>::value>>
[[nodiscard]] NThreading::TFuture<void> ForEachProtobufItemAsync(TItemStorageIt begin, TItemStorageIt end, THandler&& handler) {
    TMaybe<NThreading::TFuture<void>> executionChain;

    for (auto it = begin; it != end; ++it) {
        TString type(it.GetType());
        TProto protoItem = NAlice::NCuttlefish::ParseProtobufItem<TProto>(*it);

        executionChain = executionChain
            ? executionChain->Apply([handler, type = std::move(type), protoItem = std::move(protoItem)](const NThreading::TFuture<void>&) mutable {
                return handler(std::move(type), std::move(protoItem));
            })
            : handler(std::move(type), std::move(protoItem));
    }

    return executionChain.GetOrElse(NThreading::MakeFuture());
}

} //  namespace NCachalot
