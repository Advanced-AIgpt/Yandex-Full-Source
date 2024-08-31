import React from 'react';
import connect from 'storeon/react/connect';

import { Tabs, Col } from 'antd';
import Button from 'antd/es/button';

import GrammarViewer from './GrammarViewer';
import SearchTab from './search/Tab';
import EditorTab from './EditorTab';
import ExportGrammarModal from './ExportGrammarModal';
import ExportDatasetModal from './ExportDatasetModal';
import InfoModal from './Info';
import SettingsDrawer from './SettingsDrawer';
import SynonymsTab from './SynonymsTab';


import './App.css';

const { TabPane } = Tabs;


class App extends React.Component {
    onTabChange = (key, type) => {
        this.setState({ [type]: key });
    };

    showDrawer = () => {
        this.props.dispatch('settings/drawerVisible/update', true);
    };

    showInfo = () => {
        this.props.dispatch('info/visible', true);
    }

    render() {
        const settings = <Button type="link" shape="circle" icon="setting" style={{ paddingRight: "12px" }} onClick={this.showDrawer} />;
        const info = <Button type="link" shape="circle" icon="info-circle" style={{ paddingRight: "12px" }} onClick={this.showInfo} />;
        const tabsExtra = (
            <div>
                {info}
                {settings}
            </div>
        )
        return (
            <div>
                <Col span={7}>
                    <GrammarViewer />
                </Col>
                <Col span={17} style={{ height: '100%' }}>
                    <Tabs
                        activeKey={this.props.appTab}
                        onChange={key => { this.props.dispatch('appTab/update', key); }}
                        tabBarExtraContent={tabsExtra}
                    >
                        <TabPane tab="Search" key="search">
                            <SearchTab />
                        </TabPane>
                        <TabPane tab="Validate" key="validate">
                            <EditorTab />
                        </TabPane>
                        <TabPane tab="Synonyms" key="synonyms">
                            <SynonymsTab />
                        </TabPane>
                    </Tabs>
                </Col>

                <InfoModal />
                <SettingsDrawer />
                <ExportGrammarModal />
                <ExportDatasetModal />
            </div>
        );
    };
}

export default connect('appTab', 'info', 'settings', App);
