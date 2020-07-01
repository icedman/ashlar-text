This app is a simple SublimeText replacement.
It uses the textmate parser. And reuses some visual studio code extensions.

![screenshot](https://raw.githubusercontent.com/icedman/ashlar-text/master/screenshots/Screenshot%20from%202020-06-26%2019-37-10.png)

# native
This is not electron based like atom, vscode, brackets. It uses native qt/c++. At its bare state, it is lightning fast compared to these editors.
It does however uses webkit as its scripting engine - with the aim to easily for atom/vscode plugins to ashlar.

# features
* syntax highlighting
* keybindings
* scripting
* webkit inspector
* themes
* search
* minimap
* multi-cursor
* completions
* ui: panel (search dialog)
* ui: statusbar
* smooth-scroll
* auto-close
* auto-indent
* bracket matching
* bracket folding
* ui: fuzzy file search

# extras
* animated sidebar

# needs improvement
* extension support
* ui: fuzzy command palette (use property command title/nls)
* ui: menu (somewhat working)
* auto complete (QCompleter doesn't always work as expected - not fuzzy)
* ui improve flatlist (currently limited to 20 items)
* bracket pairing

# todo
* preview mode
* file search, fuzzy search
* file system watch (on rename)
* git gutter
* ui: dialog
* package management
* terminal
* qprocess extension

# needs refactoring
* commands
* editor/tmedit code
* everything

# issues
* re-render minimap on resize window

# wish list
* atom or vscode extensions compatibility - possible at all?
* provide wrapper/proxy for atom/vscode modules

# progress screenshots

![early shots](https://raw.githubusercontent.com/icedman/ashlar-text/master/screenshots/Screenshot%20from%202020-05-28%2022-55-06.png)
![early shots](https://github.com/icedman/ashlar-text/blob/master/screenshots/Screenshot%20from%202020-06-05%2023-10-30.png)
