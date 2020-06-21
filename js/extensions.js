const registry = {};

const extensions = {
  registerExtension: (name, ext) => {
    if (registry[name]) {
        registry[name].deactivate();
    }
    registry[name] = ext;
    console.log(name + ' registered');
  },

  activate: (name) => {
    if (registry[name] && !registry[name].activated) {
      registry[name].activate();
      registry[name].activated = true;
      console.log(name + ' activated');
    }
  }
};

export default extensions;
