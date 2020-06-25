import { fuzzy_commands, FuzzyListId, FuzzyList } from "./commandSearch";

const fuzzy = {
  activate: () => {
    fuzzy_commands.forEach(cmd => {
      let command_name = "fuzzy." + cmd.name;
      ashlar.commands.registerCommand(command_name, cmd.action, cmd.keys);
    });

    ashlar.ui.registerPanel(FuzzyListId, FuzzyList);
  },

  deactivate: () => {}
};

ashlar.extensions.registerExtension("fuzzy", fuzzy);
