#include <iostream>
#include <string>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    // Basic argument validation
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " -p <prompt>" << std::endl;
        return 1;
    }

    std::string prompt = argv[2];

    if (prompt.empty()) {
        std::cerr << "Prompt must not be empty" << std::endl;
        return 1;
    }

    // Retrieve environment variables for API access
    const char* api_key_env = std::getenv("OPENROUTER_API_KEY");
    const char* base_url_env = std::getenv("OPENROUTER_BASE_URL");

    std::string api_key = api_key_env ? api_key_env : "";
    // Default to OpenRouter if base URL is not provided
    std::string base_url = base_url_env ? base_url_env : "https://openrouter.ai/api/v1";

    if (api_key.empty()) {
        std::cerr << "OPENROUTER_API_KEY is not set" << std::endl;
        return 1;
    }

    // Construct the JSON request body including the 'tools' advertisement
    json request_body = {
        {"model", "anthropic/claude-3-haiku"}, // Or the model specified in your starter code
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

    // Send the POST request to the LLM API
    cpr::Response response = cpr::Post(
        cpr::Url{base_url + "/chat/completions"},
        cpr::Header{
            {"Authorization", "Bearer " + api_key},
            {"Content-Type", "application/json"}
        },
        cpr::Body{request_body.dump()}
    );

    // Error handling for the HTTP response
    if (response.status_code != 200) {
        std::cerr << "HTTP error: " << response.status_code << " - " << response.text << std::endl;
        return 1;
    }

    // Parse the result and output the content
    json result = json::parse(response.text);

    if (!result.contains("choices") || result["choices"].empty()) {
        std::cerr << "No choices in response" << std::endl;
        return 1;
    }

    std::cout << result["choices"][0]["message"]["content"].get<std::string>() << std::endl;

    return 0;
}