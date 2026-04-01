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
    if (!file.is_open()) return "Error: File not found.";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper to write file contents (Stage #OZ7)
std::string write_file_content(const std::string& file_path, const std::string& content) {
    std::ofstream file(file_path, std::ios::trunc); // Overwrite if exists
    if (!file.is_open()) return "Error: Could not write to file.";
    file << content;
    return "Successfully wrote to " + file_path;
}

int main(int argc, char* argv[]) {
    if (argc < 3) return 1;

    std::string user_prompt = argv[2];
    const char* api_key_env = std::getenv("OPENROUTER_API_KEY");
    const char* base_url_env = std::getenv("OPENROUTER_BASE_URL");
    std::string api_key = api_key_env ? api_key_env : "";
    std::string base_url = base_url_env ? base_url_env : "https://openrouter.ai/api/v1";

    if (api_key.empty()) return 1;

    json messages = json::array({{{"role", "user"}, {"content", user_prompt}}});

    // Advertise BOTH Read and Write tools
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
        },
        {
            {"type", "function"},
            {"function", {
                {"name", "Write"},
                {"description", "Write content to a file"},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"file_path", {{"type", "string"}, {"description", "The path of the file to write to"}}},
                        {"content", {{"type", "string"}, {"description", "The content to write to the file"}}}
                    }},
                    {"required", json::array({"file_path", "content"})}
                }}
            }}
        }
    });

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

        if (response.status_code != 200) return 1;

        json result = json::parse(response.text);
        json assistant_message = result["choices"][0]["message"];
        messages.push_back(assistant_message);

        if (assistant_message.contains("tool_calls") && !assistant_message["tool_calls"].empty()) {
            for (auto& tool_call : assistant_message["tool_calls"]) {
                std::string call_id = tool_call["id"];
                std::string function_name = tool_call["function"]["name"];
                json args = json::parse(tool_call["function"]["arguments"].get<std::string>());
                std::string tool_result;

                if (function_name == "Read") {
                    tool_result = read_file_content(args["file_path"]);
                } else if (function_name == "Write") {
                    tool_result = write_file_content(args["file_path"], args["content"]);
                }

                messages.push_back({
                    {"role", "tool"},
                    {"tool_call_id", call_id},
                    {"content", tool_result}
                });
            }
        } else {
            if (assistant_message.contains("content") && !assistant_message["content"].is_null()) {
                std::cout << assistant_message["content"].get<std::string>();
            }
            break;
        }
    }
    return 0;
}