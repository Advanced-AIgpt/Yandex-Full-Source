import { JupyterFrontEnd, JupyterFrontEndPlugin } from '@jupyterlab/application';
import { INotebookTracker, NotebookPanel } from '@jupyterlab/notebook';
import { URLExt } from '@jupyterlab/coreutils';
import { ServerConnection } from '@jupyterlab/services';
import { trustedIcon } from '@jupyterlab/ui-components';
import {
    ICommandPalette,
} from '@jupyterlab/apputils';

import { manageSecrets } from './jupytercloud-lab-vault';
import {
    isNotebookWriteable,
    NotebookButtonExtension,
    IYandexMetrika,
    IJupyterCloud
} from 'jupytercloud-lab-lib';

const EXTENSION_ID = 'jupytercloud-lab-vault';
const COMMAND_CLASS = 'vaultManage';
const SHORT_LABEL = 'Manage secrets';
const DESCRIPTION = 'Manage secrets from Yandex Vault';
const TOOLBAR_BUTTON_POSITON = 9;


const checkServerExtension = () => {
    const settings = ServerConnection.makeSettings();
    const requestUrl = URLExt.join(
        settings.baseUrl,
        'vault'
    );

    return fetch(new Request(
        requestUrl,
        { method: "GET" }
    ));
}

/**
 * Initialization data for the jupytercloud-lab-vault extension.
 */
const extension: JupyterFrontEndPlugin<void> = {
    id: EXTENSION_ID,
    autoStart: true,
    requires: [
        INotebookTracker,
        ICommandPalette
    ],
    optional: [
        IYandexMetrika,
        IJupyterCloud
    ],
    activate: (
        app: JupyterFrontEnd,
        notebookTracker: INotebookTracker,
        palette: ICommandPalette,
        yandexMetrika: IYandexMetrika,
        jupyterCloud: IJupyterCloud
    ) => {
        checkServerExtension().then((response) => {
            if (response.ok) {
                console.log("Starting Yandex Vault integration labextension");
                const command = COMMAND_CLASS;

                const shareButtonExtension = new NotebookButtonExtension({
                    action: (panel: NotebookPanel) => manageSecrets(notebookTracker, yandexMetrika),
                    icon: trustedIcon,
                    className: command,
                    tooltip: DESCRIPTION,
                    label: SHORT_LABEL,
                    position: TOOLBAR_BUTTON_POSITON
                });
                app.docRegistry.addWidgetExtension('Notebook', shareButtonExtension);

                app.commands.addCommand(command, {
                    label: DESCRIPTION,
                    caption: DESCRIPTION,
                    isEnabled: () => isNotebookWriteable(notebookTracker.currentWidget),
                    execute: args => manageSecrets(notebookTracker, yandexMetrika)
                });

                if (jupyterCloud !== null) {
                    jupyterCloud.menu.addItem({ command });
                }
                palette.addItem({ command, category: 'Notebook Operations' });
            } else {
                console.error("Yandex Vault serverextension not running!");
            }
        });
    }
};

export default extension;
