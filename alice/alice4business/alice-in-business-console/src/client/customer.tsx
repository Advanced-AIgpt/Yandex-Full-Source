import './lib/polyfills';
import url from 'url';
import React from 'react';
import ReactDOM from 'react-dom';
import '@yandex-lego/components/Theme/presets/default';
import App, { CustomerAppProps } from './pages/Customer';

export type CustomerSSRProps = Pick<CustomerAppProps, 'user' | 'yu' | 'secretkey' | 'config'>;

document.addEventListener('DOMContentLoaded', () => {
    const query = url.parse(location.search ?? '?', true).query;
    const rootElement = document.getElementById('app')!;

    const ssrProps: CustomerSSRProps = JSON.parse(rootElement.dataset.props || '{}');
    const props: CustomerAppProps = {
        isSSR: false,
        ...ssrProps,
        retpath: location.href,
        activationCode: Array.isArray(query.code) ? query.code[0] : query.code,
        activationId: Array.isArray(query.activationId)
            ? query.activationId[0]
            : query.activationId,
        isManualOpen: !/\bmanualOpen=false\b/.test(document.cookie || ''),
    };

    ReactDOM.hydrate(<App {...props} />, rootElement);
});
