import debounce from 'debounce';
import Fuse from 'fuse.js';
import { v4 as uuid } from 'uuid';

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

const FuzzyListId = 'fuzzy::commands';

const styles = StyleSheet.create({
    item: {
        padding: 4
    },
    title: {
        fontSize: '12pt'
    },
    description: {
        fontSize: '10pt'
    },
    input: {
        margin: 4
    }
});

const FuzzyListItem = ({ item, onItemSelect, index }) => {
    const onPress = evt => {
        onItemSelect({
            target: {
                ...evt.target,
                value: item
            }
        });
    };

    let desc = item.description || item.path;
    return (
        <View
            id={`fuzzy::item::${index}`}
            retained
            hoverable
            touchable
            style={styles.item}
            className="selectItem"
            onPress={onPress}
        >
            <Text id={`fuzzy::item::text::${index}`} retained style={styles.title}>{item.title}</Text>
            {desc ? <Text id={`fuzzy::item::desc::${index}`} retained style={styles.description}>{desc}</Text> : ''}
        </View>
    );
};

const options = {
    includeScore: true,
    minMatchCharLength: 1,
    limit: 20,
    keys: ['command', 'title', 'description', 'path']
};

let data = [];
let fuse = new Fuse(data, options);
let TouchState;

const FuzzyList = ({ item }) => {
    const [state, setState] = React.useState({
        find: '',
        data: data,
        update: uuid()
    });

    const onFindChanged = debounce(evt => {
        let d = [];
        if (evt.target.value !== '') {
            d = fuse.search(evt.target.value).map(r => r.item);
        }

        setState({
            ...state,
            find: evt.target.value,
            data: d
        });
    }, 150);

    const onSearch = evt => {
        console.log('execute search!');
        console.log(state.find);
    };

    const touchState = data => {
        setState({
            find: '',
            data: data,
            update: uuid()
        });
    };
    TouchState = touchState;

    const onItemSelect = evt => {
        let val = evt.target.value;
        if (val && val.command) {
            ashlar.commands.executeCommand(val.command);
        }
        if (val && val.path) {
            app.openFile(val.path);
        }
    };
    
    return (
        <React.Fragment>
            <TextInput
                id="select::input"
                // text={state.find}
                onChangeText={onFindChanged}
                onSubmitEditing={onSearch}
                style={styles.input}
            />
            <FlatList
                id="select::items::flatlist"
                parent="select::items"
                data={state.data}
                renderItem={FuzzyListItem}
                keyExtractor={(item, index) => item + index}
                extraData={{ onItemSelect }}
            ></FlatList>
        </React.Fragment>
    );
};

/* commands */
const show_command_search = args => {
    data = Object.keys(ashlar.commands.commands()).map(c => {
        return {
            title: c,
            command: c
        };
    });
    fuse = new Fuse(data, options);
    TouchState(data);
    setTimeout(() => {
        app.showCommandPalette();
    }, 50);
};

const show_file_search = args => {
    let files = app.allFiles();
    data = files.map(f => {
        let leafname = f
            .split('\\')
            .pop()
            .split('/')
            .pop();
        return { title: leafname, path: f };
    });
    fuse = new Fuse(data, { ...options, minMatchCharLength: 1 });
    TouchState([]);
    setTimeout(() => {
        app.showCommandPalette();
    }, 50);
};

const fuzzy_commands = [
    {
        name: 'show_command_search',
        action: () => {
            show_command_search();
        },
        keys: 'ctrl+p'
    },
    {
        name: 'show_file_search',
        action: () => {
            show_file_search();
        },
        keys: 'ctrl+shift+p'
    }
];

export { fuzzy_commands, FuzzyList, FuzzyListId };
