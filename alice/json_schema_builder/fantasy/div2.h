#pragma once

#include "enums.h"

#include <alice/json_schema_builder/runtime/runtime.h>

// NOTE(a-square): we decided not to support templates
// in the actual library at this time, so the part of the runtime
// responsible for them is moved here for the time being
namespace NAlice::NJsonSchemaBuilder::NRuntime {

class TTemplate : public TBuilder {
public:
    TTemplate() {
        Value_.SetType(NJson::JSON_MAP);
    }

    TTemplate&& Type(const TString& type) && {
        Value_["type"] = type;
        return std::move(*this);
    }

    TTemplate&& Set(const TString& key, const TBuilder& value) && {
        Value_[key] = value.ValueWithoutValidation();
        return std::move(*this);
    }

    TTemplate&& Set(const TString& key, TBuilder&& value) && {
        Value_[key] = std::move(value).ValueWithoutValidation();
        return std::move(*this);
    }

    TTemplate&& Set(const TString& key, const NJson::TJsonValue& value) && {
        Value_[key] = value;
        return std::move(*this);
    }

    TTemplate&& Set(const TString& key, NJson::TJsonValue&& value) && {
        Value_[key] = std::move(value);
        return std::move(*this);
    }
};

inline TTemplate Template() {
    return TTemplate{};
}

inline TTemplate Template(const TString& type) {
    return TTemplate{}.Type(type);
}

class TTemplates : public NAlice::NJsonSchemaBuilder::NRuntime::TBuilder {
public:
    TTemplates() {
        Value_.SetType(NJson::JSON_MAP);
    }

    TTemplates& AddTemplate(const TString& name, const TBuilder& tmpl) & {
        Value_[name] = tmpl.ValueWithoutValidation();
        return *this;
    }

    TTemplates& AddTemplate(const TString& name, TBuilder&& tmpl) & {
        Value_[name] = std::move(tmpl).ValueWithoutValidation();
        return *this;
    }

    TTemplates&& AddTemplate(const TString& name, const TBuilder& tmpl) && {
        Value_[name] = tmpl.ValueWithoutValidation();
        return std::move(*this);
    }

    TTemplates&& AddTemplate(const TString& name, TBuilder&& tmpl) && {
        Value_[name] = std::move(tmpl).ValueWithoutValidation();
        return std::move(*this);
    }
};

inline TTemplates Templates() {
    return TTemplates{};
}

struct TValidCard {
    NJson::TJsonValue Templates; // templates actually used by the card
    NJson::TJsonValue Card;      // actual card
};

} // namespace NAlice::NJsonSchemaBuilder::NRuntime

namespace NAlice::NJsonSchemaBuilder::NFantasy {

class TDivImageBuilder : public NAlice::NJsonSchemaBuilder::NRuntime::TBuilder {
public:
    TDivImageBuilder() {
        Value_.SetType(NJson::JSON_MAP);
    }

    TDivImageBuilder&& Url(const TString& url) && {
        // TODO(a-square): validate
        // TODO(a-square): add l-value version
        Value_.EraseValue("$url");
        Value_["url"] = url;
        return std::move(*this);
    }

    TDivImageBuilder&& Url_Key(const TString& key) && {
        // TODO(a-square): validate
        // TODO(a-square): add l-value version
        Value_.EraseValue("url");
        Value_["$url"] = key;
        return std::move(*this);
    }

    TDivImageBuilder&& PlaceholderColor(const TString& color) && {
        // TODO(a-square): validate
        // TODO(a-square): add l-value version
        Value_.EraseValue("$placeholder_color");
        Value_["placeholder_color"] = color;
        return std::move(*this);
    }

    TDivImageBuilder&& PlaceholderColor_Key(const TString& key) && {
        // TODO(a-square): validate
        // TODO(a-square): add l-value version
        Value_.EraseValue("placeholder_color");
        Value_["$placeholder_color"] = key;
        return std::move(*this);
    }

    TDivImageBuilder&& Scale(const EDivImageScale scale) && {
        // TODO(a-square): validate
        // TODO(a-square): add l-value version
        Value_.EraseValue("$scale");
        Value_["scale"] = ToString(scale);
        return std::move(*this);
    }

    TDivImageBuilder&& Scale_Key(const TString& key) {
        // TODO(a-square): validate
        // TODO(a-square): add l-value version
        Value_.EraseValue("scale");
        Value_["$scale"] = key;
        return std::move(*this);
    }

    TDivImageBuilder&& Action(const NAlice::NJsonSchemaBuilder::NRuntime::TBuilder& action) && {
        Value_.EraseValue("$action");
        Value_["action"] = action.ValueWithoutValidation();
        return std::move(*this);
    }

    TDivImageBuilder&& Action(NAlice::NJsonSchemaBuilder::NRuntime::TBuilder&& action) && {
        Value_.EraseValue("$action");
        Value_["action"] = std::move(action).ValueWithoutValidation();
        return std::move(*this);
    }

    TDivImageBuilder&& Action_Key(const TString& key) && {
        Value_.EraseValue("action");
        Value_["$action"] = key;
        return std::move(*this);
    }
};

inline TDivImageBuilder DivImage() {
    return TDivImageBuilder{};
}

inline TDivImageBuilder DivImage(const TString& url) {
    return TDivImageBuilder{}.Url(url);
}

class TDivActionBuilder : public NAlice::NJsonSchemaBuilder::NRuntime::TBuilder {
public:
    TDivActionBuilder() {
        Value_.SetType(NJson::JSON_MAP);
    }

