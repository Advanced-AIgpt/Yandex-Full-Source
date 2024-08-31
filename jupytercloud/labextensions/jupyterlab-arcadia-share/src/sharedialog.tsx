import 'bootstrap/dist/css/bootstrap.min.css';

import React, { Fragment } from 'react';
import {
    Form,
    Container,
    Row,
    Col,
    Button,
    InputGroup,
    FormControl
} from 'react-bootstrap';

import { Dialog, ReactWidget } from '@jupyterlab/apputils';

import {
    IPreUploadInfo,
    IPathInfo,
    IResponseResult,
    getResponse,
    joinHubUrl
} from './utils';

interface ICommitFormDialog {
    data: IPreUploadInfo;
    pathInfo: IPathInfo;
}

export interface ICommitFormResult {
    path: string;
    title: string;
    description: string;
    tickets: string[];
}

export class CommitFormDialog extends Dialog<ICommitFormResult> {
    constructor(options: ICommitFormDialog) {
        super({
            title: 'Save and Upload notebook to Arcadia',
            body: new CommitFormWidget(options),
            focusNodeSelector: 'textarea',
            buttons: [
                Dialog.cancelButton(),
                Dialog.okButton({ label: 'Upload' })
            ]
        });
    }

    static show = (
        options: ICommitFormDialog
    ): Promise<Dialog.IResult<ICommitFormResult>> => {
        const dialog = new CommitFormDialog(options);
        return dialog.launch();
    };

    protected _evtKeydown = (event: KeyboardEvent): void => {
        if (event.keyCode === 13 && !event.shiftKey) {
            // kill submit on Enter but not Shift+Enter
            return;
        }
        super._evtKeydown(event);
    };
}

class CommitFormWidget extends ReactWidget
    implements Dialog.IBodyWidget<ICommitFormResult> {
    data: IPreUploadInfo;
    pathInfo: IPathInfo;
    commitFormRef: React.RefObject<CommitForm>;

    constructor({ pathInfo, data }: ICommitFormDialog) {
        super();
        this.data = data;
        this.pathInfo = pathInfo;
        this.commitFormRef = React.createRef();
    }

    public render = () => {
        return (
            <CommitForm
                exists={this.data.exists}
                defaultPath={this.data.default_path}
                notebookLink={this.data.link}
                uploadPathChoices={this.data.upload_path_choices}
                pathInfo={this.pathInfo}
                ref={this.commitFormRef}
            />
        );
    };

    public getValue = (): ICommitFormResult => {
        return this.commitFormRef.current.getValue();
    };
}

interface ICommitFormProps {
    exists: boolean;
    pathInfo: IPathInfo;
    notebookLink: string;
    defaultPath: string;
    uploadPathChoices?: string[];
}

interface ICommitFormState {
    path: string;
    title: string;
    description: string;
    tickets: string[];
}

class CommitForm extends React.Component<ICommitFormProps, ICommitFormState> {
    constructor(props: ICommitFormProps) {
        super(props);
        this.state = {
            path: props.defaultPath,
            title: props.pathInfo.notebookPath,
            description: '',
            tickets: []
        };
    }

    public getValue = (): ICommitFormResult => {
        return this.state as ICommitFormResult;
    };

