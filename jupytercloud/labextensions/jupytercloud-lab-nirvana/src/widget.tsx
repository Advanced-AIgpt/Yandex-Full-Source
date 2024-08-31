import React from 'react';
import ReactDOM from 'react-dom';

import { Container } from 'react-bootstrap';

import { Widget } from '@lumino/widgets';
import { Message } from '@lumino/messaging';

import { NotebookPanel } from '@jupyterlab/notebook';
import { JupyterFrontEnd } from '@jupyterlab/application';

import {
    IArcadiaShare,
    IJupyterCloud,
    getShareHistory,
    IShareHistoryEntry,
    IYandexMetrika
} from 'jupytercloud-lab-lib';

import { HubRequester } from './requester';
import { NirvanaExportNavbar } from './navbar';
import { OAuthPopup, IOauthResult } from './oauth';

import { IExportResult } from './export/base';
import { ExportNewGraphPanel } from './export/export-to-new';
import { ExportExistingGraphPanel } from './export/export-to-existing';

import { PlaceholderPanel } from './panels/placeholder';
import { SharePanel } from './panels/share';
import { TRequestError, ErrorPanel } from './panels/error';
import { ExportTypePanel } from './panels/export-type';
import { OAuthPanel, OAuthErrorPanel } from './panels/oauth';
import { SuccessPanel } from './panels/success';

type TPanelName =
    | 'oauth-placeholder'
    | 'oauth'
    | 'oauth-error'
    | 'arcadia-share'
    | 'error'
    | 'export-type'
    | 'export-new-graph'
    | 'export-existing-graph'
    | 'export-update-exising'
    | 'export-placeholder'
    | 'success';

interface INirvanaExportWidget {
    id: string;
    notebookPanel: NotebookPanel;
    shell: JupyterFrontEnd.IShell;
    arcadiaShare: IArcadiaShare;
    jupyterCloud: IJupyterCloud;
    yandexMetrika: IYandexMetrika;
}

interface INirvanaExport extends INirvanaExportWidget {
    closeNirvanaExport: () => void;
}

interface INirvanaExportState {
    activePanel: TPanelName;
    oauthUrl: string | null;
    error: TRequestError | null;
    exportResult: IExportResult | null;
}

class NirvanaExport extends React.Component<
    INirvanaExport,
    INirvanaExportState
