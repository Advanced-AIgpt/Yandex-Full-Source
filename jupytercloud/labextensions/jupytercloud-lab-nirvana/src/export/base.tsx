import React from 'react';

import { Button, Form } from 'react-bootstrap';

import {
    IYandexMetrika,
    IShareHistoryEntry,
    OpenCopyPath
} from 'jupytercloud-lab-lib';

import { HubRequester } from '../requester';
import { NirvanaPanel } from '../panels/base';
import { PlaceholderPanel } from '../panels/placeholder';

export const DEFAULT_QUOTA = 'default';
export const DEFAULT_FOLDER_ID = '9425306';
export const DEFAULT_FOLDER_TITLE = '/jupytercloud/junk';

export interface IExportRequest {
    arcadia_path: string;
    arcadia_revision?: string;
    jupyticket_id?: number;
    workflow_id?: string;
    instance_id?: string;
    quota?: string;
    folder_id?: string;
}

export interface IExportResult {
    success: boolean;
    url?: string;
    reason?: string;
    retriable?: boolean;
}

export interface IBaseExportPanelProps {
    lastShare: IShareHistoryEntry;
    requester: HubRequester;
    showPreviousMenu: () => void;
    onSuccess: (data: IExportResult) => void;
    onError: (data: IExportResult) => void;
    yandexMetrika: IYandexMetrika;
}

export interface IBaseExportPanelState {
    isLocked: boolean;
    exportInProgress: boolean;
    alert: React.ReactNode | null;
}

export abstract class BaseExportPanel<
    P extends IBaseExportPanelProps,
    S extends IBaseExportPanelState
> extends React.Component<P, S> {
    constructor(props: P) {
        super(props);
    }

    protected abstract title: string;

    protected abstract doExport: () => void;

    protected abstract getForm: () => React.ReactNode;

    protected processExportResponse = (data: IExportResult) => {
        if (data.success) {
            this.props.onSuccess(data);
        } else {
            this.props.onError(data);
        }
    };

    public render = (): React.ReactNode => {
        const { lastShare, showPreviousMenu } = this.props;

        const { alert, isLocked, exportInProgress } = this.state;

        if (exportInProgress) {
            return <PlaceholderPanel title={this.title} />;
        }

        return (
            <NirvanaPanel title={this.title}>
                {alert}
                <Form.Group>
                    <Form.Label>Notebook path to use</Form.Label>
                    <OpenCopyPath
                        path={lastShare.path}
                        url={lastShare.link}
                        title="Path"
                        copyButton={false}
                    />
                    <Form.Text className="text-muted">
                        This path is read-only and given for a reference
                    </Form.Text>
                </Form.Group>
                {this.getForm()}
                <Form.Group>
                    <Button className="mr-2" onClick={showPreviousMenu}>
                        Back
                    </Button>
                    <Button disabled={isLocked} onClick={this.doExport}>
                        Export
                    </Button>
                </Form.Group>
            </NirvanaPanel>
        );
    };
}
