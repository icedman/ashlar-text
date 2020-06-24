const { React, useUI } = ashlar.ui;

const {
  View,
  Text,
  Image,
  TextInput,
  Button,
  Switch,
  ScrollView,
  SplitterView,
  StackedView,
  FlatList,
  SectionList,
  Window,
  StyleSheet
} = ashlar.ui.core;

const hello_commands = [
  {
    name: "say_hello",
    action: () => {
      console.log('status widget: hello world');
    }
  }
];

const StatusHello = (props) => {
    return <Text>status::widget Hello World</Text>
}

const hello = {
  activate: () => {
    hello_commands.forEach(cmd => {
      ashlar.commands.registerCommand("hello_world.say_hello", cmd.action);
    });
  },

  deactivate: () => {}
};

ashlar.extensions.registerExtension("hello-world", hello);
