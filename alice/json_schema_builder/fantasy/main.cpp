#include "div2.h"

#include <library/cpp/json/json_writer.h>
#include <util/stream/file.h>

using namespace NAlice::NJsonSchemaBuilder::NFantasy;
using namespace NAlice::NJsonSchemaBuilder::NRuntime;

// may also return TBuilder (in which case slicing will occur, leaving the object immutable)
TMenuItemBuilder MyMenuItem(const TString& text,
                            const TString& logId,
                            const NJson::TJsonValue& payload) {
    return MenuItem(text, DivAction(logId).Payload(payload));
}

int main() {
    auto templates = Templates()
        .AddTemplate("my_menu_item",
            MenuItem()
                .Text_Key("menu_text")
                .Action(DivAction()
                    .LogId_Key("action_log_id")
                    .Payload_Key("action_payload")
                    .Referer("https://ya.ru")));

    auto action = DivAction("log1")
        .Url("dialog-action://?foo=bar&bar=baz")
        .Payload(NJson::TJsonValue(NJson::JSON_ARRAY))
        .AddMenuItem(MyMenuItem("foo", "log2", NJson::TJsonValue(NJson::JSON_ARRAY)))
        .AddMenuItem(Template("my_menu_item")
            .Set("menu_text", "bar")
            .Set("action_log_id", "log3")
            .Set("action_payload", NJson::TJsonValue(NJson::JSON_ARRAY)));

    auto image = DivImage("https://example.com/kitty.jpg")
        .PlaceholderColor("#80000000")
        .Scale(EDivImageScale::Fit)
        .Action(action);

    const auto [templatesJson, cardJson] = Validate(templates, std::move(image));
    Cout << "Templates used:" << Endl;
    NJson::WriteJson(&Cout, &templatesJson, true /* formatOutput */, true /* sortKeys */);
    Cout << Endl << Endl;

    Cout << "Card:" << Endl;
    NJson::WriteJson(&Cout, &cardJson, true /* formatOutput */, true /* sortKeys */);
    Cout << Endl;
}
