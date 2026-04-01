# C++ Autonomous AI Coding Agent (Claude-Code Clone)

A high-performance CLI tool built in C++ that transforms an LLM into an autonomous coding assistant capable of managing a local filesystem and executing shell commands.

## 核心 Features
- **Recursive Agent Loop:** Implements a stateful "Thought-Action-Observation" loop, allowing the LLM to self-correct and perform multi-step tasks.
- **System Tool Integration:** - **Read/Write:** Direct filesystem access via `std::fstream`.
- **Bash Execution:** Real-time shell command execution using `popen` with combined `stdout/stderr` capture.
- **OpenAI-Compatible Architecture:** Built to interface with OpenRouter/Claude-4.5 headers using the `cpr` HTTP library and `nlohmann-json`.
- **Resource Optimized:** Engineered to run efficiently on legacy hardware (Tested on 2017 MacBook Air, 8GB RAM).

## Technical Stack
- **Language:** C++20
- **Build System:** CMake & vcpkg
- **Libraries:** `cpr` (Network), `nlohmann-json` (Serialization), `OpenSSL` (Security)

## How It Works
The agent doesn't just "chat"; it uses a **Tool-Calling Interface**. When a user provides a high-level goal (e.g., "Fix the bugs in main.cpp"), the agent:
1. **Reads** the source code.
2. **Analyzes** the logic internally.
3. **Writes** the corrected code to the file.
4. **Executes** a Bash command to verify the build.

## Installation & Usage
```bash
# Clone and build
mkdir build && cd build
cmake .. && make

# Run a task
./claude-code -p "Create a simple C++ hello world file and compile it using g++"
