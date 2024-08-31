#include <alice/cachalot/library/main_class.h>

int TCachalotMainClass::operator()(int argc, const char** argv) {
    return NCachalot::NApplication::Run(argc, argv, GetDefaultConfigResorce());
}

TString TCachalotMainClass::GetModeName() const {
    return TStringBuilder() << "run-" << Installation;
}

TString TCachalotMainClass::GetDefaultConfigResorce() const {
    return TStringBuilder() << "/alice/cachalot/library/config/cachalot-" << Installation << ".json";
}

TString TCachalotMainClass::GetHelpString() const {
    return TStringBuilder() << "Run cachalot with config " << GetDefaultConfigResorce();
}
