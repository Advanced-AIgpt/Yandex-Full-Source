export const getLanguage = (): string => {
    if (navigator.languages && navigator.languages.length) {
        return navigator.languages[0];
    } else {
        return navigator.language || "en";
    }
};

interface IRequestHub {
    uri: string;
    method: string;
    queryArgs?: [string, string][];
    json?: Record<string, any>;
}

interface IRequestHubResult {
    json: Record<string, any>;
    text: string;
    status: number;
    statusText: string;
}

const _parseHubResponse = async (response: Response): Promise<IRequestHubResult> => {
    const text = await response.text();
    let json;
    try {
        json = JSON.parse(text);
    } catch (e) {
        console.warn("error parsing response JSON", e, text);
    }

    return {
        json,
        text,
        status: response.status,
        statusText: response.statusText,
    };
};

export const requestHub = ({ uri, method, queryArgs, json }: IRequestHub): Promise<IRequestHubResult> => {
    const baseUrl = new URL(window.jhdata.base_url, window.location.href);
    if (uri.startsWith("/")) {
        uri = uri.slice(1);
    }
    const url = new URL(uri, baseUrl);
    if (queryArgs) {
        queryArgs.forEach(([key, value]) => url.searchParams.append(key, value));
    }
    let body;
    if (json) {
        body = JSON.stringify(json);
    }

    const request = new Request(url.toString(), { method, body });

    return fetch(request)
        .then(_parseHubResponse)
        .then((result) => {
            // TODO: use normal request library
            const statusType = Math.floor(result.status / 100);
            if ((statusType !== 2 && statusType !== 3) || !result.json) {
                return Promise.reject(result);
            }
            return Promise.resolve(result);
        });
};
