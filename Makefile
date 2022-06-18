all lilyplayer:
	${MAKE} -f Makefile.native "$@"

wasm:
	${MAKE} -f Makefile.wasm "$@"

clean:
	${MAKE} -f Makefile.native "$@"
	${MAKE} -f Makefile.wasm "$@"

install:
	./make-install.sh ${DESTDIR}

appimage: lilyplayer
	./make-appimage.sh

.PHONY: all lilyplayer clean appimage install wasm
