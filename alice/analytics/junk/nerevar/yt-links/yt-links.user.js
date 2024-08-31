// ==UserScript==
// @name         YT Links
// @namespace    https://wiki.yandex-team.ru/users/nerevar/analytics/yt-links/
// @version      0.2
// @description  Make YT links clickable again!
// @author       nerevar
// @include      *yt.yandex-team.ru/*
// @run-at       context-menu
// @grant        none
// ==/UserScript==

(function() {
    'use strict';

    var YT_STRING_SELECTOR = '.data-table__row .data-table__td .unipika [class="string"]:only-child';

    function isImage(url) {
        return url.match(/(?:\.(?:jpeg|jpg|gif|png)$|avatars.mds.yandex.net|get-mimcache)/) != null
    }
    function isAudio(url) {
        return url.match(/(?:\.(?:opus|wav|mp3|ogg|aac)$|get-voicetoloka|speechbase-yt.voicetech.yandex.net|speechbase.voicetech.yandex-team.ru)/) != null
    }

    function run() {
        document.querySelectorAll(YT_STRING_SELECTOR).forEach(function(item) {
            if (!item.childNodes || item.childNodes.length !== 3 || item.childNodes[1].nodeType !== 3) {
                return
            }
            var url = item.childNodes[1].textContent;
            if (!url.startsWith('http://') && !url.startsWith('https://')) {
                return;
            }

            item.innerHTML = '<a href="' + url + '" target="_blank">' + url + '</a>'
            if (isAudio(url)) {
                item.parentNode.insertAdjacentHTML('afterend', '<audio src="' + url + '" controls="" style="width: 300px;"></audio>')
            } else if (isImage(url)) {
                item.parentNode.insertAdjacentHTML('afterend', '<img src="' + url + '" style="max-width: 200px;" />')
            }
        });
    }
})();

GM_registerMenuCommand('YT Links → clickable', function() {
    run()
});
