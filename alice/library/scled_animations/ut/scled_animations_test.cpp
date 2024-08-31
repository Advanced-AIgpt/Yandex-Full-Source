#include <alice/library/scled_animations/scled_animations_builder.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/stream/str.h>
#include <util/stream/buffer.h>
#include <util/stream/format.h>
#include <util/stream/zlib.h>

#include <util/generic/buffer.h>

#include <utility>

namespace NAlice {

// Simplest example
constexpr static auto TRACK_EXAMPLE_SIMPLE = TStringBuf(
"0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 100\n"
"0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 100\n"
"0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 100\n");

// Example for custom animation "dice 23"
constexpr static auto TRACK_EXAMPLE_DICE23 = TStringBuf(
"255 0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 100\n"
"255 255 0   0   0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 20\n"
"0   255 0   0   0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 80\n"
"0   255 255 0   0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 20\n"
"0   0   255 0   0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 80\n"
"0   0   255 255 0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 20\n"
"0   0   0   255 0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 80\n"
"0   0   0   255 255 0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 20\n"
"0   0   0   0   255 0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 80\n"
"0   0   0   0   255 255 0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 20\n"
"0   0   0   0   0   255 0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 80\n"
"255 0   0   0   0   255 0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 20\n"
"255 0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 80\n"
"255 255 0   0   0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 20\n"
"0   255 0   0   0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 80\n"
"0   255 255 0   0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 20\n"
"0   0   255 0   0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 80\n"
"0   0   255 255 0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 20\n"
"0   0   0   255 0   0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 80\n"
"0   0   0   255 255 0   0   | 0   0   0   0   0   0   0   | 255 255 0   255 255 0   255 | 255 255 255 255 0   0   255 | 0   0   0   0   | 20\n"
"0   0   0   0   255 0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 80\n"
"0   0   0   0   255 255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 100\n");

// Example for standard aniation "Previous track"
constexpr static auto TRACK_EXAMPLE_PREVTRACK = TStringBuf(
"0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   255 255 0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 160\n"
"0   0   0   0   0   0   0   | 0   128 128 0   255 255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   120 120 0   255 255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   112 112 0   255 255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   104 104 0   255 255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   96  96  0   255 255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   88  88  0   255 255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   80  80  0   255 255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   72  72  0   255 255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   255 255 0   0   0   0   | 0   64  64  0   128 128 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   255 255 0   0   0   0   | 0   56  56  0   120 120 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   255 255 0   0   0   0   | 0   48  48  0   112 112 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   255 255 0   0   0   0   | 0   40  40  0   104 104 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   255 255 0   0   0   0   | 0   32  32  0   96  96  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   255 255 0   0   0   0   | 0   24  24  0   88  88  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   255 255 0   0   0   0   | 0   16  16  0   80  80  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   255 255 0   0   0   0   | 0   8   8   0   72  72  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   128 128 0   255 255 0   | 0   0   0   0   64  64  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   120 120 0   255 255 0   | 0   0   0   0   56  56  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   112 112 0   255 255 0   | 0   0   0   0   48  48  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   104 104 0   255 255 0   | 0   0   0   0   40  40  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   96  96  0   255 255 0   | 0   0   0   0   32  32  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   88  88  0   255 255 0   | 0   0   0   0   24  24  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   80  80  0   255 255 0   | 0   0   0   0   16  16  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   72  72  0   255 255 0   | 0   0   0   0   8   8   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   64  64  0   128 128 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   56  56  0   120 120 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   48  48  0   112 112 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   40  40  0   104 104 0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   32  32  0   96  96  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   24  24  0   88  88  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   16  16  0   80  80  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   8   8   0   72  72  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   64  64  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   56  56  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   48  48  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   40  40  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   32  32  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   24  24  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   16  16  0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   8   8   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n");

// Example for standard Binary animation (No compression) for 'Pause'
constexpr static auto TRACK_EXAMPLE_BIN_PAUSE_BASE64 = TStringBuf(
"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABQAAAAAAAAAAAASEgAAAAAAAAAAEhIAAAAAAAAAAAAAAAAAFAAAAAAAAAAAACQkAAAAAAAAAAAkJAAAAAAAAAAAAAAAAAAUAAAAAAAAAAAANzcAAAAAAAAAADc3AAAAAAAAAAAAAAAAABQAAAAAAAAAAA"
"BJSQAAAAAAAAAASUkAAAAAAAAAAAAAAAAAFAAAAAAAAAAAAFtbAAAAAAAAAABbWwAAAAAAAAAAAAAAAAAUAAAAAAAAAAAAbW0AAAAAAAAAAG1tAAAAAAAAAAAAAAAAABQAAAAAAAAAAACAgAAAAAAAAAAAgIAAAAAAAAAAAAAAAAAAFAAAAAAAAAAAAJKSAAAAAAAA"
"AACSkgAAAAAAAAAAAAAAAAAUAAAAAAAAAAAApKQAAAAAAAAAAKSkAAAAAAAAAAAAAAAAABQAAAAAAAAAAAC2tgAAAAAAAAAAtrYAAAAAAAAAAAAAAAAAFAAAAAAAAAAAAMjIAAAAAAAAAADIyAAAAAAAAAAAAAAAAAAUAAAAAAAAAAAA29sAAAAAAAAAANvbAAAAAA"
"AAAAAAAAAAABQAAAAAAAAAAADt7QAAAAAAAAAA7e0AAAAAAAAAAAAAAAAAFAAAAAAAAAAAAP//AAAAAAAAAAD//wAAAAAAAAAAAAAAAAa4AAAAAAAAAAAA6uoAAAAAAAAAAOrqAAAAAAAAAAAAAAAAABQAAAAAAAAAAADU1AAAAAAAAAAA1NQAAAAAAAAAAAAAAAAA"
"FAAAAAAAAAAAAL+/AAAAAAAAAAC/vwAAAAAAAAAAAAAAAAAUAAAAAAAAAAAAqqoAAAAAAAAAAKqqAAAAAAAAAAAAAAAAABQAAAAAAAAAAACVlQAAAAAAAAAAlZUAAAAAAAAAAAAAAAAAFAAAAAAAAAAAAH9/AAAAAAAAAAB/fwAAAAAAAAAAAAAAAAAUAAAAAAAAAA"
"AAamoAAAAAAAAAAGpqAAAAAAAAAAAAAAAAABQAAAAAAAAAAABVVQAAAAAAAAAAVVUAAAAAAAAAAAAAAAAAFAAAAAAAAAAAAEBAAAAAAAAAAABAQAAAAAAAAAAAAAAAAAAUAAAAAAAAAAAAKioAAAAAAAAAACoqAAAAAAAAAAAAAAAAABQAAAAAAAAAAAAVFQAAAAAA"
"AAAAFRUAAAAAAAAAAAAAAAAAFA==");

constexpr static auto TRACK_EXAMPLE_TEXT_PAUSE = TStringBuf(
"0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   18  18  0   0   0   0   | 0   0   0   0   18  18  0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   36  36  0   0   0   0   | 0   0   0   0   36  36  0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   55  55  0   0   0   0   | 0   0   0   0   55  55  0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   73  73  0   0   0   0   | 0   0   0   0   73  73  0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   91  91  0   0   0   0   | 0   0   0   0   91  91  0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   109 109 0   0   0   0   | 0   0   0   0   109 109 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   128 128 0   0   0   0   | 0   0   0   0   128 128 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   146 146 0   0   0   0   | 0   0   0   0   146 146 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   164 164 0   0   0   0   | 0   0   0   0   164 164 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   182 182 0   0   0   0   | 0   0   0   0   182 182 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   200 200 0   0   0   0   | 0   0   0   0   200 200 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   219 219 0   0   0   0   | 0   0   0   0   219 219 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   237 237 0   0   0   0   | 0   0   0   0   237 237 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   255 255 0   0   0   0   | 0   0   0   0   255 255 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 1720\n"
"0   0   0   0   0   0   0   | 0   234 234 0   0   0   0   | 0   0   0   0   234 234 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   212 212 0   0   0   0   | 0   0   0   0   212 212 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   191 191 0   0   0   0   | 0   0   0   0   191 191 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   170 170 0   0   0   0   | 0   0   0   0   170 170 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   149 149 0   0   0   0   | 0   0   0   0   149 149 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   127 127 0   0   0   0   | 0   0   0   0   127 127 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   106 106 0   0   0   0   | 0   0   0   0   106 106 0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   85  85  0   0   0   0   | 0   0   0   0   85  85  0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   64  64  0   0   0   0   | 0   0   0   0   64  64  0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   42  42  0   0   0   0   | 0   0   0   0   42  42  0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n"
"0   0   0   0   0   0   0   | 0   21  21  0   0   0   0   | 0   0   0   0   21  21  0   | 0   0   0   0   0   0   0   | 0   0   0   0   | 20\n");

Y_UNIT_TEST_SUITE(ScledAnimationBuilder) {

    Y_UNIT_TEST(BaseScledAnimationBuilderClass) {

        TScledAnimationBuilder a;
        UNIT_ASSERT_STRINGS_EQUAL(a.PrepareScled(), "");
    }

    Y_UNIT_TEST(SimpleScledAnimationBuilderPatterns1) {
        UNIT_ASSERT_EQUAL(TScledAnimationBuilder::Char2Pattern('A'), TScledAnimationBuilder::MakeDigit(true, true, true, false, true, true, true));
        UNIT_ASSERT_EQUAL(TScledAnimationBuilder::Char2Pattern(' '), TScledAnimationBuilder::MakeDigit(false, false, false, false, false, false, false));
        UNIT_ASSERT_EQUAL(TScledAnimationBuilder::String2Pattern("80:-1*"), TScledAnimationBuilder::String2Pattern("#1111111#1111110:#0000001#0110000*"));

        TScledAnimationBuilder::TPattern a1 = TScledAnimationBuilder::Char2Pattern('1');
        TScledAnimationBuilder::TPattern a2 = TScledAnimationBuilder::Char2Pattern('2');
        TScledAnimationBuilder::TPattern a3 = TScledAnimationBuilder::Char2Pattern('3');
        TScledAnimationBuilder::TPattern a4 = TScledAnimationBuilder::Char2Pattern('4');

        UNIT_ASSERT_EQUAL(TScledAnimationBuilder::MakeFullPattern(a1, a2, TScledAnimationBuilder::SpecialPlus, a3, a4), TScledAnimationBuilder::String2Pattern("12+34 "));
        UNIT_ASSERT_EQUAL(TScledAnimationBuilder::MakeFullPattern(a4, a3, TScledAnimationBuilder::SpecialMinus | TScledAnimationBuilder::SpecialDegree, a1, a2), TScledAnimationBuilder::String2Pattern("43-12*"));
    }

    Y_UNIT_TEST(SimpleScledAnimationBuilderClassSimple) {

        TScledAnimationBuilder a;

        a.SetCompactingLines(false);
        a.SetDefaultAnimationStep(100);

        a.AddDraw("      ", 255, 200);

        const auto str = a.PrepareScled();
        UNIT_ASSERT_STRINGS_EQUAL(str, TRACK_EXAMPLE_SIMPLE);
    }

    Y_UNIT_TEST(SimpleScledAnimationBuilderDice23) {
        TScledAnimationBuilder a;

        constexpr TScledAnimationBuilder::TPattern a1 = TScledAnimationBuilder::MakeDigit(true, false, false, false, false, false, false);
        constexpr TScledAnimationBuilder::TPattern a2 = TScledAnimationBuilder::MakeDigit(false, true, false, false, false, false, false);
        constexpr TScledAnimationBuilder::TPattern a3 = TScledAnimationBuilder::MakeDigit(false, false, true, false, false, false, false);
        constexpr TScledAnimationBuilder::TPattern a4 = TScledAnimationBuilder::MakeDigit(false, false, false, true, false, false, false);
        constexpr TScledAnimationBuilder::TPattern a5 = TScledAnimationBuilder::MakeDigit(false, false, false, false, true, false, false);
        constexpr TScledAnimationBuilder::TPattern a6 = TScledAnimationBuilder::MakeDigit(false, false, false, false, false, true, false);

        a.AddDraw("   23 ", 255, 1000);
        
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a1, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 0, 100);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a2, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 100, 200);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a3, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 200, 300);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a4, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 300, 400);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a5, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 400, 500);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a6, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 500, 600);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a1, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 600, 700);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a2, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 700, 800);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a3, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 800, 900);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a4, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 900, 1000);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a5, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 1000, 1100);
        a.SetDraw(TScledAnimationBuilder::MakeFullPattern(a6, 0, TScledAnimationBuilder::SpecialNothing, 0, 0), 255, 1100, 1200);

        const auto str = a.PrepareScled();
        UNIT_ASSERT_STRINGS_EQUAL(str, TRACK_EXAMPLE_DICE23);
    }

    Y_UNIT_TEST(SimpleScledAnimationBuilderClassPrevTrack) {

        TScledAnimationBuilder a;

        ui32 leftLine = TScledAnimationBuilder::MakeDigit(false, true, true, false, false, false, false);
        ui32 rightLine = TScledAnimationBuilder::MakeDigit(false, false, false, false, true, true, false);

        TScledAnimationBuilder::TFullPattern p11 = TScledAnimationBuilder::MakeFullPattern(leftLine, 0, TScledAnimationBuilder::SpecialNothing, 0, 0);
        TScledAnimationBuilder::TFullPattern p12 = TScledAnimationBuilder::MakeFullPattern(rightLine, 0, TScledAnimationBuilder::SpecialNothing, 0, 0);
        TScledAnimationBuilder::TFullPattern p21 = TScledAnimationBuilder::MakeFullPattern(0, leftLine, TScledAnimationBuilder::SpecialNothing, 0, 0);
        TScledAnimationBuilder::TFullPattern p22 = TScledAnimationBuilder::MakeFullPattern(0, rightLine, TScledAnimationBuilder::SpecialNothing, 0, 0);

        a.SetDraw(p11, 255, 340, 500);
        a.SetAnim(p11, 128, 0, 500, 820, TScledAnimationBuilder::AnimModeFade);
        a.SetDraw(p12, 255, 500, 640);
        a.SetAnim(p12, 128, 0, 660, 980, TScledAnimationBuilder::AnimModeFade);
        a.SetDraw(p21, 255, 20, 180);
        a.SetAnim(p21, 128, 0, 180, 500, TScledAnimationBuilder::AnimModeFade);
        a.SetDraw(p22, 255, 180, 340);
        a.SetAnim(p22, 128, 0, 340, 660, TScledAnimationBuilder::AnimModeFade);

        const auto res = a.PrepareScled();

        UNIT_ASSERT_STRINGS_EQUAL(res, TRACK_EXAMPLE_PREVTRACK);
    }

    Y_UNIT_TEST(SimpleScledAnimationBuilderClassPause) {

        TScledAnimationBuilder a;

        ui32 leftLine = TScledAnimationBuilder::MakeDigit(false, true, true, false, false, false, false);
        ui32 rightLine = TScledAnimationBuilder::MakeDigit(false, false, false, false, true, true, false);

        TScledAnimationBuilder::TFullPattern fp = TScledAnimationBuilder::MakeFullPattern(0, leftLine, TScledAnimationBuilder::SpecialNothing, rightLine, 0);

        a.AddAnim(fp, 0, 255, 280, TScledAnimationBuilder::EAnimMode::AnimModeFade);
        a.AddDraw(fp, 255, 1700);
        a.AddAnim(fp, 255, 21, 220, TScledAnimationBuilder::EAnimMode::AnimModeFade);

        //
        // Check a text format
        //
        const auto res1 = a.PrepareScled();

        UNIT_ASSERT_STRINGS_EQUAL(res1, TRACK_EXAMPLE_TEXT_PAUSE);

        //
        // Check a binary format
        //
        a.SetCompressionType(TScledAnimationBuilder::CompressionTypeNone);
        const TString res2 = a.PrepareBinary();
        UNIT_ASSERT_STRINGS_EQUAL(res2, TRACK_EXAMPLE_BIN_PAUSE_BASE64);

        //
        // Check a binary format with ZIP
        // This part of test uses functions from https://a.yandex-team.ru/arc/trunk/arcadia/yandex_io/libs/base/utils.cpp
        //
        a.SetCompressionType(TScledAnimationBuilder::CompressionTypeGzip);
        const TString res3 = a.PrepareBinary();

        auto decoded = Base64Decode(res3);

        TMemoryInput in(decoded.Data(), decoded.size());
        TZLibDecompress d(&in);

        TString res4;
        TStringOutput out(res4);
        TransferData(&d, &out);
        UNIT_ASSERT_STRINGS_EQUAL(Base64Encode(res4), res2);
    }

} // Y_UNIT_TEST_SUITE

} // namespace NAlice
