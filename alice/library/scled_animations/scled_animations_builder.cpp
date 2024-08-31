#include "scled_animations_builder.h"

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/array_ref.h>
#include <util/generic/buffer.h>
#include <util/generic/string.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/stream/buffer.h>
#include <util/stream/format.h>
#include <util/stream/str.h>
#include <util/stream/zlib.h>

#include <functional>
#include <math.h>

namespace NAlice {
namespace {

    //
    // Maximum number of animated steps is 1000*20 ms = 20 sec
    //
    static const ui32 MAX_ANIMATED_STEPS = 1000;

    //
    // Function for normal animation (AnimModeSpeedNormal)
    //
    static float FuncNormal_(float x) {
        return x;
    }
    //
    // Function for Slow->Fast animation (AnimModeSpeedSlowFast)
    //
    static float FuncSlowFast_(float x) {
        return x * x;
    }
    //
    // Function for Fast->Slow animation (AnimModeSpeedFastSlow)
    //
    static float FuncFastSlow_(float x) {
        x = 1.f - x;
        return 1 - x * x;
    }
    //
    // Function for smart smooth animation (AnimModeSpeedSmooth)
    //
    const float SLOPE1 = 1.4f;
    const float SLOPE2 = 0.15f;

    static float FuncHermit_(float x) {
        float slopeL, slopeR;
        float kp1, kp2;

        if (x < 0.5f) {
            x = x * 2;
            slopeL = SLOPE1;
            slopeR = SLOPE2;
            kp1 = 0.f;
            kp2 = 0.5f;
        } else {
            x = (x - 0.5f) * 2;
            slopeL = SLOPE2;
            slopeR = SLOPE1;
            kp1 = 0.5f;
            kp2 = 1.f;
        }
        float x2 = x * x;
        float x3 = x2 * x;

        float coeff_value1 = 2 * x3 - 3 * x2;
        float coeff_value0 = coeff_value1 + 1.f;
        float coeff_slope1 = x3 - x2;
        float coeff_slope0 = x3 - 2 * x2 + x;
        return coeff_value0 * kp1 - coeff_value1 * kp2 + coeff_slope1 * slopeR + coeff_slope0 * slopeL;
    }

    //
    // Interpolate index between range using selected interpolation function (see above)
    //
    template <typename T> T Interpolate_(size_t beginIdx, size_t endIdx, size_t currentIdx, T begin, T end, std::function<float(float)>& f) {
        // prevent from division by zero
        T delta = endIdx - beginIdx;
        if (delta == 0) {
            return end;
        }
        float x = static_cast<float>(currentIdx - beginIdx) / delta;
        x = f(x);
        return static_cast<T>(round(begin + x * (end - begin)));
    }

    //
    // Convert Pattern / Special symbols to compressed ui32 mask
    // We use SCLED compatible mask format
    // A B C D E F G  |  A B C D E F G  |  A B C D E F G  |  A B C D E F G  |  PLUSDOWN PLUSUP MINUS DEGREE
    // 0 1 2 ...                                                               28       29     30    bit 31
    //
    // order = 0, 1, 2 or 3 (digit placement, from left to right)
    static inline ui32 Digit2Mask_(int order, TScledAnimationBuilder::TPattern digit) {
        return (digit << (order * 7));
    }

    ///
    /// Convert special signs to SCLED mask format
    ///
    static inline ui32 Special2Mask_(int specialLeds) {
        ui32 resultMask = 0;
        if (specialLeds & TScledAnimationBuilder::SpecialPlusUp) {
            resultMask |= 1 << 29;
        }
        if (specialLeds & TScledAnimationBuilder::SpecialPlusDown) {
            resultMask |= 1 << 28;
        }
        if (specialLeds & TScledAnimationBuilder::SpecialMinus) {
            resultMask |= 1 << 30;
        }
        if (specialLeds & TScledAnimationBuilder::SpecialDegree) {
            resultMask |= 1 << 31;
        }
        return resultMask;
    }

