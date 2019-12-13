	 ____  ____  ____  ___  ____
	(_  _)(  _ \(_  _)/ __)/ _ _)
  	  )(  )    / _)(_( (__ \__  \
 	 (__) (_)\_)(____)\___)(____/


Mighty tiny text-based chiptune tracker


## Keyboard layout

### Default mode

 hjkl or arrow                            - move cursor
 CTRL + hl or CTRL + arrow or mouse click - switch tab
 ALT + hl or ALT + arrow or mouse click   - switch sub tab
 CTRL + jk or CTRL + arrow                - move cursor one screen up/down
 ALT + jk or ALT + arrow                  - move cursor to the first/last row
 SPACE or mouse click                     - edit mode
 CTRL + SPACE                             - play from selection or stop
 CTRL + r                                 - start / stop recording (in pattern tab, with focus in track)
 y                                        - copy bar/pattern/table row
 p                                        - paste bar/pattern/table row
 s                                        - save song

### Edit mode

 jk or arrow keys or mouse grab           - move parameter one step up/down
 CTRL + jk or arrow keys                  - move parameter octave / 12 steps up/down
 ALT + jk or arrow keys                   - double/half parameter
 ENTER                                    - exit from edit mode saving changes (where no auto exit)
 ESC or SPACE                             - exit from edit mode discarding changes (where no auto exit)

### Notes

```
  _ _   _ _ _   _ _   _ _ _
 |s|d| |g|h|j| |2|3| |5|6|7|
 |_|_| |_|_|_| |_|_| |_|_|_| _
|z|x|c|v|b|n|m|q|w|e|r|t|y|u|i|
|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|

```


## CLI

trics <command> [<args>]

Default command:

trics [options] <song_file>

    song_file       - song file to open

Options:
    -o output_file        - Output song file
    --midi device         - Connect MIDI input device
    --mc cc param         - Map MIDI controll to parameter
    -s                    - Play song without displaying interface
    -h, --help            - Show this help
    -v, --version         - Show version

Available commands:

  midi-list
  export


## Defautl template

Instruments:
    00 kick         - Kick drum
    01 snare        - Snare drum
    02 c_hat        - Closed high-hat
    03 o_hat        - Open high-hat

    10 lead         - Lead synth voice
    11 s_pluck      - Short plucky synth
    12 l_pluck      - Long plucky synth
    13 pad          - Pad synth
    14 sin_voice    - Synth almost sin wave
    15 synth_drop   - Resonant synth drop

    20 bass         - Classic bass close to sin wave with punchy attack
    21 saw_bass     - Classic bass based on the saw
    22 pluck_bass   - Plucky bass synth

    30 noize_up     - Long noisy build-up
    31 noize_down   - Long noisy drop

Arpeggios
    01 fast_up
    02 fast_down
    03 major_up
    04 major_down
    05 major_alt
    06 minor_up
    07 minor_down
    08 minor_alt
    09 power_alt


## Examples

[Demo-song]()




```
 *SONG  PATT  INST  ARP

  bp st title
  80 20 Song Title lo....

  00 -- 03 01 -- 03 ff r4
  00 -- 03 01 -- 03 ff r4
  00 -- 03 01 -- 03 ff r4
  00 -- 03 01 -- 03 ff r4
  00 -- 03 01 -- 03 ff r4
  00 -- 03 01 -- 03 ff r4
  01 43 -- -- -- -- -- --
> 01 43 -- -- -- -- -- --
  ...    song_title_lo...
```






```
  SONG *PATT  INST  ARP

  ## o1 o2
  00 04 04

  in nt   ar   in nt   ar
  00 C#-1 --   -- ---- --
  00 C#-1 --   -- ---- --
> 00 C#-1 --   -- ---- --
  00 C#   --   -- ---- --
  00 ==== --   -- ---- --
  00 C#   --   -- ---- --
  00 C#   --   -- ---- --
  00 C#   --   -- ---- --
  ...             ALT + K
```




```
  SONG  PATT *INST  ARP
 *MAIN  WAVE  FILT

  ## name
  00 Lead synth

  vl hr
  ff 01

  aa ad as ar ds da dr rv
  00 00 00 00 00 00 00 00



  ...    song_title_lo...
```








```
  SONG  PATT *INST  ARP
  MAIN *WAVE  FILT

  wv rm hs    pw    rp st
  00 00 01  + 00    00 10
  00 00 01  + 00
  00 00 01  + 00
  00 00 01  + 00
  00 00 01  + 00
  00 00 01  + 00
  00 00 01  + 00
  00 00 01  + 00
  ...    song_title_lo...
```




```
  SONG  PATT *INST  ARP
  MAIN  WAVE *FILT

  tp    rs    ct    rp st
  00  + 01  + 00    00 10
  00  + 01  + 00
  00  + 01  + 00
  00  + 01  + 00
  00  + 01  + 00
  00  + 01  + 00
  00  + 01  + 00
  00  + 01  + 00
  ...    song_title_lo...
```





```
  SONG  PATT  INST *ARP

  ## name
  00 Major third up

     pt    rp st
   + 00    01 10
   + 00
   + 00
   + 00
   + 00
   + 00
   + 00
   + 00
  ...    song_title_lo...

```







