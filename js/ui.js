import React from "react";
import commands from "./commands";
import events from "./events";
import { useUI } from "./uiContext";
import debounce from 'debounce';

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
    if (options.cased) {
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
    if (options.cased) {
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
  const [ state, setState ] = React.useState({ find: props.selectedText, regex: false, cased: false, word: false });

  const onFindChanged = (evt) => {
    setState({
      ...state,
      find: evt.target.value
    })
  }
  const doSearch = () => {
    console.log(state.find);
    simpleSearch(state.find, state);
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
          <View id="panel::search::view" style={{flexDirection:'row'}}>
            <Button checked={state.regex} text='.*' style={styles.button} checkable={true} onClick={evt=>{ console.log(evt); setState({...state, regex:evt.target.value })}}/>
            <Button checked={state.cased} text='Aa' style={styles.button} checkable={true} onClick={evt=>{ setState({...state, cased:evt.target.value })}}/>
            <Button checked={state.word}  text='""' style={styles.button} checkable={true} onClick={evt=>{ setState({...state, word :evt.target.value })}}/>
            <TextInput id="panel::search::input" text={state.find} onChangeText={onFindChanged} onSubmitEditing={doSearch} style={styles.input}/>
            <Button text='Find' style={styles.button} onPress={doSearch}/>
          </View>
        </View>;
}

const AdvanceSearch = (props) => {
  return <View id="panel::advance_search" style={styles.panel}>
          <Text style={styles.input}>advance search is not yet implemented</Text>
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

    qt.widget(panel + '::input').then(widget => {
    if (widget) {
        widget.focus();
        widget.select();
      }
    }).catch(err => {
      console.log(err);
    });
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