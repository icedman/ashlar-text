import React from 'react';
import ashlar from './ashlar';

import { StoreProvider as UIProvider } from './uiContext';
import { ui, Widgets } from './ui';

import StatusBar from './ui/statusbar';

const activate = () => {
    StatusBar.activate();

    ashlar.events.on('processOut', p => {
        console.log(p);
    });
    ashlar.events.on('processFinished', p => {
        console.log(p);
        console.log('done');
    });

    // this will fetch the second level files & then third level files
    setTimeout(app.allFiles, 1500);
    setTimeout(app.allFiles, 2500);
};

const deactivate = () => {
    StatusBar.deactivate();
};

const App = () => {
    React.useEffect(() => {
        activate();
        return () => {
            deactivate();
        };
    }, []);

    return (
        <UIProvider>
            <Widgets />
        </UIProvider>
    );
};

window.testGit = () => {
    ps.run(ps.git(), ['--version']);
};
// window.testGit = () => { ps.run(ps.git(), ["--no-pager", "diff"]) };

export default App;
