#include "item_adapter.h"
#include "item_names.h"

#include <alice/library/json/json.h>
#include <alice/library/metrics/names.h>
#include <alice/library/metrics/sensors.h>

#include <util/generic/scope.h>

namespace NAlice::NMegamind {
namespace {

constexpr auto WAIT_PERIOD = TDuration::Seconds(6);

void SignalOnErrorGetItem(NMetrics::ISensors& sensors, TStringBuf type) {
    NMonitoring::TLabels labels;
    labels.Add("name", NSignal::APPHOST_GET_ITEM_FAILURE);
    labels.Add("type", type);
    sensors.IncRate(labels);
}

} // namespace

// TItemProxyAdapter ----------------------------------------------------------
TItemProxyAdapter::TItemProxyAdapter(NAppHost::IServiceContext& ctx, TRTLogger& logger, IGlobalCtx& globalCtx, bool useStreaming)
    : Ctx_{ctx}
    , GlobalCtx_{globalCtx}
    , Logger_{logger}
    , Location_{Ctx_.GetLocation()}
    , EndOfStreaming_{!useStreaming}
{
}

void TItemProxyAdapter::PutIntoContext(const TProto& item, TStringBuf itemName) {
    Ctx_.AddProtobufItem(item, itemName);
}

void TItemProxyAdapter::PutJsonIntoContext(NJson::TJsonValue&& item, TStringBuf itemName) {
    Ctx_.AddItem(std::move(item), itemName);
}

void TItemProxyAdapter::CheckAndPutIntoContext(const TProto& item, TStringBuf itemName) {
    if (Ctx_.HasProtobufItem(itemName, NAppHost::EContextItemSelection::Output)) {
        LOG_DEBUG(Logger_) << "Item " << itemName << " has already been put into context";
        return;
    }
    Ctx_.AddProtobufItem(item, itemName);
}

TErrorOr<TString> TItemProxyAdapter::GetFromContextImpl(TStringBuf itemName, size_t index) {
    auto& sensors = GlobalCtx_.ServiceSensors();

    const TVector<TProxyItem>* item = Items_.FindPtr(itemName);
    for (/* empty */; (!item && !EndOfStreaming_) || (item && item->size() < index); item = Items_.FindPtr(itemName)) {
        {
            Y_SCOPE_EXIT(&sensors) {
                sensors.AddIntGauge(NSignal::APPHOST_NEXTINPUT_IN_PROGRESS, -1);
            };
            sensors.AddIntGauge(NSignal::APPHOST_NEXTINPUT_IN_PROGRESS, 1);
            // FIXME (petrk) TDuration::Seconds(6) is a really temporary solution to prevent killing mm by watchog.
            // ISSUE MEGAMIND-2959 to improve it.
            auto f = Ctx_.NextInput(WAIT_PERIOD);
            if (!f.Wait(WAIT_PERIOD)) {
                ythrow yexception() << "AppHost NextInput() future waiting for more than: " << WAIT_PERIOD << "; item name: " << itemName;
            }
            if (EndOfStreaming_ = !f.GetValueSync()) {
                break;
            }
        }

        UpdateCache();
    }

    if (!item) {
        SignalOnErrorGetItem(sensors, "not_in_context");
        return TError{TError::EType::NotFound} << "Item '" << itemName << "' is not in context";
    }

    if (index >= item->size()) {
        SignalOnErrorGetItem(sensors, "out_of_bounds");
        return TError{TError::EType::Input} << "Item '" << itemName << "' has total " << item->size() << " elements, " << index << " out of bounds";
    }

    LOG_DEBUG(Logger_) << "Item " << itemName << " has been retrieved from context";
    return item->at(index).Data;
}

void TItemProxyAdapter::UpdateCache() {
    const auto& items = Ctx_.GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
    for (auto srcIt = items.begin(), srcEnd = items.end(); srcIt != srcEnd; ++srcIt) {
        Items_[srcIt.GetType()].push_back(TProxyItem{ToString(srcIt->Raw())});
    }
    Ctx_.DeleteItems(NAppHost::EContextItemSelection::Input);
}

TVector<TString> TItemProxyAdapter::GetItemNamesFromCache() {
    UpdateCache();
    TVector<TString> names(Reserve(Items_.size()));
    for (const auto& [name, _] : Items_) {
        names.push_back(name);
    }
    return names;
}

TErrorOr<NJson::TJsonValue> TItemProxyAdapter::GetJsonFromContext(const TStringBuf itemName, size_t index) {
    UpdateCache();
    TString data;
    if (auto err = GetFromContextImpl(itemName, index).MoveTo(data)) {
        LOG_DEBUG(Logger_) << "Failed to retrieve " << itemName << " from app_host context: " << err;
        return std::move(*err);
    }

    try {
        return JsonFromString(data);
    } catch (const yexception& e) {
        return TError{TError::EType::Parse} << "Failed to parse json from '" << itemName << "\'' item, error: " << e.what();
    }
}

bool TItemProxyAdapter::WaitNextInput() {
    if (!EndOfStreaming_) {
        auto f = Ctx_.NextInput(WAIT_PERIOD);
        if (!f.Wait(WAIT_PERIOD)) {
            ythrow yexception() << "AppHost NextInput() future waiting for more than: " << WAIT_PERIOD;
        }
        EndOfStreaming_ = !f.GetValueSync();
        UpdateCache();
        return true;
    }
    return false;
}

} // namespace NAlice::NMegamind
