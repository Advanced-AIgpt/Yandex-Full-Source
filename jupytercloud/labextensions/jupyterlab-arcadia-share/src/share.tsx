import 'bootstrap/dist/css/bootstrap.min.css';

import React, { Fragment } from 'react';

import { Alert, Spinner } from 'react-bootstrap';

import { NotebookPanel } from '@jupyterlab/notebook';
import { JupyterFrontEnd } from '@jupyterlab/application';
import { showDialog, Dialog } from '@jupyterlab/apputils';
import {
    appendShareToHistory,
    OpenCopyPath,
    IYandexMetrika
} from 'jupytercloud-lab-lib';

import {
    IResponseResult,
    IPreUploadInfo,
    getUploadUrl,
    getResponse
} from './utils';
import { CommitFormDialog } from './sharedialog';

interface IDoShare {
    panel: NotebookPanel;
    paths: JupyterFrontEnd.IPaths;
    yandexMetrika: IYandexMetrika;
    showSuccessDialog?: boolean;
}

interface IShareResult {
    message: string;
    link: string;
    path: string;
    revision: string;
    jupyticket_id: number;
}

const BUTTON_RESULT_CLASS = 'jcas-result-button';

const showPlaceholderDialog = (text: string) => {
    const dialog = new Dialog({
        title: (
            <h4>
                {text}{' '}
                <Spinner animation="grow" role="status">
                    <span className="sr-only">Loading...</span>
                </Spinner>
            </h4>
        ),
        buttons: [Dialog.cancelButton()]
    });

    const cancelPromise = () =>
        dialog.launch().then(() => Promise.reject({ state: 'cancelled' }));
    const closeCallback = () => dialog.reject();

    return [cancelPromise, closeCallback];
};

const showErrorDialog = (
    title: Dialog.Header,
    body: Dialog.Body<any>,
    level: 'danger' | 'warning'
) => {
    const jcLevel = `jcas-${level}`;
    return showDialog({
        title: <h4>{title}</h4>,
        body: <Alert variant={jcLevel}>{body}</Alert>,
        buttons: [Dialog.warnButton({ label: 'Dismiss' })]
    });
};

const showUploadErrorDialog = (
    uploadUrl: string,
    data: IResponseResult<any>
) => {
    const body = (
        <Fragment>
            <p>JupyterHub request to {uploadUrl} failed with error:</p>
            <pre>
                <code>{data.errorText}</code>
            </pre>
        </Fragment>
    );

    const level = data.status === 500 ? 'danger' : 'warning';

    return showErrorDialog(data.errorTitle, body, level);
};

const showImpossibleDialog = (
    uploadUrl: string,
    data: IResponseResult<any>
) => {
    const title = "Can't upload notebook to Arcadia due to:";
    let body = (data.json && data.json.message) || data.text;
    if (data.json && data.json.exists) {
        body = (
            <Fragment>
                <p>{body}</p>
                <hr />
                <p className="mb-0">
                    <Alert.Link href={data.json.link} target="_blank">
                        Link
                    </Alert.Link>
                </p>
            </Fragment>
        );
    }

    return showErrorDialog(title, body, 'warning');
};

const showSuccessDialog_ = (shareResult: IShareResult) => {
    const dialog = new Dialog({
        title: 'Notebook successfully uploaded',
        body: (
            <div className="jcas-open-copy-box">
                <OpenCopyPath path={shareResult.link} />
            </div>
        ),
        buttons: [Dialog.okButton({ className: BUTTON_RESULT_CLASS })]
    });

    return dialog.launch();
};

export const doShare = ({
    panel,
    paths,
    yandexMetrika,
    showSuccessDialog = true
}: IDoShare) => {
    yandexMetrika.reachGoal({ target: 'arcadia_share:enter' });

    if (panel.context.model.readOnly || !panel.context.contentsModel.writable) {
        void showErrorDialog('Cannot Save', 'Document is read-only', 'warning');
        return;
    }
    void panel.context.save().then(() => {
        if (!panel.isDisposed) {
            return panel.context.createCheckpoint();
        }
    });

    const pathInfo = {
        userName: paths.urls.hubUser,
        hubPrefix: paths.urls.hubPrefix,
        notebookPath: panel.context.path
    };

    const uploadUrl = getUploadUrl(pathInfo);

    const [cancelVerify, closeVerify] = showPlaceholderDialog(
        'Verifying upload possibility...'
    );
    const [cancelUpload, closeUpload] = showPlaceholderDialog(
        'Uploading notebook to Arcadia...'
    );
    const request = new Request(uploadUrl, { method: 'GET' });

    return Promise.race([cancelVerify(), getResponse(request)])
        .then(data => {
            closeVerify();
            data = data as IResponseResult<IPreUploadInfo>;

            if (data.status !== 200) {
                void showUploadErrorDialog(uploadUrl, data);
                return Promise.reject(
                    'non-200 response while check upload possibility'
                );
            } else if (!data.json || !data.json.result) {
                void showImpossibleDialog(uploadUrl, data);
                return Promise.reject(
                    `impossible to upload due to: ${data.text}`
                );
            }

            return CommitFormDialog.show({
                data: data.json,
                pathInfo
            });
        })
        .then(clickResult => {
            if (!clickResult.button.accept) {
                return Promise.reject({ state: 'cancelled' });
            }
            const commitFormResult = clickResult.value;

            const data = {
                arcadia_path: commitFormResult.path,
                title: commitFormResult.title,
                message: commitFormResult.description,
                startrek_tickets: commitFormResult.tickets
            };

            const request = new Request(uploadUrl, {
                method: 'POST',
                body: JSON.stringify(data)
            });

            return Promise.race([cancelUpload(), getResponse(request)]);
        })
        .then(data => {
            closeUpload();

            data = data as IResponseResult<any>;

            if (data.status >= 400) {
                void showUploadErrorDialog(uploadUrl, data);
                return Promise.reject(
                    'non-200 response while check upload possibility'
                );
            }

            const shareResult = data.json as IShareResult;

            const metadata = panel.context.model.metadata;
            appendShareToHistory(metadata, {
                path: shareResult.path,
                link: shareResult.link,
                timestamp: new Date().getTime(),
                success: true,
                revision: shareResult.revision,
                jupyticketId: shareResult.jupyticket_id
            });

            yandexMetrika.reachGoal({
                target: 'arcadia_share:share',
                sessionParams: {
                    path: data.json.path
                }
            });

            const promises: Promise<any>[] = [panel.context.save()];
            if (showSuccessDialog) {
                promises.push(showSuccessDialog_(shareResult));
            }

            return Promise.all(promises);
        })
        .catch(reason => {
            if (reason instanceof Error) {
                closeVerify();
                closeUpload();
                const error = <p>{reason.toString()}</p>;
                void showErrorDialog('Unknown error', error, 'danger');
            }
        });
};
