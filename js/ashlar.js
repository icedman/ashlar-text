import React from "react";
import events from "./events";

import commands from "./commands";
import extensions from "./extensions";
import keybinding from "./keybinding";

// ui
import { StoreProvider as UIProvider } from "./uiContext";
import { ui, Panels } from "./ui";

const ashlar = {
  commands,
  extensions,
  keybinding,
  events,
  ui
};

window.ashlar = ashlar;

 /*
 * base commands
 */
/* prettier-ignore */
const baseCommands = [
    { name: "exit",                     action: () => { app.exit(); }},
    { name: "show_inspector",           action: (showHtml) => { app.showInspector(showHtml === true); }},
    { name: "toggle_comment",           action: () => { app.toggleComment(); }},
    { name: "toggle_block_comment",     action: () => { app.toggleBlockComment(); }},
    { name: "indent",                   action: () => { app.indent(); }},
    { name: "unindent",                 action: () => { app.unindent(); }},
    { name: "duplicate_line",           action: () => { app.duplicateLine(); }},
    { name: "expand_selection_to_line", action: () => { app.expandSelectionToLine(); }},
    { name: "find_and_create_cursor",   action: () => { app.findAndCreateCursor(app.selectedText()); }},
    { name: "zoom_in",                  action: () => { app.zoomIn(); }},
    { name: "zoom_out",                 action: () => { app.zoomOut(); }},
    { name: "new_tab",                  action: () => { app.newTab(); }},
    { name: "close_tab",                action: () => { app.closeTab(); }},
    { name: "open_tab",                 action: (tabIndex) => { app.tab(tabIndex); }}
]

baseCommands.forEach(cmd => {
  commands.registerCommand(cmd.name, cmd.action);
});

export const App = () => {
  return (
    <UIProvider>
      <Panels />
    </UIProvider>
  );
};
