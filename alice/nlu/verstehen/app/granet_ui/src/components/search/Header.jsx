import React from 'react';
import Menu from 'antd/es/menu';
import Dropdown from 'antd/es/dropdown';
import Icon from 'antd/es/icon';
import connect from 'storeon/react/connect';

class Header extends React.Component {
    constructor(props) {
        super(props);
        this.handleIndicesMenuClick = this.handleIndicesMenuClick.bind(this);
    }

    componentDidMount() {
        this.props.dispatch('apps/load');
    }

    handleIndicesMenuClick(e) {
        this.props.dispatch('apps/curIndex/update', e.key);
        this.props.dispatch('searchResultsVerstehen/search', e.key);
    }

    render() {
        const indices = (this.props.apps['appsIndices'] === undefined ||
            this.props.apps['appsIndices'][this.props.apps['current']] === undefined) ?
            [] 
            : 
            this.props.apps['appsIndices'][this.props.apps['current']]['indexes'];
        const menu = (
            <Menu onClick={this.handleIndicesMenuClick}>
                {indices.filter((idx) => { return idx !== "Granet" }).map((idx) => {
                    return <Menu.Item key={idx}>
                        {idx}
                    </Menu.Item>
                })}
            </Menu>
        );

        return (
            <div style={{ margin: "0px 12px", height: "32px", display: "flex", justifyContent: 'space-between', alignItems: 'center' }}>
                {this.props.searchIdx === 'Granet' ? ( 
                    <div>{this.props.searchIdx}</div>
                ) : (
                    <Dropdown overlay={menu}>
                        <div>
                            {(this.props.apps['curIndex'] === undefined) ? 'None' : this.props.apps['curIndex']} <Icon type="down" />
                        </div>
                    </Dropdown>
                )}
                <div>{(this.props.time === undefined) ? undefined : this.props.time + " ms"}</div>
            </div>
        );
    }
}

export default connect('apps', 'searchResultsVerstehen', 'searchResultsGranet', Header)
