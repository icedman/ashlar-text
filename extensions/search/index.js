const E = ashlar.ui.React.createElement;

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

const panelId = "panel::search";

const styles = StyleSheet.create({
  panel: {
    flexDirection: "column"
  },
  panelRow: {
    flexDirection: "row"
  },
  input: {
    margin: 4
  },
  button: {
    margin: 2
  }
});

const simpleSearch = (keywords, options) => {
  options = options || {};
  let searchOps = [];

  if (options.regex) {
    searchOps.push("regular_expression");
  }
  if (options.cased) {
    searchOps.push("case_sensitive");
  }
  if (options.word) {
    searchOps.push("whole_word");
  }

  console.log(JSON.stringify(searchOps));
  console.log("search for " + keywords);
  app.find(keywords, searchOps.join(","));
};

let setSelectedText;
const SearchPanel = props => {
  const [state, setState] = React.useState({
    find: "",
    regex: false,
    cased: false,
    word: false
  });

  const onFindChanged = evt => {
    setState({
      ...state,
      find: evt.target.value
    });
  };

  const onSearch = () => {
    console.log(state.find);
    simpleSearch(state.find, state);
  };
  
  setSelectedText = onFindChanged;
  
  /* prettier-ignore */
  return (
        E(View, { id: panelId, styles: styles.panel }, [
        E(View, { id:'panel::search::view', style: {flexDirection: 'row'} }, [
            E(Button, { text:'.*', style: styles.button, checkable:true, onClick: (evt)=>{ setState({...state, regex:evt.target.value })}} ),
            E(Button, { text:'Aa', style: styles.button, checkable:true, onClick: (evt)=>{ setState({...state, cased:evt.target.value })}} ),
            E(Button, { text:'""', style: styles.button, checkable:true, onClick: (evt)=>{ setState({...state, word :evt.target.value })}} ),
            E(TextInput, { id:'panel::search::input', text: state.find, 
                 onChangeText: onFindChanged,
                 onSubmitEditing: onSearch, style: styles.input }),
            E(Button, { text:'Find', style: styles.button, onPress:onSearch })
        ])
        ])
    );
};

/* commands */
const show_search = args => {
  ashlar.events.emit("requestPanel", { panel: panelId });

  setTimeout(() => {
    ashlar.qt
      .widget(panelId + "::input")
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
      })
  }, 50);
};

const search_commands = [
  {
    name: "show_search",
    action: () => {
      show_search();
    }
  }
];

const search = {
  activate: () => {
    search_commands.forEach(cmd => {
      ashlar.commands.registerCommand("search." + cmd.name, cmd.action);
    });

    setTimeout(() => {
      ashlar.ui.registerPanel(panelId, SearchPanel);
    }, 0);
  },

  deactivate: () => {}
};

ashlar.extensions.registerExtension("search", search);
