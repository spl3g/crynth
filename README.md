# *CR*appy s*YNTH*
> [!WARNING]
> This software is unfinished. Keep your expectations low.  
> Right now you can play some notes with it by pressing keys on the bottom row of your keyboard

<p>
<img width="322" alt="image" align="left" src="https://github.com/user-attachments/assets/923aa692-953d-466d-ab9b-70353bf7b018" />
Just a crappy synth using alsa and sokol. Works only on linux.
</p>
<br/>
<br/>
<br/>
<br/>
<br/>
<br/>
<br/>

## Building
If you use nix, you can just run it like that:
```sh
nix run github:spl3g/crynth
```

For other systems you have to have these libraries:
- libGL
- libX11
- libXi
- libXcursor
- asoundlib

```sh
cc -o nob nob.c
./nob
./build/crynth
```
