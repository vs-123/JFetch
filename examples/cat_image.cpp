#include "../include/jfetch.hpp"
#include <iostream>
#include <string>

struct CatImage {
  std::string url;
};

class CatImageFetcher : public jfetch::JFetch<CatImage> {
public:
  CatImageFetcher() {
    endpoint_lookup = {
      {"/v1/images/search", {jfetch::RequestMethod::GET, [](const nlohmann::json& json_data) {
        if (!json_data.empty() && json_data[0].contains("url")) {
          return CatImage{json_data[0]["url"].get<std::string>()};
        }
        throw jfetch::JFetchParsingException("[ERROR] 'url' field not found in JSON response.");
      }}},
    };
  }

protected:
  std::string get_base() const override {
    return "https://api.thecatapi.com";
  }
};

int main() {
  try {
    CatImageFetcher fetcher;

    // fetch with query parameters for a specific image type
    auto cat_image = fetcher.fetch("/v1/images/search", {{"mime_types", "jpg"}});

    std::cout << "Random cat image (URL): " << cat_image.url << std::endl;
  } catch (const jfetch::JFetchParsingException& e) {
    std::cerr << "Parsing Error: " << e.what() << std::endl;
  } catch (const jfetch::JFetchException& e) {
    std::cerr << "JFetch Error: " << e.what() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Unexpected Error: " << e.what() << std::endl;
  }

  return 0;
}
