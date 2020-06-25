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

const FuzzyListId = "fuzzy::commands";

const FuzzyListItem = ({ item }) => {
    return <Text>{item.title}</Text>
}

const FuzzyList = ({ item }) => {
    const data = [
        { title: 'hello', description: 'xxx' },
        { title: 'world', description: 'xxx' },
    ]
    
    return <FlatList
            parent='select::items'
            data={data}
            renderItem={FuzzyListItem}
            keyExtractor={(item, index) => item + index}
           >
        </FlatList>
};

/* commands */
const show_command_search = args => {
  console.log('here!');
};

const fuzzy_commands = [
  {
    name: "show_command_search",
    action: () => {
      show_command_search();
    },
    keys: "ctrl+m"
  }
];

export { fuzzy_commands, FuzzyList, FuzzyListId };
