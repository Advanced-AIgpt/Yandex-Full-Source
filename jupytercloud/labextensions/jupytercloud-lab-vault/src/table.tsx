import React from 'react';
import { Alert, Button, Table } from 'react-bootstrap';
import { Plus, X } from 'react-bootstrap-icons';

import { Dialog, ReactWidget, showDialog } from '@jupyterlab/apputils';
import { IVaultSecret } from 'jupytercloud-lab-lib';

export interface ISecretTableState {
    secrets: IVaultSecret[];
    setSecrets?: (secrets: IVaultSecret[]) => any;
    newSecret?: IVaultSecret;
    validFields?: Map<string, boolean>;
}

export interface ISecretResult {
    ok: boolean;
    reason: string;
    secret: IVaultSecret;
}

export class SecretTableWidget extends ReactWidget implements ISecretTableState {
    secrets: IVaultSecret[];

    constructor(data: ISecretTableState) {
        super();
        this.secrets = JSON.parse(JSON.stringify(data.secrets));
    };

    setSecrets = (s: IVaultSecret[]) => {
        this.secrets.length = 0;
        s.forEach((secret: IVaultSecret) => {
            this.secrets.push(secret);
        });
    };

    render = () =>
        <SecretTable
            secrets={this.secrets}
            setSecrets={this.setSecrets}
        />;


    getValue = () => this.secrets;
}

export class SecretTable extends React.Component<ISecretTableState> {
    state: ISecretTableState = {
        secrets: this.props.secrets,
        setSecrets: this.props.setSecrets,
        newSecret: { name: '', uuid: '' },
        validFields: new Map()
            .set('name', false)
            .set('uuid', false)
    };

    render = () => {
        return (
            <Table>
                <thead>
                {this.renderTableHeader()}
                </thead>
                <tbody>
                {this.renderTableRows(this.state.secrets)}
                {this.renderLastRow()}
                </tbody>
            </Table>
        );
    };

    getValue = () => this.state.secrets;

    handleRemoveSpecificRow = (idx: number) => () => {
        const secrets = [...this.state.secrets];
        secrets.splice(idx, 1);
        this.setState({ secrets });

        this.state.setSecrets(secrets);  // propagate changes back to origin
    };

    handleAddRow = () => {
        let validFields = new Map(this.state.validFields);
        for (let key in validFields) {
            validFields.set(key, false);
        }

        const secrets = [...this.state.secrets, this.state.newSecret];

        this.setState({
            secrets: secrets,
            newSecret: { name: '', uuid: '' },
            validFields: validFields
        });

        this.state.setSecrets(secrets);  // propagate changes back to origin
    };

    validateNewSecret = (secret: IVaultSecret): Map<string, boolean> => {
        const nameRe = /^[a-zA-Z_]+$/;
        const uuidRe = /^.+/;  // TODO

        return new Map()
            .set('name', nameRe.test(secret.name))
            .set('uuid', uuidRe.test(secret.uuid));
    };

    handleNewSecretChange = (e: React.FormEvent<HTMLInputElement>) => {
        const name = e.currentTarget.name as keyof IVaultSecret;
        const value: string = e.currentTarget.value;

        let newSecret = JSON.parse(JSON.stringify(this.state.newSecret));  // deep copy
        newSecret[name] = value;

        const valid = this.validateNewSecret(newSecret);

        this.setState({ newSecret: newSecret, validFields: valid });
    };

    renderTableHeader = () => {
        return <tr>
            <th>#</th>
            <th>Name</th>
            <th>UUID</th>
            <th />
        </tr>;
    };

    renderTableRows = (secrets: IVaultSecret[]) => {
        return secrets.map((item, index) =>
            <tr key={index + 1}>
                <td>{index + 1}</td>
                <td>{item.name}</td>
                <td>{item.uuid}</td>
                <td>
                    <Button variant="danger" onClick={this.handleRemoveSpecificRow(index)}>
                        <X className="text-light" /> <span className="text-light"> Remove </span>
                    </Button>
                </td>
            </tr>
        );
    };

    renderLastRow = () => {
        return <tr key={'last'}>
            <td />
            <td>
                <input
                    type={'text'}
                    name={'name'}
                    value={this.state.newSecret.name}
                    onChange={this.handleNewSecretChange} />
            </td>
            <td>
                <input
                    type={'text'}
                    name={'uuid'}
                    value={this.state.newSecret.uuid}
                    onChange={this.handleNewSecretChange} />
            </td>
            <td>
                <Button
                    variant="primary"
                    onClick={this.handleAddRow}
                    disabled={![...this.state.validFields.values()].every(x => x)}>
                    <Plus className="text-light" /> <span className="text-light"> Add secret </span>
                </Button>
            </td>
        </tr>;
    };
}

export const showVaultSecrets = (secrets: IVaultSecret[]) => {
    const dialog = new Dialog({
        title: 'Manage Yandex Vault Secrets',
        body: new SecretTableWidget({ secrets }),
        buttons: [
            Dialog.cancelButton(),
            Dialog.okButton()
        ]
    });

    return dialog.launch();
};

export const renderSecretErrors = (secretResults: ISecretResult[]) => {
    return secretResults.filter(result => !result.ok).map(result => {
        return <>
            <p>
                Secret <code>{result.secret.name}</code> with
                UUID <code>{result.secret.uuid}</code>:
            </p>
            <pre><code>{result.reason}</code></pre>
            <hr />
        </>;
    });
};


export const showSecretErrorDialog = (secretResults: ISecretResult[]) => {
    return showDialog({
        title: 'Vault error',
        body: <Alert variant="danger">
            <Alert.Heading>Your secrets have following problems</Alert.Heading>
            {renderSecretErrors(secretResults)}
        </Alert>,
        buttons: [Dialog.warnButton({ label: 'Dismiss' })]
    });
};
