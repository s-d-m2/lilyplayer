# Difference in processing music between wasm and native


The software used for producing music out of midi events used to be librtmidi. When adding a webassembly
version, a move to fluidsynth was made. The rationale behind was that using fluidsynth for native and
webassembly seemed easier than getting librtmidi to work in a wasm context. As a bonus point, fluidsynth
produces nicer quality output.  Unfortunately, using fluidsynth for native and wasm is not done the same way.

Fluidsynth can output music to several backends like OSS, ALSA, SDL and a few more. These backends are
ultimately where the operating system gets orders to send data to speakers. I'm simplifying a bit here. OSS
and ALSA are linux APIs to do so and fluidsynth automatically picks a suitable backend based on the system it
is running on.

In webassembly, the browser API for producing music is OpenAL ... which fluidsynth does not support.
Consequently, to get music to the speaker, the wasm version of lilyplayer uses an intermediate step: it
instructs fluidsynth to generate music data into a buffer and then pushes this buffer to openAL. On the native
version, there is no intermediate buffer used, as fluidsynth directly outputs to the backend.

Using the timing method described above, we got that for each music_event (i.e. user plays some keys)
generating music was taking around 250 microseconds (+/- 100 microseconds) on the native app, using a laptop
produced in 2011 running linux, versus 70 milliseconds on firefox and 100-120 milliseconds for chrome on the
same laptop.  Thes numbers are not precise as at that moment I was only looking for broad numbers to find
where the issue performance was, and not so much interested in making a good benchmark. A laptop from 2019
running macOS showed only slighty lower (around 20% lower) numbers for the wasm app.

The important thing to always keep in mind, is that when playing a song the maximum duration allowed to compute
music data is decided by the music sheet. For example, let's say we are playing a song with tempo of 60 quarter
note per minute, that is one quarter note per second, and the song consists only of four quarter notes in
succession. In this case, we have a full second to compute music for one event, before the next event arrives.
The faster paced the tempo and the music is, the less time for computation we have.

The data for `Etude as dur - op25 - 01` from Chopin is as follow:
- 1060 times a maximum duration of 96ms to compute music
- 126 times, we can compute the music in at most 48ms.
- 5 times at most 24.038ms to compute music before hitting the milestone.

Seeing the computation times on firefox and chrome, we get that the software is just not efficient enough to
play a fast-paced song like Chopin. Considering how fast computers are nowadays, and also the fact that the
software had no issue with the native version, something had to be done.

A first experiment to know if using openAL was inherently slow, or if compiling to wasm was incurring too much
overhead was to change the native program to do the same thing as the wasm one, that is use the intermediate
buffer and output to openAL too.

The numbers I got were roughly about twice lower than wasm running on firefox, that is, the music computation
was only twice as fast on native than wasm when using openAL. A computation with fluidsynth's usual backend
takes roughly 250 - 300 microseconds, versus 35 milliseconds on native and twice as much on firefox. Wasm
overhead compared to native is only a factor of 2, meaning I can't blame wasm for being too slow for my needs.

A more fine-grained measurement showed that the time was spent mostly into the fluidsynth method that computes
the music data and puts it into this intermediate buffer, and not so much into openAL itself.