> {
    private requester: HubRequester;
    private defaultState: INirvanaExportState = {
        activePanel: 'oauth-placeholder',
        oauthUrl: null,
        error: null,
        exportResult: null
    };

    constructor(props: INirvanaExport) {
        super(props);

        this.requester = new HubRequester({
            jupyterCloud: props.jupyterCloud,
            errorCallback: this.onError
        });

        this.state = this.defaultState;
    }

    componentDidMount = () => {
        this.checkOauth();
    };

    resetState = () => {
        this.setState(this.defaultState);
        this.checkOauth();
    };

    protected changePanel = (name: TPanelName) => {
        this.setState({ activePanel: name });
    };

    protected panelChanger = (name: TPanelName) => {
        return () => this.changePanel(name);
    };

    protected onError = (error: TRequestError) => {
        this.setState({
            activePanel: 'error',
            error: error
        });
    };

    protected onSuccess = (data: IExportResult) => {
        this.props.yandexMetrika.reachGoal({
            target: 'nirvana_export:success',
            sessionParams: { nirvana_url: data.url }
        });
        this.setState({
            activePanel: 'success',
            exportResult: data
        });
    };

    protected getLastShare = (): IShareHistoryEntry | null => {
        const metadata = this.props.notebookPanel.context.model.metadata;
        const shareHistory = getShareHistory(metadata);
        return shareHistory.reverse().find(
            (entry: IShareHistoryEntry): boolean => {
                return entry.success;
            }
        );
    };

    protected backToNotebook = () => {
        const notebookId = this.props.notebookPanel.id;
        this.props.shell.activateById(notebookId);
    };

    protected doShare = () => {
        this.props.arcadiaShare
            .share({
                panel: this.props.notebookPanel,
                showSuccessDialog: false
            })
            .then(this.panelChanger('export-type'));
    };

    protected checkOauth = () => {
        this.requester
            .request({
                path: '/api/nirvana/check_oauth',
                method: 'GET'
            })
            .then(data => {
                if (data.need_oauth) {
                    this.setState({
                        activePanel: 'oauth',
                        oauthUrl: data.oauth_url
                    });

                    this.approveOAuth();
                } else {
                    this.changePanel('arcadia-share');
                }
            });
    };

    protected approveOAuth = () => {
        const callback = (result: IOauthResult) => {
            if (result.success) {
                this.changePanel('arcadia-share');
            } else {
                console.error(
                    'getting oauth access has failed with message: ',
                    result.message
                );
                this.changePanel('oauth-error');
            }
        };
        OAuthPopup(this.state.oauthUrl, callback);
    };

    protected getActiveBlock = () => {
        switch (this.state.activePanel) {
            case 'oauth-placeholder':
                return <PlaceholderPanel title="Checking OAuth..." />;
            case 'oauth':
                return <OAuthPanel approveOAuth={this.approveOAuth} />;
            case 'oauth-error':
                return <OAuthErrorPanel approveOAuth={this.approveOAuth} />;
            case 'arcadia-share':
                return (
                    <SharePanel
                        doShare={this.doShare}
                        skipShare={this.panelChanger('export-type')}
                        lastShareEntry={this.getLastShare()}
                    />
                );
            case 'export-type':
                return (
                    <ExportTypePanel
                        doBack={this.panelChanger('arcadia-share')}
                        actions={[
                            {
                                title: 'Create new graph',
                                action: this.panelChanger('export-new-graph')
                            },
                            {
                                title: 'Add to existing graph',
                                action: this.panelChanger(
                                    'export-existing-graph'
                                )
                            }
                            // {
                            //     title: 'Update in existing graph **WIP**',
                            //     action: () => {}
                            // }
                        ]}
                    />
                );
            case 'export-new-graph':
                return (
                    <ExportNewGraphPanel
                        lastShare={this.getLastShare()}
                        requester={this.requester}
                        showPreviousMenu={this.panelChanger('export-type')}
                        onSuccess={this.onSuccess}
                        onError={this.onError}
                        yandexMetrika={this.props.yandexMetrika}
                    />
                );
            case 'export-existing-graph':
                return (
                    <ExportExistingGraphPanel
                        lastShare={this.getLastShare()}
                        requester={this.requester}
                        showPreviousMenu={this.panelChanger('export-type')}
                        onSuccess={this.onSuccess}
                        onError={this.onError}
                        yandexMetrika={this.props.yandexMetrika}
                    />
                );
            case 'export-placeholder':
                return (
                    <PlaceholderPanel title="Exporting notebook to Nirvana..." />
                );
            case 'error':
                return (
                    <ErrorPanel
                        error={this.state.error}
                        tryAgain={this.resetState}
                    />
                );
            case 'success':
                return (
                    <SuccessPanel
                        closeNirvanaExport={this.props.closeNirvanaExport}
                        exportResult={this.state.exportResult}
                    />
                );
        }
    };

    render = () => {
        return (
            <>
                <NirvanaExportNavbar backToNotebook={this.backToNotebook} />
                <Container fluid>{this.getActiveBlock()}</Container>
            </>
        );
    };
}

export class NirvanaExportWidget extends Widget {
    constructor(private props: INirvanaExportWidget) {
        super();
        this.id = this.props.id;
        this.addClass('jcne-widget');
    }

    protected closeNirvanaExport = () => {
        const notebookId = this.props.notebookPanel.id;
        this.props.shell.activateById(notebookId);
        this.parent.close();
    };

    protected onBeforeAttach(msg: Message) {
        ReactDOM.render(
            <NirvanaExport
                closeNirvanaExport={this.closeNirvanaExport}
                {...this.props}
            />,
            this.node
        );
    }
}
