import { search_commands, SearchPanelId, SearchPanel } from './simpleSearch';

const search = {
  activate: () => {
    search_commands.forEach(cmd => {
      let command_name = "search." + cmd.name;
      ashlar.commands.registerCommand(command_name, cmd.action);
      if (cmd.keys) {
          ashlar.keybinding.bindKeys({
              keys: cmd.keys,
              command: command_name
          });
      }
    });

    // todo.. defer/wait of engine to heat up
    setTimeout(() => {
      ashlar.ui.registerPanel(SearchPanelId, SearchPanel);
    }, 0);
  },

  deactivate: () => {}
};

ashlar.extensions.registerExtension("search", search);
