# GoblinLoader
[![Ask DeepWiki](https://devin.ai/assets/askdeepwiki.png)](https://deepwiki.com/z3vxo/GoblinLoader)

GoblinLoader is a post-exploitation framework featuring a Command & Control (C2) server and multiple Windows loaders. The C2 server is built with a Go backend and a React frontend, providing a modern web interface for managing campaigns, agents, and tasking. The loaders are designed for stealth and flexibility, employing techniques like in-memory execution, process hollowing, reflective loading, and dynamic API resolution.

## Features

- **Full-Featured C2 Server**:
    - Go backend with a REST API for agent communication and a WebSocket for real-time UI updates.
    - React frontend for interactive management of campaigns, agents, and files.
    - SQLite database for persistent storage.
- **Versatile Windows Loaders**:
    - **Core Implant (`ldr`)**: A DLL-based agent that communicates with the C2, fetches tasks, and executes payloads. Supports loading EXEs and custom modules.
    - **Reflective Loader (`rLdr`)**: A position-independent shellcode that manually maps an accompanying DLL into memory, resolving imports and handling memory permissions. Uses direct syscalls to evade API hooking.
    - **EXE Process Hollowing (`exemap`)**: A shellcode designed to be prepended to an EXE. It maps the target executable over the current process's memory space and executes it.
- **Stealth and Evasion**:
    - In-memory execution of executables and post-exploitation modules.
    - Process hollowing capabilities.
    - Dynamic API resolution via function name hashing to obscure WinAPI usage.
    - Direct syscall implementation in the reflective loader.
- **Modular Design**:
    - The core implant can be tasked to run custom, in-memory modules (`modules/ls.c` is an example).

## Components

- **`/server`**: The C2 server.
    - `backend/`: The Go-based backend providing the API and WebSocket services.
    - `frontend/`: The React-based web UI.
- **`/ldr`**: The primary implant/agent. This is the core DLL that connects to the C2 for tasking.
- **`/rLdr`**: A reflective loader PoC. This shellcode loads a bundled DLL into memory.
- **`/exemap`**: An EXE mapping shellcode that performs process hollowing of the current process.
- **`/modules`**: Contains examples of post-exploitation modules that can be loaded and run by the `ldr` implant.
- **`/test`**: Test utilities, including a simple injector and a sample payload executable.

## Building

You will need a MinGW-w64 cross-compiler (`x86_64-w64-mingw32-gcc`), `nasm`, `go`, and `npm` installed.

### Server

The server backend embeds the built frontend assets. Build everything with a single command from the `/server` directory:

```bash
# Install frontend dependencies, build the frontend,
# embed assets, and build the final server binary.
make frontend
```

Other `make` targets are available:
- `make build`: Build only the backend.
- `make secrets`: Generate new build-time secrets and build the server.

### Loaders & Modules

Each loader and module has its own `makefile`. Navigate to the respective directory (`ldr`, `rLdr`, `exemap`, `modules`) and run `make`.

```bash
# Example: Build the core implant
cd ldr/
make

# Example: Build the reflective loader and its shellcode
cd ../rLdr/
make

# Example: Build the 'ls' module
cd ../modules/
make
```

The output artifacts (e.g., `shellcode.bin`, `ls.bin`) will be placed in the corresponding directory.

## Usage

Follow these steps to set up and run the framework:

1.  **Build the Server**:
    From the `/server` directory, run:
    ```bash
    make frontend
    ```

2.  **Initial Server Setup**:
    Run the setup command with root privileges to create a machine fingerprint. This only needs to be done once.
    ```bash
    sudo ./server setup
    ```

3.  **Register an Operator**:
    Create an account for the web UI. The domain should be the public-facing URL of your C2 server.
    ```bash
    ./server register <username> <password> <http://your-c2-domain:port>
    ```

4.  **Run the C2 Server**:
    Start the server. You can specify the listening address and port.
    ```bash
    ./server run --addr 0.0.0.0 --port 8081
    ```

5.  **Build an Implant**:
    Navigate to the `ldr` directory and build the main agent.
    ```bash
    cd ldr/
    make
    ```
    This will produce `shellcode.bin`. *Note: The C2 server IP and campaign details are currently hardcoded in `ldr/src/comms/comms.c` and `ldr/src/core/config.c` respectively. You must modify these before building.*

6.  **Deploy**:
    Execute the generated `shellcode.bin` on a target machine. You can use the provided `test/inject.c` utility or your own methods.

7.  **Operate**:
    Log in to the web UI at your C2 domain to view connected agents and issue tasks.