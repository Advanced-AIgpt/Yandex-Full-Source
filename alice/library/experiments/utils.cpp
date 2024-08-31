#include "utils.h"

namespace NAlice::NMegamind {

// IExpFlagVisitor ------------------------------------------------------------
void IExpFlagVisitor::Visit(const TExperimentsProto& proto) {
    const auto& experiments = proto.GetStorage();
    for (const auto& kv : experiments) {
        switch (kv.second.GetValueCase()) {
            case TExperimentsProto::TValue::ValueCase::VALUE_NOT_SET:
                (*this)(kv.first);
                break;

            case TExperimentsProto::TValue::ValueCase::kString:
                (*this)(kv.first, kv.second.GetString());
                break;

            case TExperimentsProto::TValue::ValueCase::kBoolean:
                (*this)(kv.first, kv.second.GetBoolean());
                break;

            case TExperimentsProto::TValue::ValueCase::kNumber:
                (*this)(kv.first, kv.second.GetNumber());
                break;

            case TExperimentsProto::TValue::ValueCase::kInteger:
                (*this)(kv.first, double(kv.second.GetInteger()));
                break;
        }
    }
}

// TExpFlagsToStructVisitor ---------------------------------------------------
TExpFlagsToStructVisitor::TExpFlagsToStructVisitor(TStorage& storage)
    : Storage_{storage}
{
}

void TExpFlagsToStructVisitor::operator()(const TString& key, bool value) {
    (*Storage_.mutable_fields())[key].set_bool_value(value);
}

void TExpFlagsToStructVisitor::operator()(const TString& key, double value) {
    (*Storage_.mutable_fields())[key].set_number_value(value);
}

void TExpFlagsToStructVisitor::operator()(const TString& key, const TString& value) {
    (*Storage_.mutable_fields())[key].set_string_value(value);
}

void TExpFlagsToStructVisitor::operator()(const TString& key) {
    (*Storage_.mutable_fields())[key].set_null_value({});
}

// TExpFlagsToJsonVisitor -----------------------------------------------------
TExpFlagsToJsonVisitor::TExpFlagsToJsonVisitor(NJson::TJsonValue& json)
    : Json_{json}
{
    Json_.SetType(NJson::EJsonValueType::JSON_MAP);
}

void TExpFlagsToJsonVisitor::operator()(const TString& key, bool value) {
    Json_[key] = value;
}

void TExpFlagsToJsonVisitor::operator()(const TString& key, double value) {
    Json_[key] = value;
}

void TExpFlagsToJsonVisitor::operator()(const TString& key, const TString& value) {
    Json_[key] = value;
}

void TExpFlagsToJsonVisitor::operator()(const TString& key) {
    Json_[key].SetType(NJson::EJsonValueType::JSON_NULL);
}

} // namespace NAlice::NMegamind
