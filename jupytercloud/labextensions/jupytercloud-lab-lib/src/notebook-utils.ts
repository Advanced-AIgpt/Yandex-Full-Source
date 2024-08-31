import { IDisposable, DisposableDelegate } from '@lumino/disposable';

import { ToolbarButton } from '@jupyterlab/apputils';
import { NotebookPanel, INotebookModel } from '@jupyterlab/notebook';
import { DocumentRegistry } from '@jupyterlab/docregistry';
import { LabIcon } from '@jupyterlab/ui-components';

export const isNotebookWriteable = (
    panel: NotebookPanel,
    context?: DocumentRegistry.IContext<INotebookModel>
): boolean => {
    context = context || (panel && panel.context);

    return Boolean(
        panel &&
            context &&
            context.contentsModel &&
            context.contentsModel.writable
    );
};

export interface IActionFunction {
    (panel: NotebookPanel): void;
}

export class NotebookButtonExtension
    implements
        DocumentRegistry.IWidgetExtension<NotebookPanel, INotebookModel> {
    constructor(
        public props: {
            action: IActionFunction;
            icon: LabIcon.ILabIcon;
            className: string;
            tooltip: string;
            label: string;
            position: number;
        }
    ) {}

    createNew(
        panel: NotebookPanel,
        context: DocumentRegistry.IContext<INotebookModel>
    ): IDisposable {
        let button: ToolbarButton = null;
        let isButtonDisposed = false;

        // Подгрузка файла происходит уже после создания панели.
        // Поэтому мы добавляем кнопку только при загрузке файла.
        context.fileChanged.connect((_, __) => {
            // TODO: Disconnect this signal instead of tracking isButtonDisposed or button
            if (isButtonDisposed || button) {
                return;
            }

            button = new ToolbarButton({
                className: this.props.className,
                icon: this.props.icon,
                onClick: () => this.props.action(panel),
                enabled: isNotebookWriteable(panel, context),
                tooltip: this.props.tooltip,
                label: this.props.label
            });

            panel.toolbar.insertItem(
                this.props.position,
                this.props.className,
                button
            );
        });

        return new DisposableDelegate(() => {
            if (button) {
                button.dispose();
            }
            isButtonDisposed = true;
        });
    }
}
