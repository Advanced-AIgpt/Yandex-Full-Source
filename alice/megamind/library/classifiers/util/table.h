#pragma once

#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NAlice {

enum class EClassificationMarker {
    //Direct select markers
    Forced,
    Boosted,
    // Necessary criterions markers
    AllowedByFixlistForTurkish,
    AcceptsInput,
    SupportsLanguage,
    IsEnabled,
    ShouldNotFilterSpecificScenario,
    // Strong markers
    FramesIntersectRecognizedAction,
    ScenarioWithHint,
    IgnoreHintsOnClient,
    ShouldAvoidCuttingByThresholdForCtx,
    WaitsForSlotValue,
    ActiveScenario,
    ExceedThreshold,
    InFixlist,
    SaveVideoWithItemSelector
};

class TClassificationTable {
public:
    using TMarkersTable = THashMap<EClassificationMarker, THashMap<TStringBuf, bool>>;

    TClassificationTable(const TVector<TStringBuf> scenarios);

    void SetDirectMarker(const TStringBuf scenario, const EClassificationMarker marker);
    void SetStrongMarker(const TStringBuf scenario, const EClassificationMarker marker);
    void SetNecessaryMarker(const TStringBuf scenario, const EClassificationMarker marker);

    void SetEmptyDirectMarker(const EClassificationMarker marker);
    void SetEmptyStrongMarker(const EClassificationMarker marker);
    void SetEmptyNecessaryMarker(const EClassificationMarker marker);

    void SetDirectMarkersLossReason(const ELossReason lossReason);
    void SetStrongMarkersLossReason(const ELossReason lossReason);
    void SetNecessaryMarkersLossReason(const TStringBuf scenario, const ELossReason lossReason);

    bool AllNecessary(const TStringBuf scenario) const;
    TVector<TStringBuf> GetAllNecessary() const;
    TVector<TStringBuf> GetWinners(TQualityStorage& qualityStorage) const;
    bool StrongMarkersExist() const;

private:
    TVector<TStringBuf> Scenarios;

    TMarkersTable DirectMarkers;
    TMarkersTable StrongMarkers;
    TMarkersTable NecessaryMarkers;

    ELossReason DirectMarkersLossReason;
    ELossReason StrongMarkersLossReason;
    THashMap<TStringBuf, ELossReason> NecessaryMarkersLossReasons;

private:
    void SetMarker(const TStringBuf scenario, const EClassificationMarker marker, TMarkersTable& markersTable);
    void SetEmptyMarker(const EClassificationMarker marker, TMarkersTable& markersTable);
};

} // namespace NAlice
