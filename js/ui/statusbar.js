import React from 'react';
import ashlar from '../ashlar';

const { View, Text, StyleSheet } = ashlar.ui.core;

const styles = StyleSheet.create({
    bar: {
        flexDirection: 'row',
        background: 'transparent'
    },
    language: {
        fontSize: '10pt',
        marginLeft: 8,
        marginRight: 8,
        background: 'transparent',
        minWidth: 80
    },
    cursor: {
        fontSize: '10pt',
        marginLeft: 8,
        marginRight: 8,
        background: 'transparent',
        minWidth: 120
    }
});

// Note: These are default app behaviors and widgets. Do not refer here for extension sample.
let UpdateCursor;
let UpdateLanguage;

const tabSelected = e => {
    UpdateLanguage();
};

const tabClosed = e => {
    UpdateLanguage();
};

const cursorPositionChanged = e => {
    UpdateCursor();
};

const EditorStatus = props => {
    const [state, setState] = React.useState({
        cursor: app.cursor(),
        language: app.language()
    });
    const updateCursor = () => {
        setState({
            ...state,
            cursor: app.cursor()
        });
    };

    const updateLanguage = () => {
        setState({
            ...state,
            language: app.language()
        });
    };

    UpdateCursor = updateCursor;
    UpdateLanguage = updateLanguage;
    return (
        <View id="status::editor" permanent={true} style={styles.bar}>
            <Text id="status::cursor" style={styles.cursor}>
                Line: {state.cursor[0] + 1} Column: {state.cursor[1] + 1}
            </Text>
            <Text id="status::language" style={styles.language}>
                {state.language}
            </Text>
            {JSON.stringify(styles)}
        </View>
    );
};

export default {
    activate: () => {
        ashlar.ui.registerStatus('status::editor', EditorStatus);
        ashlar.events.on('tabSelected', tabSelected);
        ashlar.events.on('tabClosed', tabClosed);
        ashlar.events.on('cursorPositionChanged', cursorPositionChanged);
    },

    deactivate: () => {
    }
}
