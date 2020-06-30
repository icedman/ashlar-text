import { search_commands, SearchPanelId, SearchPanel } from './simpleSearch';

const search = {
    activate: () => {
        search_commands.forEach(cmd => {
            let command_name = 'search.' + cmd.name;
            ashlar.commands.registerCommand(command_name, cmd.action, cmd.keys);
        });

        ashlar.ui.registerPanel(SearchPanelId, SearchPanel);
    },

    deactivate: () => {}
};

ashlar.extensions.registerExtension('search', search);
