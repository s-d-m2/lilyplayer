all: lilyplayer

doc:
	mdbook build

lilyplayer native:
	${MAKE} -f Makefile.native "all"

wasm:
	${MAKE} -f Makefile.wasm "all"

clean:
	${MAKE} -f Makefile.native "$@"
	${MAKE} -f Makefile.wasm "$@"

install:
	./make-install.sh ${DESTDIR}

.PHONY: all lilyplayer clean install wasm native doc
