import { search_commands, SearchPanelId, SearchPanel } from './simpleSearch';

const search = {
  activate: () => {
    search_commands.forEach(cmd => {
      ashlar.commands.registerCommand("search." + cmd.name, cmd.action);
    });

    setTimeout(() => {
      ashlar.ui.registerPanel(SearchPanelId, SearchPanel);
    }, 0);
  },

  deactivate: () => {}
};

ashlar.extensions.registerExtension("search", search);