    private onTitleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
        this.setState({
            title: event.target.value
        });
    };

    private onDescriptionChange = (
        event: React.ChangeEvent<HTMLTextAreaElement>
    ) => {
        this.setState({
            description: event.target.value
        });
    };

    private onPathChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
        this.setState({
            path: event.target.value
        });
    };

    private onTicketsChange = (tickets: string[]) => {
        this.setState({ tickets });
    };

    private titleInput = () => {
        return (
            <Form.Group controlId="shareTitleInput">
                <Form.Label>Notebook title</Form.Label>
                <Form.Control
                    type="input"
                    value={this.state.title}
                    onChange={this.onTitleChange}
                    className="jcas-input"
                    placeholder="Enter notebook title"
                />
            </Form.Group>
        );
    };

    private descriptionInput = () => {
        return (
            <Form.Group controlId="shareDescriptionInput">
                <Form.Label>Notebook description</Form.Label>
                <Form.Control
                    as="textarea"
                    rows={3}
                    value={this.state.description}
                    onChange={this.onDescriptionChange}
                    className="jcas-input"
                    placeholder="Enter notebook description"
                />
            </Form.Group>
        );
    };

    private notebookPathRadio = () => {
        const doCheck = (
            label: string | React.ReactElement<any>,
            value: string,
            name: string
        ) => {
            const radioGroupName = 'share-notebook-path';
            return (
                <Form.Check
                    label={label}
                    value={value}
                    name={radioGroupName}
                    type="radio"
                    id={`${radioGroupName}-${name}`}
                    checked={this.state.path === value}
                    onChange={this.onPathChanged}
                />
            );
        };

        const defaultPath = this.props.defaultPath;
        let choices;

        if (
            this.props.uploadPathChoices &&
            this.props.uploadPathChoices.length > 0
        ) {
            const alternative = this.props.uploadPathChoices[0];
            const rewriteTitle = (
                <Fragment>
                    {'Rewrite '}
                    <a href={this.props.notebookLink} target={'_blank'}>
                        {defaultPath}
                    </a>
                </Fragment>
            );

            choices = (
                <Fragment>
                    {doCheck(rewriteTitle, defaultPath, 'rewrite')}
                    {doCheck(`Upload as ${alternative}`, alternative, 'new')}
                </Fragment>
            );
        } else {
            choices = doCheck(
                `Upload to ${defaultPath}`,
                defaultPath,
                'upload'
            );
        }

        return (
            <Form.Group controlId="shareNotebookPathRadio">
                <Form.Label>Notebook upload path</Form.Label>
                {choices}
            </Form.Group>
        );
    };

    public render = () => {
        return (
            <Container fluid={true} className="jcas-commit-form-body">
                <Row>
                    <Col md={7}>
                        <Form>
                            {this.titleInput()}
                            {this.descriptionInput()}
                            {this.notebookPathRadio()}
                        </Form>
                    </Col>
                    <Col md={5}>
                        <TicketsInput
                            tickets={this.state.tickets}
                            onChange={this.onTicketsChange}
                            pathInfo={this.props.pathInfo}
                        />
                    </Col>
                </Row>
            </Container>
        );
    };
}

interface ITicketsInputProps {
    tickets: string[];
    pathInfo: IPathInfo;
    onChange: (new_tickets: string[]) => void;
}

interface IValidateResponse {
    tickets: string[];
}

interface IExtractTicketsResponse {
    tickets: string[];
}

interface ITicketInputState {
    ticket: string;
    isValid: boolean;
    isInvalid: boolean;
    validatePromise?: Promise<IResponseResult<IValidateResponse>>;
    error?: string;
}

interface ITicketsInputState {
    tickets: ITicketInputState[];
    extractPromise?: Promise<IResponseResult<IExtractTicketsResponse>>;
}

class TicketsInput extends React.Component<
    ITicketsInputProps,
    ITicketsInputState
