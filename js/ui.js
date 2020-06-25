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
  StatusBar,
  StyleSheet
} from "./lib/core";

import qt from "./lib/engine";

let TouchState = () => {};
let RequestPanel = id => {};

const panelRegistry = {};
const statusRegistry = {};

const ShowStatus = (msg, timeout) => {
  ashlar.qt
    .widget("statusBar")
    .then(widget => {
      if (widget) {
        widget.showMessage(msg, timeout);
      }
    })
    .catch(err => {
      console.log(err);
    });
};

const KeyListener = key => {
  if (key === "esc") {
    RequestPanel("");
  }
};

export const Widgets = () => {
  const ui = useUI();
  const state = ui.state;

  const showPanel = ({ panel }) => {
    ui.dispatch(
      ui.setState({
        ...state,
        panel: panel
      })
    );
  };

  const touchState = () => {
    ui.dispatch(
      ui.setState({
        ...state,
        update: uuid()
      })
    );
  };

  TouchState = touchState;
  RequestPanel = showPanel;

  React.useEffect(() => {
    events.on("keyPressed", KeyListener);
    events.on("requestPanel", RequestPanel);
    return () => {
      events.off("keyPressed", KeyListener);
      events.off("requestPanel", RequestPanel);
    };
  }, []);

  let show = state.panel && state.panel.length ? true : false;
  let renderedPanels = Object.keys(panelRegistry).map((k, idx) => {
    const Component = panelRegistry[k];
    return <Component key={`${k}-${idx}`}></Component>;
  });
  let renderedStatus = Object.keys(statusRegistry).map((k, idx) => {
    const Component = statusRegistry[k];
    return <Component key={`${k}-${idx}`}></Component>;
  });

  return (
    <React.Fragment>
      <pre>{JSON.stringify(state, null, 4)}</pre>
      <StackedView id="panels" current={state.panel} style={{ visible: show }}>
        {renderedPanels}
      </StackedView>
      <StatusBar id="statusBar">{renderedStatus}</StatusBar>
    </React.Fragment>
  );
};

export const ui = {
  registerStatus: (id, status) => {
    statusRegistry[id] = status;
    TouchState();
  },

  registerPanel: (id, panel) => {
    panelRegistry[id] = panel;
    TouchState();
  },

  showPanel: id => {
    events.emit("requestPanel", { panel: id });
  },

  showStatus: (msg, timeout) => {
    ShowStatus(msg, timeout);
  },

  React,
  useUI,
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
    StatusBar,
    StyleSheet
  }
};
