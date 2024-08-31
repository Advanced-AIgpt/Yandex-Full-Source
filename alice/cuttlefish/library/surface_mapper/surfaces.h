#pragma once

namespace NVoice {

enum class ESurface {
    Unknown /* "" */,
    Auto /* "auto" */,
    MtsMusic /* "mts.music" */,

    kinopoisk_tv,
    keyboard,
    yphone,
    market,
    browser,
    weather,
    notanotherone,
    translate,
    mail,
    tts,
    cloud,
    taxi,
    demoapp,
    audiosender,
    searchbar,
    dusiassistant,
    pp,
    widget,
    kinopoisk,
    navigator,
    launcher,
    uber,
    quasar,
    tv,
    music,
    maps,
    test,
    reactivephone,
    aliced,
    elariwatch,
    standalone_app,
    chats,
    other_mobiles,
    sputnik,
    telemost,

    WebHandler,  // special case for legacy clients
};

}  // namespace NVoice
