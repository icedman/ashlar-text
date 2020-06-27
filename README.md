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

# extras
* animated sidebar

# needs improvement
* extension support
* ui: fuzzy command palette (somewhat working)
* ui: fuzzy file search (somewhat working)
* ui: menu (somewhat working)
* auto complete (QCompleter doesn't always work as expected)
* ui in general (stabilize js-qt - too many.. create destroy.. add cached widgets)
* ui improve flatlist (currently limited to 20 items)

# todo
* file search, fuzzy search
* file system watch (on rename)
* bracket pairing
* git gutter
* ui: dialog
* package management
* terminal
* qprocess extension
* icon fonts (fontawesome?)
* handle paste of large files

# issues
* problem with very long lines
* re-render minimap on resize window

# wish list
* atom or vscode extensions compatibility - possible at all?

# progress screenshots

![early shots](https://raw.githubusercontent.com/icedman/ashlar-text/master/screenshots/Screenshot%20from%202020-05-28%2022-55-06.png)
![early shots](https://github.com/icedman/ashlar-text/blob/master/screenshots/Screenshot%20from%202020-06-05%2023-10-30.png)
