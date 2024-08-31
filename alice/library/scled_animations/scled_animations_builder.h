#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NAlice {

/// Class to make a custom segment animation
///
/// How to use:
/// - create TScledAnimationBuilder object
/// - place any required objects using all possible methods
/// - get result segment animations script and send it into segment_animations_directive
///
class TScledAnimationBuilder: public NNonCopyable::TNonCopyable {
public:
    explicit TScledAnimationBuilder(ui32 animationStepMs = 20)
        : DefaultStep_(animationStepMs)
        , SmoothFactor_(2)
        , CompactingLines_(true)
        , CompressionType_(CompressionTypeGzip)
        , MaxTime_(0) {
        return;
    }

    //
    // Setup
    //
    /// Prepare, compile and encode scled draw format
    enum ECompressionType {
        CompressionTypeNone,
        CompressionTypeGzip
    };
    inline ECompressionType GetCompressionType() const {
        return CompressionType_;
    }
    // USED ONLY FOR Unit-tests!!!
    inline void SetCompressionType(ECompressionType type) {
        CompressionType_ = type;
    }

    /// Get default animation step to prepare final animation (default is 20 ms)
    inline ui32 GetDefaultAnimationStep() const {
        return DefaultStep_;
    }
    /// Override default animation step. Must be called BEFORE Prepare() function!
    inline void SetDefaultAnimationStep(ui32 animationStepMs) {
        Y_ENSURE(AllAnimation_.size() == 0); // This function can be called before any AddXXX() / SetXXX()
        DefaultStep_ = animationStepMs;
    }
    /// Change CompactingLines Mode
    /// CompactingLines_ true (default) - all equal lines will be merged together, duration will be summed up
    /// CompactingLines_ false - each line will have default duration = animationStepMs. Can be used for debugging purposes! 
    inline void SetCompactingLines(bool compactingLines) {
        CompactingLines_ = compactingLines;
    }
    /// Set smoothing factor for soften animation
    /// Smooth factor allows builder to start fading next group together with previous
    /// smooth factor = 1    < fade 1 >< fade 2 >< fade 3 >
    /// smooth factor = 2    < fade 1           >
    ///                                < fade 2           >
    ///                                          < fade 3           >
    /// smooth factor = 3    < fade 1                     >
    ///                                < fade 2                     >
    ///                                          < fade 3                     >
    inline void SetSmoothFactor(int factor) {
        Y_ENSURE(factor > 1 && factor < 10);
        SmoothFactor_ = factor;
    }

    //
    // Patterns and special characters
    //
    /// Pattern for a single digit (7 segments)
    using TPattern = ui8;
    /// Full pattern for all 4 Patterns and special LED indicators
    using TFullPattern = ui32;

    /// Make a symbol/digit using 7 segments:
    ///   AA
    /// F    B
    /// F    B
    ///   GG
    /// E    C
    /// E    C
    ///   DD
    static constexpr TPattern MakeDigit(bool a, bool b, bool c, bool d, bool e, bool f, bool g) {
        return (a ? 0x1 : 0) | (b ? 0x2 : 0) | (c ? 0x4 : 0) | (d ? 0x8 : 0) | (e ? 0x10 : 0) | (f ? 0x20 : 0) | (g ? 0x40 : 0);
    }

