(function (namespace) {
    'use strict';

    if (typeof namespace.ya === 'undefined') {
        namespace.ya = {};
    }
    if (typeof namespace.ya.speechkit === 'undefined') {
        namespace.ya.speechkit = {};
    }

    function _makeWorker(script) {
        var URL = window.URL || window.webkitURL;
        var Blob = window.Blob;
        var Worker = window.Worker;

        if (!URL || !Blob || !Worker || !script) {
            return null;
        }

        var blob = new Blob([script], {type: 'application/javascript'});
        var worker = new Worker(URL.createObjectURL(blob));
        return worker;
    }

    var inline_worker =

    namespace.ya.speechkit.newWorker = function () {
        return _makeWorker(inline_worker);
    };
}(this));