    ///
    /// Segments mask to make soft animation verticaly or horizontally
    ///
    static constexpr TScledAnimationBuilder::TPattern VERTICAL_1 = TScledAnimationBuilder::MakeDigit(true, false, false, false, false, false, false);
    static constexpr TScledAnimationBuilder::TPattern VERTICAL_2 = TScledAnimationBuilder::MakeDigit(false, true, false, false, false, true, false);
    static constexpr TScledAnimationBuilder::TPattern VERTICAL_3 = TScledAnimationBuilder::MakeDigit(false, false, false, false, false, false, true);
    static constexpr TScledAnimationBuilder::TPattern VERTICAL_4 = TScledAnimationBuilder::MakeDigit(false, false, true, false, true, false, false);
    static constexpr TScledAnimationBuilder::TPattern VERTICAL_5 = TScledAnimationBuilder::MakeDigit(false, false, false, true, false, false, false);

    static TVector<TScledAnimationBuilder::TFullPattern> VERTICAL_ANIMATION_TOP_BOTTOM = {
        TScledAnimationBuilder::MakeFullPattern(VERTICAL_1, VERTICAL_1, TScledAnimationBuilder::SpecialDegree, VERTICAL_1, VERTICAL_1),
        TScledAnimationBuilder::MakeFullPattern(VERTICAL_2, VERTICAL_2, TScledAnimationBuilder::SpecialPlusUp, VERTICAL_2, VERTICAL_2),
        TScledAnimationBuilder::MakeFullPattern(VERTICAL_3, VERTICAL_3, TScledAnimationBuilder::SpecialMinus, VERTICAL_3, VERTICAL_3),
        TScledAnimationBuilder::MakeFullPattern(VERTICAL_4, VERTICAL_4, TScledAnimationBuilder::SpecialPlusDown, VERTICAL_4, VERTICAL_4),
        TScledAnimationBuilder::MakeFullPattern(VERTICAL_5, VERTICAL_5, TScledAnimationBuilder::SpecialNothing, VERTICAL_5, VERTICAL_5),
    };

    static TVector<TScledAnimationBuilder::TFullPattern> VERTICAL_ANIMATION_BOTTOM_TOP = {
        TScledAnimationBuilder::MakeFullPattern(VERTICAL_5, VERTICAL_5, TScledAnimationBuilder::SpecialNothing, VERTICAL_5, VERTICAL_5),
        TScledAnimationBuilder::MakeFullPattern(VERTICAL_4, VERTICAL_4, TScledAnimationBuilder::SpecialPlusDown, VERTICAL_4, VERTICAL_4),
        TScledAnimationBuilder::MakeFullPattern(VERTICAL_3, VERTICAL_3, TScledAnimationBuilder::SpecialMinus, VERTICAL_3, VERTICAL_3),
        TScledAnimationBuilder::MakeFullPattern(VERTICAL_2, VERTICAL_2, TScledAnimationBuilder::SpecialPlusUp, VERTICAL_2, VERTICAL_2),
        TScledAnimationBuilder::MakeFullPattern(VERTICAL_1, VERTICAL_1, TScledAnimationBuilder::SpecialDegree, VERTICAL_1, VERTICAL_1)
    };

    static constexpr TScledAnimationBuilder::TPattern HORIZONTAL_1 = TScledAnimationBuilder::MakeDigit(false, false, false, false, true, true, false);
    static constexpr TScledAnimationBuilder::TPattern HORIZONTAL_2 = TScledAnimationBuilder::MakeDigit(true, false, false, true, false, false, true);
    static constexpr TScledAnimationBuilder::TPattern HORIZONTAL_3 = TScledAnimationBuilder::MakeDigit(false, true, true, false, false, false, false);

    static TVector<TScledAnimationBuilder::TFullPattern> HORIZONTAL_ANIMATION_LEFT_RIGHT = {
        TScledAnimationBuilder::MakeFullPattern(HORIZONTAL_1, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(HORIZONTAL_2, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(HORIZONTAL_3, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_1, TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_2, TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_3, TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialPlus, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, HORIZONTAL_1, TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, HORIZONTAL_2, TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, HORIZONTAL_3, TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_1),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_2),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_3),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialDegree, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace())
    };

    static TVector<TScledAnimationBuilder::TFullPattern> HORIZONTAL_ANIMATION_RIGHT_LEFT = {
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialDegree, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_3),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_2),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_1),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, HORIZONTAL_3, TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, HORIZONTAL_2, TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, HORIZONTAL_1, TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialPlus, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_3, TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_2, TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(TScledAnimationBuilder::SymbolSpace(), HORIZONTAL_1, TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(HORIZONTAL_3, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(HORIZONTAL_2, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace()),
        TScledAnimationBuilder::MakeFullPattern(HORIZONTAL_1, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SpecialNothing, TScledAnimationBuilder::SymbolSpace(), TScledAnimationBuilder::SymbolSpace())
    };

} // namespace NInternal

