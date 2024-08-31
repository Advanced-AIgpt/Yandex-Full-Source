window.AudioContext = window.AudioContext || window.webkitAudioContext;

(function ($) {
    $.widget(
    'yaspeechkit.analyser',
    {
        options: {
            recorder: null,
            alignResponse: null,
        },
        _create: function () {
            this.data = null;
            this.element.addClass('yaspeechkit-analyser')
            .css('text-align', 'center')
            .html('');
            this.element.append('<div class="table"><div class="line0">' +
            '<div class="cell" style="vertical-align: top" title="alt+p">' +
            '<div style="text-align: center; font-size: 40pt;">▶</div>' +
            '<div style="font-size: 9pt; text-align: center;">alt+p</div></div>' +
            '<div class="cell" style="text-align: center; width: 85%">' +
            '<canvas class="graf" style="width: 100%" width="1000" height="200"></canvas></div>' +
            '<div class="cell" style="width:5%"></div></div>' +
            '<div class="line0">' +
            '<div class="cell"></div><div class="cell"><div class="slider"></div></div>' +
            '<div class="cell"></div></div></div>')
            .append('<div class="status" style="text-align: center"></div>')
            .append('<center><div class="table" style="width: auto; padding-top: 20px;"></div></center>')
            .append('<audio></audio>');

            if (!navigator.cancelAnimationFrame) {
                navigator.cancelAnimationFrame = navigator.webkitCancelAnimationFrame ||
                                                 navigator.mozCancelAnimationFrame;
            }
            if (!navigator.requestAnimationFrame) {
                navigator.requestAnimationFrame = navigator.webkitRequestAnimationFrame ||
                                                  navigator.mozRequestAnimationFrame;
            }

            this.graf = this.element.find('canvas').get(0);
            this.playback = this.element.children('audio').get(0);

            if (this.options.recorder)
            {
                this.context = window.ya.speechkit.Recorder.prototype.getAudioContext.bind(this.options.recorder)();
            } else {
                this.context = window.ya.speechkit.audiocontext || new window.ya.speechkit.AudioContext();
            }

            var _this = this;
            this.fixedFrom = null;
            this.lastMouseX = null;
            this.moved = false;

            this.element.find('div.slider').slider({slide: function (event, ui) {
                    _this.fixedFrom = ui.value;
                    window.requestAnimationFrame(_this.drawAll.bind(_this));
                }
            });

            this.graf.onmousedown = function (e) {
                var x = e.pageX - _this.graf.offsetLeft - 50;
                _this.moved = false;
                if (x > 0) {
                    _this.lastMouseX = x;
                    if (_this.fixedFrom === null) {
                        var cur = Math.floor(_this.playback.currentTime * 100);
                        if (cur < 100) {
                            _this.fixedFrom = 0;
                        } else if (cur > _this.playback.duration * 100 - 200) {
                            _this.fixedFrom = Math.ceil(_this.playback.duration * 100) - 200;
                        } else {
                            _this.fixedFrom = cur - 100;
                        }
                    }
                }
            };

            this.graf.onmouseup = function (e) {
                _this.lastMouseX = null;
                if (_this.moved) {
                    return;
                }

                var x = e.pageX - _this.graf.offsetLeft - 50;
                var el = _this.graf.offsetParent;
                while (el !== null) {
                    x = parseInt(x) - parseInt(el.offsetLeft);
                    el = el.offsetParent;
                }

                if (x > 0) {
                    var width = _this.graf.width - 50;

                    var from = 0;
                    var to = 0;

                    if (_this.fixedFrom !== null) {
                        from = _this.fixedFrom;
                        to = Math.min(from + 200, Math.ceil(_this.playback.duration * 100));
                    } else {
                        from = Math.floor(_this.playback.currentTime * 100) - 100;
                        to = from + 200;
                        if (from < 0) {
                            to -= from;
                            from = 0;
                        }

                        if (to > _this.playback.duration * 100) {
                            from = Math.max(Math.ceil(_this.playback.duration * 100) - 200, 0);
                            to = Math.ceil(_this.playback.duration * 100);
                        }
                    }

                    if (isNaN(from) || isNaN(to) || from == to) {
                        return;
                    }

                    var stepx = 1.0 * width / (to - from);
                    _this.playback.currentTime = 0.01 * (from + (to - from) * x / (_this.graf.clientWidth - 50));
                    window.requestAnimationFrame(_this.drawAll.bind(_this));
                }
            };

            this.graf.onmouseout = function (e) {
                _this.lastMouseX = null;
            };

            this.graf.onmousemove = function (e) {
                if (!_this.lastMouseX) {
                    return;
                }

                var x = e.pageX - _this.graf.offsetLeft - 50;
                if (x > 0) {
                    var dx = x - _this.lastMouseX;
                    _this.lastMouseX = x;
                    _this.fixedFrom -= dx;
                    _this.moved = true;
                    _this.fixedFrom = Math.max(0, Math.min(_this.fixedFrom,
                                                    Math.ceil(_this.playback.duration * 100) - 200));
                    window.requestAnimationFrame(_this.drawAll.bind(_this));
                }
            };

            this.play = function () {
                if (_this.playback.paused || _this.playback.ended)
                {
                    _this.playback.play();
                    _this.element.find('div.cell:first').find(':first-child').html('▎▎');
                    _this.fixedFrom = null;
                    window.requestAnimationFrame(_this.drawAll.bind(_this));
                } else {
                    _this.playback.pause();
                    _this.element.find('div.cell:first').find(':first-child').html('▶');
                }
            };

            this.element.find('div.cell:first').get(0).onclick = this.play;
            $(document).unbind('keydown.analyser');
            $(document).bind('keydown.analyser', 'alt+p', this.play);

            this.timerId = setInterval(function () {
                _this.element.find('div.slider').slider('option', 'max',
                                    Math.max(0, Math.ceil(_this.playback.duration * 100) - 200));
                if (!_this.playback.ended && !_this.playback.paused && _this.playback.duration > 0) {
                    _this.element.find('div.cell:first').find(':first-child').html('▎▎');
                    window.requestAnimationFrame(_this.drawAll.bind(_this));
                } else {
                    _this.element.find('div.cell:first').find(':first-child').html('▶');
                }
            }, 50);
        },
        _destroy: function () {
            if (this.timerId) {
                clearInterval(this.timerId);
            }
            this.element.empty();
            this.element.removeClass('yaspeechkit-analyser');
        },
        _showCurrentWord: function () {
            this.element.find('div.table:last').html('');
            var i = 0;
            if (!this.options.alignResponse) {
                return;
            }
            {
                var words = this.options.alignResponse.words;
                var ali = this.options.alignResponse.loglikes ? this.options.alignResponse.loglikes.ali : null;
                var max = this.options.alignResponse.loglikes ? this.options.alignResponse.loglikes.max : null;
                var trans = {};

                if (typeof words === 'undefined')
                {
                    if (this.options.alignResponse.data) {
                        this.element.children('div.status').html(this.options.alignResponse.data);
                    } else {
                        this.element.children('div.status').html(this.options.alignResponse);
                    }
                    return;
                }

                if (ali && max) {
                    var max_diff = 0;
                    for (i = 0; i < Math.min(max.length, ali.length); i++) {
                        max_diff += max[i] - ali[i];
                    }
                    if (max_diff) {
                        max_diff /= i;
                    }

                    if (words && words.length) {
                        this.element.find('div.table:last').html('<div>Avg error: ' + max_diff + '</div>');
                    }
                }

                var max_trans = 0;
                if (trans && words) {
                    for (i = 0; i < words.length; i++) {
                        if (trans[words[i].word] &&
                            trans[words[i].word].length > max_trans) {
                            max_trans = trans[words[i].word].length;
                        }
                    }
                }

                if (words) {
                    for (i = 0; i < words.length; i++) {
                        var el = '<div class="line' + i % 2 + '"><div class="cell" style="padding: 5px 20px 5px 20px;">' +
                                                                                                words[i].word + '</div>';

                        var left_cells = max_trans;
                        if (trans && trans[words[i].word]) {
                            left_cells -= trans[words[i].word].length;
                            for (var j = 0; j < trans[words[i].word].length; j++) {
                                el += '<div class="cell" style="padding: 5px 20px 5px 20px;">' +
                                        trans[words[i].word][j] + '</div>';
                            }
                        }

                        for (var k = 0; k < left_cells; k++) {
                            el += '<div class="cell">&nbsp;</div>';
                        }

                        el += '</div>';
                        this.element.find('div.table:last').append(el);
                    }
                }

                if (!words || words.length === 0) {
                    this.element.children('div.status').html('');
                }
            }
        },
        drawAll: function () {
            var borderx = 50;
            var bordery = 50;
            var context = this.graf.getContext('2d');
            var width = this.graf.width - borderx;
            var height = this.graf.height - bordery;

            var from = 0;
            var to = 200;
            if (this.fixedFrom === null) {
                from = Math.floor(this.playback.currentTime * 100) - 100;
                to = from + 200;
                if (from < 0) {
                    to -= from;
                    from = 0;
                }

                if (to > this.playback.duration * 100) {
                    from = Math.max(Math.ceil(this.playback.duration * 100) - 200, 0);
                    to = Math.ceil(this.playback.duration * 100);
                }
            } else {
                from = this.fixedFrom;
                to = Math.min(from + 200, Math.ceil(this.playback.duration * 100));
            }

            if (isNaN(from) || isNaN(to) || from == to) {
                return;
            }

            this.element.find('div.slider').slider('option', 'value', from);

            var stepx = 1.0 * width / (to - from);

            var amp = height / 2;

            var sampleRate = this.context.sampleRate;
            var step = Math.floor((to - from) * 0.01 * sampleRate / width);
            var x = 0.0;
            var y = 0.0; // Used to draw

            context.clearRect(0, 0, width + borderx, height + bordery);
            context.fillStyle = 'gray';
            context.font = '15px Arial';

            for (var i = from; i < to; i++) {
                if (i % 100 === 0) {
                    context.fillRect(borderx + (i - from) * stepx, 1, 2, height);
                }

                if (i % 10 === 0) {
                    x = (i - from) * stepx + borderx;
                    context.fillText((i * 0.01).toFixed(2), x - 10, height + bordery);
                    context.fillRect(x, 0, 1, height);
                }
            }

            var offset = Math.ceil(from * 0.01 * sampleRate);
            context.fillStyle = 'black';
            context.fillRect(borderx, amp, width, 1);

            if (this.data) {
                context.beginPath();
                context.moveTo(borderx, amp);
                for (i = 0; i < width; i++) {
                    var min_t = 1.0;
                    var max_t = -1.0;
                    for (j = 0; j < step; j++) {
                        var datum = this.data[offset + i * step + j];
                        if (datum < min_t) {
                            min_t = datum;
                        }
                        if (datum > max_t) {
                            max_t = datum;
                        }
                    }
                    if (i >= (this.playback.currentTime * 100 - from) * stepx) {
                        context.fillStyle = 'black';
                    } else {
                        context.fillStyle = 'gray';
                    }
                    context.fillRect(borderx + i, (1 + min_t) * amp, 1, Math.max(1, (max_t - min_t) * amp));
                }
                context.stroke();
            }
            if (this.options.alignResponse) {
                context.lineWidth = 2;

                context.fillStyle = '#000000';
                var words = this.options.alignResponse.words;
                if (words) {
                    for (i = 0; i < words.length; i++)
                    {
                        if (words[i].from * 100 > from && words[i].from * 100 <= to)
                        {
                            context.fillText(words[i].word,
                            (words[i].from * 100 - from) * stepx + borderx, height + 30);
                        }
                    }
                }

                if (
                    this.maxH &&
                    this.minH != Infinity &&
                    this.maxH != Infinity &&
                    this.maxH != this.minH
                ) {

                    var stepy = 1.0 * height / (this.maxH - this.minH);

                    var ali = this.options.alignResponse.loglikes.ali;
                    if (ali) {
                        context.strokeStyle = '#FF0000';
                        context.beginPath();
                        context.moveTo(borderx, height - (ali[from] - this.minH) * stepy);
                        for (i = from; i < Math.min(to, ali.length); i++) {
                            x = (i - from) * stepx + borderx;
                            y = height - (ali[i] - this.minH) * stepy;
                            context.lineTo(x, y);
                            context.fillStyle = '#000000';
                            if (i == from ||
                                this.options.alignResponse.phones[i] != this.options.alignResponse.phones[i - 1]) {
                                context.fillText(this.options.alignResponse.phones[i], x, height + 15);
                            }
                        }
                        context.stroke();
                    }

                    var max = this.options.alignResponse.loglikes.max;
                    if (max) {
                        context.strokeStyle = '#00FF00';
                        context.beginPath();
                        context.moveTo(borderx, height - (max[from] - this.minH) * stepy);
                        for (i = from; i < Math.min(to, ali.length); i++) {
                            x = (i - from) * stepx + borderx;
                            y = height - (max[i] - this.minH) * stepy;
                            context.lineTo(x, y);
                        }

                        context.stroke();
                    }

                    if (ali || max) {
                        var dy = 0.2 * height;
                        var dyv = 0.2 * (this.maxH - this.minH);

                        for (i = 0; i < 5; i++) {
                            context.fillStyle = '#CCCCCC';

                            context.fillRect(borderx, height - i * dy, width + borderx, 1);
                            context.fillText((i * dyv + this.minH).toFixed(2), 5, height - i * dy);
                        }
                    }
                }
            }

            context.fillStyle = '#000000';
            x = (this.playback.currentTime * 100 - from) * stepx + borderx;
            context.fillRect(x - 1, 0, 2, height);
        },
        _setOption: function (key, value) {
            if (key == 'alignResponse') {
                try {
                    this.element.children('div.status').html('');
                    if (!value.loglikes && !value.words) {
                        value = JSON.parse(value);
                    }

                    if (value.loglikes) {
                        this.maxH = Math.max(
                                    Math.max.apply(null, value.loglikes.ali),
                                    /*Math.max.apply(null, value['loglikes']['avg']),*/
                                    Math.max.apply(null, value.loglikes.max)
                                    );
                        this.minH = Math.min(
                                    Math.min.apply(null, value.loglikes.ali),
                                    /*Math.min.apply(null, value['loglikes']['avg']),*/
                                    Math.min.apply(null, value.loglikes.max)
                                    );
                    } else {
                        this.maxH = null;
                        this.minH = null;
                    }
                } catch (e) {
                    if (value) {
                        this.element.children('div.status').html(e + ' ' + value);
                    }
                    value = null;
                }
                this._super(key, value);
                this.drawAll();
                this._showCurrentWord();
            } else if (key == 'recorder') {
                this.context = window.ya.speechkit.Recorder.prototype.getAudioContext.bind(this.options.recorder)();
            } else if (key == 'audio') {
                var _this = this;
                if (typeof value == 'string') {
                    var request = new XMLHttpRequest();
                    request.open('GET', value, true);
                    request.responseType = 'arraybuffer';

                    request.onload = function () {
                        var audioData = request.response;
                        _this.playback.src = window.URL.createObjectURL(new Blob([audioData]));
                        _this.context.decodeAudioData(audioData, function (buffer) {
                            _this.data = buffer.getChannelData(0);
                            window.requestAnimationFrame(_this.drawAll.bind(_this));
                        });
                    };
                    request.send();
                } else if (typeof value == 'object') {
                    var fileReader = new FileReader();
                    fileReader.onload = function () {
                        var audioData = this.result;
                        _this.playback.src = window.URL.createObjectURL(new Blob([audioData]));
                        _this.context.decodeAudioData(audioData, function (buffer) {
                            _this.data = buffer.getChannelData(0);
                            window.requestAnimationFrame(_this.drawAll.bind(_this));
                        });
                    };
                    try {
                        fileReader.readAsArrayBuffer(value);
                    } catch (e) {
                        console.log(e);
                    }
                }
            } else {
                this._super(key, value);
            }
        }
    });
}(jQuery));
