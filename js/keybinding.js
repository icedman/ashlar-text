import commands from './commands';
import events from './events';

const key_mapping = {};
const last_key = {
    time: 0,
    keys: ''
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
            return true;
        }
        last_key.time = t;
        last_key.keys = k;
        try {
            // console.log(kb.command);
            commands.executeCommand(kb.command, kb.args);
        } catch (err) {
            console.log(err);
        }
        return true;
    },

    bindKeys: kb => {
        key_mapping[kb['keys']] = kb;
    },

    loadMap: m => {
        m.forEach(kb => {
            key_mapping[kb['keys']] = kb;
        });
    }
};

console.log('js::keybinding');

window.keybinding = keybinding;
export default keybinding;
