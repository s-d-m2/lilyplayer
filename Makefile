all lilyplayer:
	${MAKE} -C ./src "$@"

wasm:
	./make_wasm.sh

clean:
	${MAKE} -C ./src "$@"
	${MAKE} -C ./3rd-party/rtmidi/ "$@"

install:
	./make-install.sh ${DESTDIR}

appimage: lilyplayer
	./make-appimage.sh

.PHONY: all lilyplayer clean appimage install