    /// Standard digits (0123456789)
    static constexpr TPattern Digit0() {
        return MakeDigit(true, true, true, true, true, true, false);
    }
    static constexpr TPattern Digit1() {
        return MakeDigit(false, true, true, false, false, false, false);
    }
    static constexpr TPattern Digit2() {
        return MakeDigit(true, true, false, true, true, false, true);
    }
    static constexpr TPattern Digit3() {
        return MakeDigit(true, true, true, true, false, false, true);
    }
    static constexpr TPattern Digit4() {
        return MakeDigit(false, true, true, false, false, true, true);
    }
    static constexpr TPattern Digit5() {
        return MakeDigit(true, false, true, true, false, true, true);
    }
    static constexpr TPattern Digit6() {
        return MakeDigit(true, false, true, true, true, true, true);
    }
    static constexpr TPattern Digit7() {
        return MakeDigit(true, true, true, false, false, false, false);
    }
    static constexpr TPattern Digit8() {
        return MakeDigit(true, true, true, true, true, true, true);
    }
    static constexpr TPattern Digit9() {
        return MakeDigit(true, true, true, true, false, true, true);
    }
    /// Standard letters supported by 7-segment indicator (ABCEFGHJLPSU)
    static constexpr TPattern LetterA() {
        return MakeDigit(true, true, true, false, true, true, true);
    }
    static constexpr TPattern LetterALow() {
        return MakeDigit(true, true, true, true, true, false, true);
    }
    static constexpr TPattern LetterB() {
        return Digit8();
    }
    static constexpr TPattern LetterBLow() {
        return MakeDigit(false, false, true, true, true, true, true);
    }
    static constexpr TPattern LetterC() {
        return MakeDigit(true, false, false, true, true, true, false);
    }
    static constexpr TPattern LetterCLow() {
        return MakeDigit(false, false, false, true, true, false, true);
    }
    static constexpr TPattern LetterD() {
        return Digit0();
    }
    static constexpr TPattern LetterDLow() {
        return MakeDigit(false, true, true, true, true, false, true);
    }
    static constexpr TPattern LetterE() {
        return MakeDigit(true, false, false, true, true, true, true);
    }
    static constexpr TPattern LetterF() {
        return MakeDigit(true, false, false, false, true, true, true);
    }
    static constexpr TPattern LetterG() {
        return MakeDigit(true, false, true, true, true, true, false);
    }
    static constexpr TPattern LetterH() {
        return MakeDigit(false, true, true, false, true, true, true);
    }
    static constexpr TPattern LetterHLow() {
        return MakeDigit(false, false, true, false, true, true, true);
    }
    static constexpr TPattern LetterI() {
        return Digit1();
    }
    static constexpr TPattern LetterJ() {
        return MakeDigit(false, true, true, true, false, false, false);
    }
    static constexpr TPattern LetterL() {
        return MakeDigit(false, false, false, true, true, true, false);
    }
    static constexpr TPattern LetterLLow() {
        return MakeDigit(false, false, false, false, true, true, false);
    }
    static constexpr TPattern LetterO() {
        return Digit0();
    }
    static constexpr TPattern LetterOLow() {
        return MakeDigit(false, false, true, true, true, false, true);
    }
    static constexpr TPattern LetterP() {
        return MakeDigit(true, true, false, false, true, true, true);
    }
    static constexpr TPattern LetterS() {
        return MakeDigit(true, false, true, true, false, true, true);
    }
    static constexpr TPattern LetterU() {
        return MakeDigit(false, true, true, true, true, true, false);
    }
    static constexpr TPattern LetterULow() {
        return MakeDigit(false, false, true, true, true, false, false);
    }
    static constexpr TPattern LetterZ() {
        return Digit2();
    }
    /// Additional symbols suported by 7-segment indicator (-_[])
    static constexpr TPattern SymbolSpace() {
        return MakeDigit(false, false, false, false, false, false, false);
    }
    static constexpr TPattern SymbolDash() {
        return MakeDigit(false, false, false, false, false, false, true);
    }
    static constexpr TPattern SymbolUnderline() {
        return MakeDigit(false, false, false, true, false, false, false);
    }
    static constexpr TPattern SymbolOpenBrace() {
        return MakeDigit(true, false, false, true, true, true, false);
    }
    static constexpr TPattern SymbolCloseBrace() {
        return MakeDigit(true, true, true, true, false, false, false);
    }
    /// Universal mapper
    static TPattern Char2Pattern(char c);

    /// Special segments in the middle of the display
    enum SpecialLeds {
        SpecialNothing = 0,
        SpecialMinus = 0x0001,
        SpecialPlusUp = 0x0002,
        SpecialPlusDown = 0x0004,
        SpecialDegree = 0x0008,
        SpecialColon = (SpecialPlusUp | SpecialPlusDown),
        SpecialPlus = (SpecialPlusUp | SpecialPlusDown | SpecialMinus)
    };

    static TFullPattern MakeFullPattern(TPattern p1, TPattern p2, int specialLeds, TPattern p3, TPattern p4);
    static TFullPattern String2Pattern(const TString& pattern);

    //
    // AddXxx methods - make animations fast and easy
    //
    /// Predefined animation modes
    enum EAnimMode {
        AnimModeSolid = 0x1,
        /// Start animation with fade (in/out)
        AnimModeFade = 0x2,
        /// Start show digit from left to right side
        AnimModeFromLeft = 0x4,
        /// Start show digit from right to left side
        AnimModeFromRight = 0x8,
        /// Start show digit from top to bottom side
        AnimModeFromTop = 0x10,
        /// Start show digit from bottom to top side
        AnimModeFromBottom = 0x20,
        /// Animation combinations
        AnimModeFromBottomLeft = (AnimModeFromLeft | AnimModeFromBottom),
        /// Animation speed
        AnimModeSpeedNormal = 0x1000,
        AnimModeSpeedSlowFast = 0x2000,
        AnimModeSpeedFastSlow = 0x4000,
        AnimModeSpeedSmooth = 0x8000
    };

