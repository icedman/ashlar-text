import React from "react";
import commands from "./commands";
import events from "./events";
import { useUI } from "./uiContext";
import { v4 as uuid } from "uuid";

import {
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
} from "./lib/core";

import qt from './lib/engine';

let RequestPanel = (id) => {};

const registry = {};

const KeyListener = (key) => {
  if (key === 'esc') {
    RequestPanel('');
  }
}

export const Panels = () => {
  const ui = useUI();
  const state = ui.state;

  const showPanel = ({ panel }) => {
    ui.dispatch(
      ui.setState({
        panel: panel
      })
    );
  };

  RequestPanel = showPanel;
  
  React.useEffect(() => {
    events.on("keyPressed", KeyListener);
    events.on("requestPanel", RequestPanel);
    return () => {
      events.off("keyPressed", keyListener);
      events.off("requestPanel", RequestPanel);
    }
  }, []);

  let show = (state.panel && state.panel.length) ? true: false;
  let renderedPanels= Object.keys(registry).map((k, idx) => {
    const ExtraPanel = registry[k];
    return <ExtraPanel key={`${k}-${idx}`}></ExtraPanel>
  });
 
  return (
    <React.Fragment>
      <pre>{JSON.stringify(state, null, 4)}</pre>
      <StackedView id="panels" current={state.panel} style={{visible:show}}>
        {renderedPanels}
      </StackedView>
    </React.Fragment>
  );
};

export const ui = {
    registerPanel: (id, panel) => {
        registry[id] = panel;
    },
    React,
    useUI,
    uuid,
    core: {
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
    }
}