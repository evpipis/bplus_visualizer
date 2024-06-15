# B+ Tree Implementation & Visualizer

This project was created for the purposes of the "Databases" course at ECE NTUA by Evangelos Pipis (evpipis). We implement a Beta Plus Tree in C and compile it to WebAssembly for efficient execution in web browsers. The visualizer is built using pure HTML and CSS, ensuring maximum efficiency and responsiveness. The implementation follows the principles outlined in the "Database System Concepts" book by Silberschatz, Korth, and Sudarshan, with support for duplicate keys managed by making them unique based on their insertion time.

## Installation:

To test the visualizer, you should do the following:

1.  Clone our repository:
    ```
    git clone https://github.com/evpipis/bplus_visualizer.git
    ```

2.  Change directory to the cloned repository.

3.  Open a localhost, e.g. by using python:
    ```
    python3 -m http.server 8000
    ```

3. Open the page `http://localhost:8000/` in your favorite browser.

## Compilation:

To recompile the implementation, you should do the following:

1.  Install the Emscripten compiler by executing: 
    ```
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk
    ./emsdk install latest
    ./emsdk activate latest
    source ./emsdk_env.sh
    ```

2. Make any changes you wish to the implementation file `bplus.c`.

3. Open the terminal in the cloned repository.

4. Compile the modified file by using the following command:
    ```
    emcc bplus.c -o bplus.js
    ```
