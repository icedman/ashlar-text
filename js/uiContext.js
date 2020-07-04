import React from 'react';
import { v4 as uuid } from 'uuid';

export const Store = React.createContext();

let initialState = JSON.parse(window.localStorage.getItem('ui', '{}'));

if (!initialState || !initialState.ui) {
    initialState = {};
}
/* params: { path:value } */
export function setState(params) {
    return {
        type: 'SET_STATE',
        ...params
    };
}

export function reducer(state, action) {
    switch (action.type) {
        case 'SET_STATE':
            let newState = { ...state, ...action };
            delete newState.type;
            window.localStorage.setItem('ui', JSON.stringify(newState));
            return newState;
        default:
            return state;
    }
}

export function StoreProvider(props) {
    const config = { ...initialState, ...(props.config || {}) };
    const [state, dispatch] = React.useReducer(reducer, config);
    const value = { state, dispatch, setState };
    return <Store.Provider value={value}>{props.children}</Store.Provider>;
}

export function useUI() {
    return React.useContext(Store);
}
