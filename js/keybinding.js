import commands from "./commands";
import events from "./events";

const key_mapping = {};
const last_key = {
  time: 0,
  keys: ""
};

const keybinding = {
  processKeys: k => {
    // console.log(k);
    let kb = key_mapping[k];
    if (!kb) {
      return false;
    }
    // prevent double call (both mainwindow & editor are trapping keyevents)
    let t = new Date().getTime();
    if (last_key.keys == k && t - last_key.time < 150) {
      return false;
    }
    last_key.time = t;
    last_key.keys = k;
    try {
      commands.executeCommand(kb.command, kb.args);
    } catch (err) {
      console.log(err);
    }
    return true;
  },

  loadMap: m => {
    try {
      let jm = m; //  JSON.parse(m);
      jm.forEach(kb => {
        key_mapping[kb["keys"]] = kb;
      });

      // app.log(JSON.stringify(key_mapping));
    } catch (err) {
      console.log(err);
    }
  }
};

console.log("js::keybinding");

window.keybinding = keybinding;
export default keybinding;
