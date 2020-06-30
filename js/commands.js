import keybinding from './keybinding.js';

const registry = {};

const commands = {
    registerCommand: (name, fn, keys) => {
        registry[name] = fn;

        if (keys) {
            keybinding.bindKeys({
                keys: keys,
                command: name
            });
        }

        console.log('command registered: ' + name);
    },

    executeCommand: (name, args) => {
        if (registry[name]) {
            registry[name](args);
        }
    },

    commands: () => {
        return registry;
    }
};

export default commands;
