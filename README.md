# OSCMessenger

Send the output of any UGen via OSC messages from *scsynth* to any other program.
This extends `SendReply` by not relying on *sclang* as an additional dispatcher anymore and has a *doneMessage* feature.
Currently this is limited to control rate.

## Build

Make sure you have the SuperCollider source code locally available and set it as an environment variable `SC_SRC_PATH`.

See all configuration preset via

```shell
cmake --list-presets
```

Choose one and prepend the path to your local SuperCollider source files, e.g. for macOS this becomes

```shell
SC_SRC_PATH=/path/to/src/of/SuperCollider cmake --preset macOS
```

Build the extension via

```shell
cmake --build build --target install --config Release
```

All necessary files for the plugin can then be found in `install` folder which then need to be copied into the extension directory.

```supercollider
Platform.systemExtensionDir.openOS;
Platform.userExtensionDir.openOS;
```

## License

AGPL-3.0
