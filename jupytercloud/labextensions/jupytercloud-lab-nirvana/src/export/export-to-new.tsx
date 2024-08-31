import React from 'react';
import { Alert } from 'react-bootstrap';

import {
    IBaseExportPanelProps,
    IBaseExportPanelState,
    BaseExportPanel,
    IExportRequest,
    DEFAULT_FOLDER_ID,
    DEFAULT_FOLDER_TITLE,
    DEFAULT_QUOTA
} from './base';
import { QuotaInput } from './quota-input';
import { FolderInput } from './folder-input';

type IExportNewGraphPanelProps = IBaseExportPanelProps;

interface IExportNewGraphCardPanel extends IBaseExportPanelState {
    quota: string;
    folderId: string;
    folderTitle: string;
}

export class ExportNewGraphPanel extends BaseExportPanel<
    IExportNewGraphPanelProps,
    IExportNewGraphCardPanel
> {
    protected title = 'Creating new graph';

    constructor(props: IExportNewGraphPanelProps) {
        super(props);
        const alert = (
            <Alert variant="warning">
                <Alert.Heading>
                    Few words about quota in beta-version
                </Alert.Heading>
                <p>Graph will be created in "Default" quota.</p>
                <p>
                    Unfortunately "Default" quota is often overflow for creating
                    new graphs, so in case of a quota error just try again
                </p>
            </Alert>
        );
        this.state = {
            isLocked: false,
            alert: alert,
            exportInProgress: false,
            quota: DEFAULT_QUOTA,
            folderId: DEFAULT_FOLDER_ID,
            folderTitle: DEFAULT_FOLDER_TITLE
        };
    }

    protected doExport = () => {
        const { folderId, quota } = this.state;
        const { lastShare } = this.props;

        this.setState({ exportInProgress: true });

        this.props.yandexMetrika.reachGoal({
            target: 'nirvana_export:export',
            sessionParams: { nirvana_export_type: 'new_graph' }
        });

        const data: IExportRequest = {
            arcadia_path: lastShare.path,
            arcadia_revision: lastShare.revision,
            jupyticket_id: lastShare.jupyticketId,
            folder_id: folderId,
            quota
        };

        this.props.requester
            .request({
                path: '/api/nirvana/new_jupyter_workflow',
                method: 'POST',
                data
            })
            .then(this.processExportResponse);
    };

    protected getForm = () => {
        const { quota, folderId, folderTitle } = this.state;

        return (
            <>
                <QuotaInput quota={quota} />
                <FolderInput folderId={folderId} folderTitle={folderTitle} />
            </>
        );
    };
}
