import debounce from "debounce";
import Fuse from "fuse.js";
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

const FuzzyListId = "fuzzy::commands";

const styles = StyleSheet.create({
  item: {
    padding: 4
  },
  input: {
    margin: 4
  }
});

const FuzzyListItem = ({ item, onItemSelect }) => {
  const onPress = (evt) => {
      onItemSelect({
          target: {
              ...evt.target,
              value: item
          }
      })
  }
  return <View hoverable touchable style={styles.item} className='selectItem' onPress={onPress}>
      <Text>{JSON.stringify(item)}</Text>
      </View>
};

const options = {
  includeScore: true,
  minMatchCharLength: 1,
  limit: 20,
  keys: ["command", "title", "description" ]
};

let data = [];
let fuse = new Fuse(data, options);
let TouchState;

setTimeout(() => {
  data = Object.keys(ashlar.commands.commands()).map(c => {
      return {
          command: c
      }
  });
  fuse = new Fuse(data, options);
  TouchState(data);
}, 1000);

const FuzzyList = ({ item }) => {
  const [state, setState] = React.useState({
    find: "",
    data: data,
    update: uuid()
  });

  const onFindChanged = debounce(evt => {
    let d = data;
    if (evt.target.value !== '') {
      d = fuse.search(evt.target.value).map(r => r.item);
    }
    
    setState({
      ...state,
      find: evt.target.value,
      data: d
    });
  }, 100);

  const onSearch = evt => {
    console.log("execute search!");
    console.log(state.find);
  };

  const touchState = (data) => {
    setState({
        ...state,
        data: data,
        update: uuid()
    })
  }
  TouchState = touchState;
  
  const onItemSelect = (evt) => {
      let val = evt.target.value;
      if (val && val.command) {
          ashlar.commands.executeCommand(val.command);
      }
  }
  
  return (
    <React.Fragment>
      <TextInput
        id="select::input"
        text={state.find}
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
        extraData={{onItemSelect}}
      ></FlatList>
    </React.Fragment>
  );
};

/* commands */
const show_command_search = args => {
  console.log("here!");
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
