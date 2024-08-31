import React from 'react';
import Col from 'antd/es/col';

import ResultsTable from './ResultsTable';

class SearchTab extends React.Component {
    render() {
        return (
            <div>
                <Col span={12}>
                    <ResultsTable searchIdx='Granet' searchResultCol='searchResultsGranet'/>
                </Col>
                <Col span={12}>
                    <ResultsTable searchIdx='verstehen' searchResultCol='searchResultsVerstehen'/>
                </Col>
            </div>
        );
    }
}

export default SearchTab;