///
/// Convert any symbol to pattern
///
TScledAnimationBuilder::TPattern TScledAnimationBuilder::Char2Pattern(char c) {
    switch (c) {
        case '0':
            return Digit0();
        case '1':
            return Digit1();
        case '2':
            return Digit2();
        case '3':
            return Digit3();
        case '4':
            return Digit4();
        case '5':
            return Digit5();
        case '6':
            return Digit6();
        case '7':
            return Digit7();
        case '8':
            return Digit8();
        case '9':
            return Digit9();
        case 'A':
            return LetterA();
        case 'a':
            return LetterALow();
        case 'B':
            return LetterB();
        case 'b':
            return LetterBLow();
        case 'C':
            return LetterC();
        case 'c':
            return LetterCLow();
        case 'D':
            return LetterD();
        case 'd':
            return LetterDLow();
        case 'E':
        case 'e':
            return LetterE();
        case 'F':
        case 'f':
            return LetterF();
        case 'G':
        case 'g':
            return LetterG();
        case 'H':
            return LetterH();
        case 'h':
            return LetterHLow();
        case 'I':
        case 'i':
            return LetterI();
        case 'J':
        case 'j':
            return LetterJ();
        case 'L':
            return LetterL();
        case 'l':
            return LetterLLow();
        case 'O':
            return LetterO();
        case 'o':
            return LetterOLow();
        case 'P':
        case 'p':
            return LetterP();
        case 'S':
        case 's':
            return LetterS();
        case 'U':
            return LetterU();
        case 'u':
            return LetterULow();
        case 'Z':
        case 'z':
            return LetterZ();
        case ' ':
            return SymbolSpace();
        case '-':
            return SymbolDash();
        case '_':
            return SymbolUnderline();
        case '[':
            return SymbolOpenBrace();
        case ']':
            return SymbolCloseBrace();
        default:
            // This simbol is not supported by 7-segments LED display
            // Will draw nothing
            break;
    }
    return 0;
}

///
/// Add animated pattern to the end of current scled
/// @pattern - prepared pattern with standard or custom symbol
/// @brightnessStart...@brightnessEnd - LED brightness on first and last frame of animation
/// @durationMs - times in ms of animation
/// @mode - animation mode
///
void TScledAnimationBuilder::AddAnim(TFullPattern pattern, ui8 brightnessStart, ui8 brightnessEnd, int durationMs, int animMode) {
    
    TAnimData a = {pattern, brightnessStart, brightnessEnd, MaxTime_, MaxTime_ + durationMs, DrawModeMerge, (EAnimMode)animMode};
    AllDirectives_.push_back(a);

    MaxTime_ += durationMs;
    return;
}

///
/// Add any pattern (without animation) to the end of current scled
/// @pattern - prepared pattern with standard or custom symbol
/// @bright - LED brightness (0...255)
/// @durationMs - times in ms of animation
///
void TScledAnimationBuilder::AddDraw(TFullPattern pattern, ui8 bright, int durationMs) {

    TAnimData a = {pattern, bright, bright, MaxTime_, MaxTime_ + durationMs, DrawModeMerge, AnimModeSolid};
    AllDirectives_.push_back(a);

    MaxTime_ += durationMs;
    return;
}

///
/// Customize an existing SCLED animation with additional pattern
/// @pattern - prepared pattern with standard or custom symbol
/// @brightnessStart...@brightnessEnd - LED brightness on first and last frame of animation
/// @from...@to - times to add data
/// @drawMode - drawing mode (merge with current or replace)
///
void TScledAnimationBuilder::SetAnim(TFullPattern pattern, ui8 brightnessStart, ui8 brightnessEnd, ui32 from, ui32 to, int animMode, EDrawMode drawMode /*= DrawModeMerge*/) {

    Y_ENSURE(from < to);

    TAnimData a = {pattern, brightnessStart, brightnessEnd, from, to, drawMode, (EAnimMode)animMode};
    AllDirectives_.push_back(a);

    MaxTime_ = Max(MaxTime_, (ui32)to);
    return;
}

