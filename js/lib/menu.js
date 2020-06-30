import React from 'react';
import View from './view';

const MenuBar = props => {
    return <View {...props} type="MenuBar" />;
};

const Menu = props => {
    return <View {...props} type="Menu" />;
};

const MenuItem = props => {
    return <View {...props} type="MenuItem" />;
};

export { MenuBar, Menu, MenuItem };
