// the code here is mostly copy/pasted from https://github.com/msorvig/qt-webassembly-examples/blob/cfa2343205a533ba61fdd199a6de56972d30e8b7/gui_localfiles/qhtml5file_html5.cpp

#include <emscripten.h>
#include <emscripten/html5.h>

#include <iostream>
#include <stdexcept>

#include "scope_exit.hh"

#include "load_file_from_wasm.hh"

//
// This file implements file load via HTML file input element and file save via browser download.
//

// Global user file data ready callback and C helper function. JavaScript will
// call this function when the file data is ready; the helper then forwards
// the call to the current handler function. This means there can be only one
// file open in proress at a given time.
std::function<void(char *, size_t)> g_FileDataReadyCallback;
extern "C" __attribute__((used)) void callFileDataReady(char *content, size_t contentSize)
{
    if (g_FileDataReadyCallback == nullptr) {
      return;
    }

    try {
      g_FileDataReadyCallback(content, contentSize);
    } catch (const std::exception& e) {
      std::cerr << "Error occured when calling the g_FileDataReadyCallback callback. Error message is: " << e.what() << "\n";
    } catch (...) {
      std::cerr << "Error occured when calling the g_FileDataReadyCallback callback\n";
    }
    g_FileDataReadyCallback = nullptr;
}

namespace {
    void loadFile(const char *accept, std::function<void(char *, size_t)> fileDataReady)
    {
        if (::g_FileDataReadyCallback) {
            puts("Warning: Concurrent loadFile() calls are not supported. Cancelling earlier call");
        }

        // Call callFileDataReady to make sure the emscripten linker does not
        // optimize it away, which may happen if the function is called from JavaScript
        // only. Set g_FileDataReadyCallback to null to make it a a no-op.
        ::g_FileDataReadyCallback = nullptr;
        ::callFileDataReady(nullptr, 0);

        ::g_FileDataReadyCallback = fileDataReady;
        EM_ASM_({
            const accept = UTF8ToString($0);

            // Crate file file input which whil display the native file dialog
            var fileElement = document.createElement("input");
            document.body.appendChild(fileElement);
            fileElement.type = "file";
            fileElement.style = "display:none";
            fileElement.accept = accept;
            fileElement.onchange = function(event) {
                const files = event.target.files;

                // Read files
                for (var i = 0; i < files.length; i++) {
                    const file = files[i];
                    var reader = new FileReader();
                    reader.onload = function() {
                        var contentArray = new Uint8Array(reader.result);
                        const contentSize = reader.result.byteLength;

                        // Copy the file file content to the C++ heap.
                        // Note: this could be simplified by passing the content as an
                        // "array" type to ccall and then let it copy to C++ memory.
                        // However, this built-in solution does not handle files larger
                        // than ~15M (Chrome). Instead, allocate memory manually and
                        // pass a pointer to the C++ side (which will free() it when done).

                        // TODO: consider slice()ing the file to read it picewise and
                        // then assembling it in a QByteArray on the C++ side.

                        const heapPointer = _malloc(contentSize);
                        const heapBytes = new Uint8Array(Module.HEAPU8.buffer, heapPointer, contentSize);
                        heapBytes.set(contentArray);

                        // Null out the first data copy to enable GC
                        reader = null;
                        contentArray = null;

                        // Call the C++ file data ready callback
                        ccall("callFileDataReady", null,
                            ["number", "number"], [heapPointer, contentSize]);
                    };
                    reader.readAsArrayBuffer(file);
                }

                // Clean up document
                document.body.removeChild(fileElement);

            }; // onchange callback

            // Trigger file dialog open
            fileElement.click();

        }, accept);
    }
}

/*!
    \brief Read local file via file dialog.

    Call this function to make the browser display an open-file dialog. This function
    returns immediately, and \a fileDataReady is called when the user has selected a file
    and the file contents has been read.

    \a The accept argument specifies which file types to accept, and must follow the
    <input type="file"> html standard formatting, for example ".png, .jpg, .jpeg".

    This function is implemented on Qt for WebAssembly only. A nonfunctional cross-
    platform stub is provided so that code that uses it can compile on all platforms.
*/
void load_from_wasm(const char* const accept, std::function<void(const std::string_view /* content */)> fileDataReady)
{
    loadFile(accept, [=](char *content, size_t size) {
      SCOPE_EXIT(free(content));
        // Copy file data into QByteArray and free buffer that was allocated
        // on the JavaScript side. We could have used QByteArray::fromRawData()
        // to avoid the copy here, but that would make memory management awkward.
      std::string_view view (content, size);
      // Call user-supplied data ready callback
      fileDataReady(view);
    });
}
