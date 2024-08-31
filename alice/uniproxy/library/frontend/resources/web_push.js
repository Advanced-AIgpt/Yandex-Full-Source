function base64UrlToUint8Array (base64UrlData) {
    var padding = '='.repeat((4 - base64UrlData.length % 4) % 4);
    var base64 = (base64UrlData + padding).replace(/\-/g, '+').replace(/_/g, '/');

    var rawData = window.atob(base64);
    var buffer = new Uint8Array(rawData.length);

    for (var i = 0; i < rawData.length; ++i) {
        buffer[i] = rawData.charCodeAt(i);
    }
    return buffer;
}

(function (namespace) {
    var _window = this;

    var _updatePermissionStatus = function (status) {
        var self = this;
        var text = 'Unknown';

        if (status === 'granted') {
            text = 'Permission granted';
        } else if (status === 'denied') {
            text = 'Permission denied';
        } else if (status === 'default') {
            text = 'Requires user permission';
        } else if (status === 'unsupported') {
            text = 'Unsupported';
        }

        console.log('Updated notification status: ' + text);
        document.getElementById("web_push_notification_status").value = text;
    };

    var _notificationPermission = function () {
        var permission;

        if (!window.Notification) {
            permission = 'unsupported';
        } else {
            permission = Notification.permission;
        }

        _updatePermissionStatus(permission);

        return permission;
    };

    var _requireNotificationPermission = function () {
        var self = this;

        return new Promise(function (resolve, reject) {
            if (Notification.permission === 'denied') {
                return reject(new Error('Push messages are blocked.'));
            }

            if (Notification.permission === 'granted') {
                return resolve();
            }

            if (Notification.permission === 'default') {
                Notification.requestPermission(function (result) {
                    _updatePermissionStatus(result);

                    if (result !== 'granted') {
                        reject(new Error('Bad permission result'));
                    }

                    resolve();
                });
            }
        });
    };

    var _updateServiceWorkerStatus = function (status) {
        console.log('Update service worker status:', status);
        document.getElementById("service_worker_status").value = status;
    };

    var _serviceWorkerStatus = function () {
        var self = this;

        if (!navigator.serviceWorker) {
            _updateServiceWorkerStatus('Not supported');
            throw new Error('browser not support service worker');
        }

        return navigator.serviceWorker.getRegistration('./').then(function (res) {
            if (res) {
                $('#web_push_update_sw_btn').prop('disabled', false);
            }
            _updateServiceWorkerStatus(res ? 'Installed' : 'Not installed');

            return res;
        }, function (err) {
            console.error(err);
            _updateServiceWorkerStatus('Something went wrong');

            throw err;
        });
    };

    var _registerServiceWorker = function () {
        var self = this;

        return navigator.serviceWorker.register('service_worker.js', {scope: './'}).then(function (res) {
            console.log('Service worker ' + (res ? 'installed' : 'not installed'));
            _updateServiceWorkerStatus(res ? 'Installed' : 'Not installed');
            var has_sw = !!res;
            // enable update service_worker button
            $('#web_push_update_sw_btn').prop('disabled', !has_sw);
            if (has_sw) {
                console.log('Add Updater service worker');
                document.getElementById("web_push_update_sw_btn").onclick = function () {
                    console.log('Update service worker');
                    res.update();
                };
            }
            return res;
        }, function (err) {
            console.error(err);
            _updateServiceWorkerStatus('Something went wrong');

            throw err;
        });
    };

    var _updatePushStatus = function (status, subscription) {
        document.getElementById("push_subscription_status").value = status;
        this._subscription = subscription;
        $('#web_push_send_subscription_btn').prop('disabled', !subscription);
        var subscribeButton = document.getElementById('web_push_subscribe_btn');
        subscribeButton.value = subscription ? 'Unsubscribe' : 'Subscribe';
        console.log('Update push subscription status:', status);
    };

    var _pushStatus = function () {
        var self = this;

        return _serviceWorkerStatus().then(function (reg) {
            if (reg) {
                if (reg.pushManager) {
                    return reg.pushManager.getSubscription().then(function (subscription) {
                        // console.log(subscription);
                        _updatePushStatus(subscription ? 'Subscribed' : 'Not subscribed', subscription);
                        return subscription;
                    }, function (err) {
                        _updatePushStatus('Something went wrong');
                        throw err;
                    });
                } else {
                    _updatePushStatus('Push not supported');
                }
            } else {
                _updatePushStatus('Service worker is not installed');
            }
        });
    };

    var _waitInstallingSeviceWorker = function (reg) {
        return new Promise(function (resolve) {
            if (reg.active) {
                resolve();
            } else {
                (reg.waiting || reg.installing).onstatechange = function () {
                    if (reg.active) {
                        resolve();
                    }
                };
            }
        });
    };

    var _subscribePush = function () {
        var self = this;

        return _registerServiceWorker().then(function (reg) {
            return _waitInstallingSeviceWorker(reg).then(function () {
                // enable button for update service worker
                $('#web_push_update_sw_btn').prop('disabled', false);
                // got vapid key and subscribe on push messages
                // (later store subscription for register on SUP)
                return fetch('/registrations/web/vapid'
                ).then(function(response) {
                    if (response.status == 200) {
                        return response.text();
                    } else {
                        throw new Error('fail got VAPID key - response.status=' + response.status.toString());
                    }
                }).then(function(vapidPublicKey) {
                    console.log('Got VAPID key: ', vapidPublicKey);
                    return reg.pushManager.subscribe({
                         userVisibleOnly: true,
                         applicationServerKey: base64UrlToUint8Array(vapidPublicKey)
                     });
                }).catch(function(ex) {
                    throw ex;
                });
            });
        }).then(function (subscription) {
            console.log(subscription, subscription.toJSON());
            console.log(JSON.stringify(subscription.toJSON()));
            _updatePushStatus('Subscribed', subscription);
            document.getElementById('web_push_subscribe_btn').value = 'Unsubscribe';
        }).catch(function (err) {
            console.log('failed subscribe to PUSH', err);
        });
    };

    var _yandexuid = function () {
        var parts = document.cookie.split('; ');

        for (var i = 0; i < parts.length; ++i) {
            var splitted = parts[i].split('=');

            if (splitted[0] === 'yandexuid') {
                return splitted[1];
            }
        }

        // if not found yandexuid search local(fake) unidemoiid
        for (var i = 0; i < parts.length; ++i) {
            var splitted = parts[i].split('=');

            if (splitted[0] === 'unidemoiid') {
                return splitted[1];
            }
        }

        // unidemoiid not found so create it
        var timeStampInMs = Math.floor(window.performance && window.performance.now && window.performance.timing
            && window.performance.timing.navigationStart ? window.performance.now() + window.performance.timing.navigationStart : Date.now());
        var iid = 'ud' + timeStampInMs.toString() + 'r' + Math.floor(Math.random() * 1000000000).toString();
        var expDate = new Date(new Date().getTime() + 90 * 24 * 60 * 60 * 1000); // 90 days
        document.cookie = "unidemoiid=" + iid + "; expires=" + expDate.toUTCString();

        return iid;
    };

    var _sendSubscription = function () {
        var self = this;
        var id = _yandexuid();

        console.log('run SUP registration (uu)id=', id);
        fetch('/registrations/web', {
            method: 'post',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                'id': id,
                'webDomain': 'yandex.ru',
                'subscription': _window._subscription.toJSON()
            })
        }).then(function(response) {
            console.log('SUP registration status_code=', response.status);
            if (response.status == 200) {
                document.getElementById("sup_registration_status").value = _yandexuid();
            }
            return response.text();
        }).then(function(text) {
            if (text) {
                console.log('SUP registration response text:', text)
            }
        }).catch(function(ex) {
            console.log('Failed', ex)
        })
    };

    document.getElementById('web_push_subscribe_btn').onclick = function () {
        var self = this;

        var button = document.getElementById('web_push_subscribe_btn');
        if (button.value === 'Subscribe') {
            console.log('Subscribe');

            _requireNotificationPermission().then(function () {
                _subscribePush();
            });
        } else if (button.value === 'Unsubscribe') {
            console.log('Unsubscribe');
            if (_window._subscription) {
                _window._subscription.unsubscribe().then(function (res) {
                    _updatePushStatus(res ? 'Unsubscribed' : 'Subscribed');
                }, function (err) {
                    console.log(err);
                });
            }
        }
    };

    document.getElementById('web_push_send_subscription_btn').onclick = _sendSubscription;

    document.getElementById('copy_sup_iid').onclick = function() {
        var copyText = document.getElementById('sup_registration_status');
        copyText.select();
        document.execCommand("Copy");
    }

    $('#web_push_notification_status').prop('disabled', true);
    $('#web_push_send_subscription_btn').prop('disabled', true);
    $('#web_push_update_sw_btn').prop('disabled', true);
    $('#service_worker_status').prop('disabled', true);
    $('#push_subscription_status').prop('disabled', true);
    // copy not work on disables input: $('#sup_registration_status').prop('disabled', true);

    // set statuses info
    _notificationPermission();
    _serviceWorkerStatus();
    _pushStatus();
}(this));
