#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

using json = nlohmann::json;

// Helper function to read file contents
std::string read_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return "Error: Could not open file " + file_path;
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

    std::string prompt = argv[2];
    if (prompt.empty()) {
        std::cerr << "Prompt must not be empty" << std::endl;
        return 1;
    }

    const char* api_key_env = std::getenv("OPENROUTER_API_KEY");
    const char* base_url_env = std::getenv("OPENROUTER_BASE_URL");

    std::string api_key = api_key_env ? api_key_env : "";
    std::string base_url = base_url_env ? base_url_env : "https://openrouter.ai/api/v1";

    if (api_key.empty()) {
        std::cerr << "OPENROUTER_API_KEY is not set" << std::endl;
        return 1;
    }

    // 1. Advertise the "Read" tool (from previous stage)
    json request_body = {
        {"model", "anthropic/claude-haiku-4.5"},
        {"messages", json::array({
            {{"role", "user"}, {"content", prompt}}
        })},
        {"tools", json::array({
            {
                {"type", "function"},
                {"function", {
                    {"name", "Read"},
                    {"description", "Read and return the contents of a file"},
                    {"parameters", {
                        {"type", "object"},
                        {"properties", {
                            {"file_path", {
                                {"type", "string"},
                                {"description", "The path to the file to read"}
                            }}
                        }},
                        {"required", json::array({"file_path"})}
                    }}
                }}
            }
        })}
    };

    cpr::Response response = cpr::Post(
        cpr::Url{base_url + "/chat/completions"},
        cpr::Header{
            {"Authorization", "Bearer " + api_key},
            {"Content-Type", "application/json"}
        },
        cpr::Body{request_body.dump()}
    );

    if (response.status_code != 200) {
        std::cerr << "HTTP error: " << response.status_code << " - " << response.text << std::endl;
        return 1;
    }

    json result = json::parse(response.text);
    auto& message = result["choices"][0]["message"];

    // 2. Check if the LLM wants to use a tool
    if (message.contains("tool_calls") && !message["tool_calls"].empty()) {
        // Extract the first tool call
        auto& tool_call = message["tool_calls"][0];
        std::string function_name = tool_call["function"]["name"];
        
        if (function_name == "Read") {
            // The arguments are provided as a JSON string, so we must parse them again
            json arguments = json::parse(tool_call["function"]["arguments"].get<std::string>());
            std::string file_path = arguments["file_path"];
            
            // 3. Execute the tool and print the file contents
            std::cout << read_file(file_path);
        }
    } else {
        // 4. Default: Print the text content if no tool was called
        if (message.contains("content") && !message["content"].is_null()) {
            std::cout << message["content"].get<std::string>();
        }
    }

    return 0;
}