# Clipper

FFmpeg Frontend to clip live streams. FFmpeg is great because it is light weight; There is NO re-encoding!

Typical use case is you watch through and realize you want to clip what you just watched.
Press `O` to marks the clip end.
Navigate back desired start and press `I`.

(If you get the finalize prompt, press `Esc` to continue to adjust `I` and `O`, and use `Enter` to get back to finalize prompt.).

Then `H` to get back where you left off.

Use `S`/`E` then `L`/`J` to re-adjust clip start and end.

| Key   | Function                                        |
|-------|-------------------------------------------------|
| I     | Mark In (clip start)                            |
| O     | Mark Out (clip end)                             |
| S     | Jump to In/start marker                         |
| E     | Jump to Out/end marker                          |
| H     | Jump to furthest position watched               |
| J     | Jump forward x seconds                          |
| L     | Jump backward x seconds                         |
| Esc   | Exit finalize prompt/Bring focus off media controls |
| Enter | Enter finalize prompt                           |