    TDivActionBuilder&& LogId(const TString& logId) && {
        Value_.EraseValue("$log_id");
        Value_["log_id"] = logId;
        return std::move(*this);
    }

    TDivActionBuilder&& LogId_Key(const TString& key) && {
        Value_.EraseValue("log_id");
        Value_["$log_id"] = key;
        return std::move(*this);
    }

    TDivActionBuilder&& Url(const TString& url) && {
        Value_.EraseValue("$url");
        Value_["url"] = url;
        return std::move(*this);
    }

    TDivActionBuilder&& Url_Key(const TString& key) && {
        Value_.EraseValue("url");
        Value_["$url"] = key;
        return std::move(*this);
    }

    TDivActionBuilder&& Payload(const NJson::TJsonValue& payload) && {
        Value_.EraseValue("$payload");
        Value_["payload"] = payload;
        return std::move(*this);
    }

    TDivActionBuilder&& Payload(NJson::TJsonValue&& payload) && {
        Value_.EraseValue("$payload");
        Value_["payload"] = std::move(payload);
        return std::move(*this);
    }

    TDivActionBuilder&& Payload_Key(const TString& key) && {
        Value_.EraseValue("payload");
        Value_["$payload"] = key;
        return std::move(*this);
    }

    TDivActionBuilder&& AddMenuItem(const NAlice::NJsonSchemaBuilder::NRuntime::TBuilder& builder) && {
        Value_.EraseValue("$menu_items");
        Value_["menu_items"].AppendValue(builder.ValueWithoutValidation());
        return std::move(*this);
    }

    TDivActionBuilder&& AddMenuItem(NAlice::NJsonSchemaBuilder::NRuntime::TBuilder&& builder) && {
        Value_.EraseValue("$menu_items");
        Value_["menu_items"].AppendValue(std::move(builder).ValueWithoutValidation());
        return std::move(*this);
    }

    TDivActionBuilder&& MenuItems(const NAlice::NJsonSchemaBuilder::NRuntime::TBuilder& builder) && {
        Value_.EraseValue("$menu_items");
        Value_["menu_items"] = builder.ValueWithoutValidation();
        return std::move(*this);
    }

    TDivActionBuilder&& MenuItems(NAlice::NJsonSchemaBuilder::NRuntime::TBuilder&& builder) && {
        Value_.EraseValue("$menu_items");
        Value_["menu_items"] = std::move(builder).ValueWithoutValidation();
        return std::move(*this);
    }

    TDivActionBuilder&& MenuItems_Key(const TString& key) && {
        Value_.EraseValue("menu_items");
        Value_["$menu_items"] = key;
        return std::move(*this);
    }

    TDivActionBuilder&& Referer(const TString& referer) && {
        Value_.EraseValue("$referer");
        Value_["referer"] = referer;
        return std::move(*this);
    }

    TDivActionBuilder&& Referer_Key(const TString& key) && {
        Value_.EraseValue("referer");
        Value_["$referer"] = key;
        return std::move(*this);
    }
};

inline TDivActionBuilder DivAction() {
    return TDivActionBuilder{};
}

inline TDivActionBuilder DivAction(const TString& logId) {
    return TDivActionBuilder{}.LogId(logId);
}

class TMenuItemBuilder : public NAlice::NJsonSchemaBuilder::NRuntime::TBuilder {
public:
    TMenuItemBuilder() {
        Value_.SetType(NJson::JSON_MAP);
    }

    TMenuItemBuilder&& Text(const TString& text) && {
        Value_.EraseValue("$text");
        Value_["text"] = text;
        return std::move(*this);
    }

    TMenuItemBuilder&& Text_Key(const TString& key) && {
        Value_.EraseValue("text");
        Value_["$text"] = key;
        return std::move(*this);
    }

    TMenuItemBuilder&& Action(const NAlice::NJsonSchemaBuilder::NRuntime::TBuilder& action) && {
        Value_.EraseValue("$action");
        Value_["action"] = action.ValueWithoutValidation();
        return std::move(*this);
    }

    TMenuItemBuilder&& Action(NAlice::NJsonSchemaBuilder::NRuntime::TBuilder&& action) && {
        Value_.EraseValue("$action");
        Value_["action"] = std::move(action).ValueWithoutValidation();
        return std::move(*this);
    }

    TMenuItemBuilder&& Action_Key(const TString& key) && {
        Value_.EraseValue("action");
        Value_["$action"] = key;
        return std::move(*this);
    }
};

inline TMenuItemBuilder MenuItem() {
    return TMenuItemBuilder{};
}

inline TMenuItemBuilder MenuItem(const TString& text, const NAlice::NJsonSchemaBuilder::NRuntime::TBuilder& action) {
    return TMenuItemBuilder{}.Text(text).Action(action);
}

inline TMenuItemBuilder MenuItem(const TString& text, NAlice::NJsonSchemaBuilder::NRuntime::TBuilder&& action) {
    return TMenuItemBuilder{}.Text(text).Action(std::move(action));
}

// templates are supposed to be reused, builder is not
NAlice::NJsonSchemaBuilder::NRuntime::TValidCard
Validate(const NAlice::NJsonSchemaBuilder::NRuntime::TTemplates& templates,
         NAlice::NJsonSchemaBuilder::NRuntime::TBuilder&& builder);

} // namespace NAlice::NJsonSchemaBuilder::NFantasy
