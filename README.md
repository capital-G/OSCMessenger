# OSCMessenger

Send the output of any UGen via OSC messages from *scsynth* to any other program.
This extends [`SendReply`](https://docs.supercollider.online/Classes/SendReply.html) by not relying on *sclang* as an additional dispatcher anymore and has a *doneMessage* feature.

## Example

```supercollider
// start some signal
(
Ndef(\myOscBridge, {
	var sig = SinOsc.ar(0.4);
	OSCMessenger.kr(
		portNumber: 5553,
		oscAddress: "/sine",
		trigger: 60.0,
		values: sig,
		doneAddress: "/doneSine",
		doneValue: 1.0,
	);
	sig;
});
)
```

Now receive the values of the SinOsc via any application which can listens for OSC messages.
For sclang this becomes

```supercollider
// normal messages
OSCdef(\myOsc, {|m| m.postln}, path: "/sine", recvPort: 5553);
// [ /helloSine, 0.044355537742376 ]

// done message
OSCdef(\stopOsc, {|m| "received done".postln; m.postln}, path: "/doneSine", recvPort: 5553)
Ndef(\myOscBridge).clear;
// [ /stopSine, 1.0 ]
```

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
