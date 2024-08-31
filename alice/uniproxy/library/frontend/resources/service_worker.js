self.addEventListener('push', function (event) {
    console.log('On receive push');
    var data = {};
    if (event.data) {
        data = event.data.json();
    }
    var title = data.notification.title ? data.notification.title : "Push message";
    var options = {};
    if (data.notification.body) {
        options.body = data.notification.body;
    }
    if (data.notification.icon) {
        options.icon = data.notification.icon;
    }
    if (data.data) {
        options.data = data.data;
    }
    event.waitUntil(self.registration.showNotification(title, options));
});

self.addEventListener('notificationclick', function (event) {
    console.log('On notification click');
    event.notification.close();

    var clickResponsePromise;
    if (event.notification.data && event.notification.data.push_uri) {
        console.log('Open push uri');
        clickResponsePromise = clients.openWindow(event.notification.data.push_uri);
    } else {
        console.log('Ignore notificationclick');
        clickResponsePromise = Promise.resolve();
    }

    event.waitUntil(clickResponsePromise);
});
