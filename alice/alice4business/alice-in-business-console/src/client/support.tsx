import React from 'react';
import ReactDOM from 'react-dom';
import '@yandex-lego/components/Theme/presets/default';

import './lib/polyfills';
import App, { SupportAppProps } from './pages/Support';

export type SupportSSRProps = Pick<SupportAppProps, 'user' | 'yu' | 'secretkey' | 'config'>;

document.addEventListener('DOMContentLoaded', () => {
    const rootElement = document.getElementById('app')!;
    const props: SupportSSRProps = JSON.parse(rootElement.dataset.props || '{}');

    ReactDOM.render(<App {...props} />, rootElement);
});
