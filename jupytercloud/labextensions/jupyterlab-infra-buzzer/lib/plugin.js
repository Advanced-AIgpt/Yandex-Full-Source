import '../style/plugin.css';
import "@yandex-infracloud-ui/infra-buzzer-bundled/infraBuzzer/styles.css";

import {Widget} from '@lumino/widgets';
import {IMainMenu} from '@jupyterlab/mainmenu';
import {ISettingRegistry} from '@jupyterlab/settingregistry';
import {mount} from '@yandex-infracloud-ui/infra-buzzer-bundled';


export default [{
    id: 'jupyterlab-infra-buzzer',
    autoStart: true,
    requires: [IMainMenu, ISettingRegistry],
    activate: async (app, mainMenu, settingRegistry) => {
        const spacer = new Widget();
        spacer.addClass('jc-top-panel-spacer');
        spacer.id = 'jc-top-panel-spacer';
        app.shell.add(spacer, 'top');

        const infra_buzzer_box = new Widget();
        infra_buzzer_box.id = 'infra-buzzer-box';
        infra_buzzer_box.addClass('infra-buzzer-box');
        app.shell.add(infra_buzzer_box, 'top');

        const style = document.createElement('link');
        style.rel = "stylesheet";
        style.href = "https://infracloudui-cdn.s3.mds.yandex.net/infraBuzzer/1.7.1/styles.css";

        Promise.all([
            settingRegistry.load('jupyterlab-infra-buzzer:plugin'),
            app.restored
        ]).then(([settings]) => {
            const subscribeTo = settings.get("subscribeTo").composite;

            const {ref, unmount} = mount({
                container: infra_buzzer_box.node,
                subscribeTo: subscribeTo,
                test: false,
            });

            // in normal case mount call inserts style to head;
            // but somewhy it sometimes does not happens;
            // so we inserting remote css from CDN with version,
            // equals to our dep version.
            infra_buzzer_box.node.appendChild(style);
        });
    }
}];
