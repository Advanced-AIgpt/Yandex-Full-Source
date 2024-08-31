import { PartialJSONObject } from '@lumino/coreutils';
import { IObservableJSON } from '@jupyterlab/observables';
import { IJupyterCloudMeta } from './metadata';

export interface IVaultSecret {
    name: string;
    uuid: string;
}

export interface IVaultMeta {
    secrets: IVaultSecret[];
}

export const getVaultSecrets = (metadata: IObservableJSON): IVaultSecret[] => {
    const jupyterCloudMeta = (metadata.get('jupytercloud') ||
        {}) as IJupyterCloudMeta;
    const vaultMeta = (jupyterCloudMeta.vault || {}) as IVaultMeta;
    return vaultMeta.secrets || [];
};

export const setVaultSecrets = (
    metadata: IObservableJSON,
    secrets: IVaultSecret[]
) => {
    const jupyterCloudMeta = (metadata.get('jupytercloud') ||
        {}) as IJupyterCloudMeta;
    const vaultMeta = (jupyterCloudMeta.vault || {}) as IVaultMeta;
    vaultMeta.secrets = secrets;
    jupyterCloudMeta.vault = vaultMeta;
    metadata.set('jupytercloud', jupyterCloudMeta as PartialJSONObject);
};

export const addVaultSecret = (
    metadata: IObservableJSON,
    entry: IVaultSecret
): void => {
    const jupyterCloudMeta = (metadata.get('jupytercloud') ||
        {}) as IJupyterCloudMeta;
    const vaultMeta = (jupyterCloudMeta.vault || {}) as IVaultMeta;
    const vaultSecrets = vaultMeta.secrets || [];
    vaultSecrets.push(entry);
    vaultMeta.secrets = vaultSecrets;
    jupyterCloudMeta.vault = vaultMeta;
    metadata.set('jupytercloud', jupyterCloudMeta as PartialJSONObject);
};