///
/// Customize an existing SCLED animation with additional pattern
/// @pattern - prepared pattern with standard or custom symbol
/// @bright - LED brightness
/// @from...@to - times to add data
/// @drawMode - drawing mode (merge with current or replace)
///
void TScledAnimationBuilder::SetDraw(TFullPattern pattern, ui8 bright, ui32 from, ui32 to, EDrawMode drawMode /*= DrawModeMerge*/) {
    
    Y_ENSURE(from < to);

    TAnimData a = {pattern, bright, bright, from, to, drawMode, AnimModeSolid};
    AllDirectives_.push_back(a);

    MaxTime_ = Max(MaxTime_, (ui32)to);
    return;
}

///
/// Creates a full pattern using 4 digits/letters and additional led segments (plus/minus, degree)
///
TScledAnimationBuilder::TFullPattern TScledAnimationBuilder::MakeFullPattern(TPattern p1, TPattern p2, int specialLeds, TPattern p3, TPattern p4) {
    
    TFullPattern fp;

    fp = Digit2Mask_(0, p1) | Digit2Mask_(1, p2) | Digit2Mask_(2, p3) | Digit2Mask_(3, p4);
    fp |= Special2Mask_(specialLeds);

    return fp;
}

///
/// Convert text pattern to full pattern
/// @pattern: string with following format: "aa@aa*", where
/// - a - any digit or supported characters or sequence like '#0000000'
/// - @ - space, + - :
/// - * - * or space
///
TScledAnimationBuilder::TFullPattern TScledAnimationBuilder::String2Pattern(const TString& pattern) {
    TString patternCopy = pattern;
    size_t currentStringPos = 0;
    TFullPattern fp = 0;
    TPattern digitsFpStorage[6] = {0};
    TArrayRef<TPattern> digitsFp(digitsFpStorage, sizeof(digitsFpStorage)/sizeof(TPattern));

    for (int ledPos = 0; ledPos <= 5; ledPos++) {
        // 0, 1 - first and second digits/letters
        // 2 - central symbol (space - : +)
        // 3, 4 - third and fourth digits/letters
        // 5 - degree sign (space or *)
        if (pattern.length() <= currentStringPos) {
            break;
        }
        if (ledPos == 2) {
            // Handle central symbol
            switch (pattern[currentStringPos]) {
            case ' ':
                // Does nothing
                break;
            case ':':
                fp |= Special2Mask_(SpecialColon);
                break;
            case '+':
                fp |= Special2Mask_(SpecialPlus);
                break;
            case '-':
                fp |= Special2Mask_(SpecialMinus);
                break;
            default:
                // Incorrect pattern format. Must be "##:##*"
                break;
            }
            currentStringPos++;
            continue;
        }
        if (ledPos == 5) {
            // Handle last symbol
            switch (pattern[currentStringPos]) {
            case ' ':
                // Does nothing
                break;
            case '*':
                fp |= Special2Mask_(SpecialDegree);
                break;
            default:
                // Incorrect pattern format. Must be "##:##*"
                break;
            }
            currentStringPos++;
            continue;
        }

        // Handle digits
        if (pattern[currentStringPos] != '#') {
            // Use default pattern
            digitsFp[ledPos] = Char2Pattern(pattern[currentStringPos]);
            currentStringPos++;
            continue;
        }
        // Work with sequence #0000000
        currentStringPos++; // skip current symbol '#'
        bool ledsStorage[7] = {false};
        TArrayRef<bool> leds(ledsStorage, sizeof(ledsStorage)/sizeof(bool));
        for (size_t i = 0; i < leds.size(); i++, currentStringPos++) {
            if (pattern.length() <= currentStringPos) {
                break;
            }
            leds[i] = (pattern[currentStringPos] == '1');
        }
        digitsFp[ledPos] = MakeDigit(leds[0], leds[1], leds[2], leds[3], leds[4], leds[5], leds[6]);
    }
    fp |= MakeFullPattern(digitsFp[0], digitsFp[1], SpecialNothing, digitsFp[3], digitsFp[4]);
    return fp;
}

