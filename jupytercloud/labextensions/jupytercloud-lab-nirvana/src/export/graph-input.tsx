import React from 'react';

import {
    Form,
    InputGroup,
    Spinner,
    Tooltip,
    OverlayTrigger
} from 'react-bootstrap';

import { CheckCircleFill, XCircleFill, Clipboard } from 'react-bootstrap-icons';

import { HubRequester } from '../requester';

import { DEFAULT_FOLDER_ID, DEFAULT_FOLDER_TITLE } from './base';

export interface IGraphInfo {
    instanceModifiable: boolean;
    instanceCloneable: boolean;
    quota: string;
    workflowId: string;
    instanceId: string | null;
    folderId: string | null;
    folderTitle: string | null;
}

interface IGraphInputProps {
    setGraphInfo: (info: IGraphInfo | null) => void;
    requester: HubRequester;
}

interface IGraphInputState {
    verifyInProgress: boolean;
    success: boolean | null;
    failReason: string | null;
}

export class GraphInput extends React.Component<
    IGraphInputProps,
    IGraphInputState
> {
    private graphInputRef: React.RefObject<HTMLInputElement>;

    constructor(props: IGraphInputProps) {
        super(props);

        this.graphInputRef = React.createRef();

        this.state = {
            verifyInProgress: false,
            success: null,
            failReason: null
        };
    }

    componentDidMount = () => {
        this.graphInputRef.current.focus();
    };

    private onChange = (event: React.ChangeEvent<HTMLInputElement>) => {
        const workflowUrl = event.target.value;
        this.fetchGraphInfo(workflowUrl);
    };

    private fetchGraphInfo = (workflowUrl: string) => {
        const { setGraphInfo, requester } = this.props;

        if (!workflowUrl) {
            this.clear();
            return;
        }

        this.setState({
            verifyInProgress: true
        });

        requester
            .request({
                path: '/api/nirvana/workflow_info',
                method: 'GET',
                query: {
                    workflow_url: workflowUrl
                }
            })
            .then(data => {
                if (data.success) {
                    const folderId = data.folder_id;

                    // XXX: Temporary patch until we learn how to get title from id
                    let folderTitle = folderId
                        ? `Unknown (id: ${data.folder_id})`
                        : null;
                    if (folderId === DEFAULT_FOLDER_ID) {
                        folderTitle = DEFAULT_FOLDER_TITLE;
                    }

                    const graphInfo: IGraphInfo = {
                        workflowId: data.workflow_id,
                        instanceId: data.instance_id,
                        instanceModifiable: data.instance_modifiable,
                        instanceCloneable: data.instance_cloneable,
                        quota: data.quota,
                        folderId,
                        folderTitle
                    };

                    this.setState({
                        success: true,
                        verifyInProgress: false,
                        failReason: null
                    });
                    setGraphInfo(graphInfo);
                } else {
                    this.setState({
                        success: false,
                        verifyInProgress: false,
                        failReason: data.reason
                    });
                    setGraphInfo(null);
                }
            });
    };

    protected pasteFromClipboard = () => {
        navigator.clipboard.readText().then(clipText => {
            this.graphInputRef.current.value = clipText;
            this.fetchGraphInfo(clipText);
        });
    };

    protected clear = () => {
        this.setState({
            verifyInProgress: false,
            success: null,
            failReason: null
        });
        this.props.setGraphInfo(null);
        this.graphInputRef.current.value = '';
    };

    protected getStatusAddon = () => {
        const { verifyInProgress, success, failReason } = this.state;

        let icon;

        if (verifyInProgress) {
            icon = (
                <Spinner animation="grow" role="status" size="sm">
                    <span className="sr-only">Verifying graph...</span>
                </Spinner>
            );
        } else if (success) {
            icon = <CheckCircleFill className="text-success" />;
        } else if (success === false) {
            const tooltip = (
                <Tooltip id="verify-fail-tooltip" placement="right">
                    {failReason}
                </Tooltip>
            );
            icon = (
                <OverlayTrigger
                    placement="right"
                    delay={{ show: 100, hide: 400 }}
                    overlay={tooltip}
                >
                    <XCircleFill className="text-danger" onClick={this.clear} />
                </OverlayTrigger>
            );
        } else {
            icon = <Clipboard onClick={this.pasteFromClipboard} />;
        }

        return (
            <InputGroup.Append>
                <InputGroup.Text>{icon}</InputGroup.Text>
            </InputGroup.Append>
        );
    };

    render = () => {
        const { verifyInProgress } = this.state;

        return (
            <Form.Group>
                <Form.Label>Workflow/instance link to add to</Form.Label>
                <InputGroup>
                    <Form.Control
                        type="text"
                        placeholder="https://nirvana.yandex-team.ru/flow/<...>/<...>/graph"
                        onChange={this.onChange}
                        disabled={verifyInProgress}
                        ref={this.graphInputRef}
                    />
                    {this.getStatusAddon()}
                </InputGroup>
                <Form.Text className="text-muted">
                    Just paste a link to a graph upwards
                </Form.Text>
            </Form.Group>
        );
    };
}
