#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

using json = nlohmann::json;

// Helper to read file contents
std::string read_file_content(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return "Error: File not found.";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " -p <prompt>" << std::endl;
        return 1;
    }

    std::string user_prompt = argv[2];
    const char* api_key_env = std::getenv("OPENROUTER_API_KEY");
    const char* base_url_env = std::getenv("OPENROUTER_BASE_URL");
    std::string api_key = api_key_env ? api_key_env : "";
    std::string base_url = base_url_env ? base_url_env : "https://openrouter.ai/api/v1";

    if (api_key.empty()) return 1;

    // 1. Initialize Conversation History
    json messages = json::array({
        {{"role", "user"}, {"content", user_prompt}}
    });

    json tools_spec = json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "Read"},
                {"description", "Read and return the contents of a file"},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"file_path", {{"type", "string"}, {"description", "The path to the file to read"}}}
                    }},
                    {"required", json::array({"file_path"})}
                }}
            }}
        }
    });

    // 2. Enter the Agent Loop
    while (true) {
        json request_body = {
            {"model", "anthropic/claude-haiku-4.5"},
            {"messages", messages},
            {"tools", tools_spec}
        };

        cpr::Response response = cpr::Post(
            cpr::Url{base_url + "/chat/completions"},
            cpr::Header{{"Authorization", "Bearer " + api_key}, {"Content-Type", "application/json"}},
            cpr::Body{request_body.dump()}
        );

        if (response.status_code != 200) {
            std::cerr << "API Error: " << response.text << std::endl;
            return 1;
        }

        json result = json::parse(response.text);
        json assistant_message = result["choices"][0]["message"];

        // 3. Record Assistant's Response in history
        messages.push_back(assistant_message);

        // 4. Check for Tool Calls
        if (assistant_message.contains("tool_calls") && !assistant_message["tool_calls"].empty()) {
            for (auto& tool_call : assistant_message["tool_calls"]) {
                std::string call_id = tool_call["id"];
                std::string function_name = tool_call["function"]["name"];
                json args = json::parse(tool_call["function"]["arguments"].get<std::string>());

                if (function_name == "Read") {
                    std::string path = args["file_path"];
                    std::string content = read_file_content(path);

                    // 5. Add Tool Result to history (Crucial Step)
                    messages.push_back({
                        {"role", "tool"},
                        {"tool_call_id", call_id},
                        {"content", content}
                    });
                }
            }
            // Loop continues: Send updated history (User + Assistant + Tool) back to LLM
        } else {
            // 6. Exit Condition: No more tools requested
            if (assistant_message.contains("content") && !assistant_message["content"].is_null()) {
                std::cout << assistant_message["content"].get<std::string>();
            }
            break;
        }
    }

    return 0;
}