///
/// Finalize animation and prepare destination buffer
/// This functions is OBSOLETE!!! Use it for debugging purpose only
///
TString TScledAnimationBuilder::PrepareScled() {

    PrepareAllAnim_();

    // Convert segmentAnimation into scled format
    // Current SCLED format is:
    // 213 255 255 213 85  85  0   | 255 255 255 255 0   0   255 | 255 255 255 255 255 255 0   | 255 255 255 255 255 255 0   | 255 255 0   0   | 20
    TStringStream s;

    for (const auto it : AllAnimation_) {
        if (it.Duration > 0) {
            for (int i = 0; i < 28; i++) {
                s << RightPad((ui32)(it.Segments[i]), 3) << " ";
                if (i == 6 || i == 13 || i == 20) {
                    s << "| ";
                }
            }
            s << "| " << RightPad((ui32)(it.Segments[28]), 3) << " " << RightPad((ui32)(it.Segments[29]), 3) << " "
            << RightPad((ui32)(it.Segments[30]), 3) << " " << RightPad((ui32)(it.Segments[31]), 3);
            s << " | " << it.Duration << "\n";
        }
    }
    return s.Str();
}

///
/// Finalize animation and prepare destination buffer
/// This is new version with base64-encoded protocol (compatible with Mini-2 since 15 Sep 2021)
///
TString TScledAnimationBuilder::PrepareBinary() {

    PrepareAllAnim_();

    // Finally - convert segmentAnimation into scled-binary format
    // Current Binary format is:
    // Repeat <7 bytes> <7 bytes> <7 bytes> <7 bytes> PLUS1 PLUS2 MINUS DEGREE Duration
    // Repeat (1 byte) - 0 - don't loop, 1 - loop animation
    // <7 bytes> - brighness for all 7 segments in each diget
    // PLUS1 PLUS2 MINUS DEGREE (1 byte each) - brighness for additional segments
    // Duration (2 bytes) - duration in milliseconds
    TVector<ui8> binaryStream;

    for (const auto it : AllAnimation_) {
        if (it.Duration > 0) {
            binaryStream.push_back(0); // Repeat flag
            for (int i = 0; i < 28; i++) {
                binaryStream.push_back(it.Segments[i]);
            }
            binaryStream.push_back(it.Segments[28]);
            binaryStream.push_back(it.Segments[29]);
            binaryStream.push_back(it.Segments[30]);
            binaryStream.push_back(it.Segments[31]);

            // TODO [DD] This may be wrong on LE/BE platforms? 
            binaryStream.push_back(((it.Duration >> 8) & 0xFF));
            binaryStream.push_back((it.Duration & 0xFF));
        }
    }

    switch (CompressionType_) {
        case CompressionTypeNone:
            // don't need to convert binary stream
            // use buf "as is"
            return Base64Encode(TStringBuf(reinterpret_cast<const char*>(&binaryStream[0]), binaryStream.size()));
        case CompressionTypeGzip:
            // Use gzip to merge
            {
                TBuffer zipBuffer;
                TBufferOutput outStream(zipBuffer);
                TZLibCompress compressor(&outStream, ZLib::StreamType::GZip);

                compressor.Write(&binaryStream[0], binaryStream.size());
                compressor.Finish();

                return Base64Encode(TStringBuf(zipBuffer.data(), zipBuffer.size()));
            }
        default:
            // Undefined compression type
            Y_ENSURE(false);
            break;
    }
    return {};
}

///
/// Fill all segments using instructions stored in TAnimData structure
///
void TScledAnimationBuilder::FillAllSegments_(const TAnimData& a) {

    size_t startIndex = a.From / DefaultStep_;
    size_t endIndex = a.To / DefaultStep_;

    Y_ENSURE(startIndex >= 0 && startIndex <= AllAnimation_.size());
    Y_ENSURE(endIndex >= 0 && endIndex <= AllAnimation_.size());

    // Select animation speed
    std::function<float(float)> animationFunction;
    if (a.AnimMode & AnimModeSpeedSlowFast) {
        animationFunction = FuncSlowFast_;
    } else if (a.AnimMode & AnimModeSpeedFastSlow) {
        animationFunction = FuncFastSlow_;
    } else if (a.AnimMode & AnimModeSpeedSmooth) {
        animationFunction = FuncHermit_;
    } else { // if (a.AnimMode & AnimModeSpeedNormal) or value not set - set normal by default
        animationFunction = FuncNormal_;
    }

    if (a.AnimMode & AnimModeSolid) {
        FillSolid_(startIndex, endIndex, a.Bright2, a.FullPattern, a.DrawMode);
    } else if (a.AnimMode & AnimModeFade) {
        FillFade_(startIndex, endIndex, a.Bright1, a.Bright2, a.FullPattern, a.DrawMode, animationFunction);
    } else if ((a.AnimMode & AnimModeFromBottomLeft) == AnimModeFromBottomLeft) {
        FillAnimation_(startIndex, endIndex, a, HORIZONTAL_ANIMATION_LEFT_RIGHT, animationFunction);
        // Add multiplyer to the end of animation
        FillMultiply_(startIndex, endIndex);
    } else if (a.AnimMode & AnimModeFromLeft) {
        FillAnimation_(startIndex, endIndex, a, HORIZONTAL_ANIMATION_LEFT_RIGHT, animationFunction);
    } else if (a.AnimMode & AnimModeFromRight) {
        FillAnimation_(startIndex, endIndex, a, HORIZONTAL_ANIMATION_RIGHT_LEFT, animationFunction);
    } else if (a.AnimMode & AnimModeFromTop) {
        FillAnimation_(startIndex, endIndex, a, VERTICAL_ANIMATION_TOP_BOTTOM, animationFunction);
    } else if (a.AnimMode & AnimModeFromBottom) {
        FillAnimation_(startIndex, endIndex, a, VERTICAL_ANIMATION_BOTTOM_TOP, animationFunction);
    }
    return;
}

