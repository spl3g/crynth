# *CR*appy s*YNTH*
Just a crappy synth using alsa and raylib. Works only on linux.

## Building
If you use nix, you can just run it like that:
```sh
nix run github:spl3g/crynth
```

For other systems you have to have these libraries:
- raylib
- asoundlib

```sh
cc -o nob nob.c
./nob
./build/crynth
```
