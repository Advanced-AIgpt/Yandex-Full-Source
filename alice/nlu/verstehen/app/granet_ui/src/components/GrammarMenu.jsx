import React from 'react';
import connect from 'storeon/react/connect';
import { hotkeys } from 'react-keyboard-shortcuts'

import Menu from 'antd/es/menu';
import Icon from 'antd/es/icon';
import notification from 'antd/es/notification';

const { SubMenu } = Menu;

class GrammarMenu extends React.Component {
    hot_keys = {
        'shift+enter': {
            priority: 1,
            handler: (event) => { 
                const e = {key: 'run'}; 
                this.onClick(e) 
            },
        },
    }

    constructor(props) {
        super(props);

        // This binding is necessary to make `this` work in the callback
        this.onClick = this.onClick.bind(this);
    }

    onClick(e) {
        if (e.key === 'run') {
            this.props.dispatch('searchResultsGranet/search');
        }
        else if (e.key.startsWith('app/')) {
            const app = e.key.substr(4);
            if (app !== this.props.apps.current) {
                // show notification
                console.warn('Changing app, search and validate tabs are cleared');
                notification.warning({
                    message: 'Changing app, search and validate tabs are cleared',
                    duration: 10
                })
                this.props.dispatch('userSamples/reset');
                this.props.dispatch('searchResultsGranet/reset');
                this.props.dispatch('searchResultsVerstehen/reset');
                this.props.dispatch('addButtons/reset');
            }
            this.props.dispatch('apps/current/update', app);
        }
        else if (e.key === 'export/grammarAsExperiment') {
            this.props.dispatch('export/grammarAsExperiment/get');
            this.props.dispatch('export/visible/update', true);
        }
        else if (e.key == 'export/validatedDataset') {
            this.props.dispatch('appTab/update', 'validate')
            this.props.dispatch('exportDataset/fetchMocks');
            this.props.dispatch('exportDataset/update', {visible: true});
        }
    }

    render() {
        return (
            <Menu onClick={this.onClick} selectedKeys={[]} mode="horizontal" style={{ height: "44px" }}>
                <SubMenu key="app" title={("App: " + this.props.apps['current']).substr(0, 30)} selectable>
                    {this.props.apps['all'].map((app) => {
                        return <Menu.Item key={"app/" + app}>{app}</Menu.Item>
                    })}
                </SubMenu>
                <SubMenu key="export" title="Export" selectable>
                    <Menu.Item key="export/grammarAsExperiment">Export grammar as experiment</Menu.Item>
                    <Menu.Item key="export/validatedDataset">Export dataset from 'Validate' tab</Menu.Item>
                </SubMenu>
                <Menu.Item key="run" disabled={this.props.searchResultsGranet['inProgress']}>
                    <Icon type="caret-right"/>
                    Run
                </Menu.Item>
            </Menu>
        );
    }
}

export default connect(
    'apps', 
    'appTab',
    'searchResultsGranet', 
    'exportGrammar', 
    'exportDtaset',
    'searchResultsVerstehen', 'userSamples', 'appButtons',
    hotkeys(GrammarMenu));