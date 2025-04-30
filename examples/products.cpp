#include "../include/jfetch.hpp"
#include <iostream>
#include <vector>

struct Product {
  int id;
  std::string title;
  double price;
};

class ProductFetcher : public jfetch::JFetch<Product> {
public:
  ProductFetcher() {
    endpoint_lookup = {
      {"/products/1", {jfetch::RequestMethod::GET, [](const nlohmann::json& json_data) {
        return Product{
          json_data["id"].get<int>(),
          json_data["title"].get<std::string>(),
          json_data["price"].get<double>()
        };
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
    ProductFetcher fetcher;

    Product product = fetcher.fetch(
      "/products/1"  // endpoint
    );

    std::cout << "Product ID: " << product.id << std::endl;
    std::cout << "Product Title: " << product.title << std::endl;
    std::cout << "Product Price: $" << product.price << std::endl;
  } catch (const jfetch::JFetchParsingException& e) {
    std::cerr << "Parsing Error: " << e.what() << std::endl;
  } catch (const jfetch::JFetchException& e) {
    std::cerr << "JFetch Error: " << e.what() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Unexpected Error: " << e.what() << std::endl;
  }

  return 0;
}
