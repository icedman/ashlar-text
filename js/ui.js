import React from "react";
import commands from "./commands";
import events from "./events";
import { useUI } from "./uiContext";

import { Window, Text, TextInput, View, StackedView, Button, StyleSheet } from "./lib/core";
import qt from './lib/engine';

let ShowPanel = (panel) => {};

const keyListener = (key) => {
  if (key === 'esc') {
    ShowPanel('');
  }
}

const simpleSearch = (keywords, options) => {
       
    options = options || {};
    let searchOps = [];
    
    if (options.regex) {
        searchOps.push("regular_expression");
    }
    if (options.case) {
        searchOps.push("case_sensitive");
    }
    if (options.word) {
        searchOps.push("whole_word");
    }
    
    // console.log(JSON.stringify(searchOps));
    // console.log("search for " + keywords);
    app.find(keywords, searchOps.join(','));
}
    
const fileSearch = (keywords, replace, where, options) => {
       
    options = options || {};
    let searchOps = [];
    
    if (options.regex) {
        searchOps.push("regular_expression");
    }
    if (options.case) {
        searchOps.push("case_sensitive");
    }
    if (options.word) {
        searchOps.push("whole_word");
    }
    console.log("advance search for " + keywords);
    console.log("not yet working");
    // app.find(keywords, searchOps.join(','));
}

const Search = (props) => {
  const [ state, setState ] = React.useState({ find: props.selectedText });

  const onFindChanged = (evt) => {
    setState({
      ...state,
      find: evt.target.value
    })
  }
  const doSearch = () => {
    console.log(state.find);
    simpleSearch(state.find);
  }

  React.useEffect(() => {
    console.log(props.selectedText);
    onFindChanged({
      target: {
        value: props.selectedText
      }
    })
  }, [ props.selectedText ]);

  return <View id="panel::search" style={styles.panel}>
          <View style={{flexDirection:'row'}}>
            <Button text='.*' style={styles.button}/>
            <Button text='Aa' style={styles.button}/>
            <Button text='❝❞' style={styles.button}/>
            <TextInput id="panel::search::input" text={state.find} onChangeText={onFindChanged} onSubmitEditing={doSearch} style={styles.input}/>
            <Button text='Find' style={styles.button} onPress={doSearch}/>
          </View>
        </View>
}

const AdvanceSearch = (props) => {
  return <View id="panel::advance_search">
          <Text>advance search</Text>
        </View>
}

export const Panels = () => {
  const ui = useUI();
  const state = ui.state;

  const showPanel = panel => {
    ui.dispatch(
      ui.setState({
        panel: panel
      })
    );

    setTimeout(() => {
      let widget = window.$widgets[panel + '::input'] ? window.$widgets[panel + '::input'].$widget : null;
      if (widget) {
        widget.focus();
        widget.select();
      }
    }, 5);

  };

  ShowPanel = showPanel;

  React.useEffect(() => {
    events.on("keyPressed", keyListener);
    return () => {
      events.off("keyPressed", keyListener);
    }
  }, []);

  let show = (state.panel && state.panel.length) ? true: false;
  let selectedText = app.selectedText();
  return (
    <React.Fragment>
      <StackedView id="panels" current={state.panel} style={{visible:show}}>
        <Search selectedText={selectedText}/>
        <AdvanceSearch selectedText={selectedText}/>
      </StackedView>
    </React.Fragment>
  );
};

commands.registerCommand("show_search", () => {
  ShowPanel('panel::search');
});

commands.registerCommand("show_advance_search", () => {
  ShowPanel('panel::advance_search');
});


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
  button: {
    margin: 2,
  }
})