(function (namespace) {
    'use strict';

    if (typeof namespace.ya === 'undefined') {
        namespace.ya = {};
    }
    if (typeof namespace.ya.speechkit === 'undefined') {
        namespace.ya.speechkit = {};
    }

    namespace.ya.speechkit.SpeakerId = function () {
        if (!(this instanceof namespace.ya.speechkit.SpeakerId)) {
            return new namespace.ya.speechkit.SpeakerId();
        }

        if (!namespace.ya.speechkit._recorderInited) {
            namespace.ya.speechkit.initRecorder(
                this.onInited.bind(this),
                function (error) {alert('Failed to init recorder: ' + error);}
            );
        }
    };

    namespace.ya.speechkit.SpeakerId.prototype = {
        onInited: function () {
            this.recorder = new namespace.ya.speechkit.Recorder();
        },

        startRecord: function () {
            console.log('Start recording...');
            this.recorder.start(
                function (data) {
                    console.log('Recorder callback, recorded data length: ' + data.byteLength);
                },
                namespace.ya.speechkit.FORMAT.PCM8);
        },

        completeRecordAndRegister: function (userid, keepPrev, text, onRegister) {
            console.log('completeRecordAndRegister');
            this.recorder.stop(function (wav) {
                console.log('Wav is ready:');
                console.log(wav);
                var fd = new FormData();
                fd.append('name', userid);
                fd.append('text', text);
                fd.append('audio', wav);
                fd.append('keepPrev', keepPrev ? 'true' : 'false');

                var xhr = new XMLHttpRequest();

                xhr.open('POST', namespace.ya.speechkit.settings.voicelabUrl + 'register_voice');

                xhr.onreadystatechange = function () {
                    if (this.readyState == 4) {
                        if (this.status == 200) {
                            console.log(this.responseText);
                            onRegister(this.responseText);
                        } else {
                            onRegister('Failed to register data, could not access ' +
                               namespace.ya.speechkit.settings.voicelabUrl +
                               ' Check out developer tools -> console for more details.');
                        }
                    }
                };

                xhr.send(fd);

            });
        },

        completeRecordAndIdentify: function (onFoundUser) {
            console.log('Indentify');
            this.recorder.stop(function (wav) {
                console.log('Wav is ready:');
                console.log(wav);
                var fd = new FormData();
                fd.append('audio', wav);

                var xhr = new XMLHttpRequest();

                xhr.open('POST', namespace.ya.speechkit.settings.voicelabUrl + 'detect_voice');

                xhr.onreadystatechange = function () {
                    if (this.readyState == 4) {
                        if (this.status == 200) {
                            console.log(this.responseText);
                            var data = {};
                            try {
                                data = JSON.parse(this.responseText);
                            } catch (e) {
                                onFoundUser(false, 'Failed to find user, internal server error: ' + e);
                                return;
                            }
                            onFoundUser(true, data);
                        } else {
                            onFoundUser(false, 'Failed to find user, could not access ' +
                                namespace.ya.speechkit.settings.voicelabUrl +
                                ' Check out developer tools -> console for more details.');
                        }
                    }
                };

                xhr.send(fd);
            }, 1);
        },

        feedback: function (requestId, feedback) {
            console.log('Post feedback');
            var fd = new FormData();
            fd.append('requestId', requestId);
            fd.append('feedback', feedback);

            var xhr = new XMLHttpRequest();

            xhr.open('POST', namespace.ya.speechkit.settings.voicelabUrl + 'postFeedback');

            xhr.onreadystatechange = function () {
                if (this.readyState == 4) {
                    console.log(this.responseText);
                }
            };

            xhr.send(fd);
        },
    };
}(this));
