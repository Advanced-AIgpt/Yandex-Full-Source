import { PartialJSONObject } from '@lumino/coreutils';
import { IObservableJSON } from '@jupyterlab/observables';
import { IJupyterCloudMeta } from './metadata';

export interface IShareHistoryEntry {
    path: string;
    link: string;
    timestamp: number;
    success: boolean;
    revision?: string;
    jupyticketId?: number;
}

export interface IShareMeta {
    history: IShareHistoryEntry[];
}

export const getShareHistory = (metadata: IObservableJSON) => {
    const jupyterCloudMeta = (metadata.get('jupytercloud') ||
        {}) as IJupyterCloudMeta;
    const shareMeta = (jupyterCloudMeta.share || {}) as IShareMeta;
    return shareMeta.history || [];
};

export const appendShareToHistory = (
    metadata: IObservableJSON,
    entry: IShareHistoryEntry
) => {
    const jupyterCloudMeta = (metadata.get('jupytercloud') ||
        {}) as IJupyterCloudMeta;
    const shareMeta = (jupyterCloudMeta.share || {}) as IShareMeta;
    const shareHistory = shareMeta.history || [];
    shareHistory.push(entry);
    shareMeta.history = shareHistory;
    jupyterCloudMeta.share = shareMeta;
    metadata.set('jupytercloud', jupyterCloudMeta as PartialJSONObject);
};
