#pragma once

#include "client_directive_model.h"

namespace NAlice::NMegamind {

enum class ESettingsTarget {
    Accessibility /* "accessibility" */,
    Archiving /* "archiving" */,
    Colors /* "colors" */,
    Datetime /* "datetime" */,
    DefaultApps /* "defaultapps" */,
    Defender /* "defender" */,
    Desktop /* "desktop" */,
    Display /* "display" */,
    Firewall /* "firewall" */,
    Folders /* "folders" */,
    HomeGroup /* "homegroup" */,
    Indexing /* "indexing" */,
    Keyboard /* "keyboard" */,
    Language /* "language" */,
    LockScreen /* "lockscreen" */,
    Microphone /* "microphone" */,
    Mouse /* "mouse" */,
    Network /* "network" */,
    Notifications /* "notifications" */,
    Power /* "power" */,
    Print /* "print" */,
    Privacy /* "privacy" */,
    Sound /* "sound" */,
    StartMenu /* "startmenu" */,
    System /* "system" */,
    TabletMode /* "tabletmode" */,
    Tablo /* "tablo" */,
    Themes /* "themes" */,
    UserAccount /* "useraccount" */,
    Volume /* "volume" */,
    Vpn /* "vpn" */,
    WiFi /* "wifi" */,
    WinUpdate /* "winupdate" */,
    AddRemove /* "addremove" */,
    TaskManager /* "taskmanager" */,
    DeviceManager /* "devicemanager" */,
};

class TOpenSettingsDirectiveModel final : public TClientDirectiveModel {
public:
    TOpenSettingsDirectiveModel(const TString& analyticsType, ESettingsTarget target);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] ESettingsTarget GetTarget() const;

private:
    const ESettingsTarget Target;
};

} // namespace NAlice::NMegamind
