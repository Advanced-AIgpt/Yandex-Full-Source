(function (namespace) {
    'use strict';

    if (typeof namespace.ya === 'undefined') {
        namespace.ya = {};
    }
    if (typeof namespace.ya.speechkit === 'undefined') {
        namespace.ya.speechkit = {};
    }

    namespace.ya.speechkit.settings = {
        websocketProtocol: window.location.protocol.replace('http', 'ws') + '//',
        uniUrl: document.location.host
            + document.location.pathname.substring(0, document.location.pathname.lastIndexOf("/"))
            + '/uni.ws',
        asrUrl: document.location.host
            + document.location.pathname.substring(0, document.location.pathname.lastIndexOf("/"))
            + '/asrsocket.ws',
        ttsStreamUrl: document.location.host
            + document.location.pathname.substring(0, document.location.pathname.lastIndexOf("/"))
            + '/ttssocket.ws',
        apikey: '{{ apiKey }}',
        uuid: 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(
            /[xy]/g,
            function (c) {
                var r = Math.random() * 16 | 0;
                var v = c == 'x' ? r : (r & 0x3 | 0x8);
                return v.toString(16);
            }
        ),
        model: 'notes',
        lang: 'ru-RU',
        langWhitelist: ['ru-RU', 'en-US', 'tr-TR', 'uk-UA', 'ru', 'en', 'tr', 'de', 'uk', 'es', 'it', 'it-IT', 'en-GB', 'en-EN', 'es-ES', 'fr-FR', 'de-DE'],
    };

})(this);
