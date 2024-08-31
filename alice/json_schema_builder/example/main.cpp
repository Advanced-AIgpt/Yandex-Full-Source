#include <alice/json_schema_builder/div2/builders.h>
#include <alice/json_schema_builder/div2/validate.h>
#include <library/cpp/json/json_writer.h>
#include <util/stream/file.h>

using namespace NAlice::NJsonSchemaBuilder::NDiv2;
using namespace NAlice::NJsonSchemaBuilder::NRuntime;

// NOTE(a-square): May also return TBuilder
//
// When returning TBuilder, you can't modify the fields of the return value
// because it's sliced down to a simple JSON container, but you can still
// pass it to other builders' methods, which coincidentally expect a TBuilder
TDivActionMenuItemBuilder MyMenuItem(const TString& text,
                                                const TString& logId,
                                                const NJson::TJsonValue& payload) {
    return DivActionMenuItem(text, DivAction(logId).Payload(payload));
}

int main() {
    auto menuItems = TArrayBuilder()
        .Add(DivActionMenuItem("hello", DivAction("log1")));

    auto action = DivAction("log1")
        .Url("dialog-action://?foo=bar&bar=baz")
        .Payload(NJson::TJsonValue(NJson::JSON_MAP));

    action
        .MenuItems(std::move(menuItems))
        .AddMenuItem(MyMenuItem("foo", "log2", NJson::TJsonValue(NJson::JSON_MAP)))
        .AddMenuItem(MyMenuItem("bar", "log3", NJson::TJsonValue(NJson::JSON_MAP)));

    auto image = DivImage("https://example.com/kitty.jpg")
        .PlaceholderColor("#80000000")
        .Scale(EDivImageScale::Fit)
        .Action(action);

    const auto imageJson = Validate(std::move(image));

    NJson::WriteJson(&Cout, &imageJson, true /* formatOutput */, true /* sortKeys */);
    Cout << Endl;
}