///
/// Set brightness value for all segments in the current row
/// @index - AllAnimation_ array index
/// @brightness - 0..255 LED value
/// @mask - ful 32 bit mask for all LEDS
/// @mode - need to clear zero bits (DrawModeReplace) or keep current values (DrawModeMerge)
///
void TScledAnimationBuilder::FillSegment_(size_t index, ui8 brightness, TFullPattern mask, EDrawMode mode) {

    //
    // Note that the current Segment LED format can have 32 LEDs only because TFullPattern is ui32
    // In case if you will need to support another Scled displays, you must refactor TFullPattern class
    //
    static_assert(sizeof(AllAnimation_[0].Segments) == 32);

    // Set brightness value for all bits set in maskCurrent;
    for (int i = 0; i < 32; i++) {
        if (mask & 1) {
            // Set brightness for an active segment
            if (mode == DrawModeMultiply) {
                // Use multiply effect
                AllAnimation_[index].Segments[i] = (static_cast<ui32>(AllAnimation_[index].Segments[i]) * brightness) / 256;
            } else {
                // Draw directly
                AllAnimation_[index].Segments[i] = brightness;
            }
        }
        else if (mode == DrawModeReplace) {
            // In case if we are using DrawModeReplace all other segments in this slot must be set to zero
            AllAnimation_[index].Segments[i] = 0;
        }
        mask >>= 1;
    }
    return;
}

///
/// Convert AllDirectives_ into AllAnimation_ array
///
void TScledAnimationBuilder::PrepareAllAnim_() {

    if (MaxTime_ == 0 || DefaultStep_ == 0) {
        // No animation data provided or wrong animation step value
        return;
    }
    ui32 stepCount = MaxTime_ / DefaultStep_ + 1;

    if (stepCount > MAX_ANIMATED_STEPS) {
        Y_ENSURE(false); // Too many step counts, this may cause poor perfrmance of event fails
        return;
    }

    // Resize destination draw vector and setup default step to play all animations
    AllAnimation_.resize(stepCount);
    for (auto& it : AllAnimation_) {
        it.Duration = DefaultStep_;
    }

    // Iterate with all stored animations and save data into the segmentAnimation array
    for (const auto it : AllDirectives_) {
        FillAllSegments_(it);
    }

    // Compacting data (merge all equal lines together)
    if (AllAnimation_.size() > 1 && CompactingLines_) {

        TVector<TSegmentLine>::iterator itMerged = AllAnimation_.begin();
        TVector<TSegmentLine>::iterator itCurrent = itMerged + 1;

        for (; itCurrent != AllAnimation_.end(); ++itCurrent) {
            if (*itCurrent == *itMerged) {
                // Lines are equal, add itCurrent duration to the previous line and clear it 
                // (line with zero duration will not be saved to output stream)
                itMerged->Duration += itCurrent->Duration;
                itCurrent->Duration = 0;
            } 
            else {
                // Not equal, just move itMerged forward 
                itMerged = itCurrent;
            } 
        }
    }
    return;
}