> {
    constructor(props: ITicketsInputProps) {
        super(props);

        this.state = {
            tickets: props.tickets.map((ticket: string) => ({
                ticket,
                isValid: true,
                isInvalid: false
            }))
        };
    }

    public componentDidMount = () => {
        if (this.state.extractPromise) {
            return;
        }

        const url = joinHubUrl(
            this.props.pathInfo.hubPrefix,
            '/api/statrek/extract_tickets'
        );

        const request = new Request(url, {
            method: 'POST',
            body: JSON.stringify({
                notebook_path: this.props.pathInfo.notebookPath
            })
        });

        const promise = getResponse(request);

        this.setState({ extractPromise: promise });

        promise
            .then((data: IResponseResult<IExtractTicketsResponse>) => {
                if (data.errorText) {
                    return Promise.reject(data.errorText);
                }
                this.setState(
                    {
                        extractPromise: null,
                        tickets: data.json.tickets.map((ticket: string) => ({
                            ticket,
                            isValid: true,
                            isInvalid: false
                        }))
                    },
                    this.propagateTickets
                );
            })
            .catch((e: Error | string) => {
                let text: string;
                if (e instanceof Error) {
                    text = e.toString();
                } else {
                    text = e;
                }

                console.warn(`error while extracting tickets: ${text}`);

                this.setState({ extractPromise: null });
            });
    };

    private addTicketInput = () => {
        const newInputState = {
            ticket: '',
            isValid: false,
            isInvalid: false
        };

        this.setState(prevState => ({
            tickets: [...prevState.tickets, newInputState]
        }));
    };

    private removeTicketInput = (index: number) => {
        const tickets = [...this.state.tickets];
        tickets.splice(index, 1);
        this.setState({ tickets }, this.propagateTickets);
    };

    private changeTicketInput = (
        index: number,
        event: React.ChangeEvent<HTMLInputElement>
    ) => {
        this._updateTicket(index, { ticket: event.target.value });
    };

    private _updateTicket = (
        index: number,
        value: Partial<ITicketInputState>
    ) => {
        const tickets = [...this.state.tickets];
        tickets[index] = { ...tickets[index], ...value };
        this.setState({ tickets }, this.propagateTickets);
    };

    private propagateTickets = () => {
        this.props.onChange(
            this.state.tickets
                .filter(ticket => ticket.isValid)
                .map(ticket => ticket.ticket)
        );
    };

    private validateTicket = (index: number) => {
        const ticketInfo = this.state.tickets[index];

        if (ticketInfo.validatePromise || !ticketInfo.ticket) {
            return;
        }

        const url = joinHubUrl(
            this.props.pathInfo.hubPrefix,
            '/api/statrek/validate_tickets'
        );

        const request = new Request(url, {
            method: 'POST',
            body: JSON.stringify({
                tickets: [ticketInfo.ticket]
            })
        });

        const promise = getResponse(request);

        this._updateTicket(index, { validatePromise: promise });

        promise
            .then((data: IResponseResult<IValidateResponse>) => {
                if (data.errorText) {
                    return Promise.reject(data.errorText);
                }

                const tickets = data.json.tickets;

                this._updateTicket(index, {
                    ticket: tickets[0],
                    isValid: true,
                    isInvalid: false,
                    error: null
                });
            })
            .catch((e: Error | string) => {
                let text: string;
                if (e instanceof Error) {
                    text = e.toString();
                } else {
                    text = e;
                }
                this._updateTicket(index, {
                    error: text,
                    isValid: false,
                    isInvalid: true
                });
            })
            .finally(() => {
                this._updateTicket(index, { validatePromise: null });
            });
    };

    public render = () => {
        return (
            <Form>
                <Form.Group controlId="ticket-inputs">
                    <Form.Label>Startrek tickets</Form.Label>
                    {this.state.tickets.map(
                        (state: ITicketInputState, i: number) => (
                            <TicketInput
                                key={i}
                                ticket={state.ticket}
                                isValid={state.isValid}
                                isInvalid={state.isInvalid}
                                validatePromise={state.validatePromise}
                                error={state.error}
                                onChange={this.changeTicketInput.bind(this, i)}
                                remove={this.removeTicketInput.bind(this, i)}
                                validate={this.validateTicket.bind(this, i)}
                            />
                        )
                    )}
                </Form.Group>
                <Button
                    bsPrefix="btn-jcas"
                    variant="primary"
                    onClick={this.addTicketInput}
                >
                    Add new link
                </Button>
            </Form>
        );
    };
}

interface ITicketInputProps extends ITicketInputState {
    onChange: (event: React.ChangeEvent<HTMLInputElement>) => void;
    remove: () => void;
    validate: () => void;
}

class TicketInput extends React.Component<ITicketInputProps> {
    constructor(props: ITicketInputProps) {
        super(props);
    }

    public render = () => {
        const disabled = Boolean(this.props.validatePromise);
        const validated = this.props.isInvalid || this.props.isValid;

        return (
            <InputGroup
                className={`jcas-input-group ${
                    validated ? '.was-validated' : ''
                }`}
                hasValidation={true}
            >
                <FormControl
                    placeholder="Startrek ticket"
                    aria-label="Startrek ticket id"
                    className="jcas-input"
                    value={this.props.ticket}
                    onChange={this.props.onChange}
                    disabled={disabled || this.props.isValid}
                    isInvalid={this.props.isInvalid}
                    isValid={this.props.isValid}
                />
                <Button
                    bsPrefix="btn-jcas"
                    variant="primary"
                    onClick={this.props.validate}
                    disabled={disabled || this.props.isValid}
                >
                    Ok
                </Button>
                <Button
                    bsPrefix="btn-jcas"
                    variant="reject"
                    onClick={this.props.remove}
                    disabled={disabled}
                >
                    Del
                </Button>
                <Form.Control.Feedback type="invalid" tooltip>
                    {this.props.error}
                </Form.Control.Feedback>
            </InputGroup>
        );
    };
}
