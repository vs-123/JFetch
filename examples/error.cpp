#include "../include/jfetch.hpp"
#include <iostream>
#include <string>

struct GenericJSON {
  nlohmann::json data;
};

class InvalidAPI : public jfetch::JFetch<GenericJSON> {
public:
  InvalidAPI() {
    endpoint_lookup = {
      {"/data", {jfetch::RequestMethod::GET, [](const nlohmann::json& json_data) {
        return GenericJSON{json_data};
      }}},
    };
  }

protected:
  std::string get_base() const override {
    return "https://this-api-does-not-exist.com";  // Invalid base URL
  }
};

int main() {
  try {
    InvalidAPI api;

    // Attempt to fetch from an invalid endpoint
    auto response = api.fetch("/data");

    std::cout << "Received JSON: " << response.data.dump(2) << std::endl;
  } catch (const jfetch::JFetchHTTPException& e) {
    std::cerr << "HTTP Error: " << e.what() << std::endl;
  } catch (const jfetch::JFetchEndpointNotFoundException& e) {
    std::cerr << "Endpoint Error: " << e.what() << std::endl;
  } catch (const jfetch::JFetchException& e) {
    std::cerr << "JFetch Error: " << e.what() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Unexpected Error: " << e.what() << std::endl;
  }

  return 0;
}
