# *CR*appy s*YNTH*
> [!WARNING]
> This software is unfinished. Keep your expectations low.  
> Right now you can play some notes with it by pressing keys on the bottom row of your keyboard

Just a crappy synth using alsa and SDL3. Works only on linux.

## Building
If you use nix, you can just run it like that:
```sh
nix run github:spl3g/crynth
```

For other systems you have to have these libraries:
- sdl3
- asoundlib

```sh
cc -o nob nob.c
./nob
./build/crynth
```
