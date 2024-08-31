import {
    JupyterFrontEnd,
    JupyterFrontEndPlugin
} from '@jupyterlab/application';
import {
    ICommandPalette,
    MainAreaWidget,
    WidgetTracker
} from '@jupyterlab/apputils';
import { INotebookTracker, NotebookPanel } from '@jupyterlab/notebook';
import { LabIcon } from '@jupyterlab/ui-components';

import {
    isNotebookWriteable,
    NotebookButtonExtension,
    IJupyterCloud,
    IArcadiaShare,
    IYandexMetrika
} from 'jupytercloud-lab-lib';

import { NirvanaExportWidget } from './widget';
import nirvanaLogo from '../style/nirvana-logo.svg';

const EXTENSION_ID = 'jupytercloud-lab-nirvana';
const COMMAND_CLASS = 'nirvanaExport';
const LABEL = 'Export notebook to Nirvana';
const SHORT_LABEL = 'Export to Nirvana';
const CAPTION = LABEL;
const TOOLBAR_BUTTON_POSITON = 11;

const extension: JupyterFrontEndPlugin<void> = {
    id: EXTENSION_ID,
    autoStart: true,
    requires: [
        ICommandPalette,
        INotebookTracker,
        IJupyterCloud,
        IArcadiaShare,
        IYandexMetrika
    ],
    activate: (
        app: JupyterFrontEnd,
        palette: ICommandPalette,
        notebookTracker: INotebookTracker,
        jupyterCloud: IJupyterCloud,
        arcadiaShare: IArcadiaShare,
        yandexMetrika: IYandexMetrika
    ) => {
        const namespace = 'nirvana-export';
        const command = COMMAND_CLASS;
        if (!jupyterCloud.isHubAvailable(EXTENSION_ID)) {
            return;
        }

        const icon = new LabIcon({
            name: 'nirvana-export:nirvana-logo',
            svgstr: nirvanaLogo
        });
        const tracker = new WidgetTracker<MainAreaWidget<NirvanaExportWidget>>({
            namespace
        });

        const doExport = (notebookPanel: NotebookPanel) => {
            const id = `${namespace}-${notebookPanel.id}`;
            let main = tracker.find(widget => {
                return widget.content.id === id;
            });
            if (main) {
                app.shell.activateById(main.id);
                return;
            }

            const nirvanaWidget = new NirvanaExportWidget({
                id,
                notebookPanel,
                arcadiaShare,
                jupyterCloud,
                yandexMetrika,
                shell: app.shell
            });
            nirvanaWidget.title.icon = icon;
            nirvanaWidget.title.label = `Export ${
                notebookPanel.context.contentsModel.name
            }`;

            main = new MainAreaWidget({ content: nirvanaWidget });

            notebookPanel.disposed.connect((sender: any) => {
                main.close();
            });
            tracker.add(main);
            app.shell.add(main);

            yandexMetrika.reachGoal({ target: 'nirvana_export:enter' });
        };

        app.commands.addCommand(command, {
            label: LABEL,
            caption: CAPTION,
            icon: icon,
            isEnabled: () => isNotebookWriteable(notebookTracker.currentWidget),
            execute: args => doExport(notebookTracker.currentWidget)
        });

        jupyterCloud.menu.addItem({ command });
        palette.addItem({ command, category: 'Notebook Operations' });

        const exportButtonExtension = new NotebookButtonExtension({
            action: doExport,
            icon: icon,
            className: COMMAND_CLASS,
            tooltip: CAPTION,
            label: SHORT_LABEL,
            position: TOOLBAR_BUTTON_POSITON
        });

        app.docRegistry.addWidgetExtension('Notebook', exportButtonExtension);
    }
};

export default extension;
