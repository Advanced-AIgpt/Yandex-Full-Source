import ReactDOM from "react-dom";
import { BrowserRouter as Router, Switch, Route } from "react-router-dom";

import { JupyTicketPage } from "./jupyticket";
import { Collapse } from "bootstrap";

import "../style/index.scss";

const App = () => {
    const jhdata = window.jhdata;

    // initialize navbar collapsing in external navbar html
    const nav = document.querySelector('[data-bs-target="#thenavbar"]');
    Collapse.getOrCreateInstance(nav);

    // TODO: add default route to print error about not matching any route
    return (
        <Router basename={jhdata.base_url}>
            <Switch>
                <Route path="/jupyticket/:jupyTicketId" component={JupyTicketPage} />
            </Switch>
        </Router>
    );
};

const rootEl = document.getElementById("jc_container");

ReactDOM.render(<App />, rootEl);
