#pragma once

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NMetronome {

inline constexpr TStringBuf START_FRAME = "alice.metronome.start";
inline constexpr TStringBuf FASTER_FRAME = "alice.metronome.faster";
inline constexpr TStringBuf SLOWER_FRAME = "alice.metronome.slower";

inline constexpr TStringBuf BPM_SLOT_NAME = "bpm";
inline constexpr TStringBuf BPM_EXACT_SLOT_NAME = "bpm_exact";
inline constexpr TStringBuf BPM_SHIFT_SLOT_NAME = "bpm_shift";
inline constexpr TStringBuf BPM_UNCERTAIN_SHIFT_SLOT_NAME = "bpm_uncertain_shift";

struct MetronomeStartFrame : public TFrame {
    MetronomeStartFrame(const TRequest::TInput& input)
        : TFrame(input, START_FRAME)
        , Bpm(this, BPM_SLOT_NAME)
    {
    }
    TOptionalSlot<i64> Bpm;
};

struct MetronomeChangeFrame : public TFrame {
    // frameName can be either FASTER_FRAME or SLOWER_FRAME
    MetronomeChangeFrame(const TRequest::TInput& input, const TStringBuf frameName)
        : TFrame(input, frameName)
        , Exact(this, BPM_EXACT_SLOT_NAME)
        , Shift(this, BPM_SHIFT_SLOT_NAME)
        , UncertainShift(this, BPM_UNCERTAIN_SHIFT_SLOT_NAME)
    {
    }
    TOptionalSlot<i64> Exact;
    TOptionalSlot<i64> Shift;
    TOptionalSlot<TString> UncertainShift;
};


}  // namespace NAlice::NHollywoodFw::NMetronome
