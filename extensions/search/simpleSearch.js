const { React, useUI } = ashlar.ui;

const {
    View,
    Text,
    Image,
    TextInput,
    Button,
    Switch,
    ScrollView,
    SplitterView,
    StackedView,
    FlatList,
    SectionList,
    Window,
    StyleSheet
} = ashlar.ui.core;

const SearchPanelId = 'panel::search';

const styles = StyleSheet.create({
    panel: {
        flexDirection: 'column'
    },
    panelRow: {
        flexDirection: 'row'
    },
    input: {
        margin: 4
    },
    icon: {
        margin: 2,
        padding: 4,
        border: 'none',
        iconWidth: 16,
        iconHeight: 16
    },
    button: {
        margin: 2
    }
});

const simpleSearch = (keywords, options) => {
    options = options || {};
    let searchOps = [];

    if (options.regex) {
        searchOps.push('regular_expression');
    }
    if (options.cased) {
        searchOps.push('case_sensitive');
    }
    if (options.word) {
        searchOps.push('whole_word');
    }
    if (options.wrap) {
        searchOps.push('wrap_around');
    }
    if (options.prev) {
        searchOps.push('search_up');
    }

    console.log(JSON.stringify(searchOps));
    console.log('search for ' + keywords);
    app.find(keywords, searchOps.join(','));
};

let setSelectedText;
const SearchPanel = props => {
    const [state, setState] = React.useState({
        find: '',
        regex: false,
        cased: false,
        word: false,
        wrap: true
    });

    const onFindChanged = evt => {
        setState({
            ...state,
            find: evt.target.value
        });
    };

    const onSearch = () => {
        // console.log(state.find);
        simpleSearch(state.find, state);
    };
    
    const onSearchPrev = () => {
        simpleSearch(state.find, {...state, prev: true });
    };

    setSelectedText = onFindChanged;

    /* prettier-ignore */
    return <View id={SearchPanelId} style={styles.panel}>
        <View id='panel::search::view'  style={{'flex-direction': 'row'}}>
          <Button text='.*' style={styles.icon} checkable onClick={(evt)=>{ setState({...state, regex: evt.target.value}); }}/>
          <Button text='Aa' style={styles.icon} checkable onClick={(evt)=>{ setState({...state, cased: evt.target.value}); }}/>
          <Button text='""' style={styles.icon} checkable onClick={(evt)=>{ setState({...state, word:  evt.target.value}); }}/>
          <Button icon='wrap' style={styles.icon} checked={state.wrap} checkable onClick={(evt)=>{ setState({...state, wrap:  evt.target.value}); }}/>
          <TextInput id='panel::search::input' text={state.find} style={styles.input}
              onChangeText={onFindChanged}
              onSubmitEditing={onSearch}
          />
          <Button text='Find' style={styles.button} onClick={onSearch}/>
          <Button text='Find Prev' style={styles.button} onClick={onSearchPrev}/>
        </View>
      </View>
};

/* commands */
const show_search = args => {
    ashlar.events.emit('requestPanel', { panel: SearchPanelId });

    ashlar.qt
        .widget(SearchPanelId + '::input')
        .then(widget => {
            if (widget) {
                widget.focus();
                widget.select();
            }
        })
        .catch(err => {
            console.log(err);
        });

    setSelectedText({
        target: {
            value: app.selectedText()
        }
    });
};

const search_commands = [
    {
        name: 'show_search',
        action: () => {
            show_search();
        },
        keys: 'ctrl+f'
    }
];

export { search_commands, SearchPanel, SearchPanelId };
