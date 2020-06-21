const E = ashlar.ui.React.createElement;

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

const HelloView = props => {
  const onClick = evt => {
    console.log("some kind of click!");
  };
  return E(View, { id: "panel::hello-world" }, [
    E(Text, { key: "t", text: "Hello World" }),
    E(Button, { key: "b", text: "Click Me", onClick })
  ]);
};

const keyListener = key => {
  console.log("hello:" + key);
};

const say_hello = evt => {
  console.log("hello world!!!");
  ashlar.commands.executeCommand("new_tab");
};

const hello = {
  activate: () => {
    // ashlar.events.on("keyPressed", keyListener);
    ashlar.commands.registerCommand("hello_world.say_hello", say_hello);
    // ashlar.events.emit('registerPanel', { id: 'hello-world', panel: HelloView });
  },

  deactivate: () => {
    // ashlar.events.off("keyPressed", keyListener);
  }
};

ashlar.extensions.registerExtension("hello-world", hello);
