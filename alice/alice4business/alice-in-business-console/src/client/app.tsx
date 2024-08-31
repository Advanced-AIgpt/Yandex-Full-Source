import React from 'react';
import ReactDOM from 'react-dom';

import './lib/polyfills';
import App, { AppProps } from './blocks/App/App';

document.addEventListener('DOMContentLoaded', () => {
    const rootElement = document.getElementById('app')!;
    const props = JSON.parse(rootElement.dataset.props || '{}') as AppProps;

    ReactDOM.render(<App {...props} />, rootElement);
});
