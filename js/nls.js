const registry = {};

const nls = {
    registerNLS: (name, def) => {
        registry[name] = fn;

        console.log('nls registered: ' + name);
    },

    queryNLS: (name) => {
        return registry[name] || name;
    },

    definitions: () => {
        return registry;
    }
};

export default nls;
