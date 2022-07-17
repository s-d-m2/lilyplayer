# Optimisation #2 (didn't work): precompute all the music buffers

An interesting observation that we got with the first optimisation was that the music computation time, which
has to be fast, is mostly dominated by filling a buffer. And this buffer is used later on. Therefore, one "simple"
optimisation here it to precompute the buffers of all the music events, and then when playing a music event,
simply pass the relevant one to openAL. This would fix the issue of uncontrolled variance between music being
played.

This failed for two reasons. The first one is due to startup load time. After the optimisation described
above, computing the buffer for a music event takes about 20ms on chrome. For a song like `Etude as dur -
op25 - 01` from Chopin - which is not even a long song - we are talking of 1209 music events. This multiplied
by approximately 20ms, means the loading time would be about 24 seconds. This is simply
unacceptable. Circumventing this issue could be done by having one thread computing the buffers running in
parallel to the one consuming them. Since the one consuming the event needs to wait between music_events, it
is pretty much guaranteed that it will never need to wait (except for the very first event) to get a buffer to
pass to openAL.

The second issue is related to memory usage. I capped the memory usage of the wasm
application at 120 MiB which I already think is a lot. Each buffer here take `44100 * sizeof(uint16_t) * 6`
bytes which is slightly above half-megabyte. For the Chopin music sheet from above, this would mean using 2GiB
of memory just for the music data, which I consider unacceptable.

It would be possible to circumvent this by capping the number of buffers available to never run out of the
120MiB of memory (which includes the memory used by the app itself for everything else). On firefox, I could
use about 100 buffers before the app crashes with an `out-of-memory` errorm. It was approximately 80 on
chrome. Now comes the issue of finding out how many buffers can be allocated before the app crashes. One way
would be to simply keep allocating buffers, and catch the `std::bad_alloc` exception when reaching the
limit. Unfortunately the memory allocator on wasm doesn't throw `std::bad_alloc`, instead it simply
panics. This means that relying on this behaviour is not feasible. Ultimately, I decided not to try to get
this optimisation working as the benefits were not worth the cost when it would work anyway. It would allow
for about 2 seconds of pre computed music only. If a music sheet is fast paced enough to require music
computed in less than 20ms (the slowest time on chrome), this 2 second time window won't probably help much to
hide the slowness issue from the user, only delay it a bit.
