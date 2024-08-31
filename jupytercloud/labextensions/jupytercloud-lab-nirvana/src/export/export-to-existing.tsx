import React from 'react';

import { IGraphInfo, GraphInput } from './graph-input';
import {
    IBaseExportPanelProps,
    IBaseExportPanelState,
    BaseExportPanel,
    IExportRequest,
    DEFAULT_FOLDER_ID,
    DEFAULT_FOLDER_TITLE,
    DEFAULT_QUOTA
} from './base';
import { TAddType, AddTypeChoice } from './add-type-choice';
import { QuotaInput } from './quota-input';
import { FolderInput } from './folder-input';

type IExportExistingGraphPanelProps = IBaseExportPanelProps;

interface IExportExistingGraphPanelState extends IBaseExportPanelState {
    graphInfo: IGraphInfo | null;
    addType: TAddType | null;
    quota: string;
    folderId: string;
    folderTitle: string;
}

export class ExportExistingGraphPanel extends BaseExportPanel<
    IExportExistingGraphPanelProps,
    IExportExistingGraphPanelState
> {
    protected title = 'Adding jupyter notebook to existing graph';

    constructor(props: IExportExistingGraphPanelProps) {
        super(props);
        this.state = {
            alert: null,
            exportInProgress: false,
            graphInfo: null,
            addType: null,
            isLocked: true,
            quota: DEFAULT_QUOTA,
            folderId: DEFAULT_FOLDER_ID,
            folderTitle: DEFAULT_FOLDER_TITLE
        };
    }

    protected setGraphInfo = (graphInfo: IGraphInfo | null) => {
        let quota: string = null;
        let folderId: string = null;
        let folderTitle: string = null;

        if (graphInfo) {
            quota = graphInfo.quota || DEFAULT_QUOTA;
            folderId = graphInfo.folderId || DEFAULT_FOLDER_ID;
            folderTitle = graphInfo.folderTitle || DEFAULT_FOLDER_TITLE;
        }

        this.setState({
            graphInfo,
            addType: null,
            isLocked: !graphInfo,
            quota,
            folderId,
            folderTitle
        });
    };

    protected setAddType = (addType: TAddType) => {
        this.setState({
            addType
        });
    };

    protected doExport = () => {
        const { addType, quota, graphInfo, folderId } = this.state;
        const { lastShare } = this.props;

        this.setState({ exportInProgress: true });

        this.props.yandexMetrika.reachGoal({
            target: 'nirvana_export:export',
            sessionParams: { nirvana_export_type: 'existing_graph' }
        });

        const data: IExportRequest = {
            arcadia_path: lastShare.path,
            arcadia_revision: lastShare.revision,
            jupyticket_id: lastShare.jupyticketId,
            workflow_id: graphInfo.workflowId,
            instance_id: graphInfo.instanceId
        };
        let path: string;

        switch (addType) {
            case 'add-to-current-draft':
                path = '/api/nirvana/add_to_draft';
                break;
            case 'clone-to-new-instance':
                path = '/api/nirvana/add_to_cloned_instance';
                break;
            case 'clone-to-new-workflow':
                path = '/api/nirvana/add_to_cloned_workflow';
                data.quota = quota;
                data.folder_id = folderId;
                break;
        }

        this.props.requester
            .request({
                path,
                method: 'POST',
                data
            })
            .then(this.processExportResponse);
    };

    protected getForm = () => {
        const { requester } = this.props;
        const { graphInfo, addType, quota, folderId, folderTitle } = this.state;

        return (
            <>
                <GraphInput
                    setGraphInfo={this.setGraphInfo}
                    requester={requester}
                />
                <AddTypeChoice
                    setAddType={this.setAddType}
                    current={addType}
                    graphInfo={graphInfo}
                />
                <QuotaInput quota={quota} />
                {addType === 'clone-to-new-workflow' && (
                    <FolderInput
                        folderId={folderId}
                        folderTitle={folderTitle}
                    />
                )}
            </>
        );
    };
}
