const registry = {};

const commands = {
  registerCommand: (name, fn) => {
    registry[name] = fn;
  },

  executeCommand: (name, args) => {
    if (registry[name]) {
      registry[name](args);
    }
  }
};

export default commands;
