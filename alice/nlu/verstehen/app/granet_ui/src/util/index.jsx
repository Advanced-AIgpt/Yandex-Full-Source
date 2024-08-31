import React from 'react';

// Wrapper of function for measuring the time passed between returning of the funciton
// and its call.
export function timeWrapper(fn) {
    let startTime = new Date().getTime();
    return function (data) {
        fn(data, new Date().getTime() - startTime);
    }
}

export function text2pars(text) {
    const lines = text.split("\n")
    return (<code style={{whiteSpace: "pre", fontSize: "12px"}}>{
        lines.map((line) => {
            return <div>{line}<br/></div>;
            })
        }</code>);
}

// function to generate fairly good uuid equivalent. From https://stackoverflow.com/a/2117523/3934013
export function generate_uuidv4() {
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
        var r = Math.random() * 16 | 0, v = c == 'x' ? r : (r & 0x3 | 0x8);
        return v.toString(16);
    });
}