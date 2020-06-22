const hello_commands = [
  {
    name: "say_hello",
    action: () => {
      console.log('hello world');
    }
  }
];

const hello = {
  activate: () => {
    hello_commands.forEach(cmd => {
      ashlar.commands.registerCommand("hello_world.say_hello", cmd.action);
    });
  },

  deactivate: () => {}
};

ashlar.extensions.registerExtension("hello-world", hello);
