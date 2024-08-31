import React from 'react';

import Button from 'antd/es/button';
import Drawer from 'antd/es/drawer';
import Switch from 'antd/es/switch';
import Typography from 'antd/es/typography';
import Menu from 'antd/es/menu';
import InputNumber from 'antd/es/input-number';

import connect from 'storeon/react/connect';

const { Text } = Typography;

class SettingsDrawer extends React.Component {
    state = {
        settingsVisible: false
    };

    onClose = () => {
        this.props.dispatch('settings/drawerVisible/update', false);
    };

    onSwitch = (setting, checked) => {
        this.props.dispatch('settings/update', { [setting]: checked });
    }

    onSwitchSlotsValues = (checked) => {
        this.props.dispatch('settings/update', { printSlotsValues: checked });
        this.props.dispatch('searchResultsGranet/search');
    };

    onSwitchSlotsTypes = (checked) => {
        this.props.dispatch('settings/update', { printSlotsTypes: checked });
        this.props.dispatch('searchResultsGranet/search');
    };

    onChangeLinesToShow = (value) => {
        this.props.dispatch('settings/update', { linesToShow: value });
    }

    onClickLinesShowAll = (e) => {
        this.props.dispatch('settings/update', { linesToShow: undefined });
    }

    onClick = (e) => {
        if (this.props.settings[e.key] !== undefined) {
            this.onSwitch(e.key, !this.props.settings[e.key]);
        }
        if (e.key === "slotsValues") {
            this.onSwitchSlotsValues(!this.props.settings['printSlotsValues']);
        }
        if (e.key === "slotsTypes") {
            this.onSwitchSlotsTypes(!this.props.settings['printSlotsTypes']);
        }
    }

    render() {
        return (<Drawer
            title="Settings"
            placement="right"
            visible={this.props.settings['drawerVisible']}
            onClose={this.onClose}
            width="23vw"
        >
            <Menu onClick={this.onClick} selectable={false} style={{ borderRight: 0 }}>
                <Menu.Item key="showAddButtons">
                    <Switch
                        checked={this.props.settings['showAddButtons']}
                        onChange={ch => this.onSwitch('showAddButtons', ch)}
                        size="small" style={{ marginRight: 12 }}
                    />
                    <Text>Show "Add to pos/neg" buttons</Text>
                </Menu.Item>

                <Menu.Item key="showOccurance">
                    <Switch
                        checked={this.props.settings['showOccurance']}
                        onChange={ch => this.onSwitch('showOccurance', ch)}
                        size="small" style={{ marginRight: 12 }}
                    />
                    <Text>Show occurance in search</Text>
                </Menu.Item>

                <Menu.Item key="showClfScore">
                    <Switch
                        checked={this.props.settings['showClfScore']}
                        onChange={ch => this.onSwitch('showClfScore', ch)}
                        size="small" style={{ marginRight: 12 }}
                    />
                    <Text>Show classifier score in search</Text>
                </Menu.Item>

                <Menu.Item key="showMarkup">
                    <Switch
                        checked={this.props.settings['showMarkup']}
                        onChange={ch => this.onSwitch('showMarkup', ch)}
                        size="small" style={{ marginRight: 12 }}
                    />
                    <Text>Show granet slots</Text>
                </Menu.Item>

                <Menu.Item key="slotsValues">
                    <Switch
                        checked={this.props.settings['printSlotsValues']}
                        onChange={this.onSwitchSlotsValues}
                        size="small" style={{ marginRight: 12 }}
                    />
                    <Text>Print values for granet slots</Text>
                </Menu.Item>

                <Menu.Item key="slotsTypes">
                    <Switch
                        checked={this.props.settings['printSlotsTypes']}
                        onChange={this.onSwitchSlotsTypes}
                        size="small" style={{ marginRight: 12 }}
                    />
                    <Text>Print types for granet slots</Text>
                </Menu.Item>

                <Menu.Item key="linesToShow">
                    <InputNumber
                        min={0}
                        value={this.props.settings['linesToShow']}
                        onChange={this.onChangeLinesToShow}
                        style={{ marginRight: 12 }}
                    />
                    <Button onClick={this.onClickLinesShowAll} style={{ marginRight: 12 }}>All</Button>
                    <Text>Lines to show</Text>
                </Menu.Item>
            </Menu>
        </Drawer>);
    }
}

export default connect('settings', SettingsDrawer);