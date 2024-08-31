import { URLExt } from '@jupyterlab/coreutils';

export interface IPathInfo {
    userName: string;
    notebookPath: string;
    hubPrefix: string;
}

export interface IResponseResult<T> {
    json: T;
    text: string;
    status: number;
    statusText: string;
    errorTitle?: string;
    errorText?: string;
}

export interface IPreUploadInfo {
    result: boolean;
    exists: boolean;
    message: string;
    link: string;
    default_path: string;
    additional_choices?: string[];
    upload_path_choices?: string[];
}

export const joinHubUrl = (...parts: string[]) => {
    return '/' + URLExt.encodeParts(URLExt.join(...parts));
};

export const getUploadUrl = ({
    userName,
    notebookPath,
    hubPrefix
}: IPathInfo) => {
    return joinHubUrl(hubPrefix, '/api/arcadia/upload', userName, notebookPath);
};

export const getResponse = async (
    request: Request
): Promise<IResponseResult<any>> => {
    const response = await fetch(request);
    const text = await response.text();

    let json;
    try {
        json = JSON.parse(text);
    } catch (e) {
        console.warn('error parsing response JSON', e, text);
    }

    let errorTitle;
    let errorText;

    if (response.status >= 400) {
        errorTitle = `${response.status}: ${response.statusText}`;
        errorText = (json && json.message) || text;
    }

    return {
        json,
        text,
        status: response.status,
        statusText: response.statusText,
        errorTitle,
        errorText
    };
};
