import React from 'react';
import ReactDOM from 'react-dom';
import StoreContext from 'storeon/react/context'

import './index.css';
import App from './components/App.jsx';
import * as serviceWorker from './serviceWorker';

import store from './store/index.jsx';

ReactDOM.render(
            <StoreContext.Provider value={store} className="App">
                <App />
            </StoreContext.Provider>, document.getElementById('root'));

// If you want your app to work offline and load faster, you can change
// unregister() to register() below. Note this comes with some pitfalls.
// Learn more about service workers: https://bit.ly/CRA-PWA
serviceWorker.unregister();
