define(function(){
    // Special thanks to khoden@
    // https://bb.yandex-team.ru/projects/INFRACLOUDUI/repos/infra-buzzer-bundled/browse
    function load_infra_buzzer_code(d, w, e) {
        var b = '//infracloudui-cdn.s3.mds.yandex.net/infraBuzzer/latest';
        var p = d.getElementsByTagName('script')[0];

        var l = d.createElement('link');
        l.href = b + '/styles.css';
        l.rel = 'stylesheet';
        p.parentNode.insertBefore(l, p);

        var s = d.createElement('script');
        s.type = 'text/javascript';
        s.async = true;
        s.src = b + '/index_skip_define.js';
        s.onload = function () {
            w.dispatchEvent(new Event(e));
        };
        p.parentNode.insertBefore(s, p);
    }

    function attachInfraBuzzer() {
        window.removeEventListener('infraBuzzerLoaded', attachInfraBuzzer);

        return window.infraBuzzer.mount({
            container: document.getElementById('infra-buzzer'),
            subscribeTo: [
                // TODO: Give a way to customize this through Jupyter
                // config or some other config
                {serviceId: 931, environmentId: 1271},  // JupyterCloud Production
                {serviceId: 277, environmentId: 357},  // QYP Stable
                {serviceId: 6, environmentId: 13},  // YP Production
            ],
        });
    }

    function load_ipython_extension(){
        var element = (
            // notebook page
            document.getElementById('kernel_logo_widget') ||
            // tree page
            document.getElementById('shutdown_widget') ||
            // edit and console pages
            document.getElementById('login_widget')
        );

        var buzzler_span = document.createElement('span');
        buzzler_span.id = 'infra-buzzer';

        element.parentNode.insertBefore(buzzler_span, element);

        load_infra_buzzer_code(document, window, 'infraBuzzerLoaded');
        window.addEventListener('infraBuzzerLoaded', attachInfraBuzzer);
    }

    return {
        load_ipython_extension: load_ipython_extension
    };
});

