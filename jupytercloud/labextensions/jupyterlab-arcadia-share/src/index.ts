import {
    JupyterFrontEnd,
    JupyterFrontEndPlugin
} from '@jupyterlab/application';
import { ICommandPalette } from '@jupyterlab/apputils';
import { INotebookTracker, NotebookPanel } from '@jupyterlab/notebook';
import { fileUploadIcon } from '@jupyterlab/ui-components';

import { doShare } from './share';
import {
    isNotebookWriteable,
    NotebookButtonExtension,
    IJupyterCloud,
    IArcadiaShare,
    IYandexMetrika,
    IShare
} from 'jupytercloud-lab-lib';

const EXTENSION_ID = 'jupyterlab-arcadia-share';
const COMMAND_CLASS = 'arcadiaShare';
const LABEL = 'Save and share to Arcadia';
const SHORT_LABEL = 'Save and share';
const DESCRIPTION = 'Save notebook and share to Arcadia';
const TOOLBAR_BUTTON_POSITON = 10;

const extension: JupyterFrontEndPlugin<IArcadiaShare> = {
    id: EXTENSION_ID,
    autoStart: true,
    requires: [
        ICommandPalette,
        INotebookTracker,
        JupyterFrontEnd.IPaths,
        IJupyterCloud,
        IYandexMetrika
    ],
    provides: IArcadiaShare,
    activate: (
        app: JupyterFrontEnd,
        palette: ICommandPalette,
        notebookTracker: INotebookTracker,
        paths: JupyterFrontEnd.IPaths,
        jupyterCloud: IJupyterCloud,
        yandexMetrika: IYandexMetrika
    ): IArcadiaShare => {
        const command = COMMAND_CLASS;
        if (!jupyterCloud.isHubAvailable(EXTENSION_ID)) {
            return;
        }

        app.commands.addCommand(command, {
            label: LABEL,
            caption: DESCRIPTION,
            icon: fileUploadIcon,
            isEnabled: () => isNotebookWriteable(notebookTracker.currentWidget),
            execute: args =>
                doShare({
                    panel: notebookTracker.currentWidget,
                    paths,
                    yandexMetrika
                })
        });

        jupyterCloud.menu.addItem({ command });
        palette.addItem({
            command,
            category: 'Notebook Operations'
        });

        const shareButtonExtension = new NotebookButtonExtension({
            action: (panel: NotebookPanel) =>
                doShare({ panel, paths, yandexMetrika }),
            icon: fileUploadIcon,
            className: COMMAND_CLASS,
            tooltip: DESCRIPTION,
            label: SHORT_LABEL,
            position: TOOLBAR_BUTTON_POSITON
        });
        app.docRegistry.addWidgetExtension('Notebook', shareButtonExtension);

        return {
            share: ({ panel, showSuccessDialog = true }: IShare) =>
                doShare({ panel, paths, showSuccessDialog, yandexMetrika })
        };
    }
};

export default extension;
