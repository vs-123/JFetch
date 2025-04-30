#include "../include/jfetch.hpp"
#include <iostream>
#include <string>

struct LoginResponse {
  std::string accessToken;
};

class MyFetcher : public jfetch::JFetch<LoginResponse> {
public:
  MyFetcher() {
    endpoint_lookup = {
      {"/auth/login", {jfetch::RequestMethod::POST, [](const nlohmann::json& json_data) {
        if (json_data.contains("accessToken")) {
          return LoginResponse{json_data["accessToken"].get<std::string>()};
        }
        throw jfetch::JFetchParsingException("[ERROR] 'accessToken' field not found in JSON response.");
      }}},
    };
  }

protected:
  std::string get_base() const override {
    return "https://dummyjson.com";
  }
};

int main() {
  try {
    MyFetcher fetcher;

    // Fetch with custom body and headers
    auto response = fetcher.fetch(
      "/auth/login",
      {},  // no query parameters
      {"Content-Type: application/json"},  // custom headers
      R"({"username": "emilys", "password": "emilyspass", "expiresInMins": 30})"  // custom body
    );

    std::cout << "API Key: " << response.accessToken << std::endl;
  } catch (const jfetch::JFetchParsingException& e) {
    std::cerr << "Parsing Error: " << e.what() << std::endl;
  } catch (const jfetch::JFetchException& e) {
    std::cerr << "JFetch Error: " << e.what() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Unexpected Error: " << e.what() << std::endl;
  }

  return 0;
}