    void AddAnim(TFullPattern pattern, ui8 bright1, ui8 bright2, int durationMs, int animMode);
    inline void AddAnim(const TString& pattern, ui8 bright1, ui8 bright2, int durationMs, int animMode) {
        AddAnim(String2Pattern(pattern), bright1, bright2, durationMs, animMode);
    }
    void AddDraw(TFullPattern pattern, ui8 bright, int durationMs);
    inline void AddDraw(const TString& pattern, ui8 bright, int durationMs) {
        AddDraw(String2Pattern(pattern), bright, durationMs);
    }

    //
    // SetXxx methoss - customize animations or create special effects
    //

    /// Options for SetXxx functions
    enum EDrawMode {
        /// New pattern for this group will be merged with currently existing samples
        DrawModeMerge,
        /// New pattern for this group will replace all currently existing LEDs in this time range
        DrawModeReplace,
        /// New pattern will be multiplied as 1/255 to the current pattern
        DrawModeMultiply
    };

    void SetAnim(TFullPattern pattern, ui8 bright1, ui8 bright2, ui32 from, ui32 to, int animMode, EDrawMode drawMode = DrawModeMerge);
    inline void SetAnim(const TString& pattern, ui8 bright1, ui8 bright2, ui32 from, ui32 to, int animMode, EDrawMode drawMode = DrawModeMerge) {
        SetAnim(String2Pattern(pattern), bright1, bright2, from, to, animMode, drawMode);
    }
    void SetDraw(TFullPattern pattern, ui8 bright, ui32 from, ui32 to, EDrawMode drawMode = DrawModeMerge);
    inline void SetDraw(const TString& pattern, ui8 bright, ui32 from, ui32 to, EDrawMode drawMode = DrawModeMerge) {
        SetDraw(String2Pattern(pattern), bright, from, to, drawMode);
    }

    /// OBSOLETE function, used for debugging purpose only
    TString PrepareScled();
    TString PrepareBinary();

private:
    // default animation step in milliseconds
    ui32 DefaultStep_;
    // smooth factor
    int SmoothFactor_;
    // true if identical lines in final animation should be merged together
    bool CompactingLines_;
    // Type of compression (none/gzip)
    ECompressionType CompressionType_;
    // Current max time for next AddXxxx() call
    ui32 MaxTime_;

    // Internal instruction to highlight LEDs
    struct TAnimData {
        TFullPattern FullPattern; // data segment mask
        ui8 Bright1;              // Initial brghtness
        ui8 Bright2;              // Target brightness
        ui32 From;                // Time in milleseconds to start LEDs
        ui32 To;                  // Time in milleseconds to end LEDs
        EDrawMode DrawMode;       // Merge or replace
        EAnimMode AnimMode;       // Type of animation
    };

    // final map to store all anmation data
    struct TSegmentLine {
        ui8 Segments[32]; // 4 digits with 7 segments each, plus (two segments), minus and degree
        ui32 Duration;
        bool operator == (const TSegmentLine& rhs) const {
            return memcmp(Segments, rhs.Segments, sizeof(Segments)) == 0;
        }
    };

    // Full map of all stored animations
    TVector<TAnimData> AllDirectives_;
    TVector<TSegmentLine> AllAnimation_;

    void PrepareAllAnim_();
    void FillAllSegments_(const TAnimData& a);
    void FillSegment_(size_t index, ui8 brightness, TFullPattern mask, EDrawMode mode);
    void FillSolid_(size_t startIndex, size_t endIndex, ui8 brightness, TFullPattern p, EDrawMode drawMode);
    void FillFade_(size_t startIndex, size_t endIndex, ui8 bright1, ui8 bright2, TFullPattern p, EDrawMode drawMode, std::function<float(float)>& f);
    void FillAnimation_(size_t startIndex, size_t endIndex, const TAnimData& a, const TVector<TFullPattern>& slides, std::function<float(float)>& f);
    void FillMultiply_(size_t startIndex, size_t endIndex);
};

} // namespace NAlice
