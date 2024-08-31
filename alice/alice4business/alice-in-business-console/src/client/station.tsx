import React from 'react';
import ReactDOM from 'react-dom';
import './lib/polyfills';
import App, { StationAppProps } from './blocks-station/App/App';
import './blocks-station/App/App.scss';
import * as url from 'url';

export interface StationSSRProps {
    urlRoot: string;
    assetsRoot: string;
    codeRefreshInterval: number;
}

document.addEventListener('DOMContentLoaded', () => {
    const query = url.parse((window && window.location && window.location.search) || '?', true).query;
    const rootElement = document.getElementById('app')!;

    const ssrProps: StationSSRProps = JSON.parse(rootElement.dataset.props || '{}');
    const props: StationAppProps = {
        activationCode: Array.isArray(query.code) ? query.code[0] : query.code,
        config: {
            ...ssrProps,
            host: window.location.hostname,
        },
    };

    ReactDOM.hydrate(<App {...props} />, rootElement);
});
