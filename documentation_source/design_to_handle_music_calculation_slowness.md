Issues with slowness of computing music data
===========

When porting the software to webassembly, a performance issue got uncovered.  Since lilyplayer is a
music-playing software, it has real-time constraints, even though it only deals with "soft real-time", not
hard real-time. When listening to the music, it was clear that the wasm version was too slow.  The music was
clearly played too slowly compared to the real deed.

However, the performance issue was only appearing on the wasm version, not the native one.

Since webassembly does not provide good tools for troubleshooting performance issues, the method to find out
what was taking too slow was simply to manually log how long operations were taking. This was done using the
old and reliable method of starting a chrono before, doing work, and stopping it once the work is done.

Turned out, the slowness was all in computing music data.
