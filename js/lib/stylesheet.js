import React from 'react';

const camelToDash = str => str
    .replace(/(^[A-Z])/, ([first]) => first.toLowerCase())
    .replace(/([A-Z])/g, ([letter]) => `-${letter.toLowerCase()}`);

const styleMap = {
    'background-color': 'background'
}

const styleExclude = [
]

const distillStyle = (style) => {
    let res = {}
    Object.keys(style).forEach(k => {
        let v = style[k];
        k = camelToDash(k)
        switch(k) {
            case 'margin-horizontal':
                res['margin-left'] = v;
                res['margin-right'] = v;
            return;
            case 'margin-vertical':
                res['margin-top'] = v;
                res['margin-bottom'] = v;
            return;
        }
        if (styleMap[k]) {
            res[styleMap[k]] = v;
            return;
        }

        if (styleExclude.indexOf(k) != -1) {
            return;
        }
        res[k] = style[k];
    })
    return res;
}

const StyleSheet = {
    create: (styles) => {
        let res = {};
        Object.keys(styles).forEach(k => {
            res[k] = distillStyle(styles[k]);
        })
        console.log(res);
        return res;
    }
}

export default StyleSheet