///
/// Fill patern with fixed brightness
///
void TScledAnimationBuilder::FillSolid_(size_t startIndex, size_t endIndex, ui8 brightness, TFullPattern p, EDrawMode drawMode) {

    for (auto i = startIndex; i <= endIndex; i++) {
        FillSegment_(i, brightness, p, drawMode);
    }
    return;
}

///
/// Fill patern with animated brightness from brightnessStart to brightnessEnd
///
void TScledAnimationBuilder::FillFade_(size_t startIndex, size_t endIndex, ui8 brightnessStart, ui8 brightnessEnd, TFullPattern p, EDrawMode drawMode, std::function<float(float)>& f) {

    for (auto i = startIndex; i <= endIndex; i++) {
        FillSegment_(i, Interpolate_<ui8>(startIndex, endIndex, i, brightnessStart, brightnessEnd, f), p, drawMode);
    }
    return;
}

///
/// Fill segments using soft animation from left to right or from top to bottom
///
void TScledAnimationBuilder::FillAnimation_(size_t startIndex, size_t endIndex, const TAnimData& a, const TVector<TFullPattern>& slides, std::function<float(float)>& f) {

    if (startIndex == endIndex) {
        // Not enough steps to save animation
        FillSegment_(startIndex, a.Bright2, a.FullPattern, a.DrawMode);
        return;
    }

    // For each mask in slides we add 3 animated groups: brightnessStart.....fade from 1 to 2....brightnessEnd
    int smooth = Max(SmoothFactor_, 1);
    size_t step = 0;
    size_t stepCount = slides.size()+smooth; // Use overlaped animation for all segments in slides
    // [ ....... slide1 ....... ]
    //           [ ....... slide2 ....... ]
    //                          [ ....... slide3 ....... ]
    // ...

    for (TVector<TFullPattern>::const_iterator it = slides.begin(); it != slides.end(); ++it) {

        size_t startFading = Interpolate_<size_t>(0, stepCount, step, startIndex, endIndex, f);
        size_t endFading = Interpolate_<size_t>(0, stepCount, step+smooth, startIndex, endIndex, f);

        FillSolid_(startIndex, startFading, a.Bright1, a.FullPattern & *it, DrawModeMerge);
        FillFade_(startFading, endFading, a.Bright1, a.Bright2, a.FullPattern & *it, DrawModeMerge, f);
        FillSolid_(endFading, endIndex, a.Bright2, a.FullPattern & *it, DrawModeMerge);
        ++step;
    }
    return;
}

///
/// Add muplitpilcation effect
/// This function works for AnimModeToRightUpper mode only!!!
///
void TScledAnimationBuilder::FillMultiply_(size_t startIndex, size_t endIndex) {

    static TFullPattern LOW_LINE = TScledAnimationBuilder::MakeFullPattern(VERTICAL_5, VERTICAL_5, TScledAnimationBuilder::SpecialNothing, VERTICAL_5, VERTICAL_5);
    static TFullPattern LOW_VERT = TScledAnimationBuilder::MakeFullPattern(VERTICAL_4, VERTICAL_4, TScledAnimationBuilder::SpecialPlusDown, VERTICAL_4, VERTICAL_4);
    static TFullPattern MID_LINE = TScledAnimationBuilder::MakeFullPattern(VERTICAL_3, VERTICAL_3, TScledAnimationBuilder::SpecialMinus, VERTICAL_3, VERTICAL_3);
    static TFullPattern HIGH_VERT = TScledAnimationBuilder::MakeFullPattern(VERTICAL_2, VERTICAL_2, TScledAnimationBuilder::SpecialPlusUp, VERTICAL_2, VERTICAL_2);

    size_t delta = endIndex - startIndex;
    std::function<float(float)> animationFunction = FuncNormal_;

    // Add multiplicative effect to the end of animation
    FillFade_(endIndex - 4 * delta / 5, endIndex, 255, 0, LOW_LINE, DrawModeMultiply, animationFunction);
    FillFade_(endIndex - 3 * delta / 5, endIndex, 255, 0, LOW_VERT, DrawModeMultiply, animationFunction);
    FillFade_(endIndex - 2 * delta / 5, endIndex, 255, 0, MID_LINE, DrawModeMultiply, animationFunction);
    FillFade_(endIndex - delta / 5, endIndex, 255, 0, HIGH_VERT, DrawModeMultiply, animationFunction);
    return;
}

} // namespace NAlice
