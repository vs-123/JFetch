# JFetch
A lightweight, header-only library for fetching and parsing JSON responses with libcurl in C++.

## Overview
JFetch simplifies HTTP requests and JSON handling by providing an intuitive abstraction over `libcurl` and `nlohmann::json`. It allows you to fetch JSON data from APIs in C++ without having to deal with C-style curl functions.

## Features
- **Single-Header Only**: No build required, simply `#include "jfetch.hpp"`
- **Encapsulated HTTP Handling**: Eliminates the need of manually dealing with `libcurl`
- **Simple JSON Parsing**: Converts API response into usable JSON (via `nlohmann::json`)
- **Extensible**: Easily derive classes for different API endpoints

## Installation
Since JFetch is header-only, you can simply copy `jfetch.hpp` into your project and then:
```cpp
#include "jfetch.hpp"
```
It just works!

## Dependencies
This project depends on:
- `libcurl` - [Website](https://curl.se/libcurl/) [Repo](https://github.com/curl/curl)
- `nlohmann::json` - [Website](https://json.nlohmann.me/) [Repo](https://github.com/nlohmann/json)

## Example
Here’s an example of a fetcher that retrieves product details from an API and processes the JSON response into a custom `struct` called `Product`.
```cpp
#include "jfetch.hpp"
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
      "/products/1",  // Endpoint
      {{"currency", "USD"}},  // Query parameters
      {"Authorization: Bearer my_api_token"}  // Custom headers
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

```

For actual working examples, see `examples/` directory of this project.

## Motive Behind This Project
This project originally started out as my personal utility library. Some time later, I was given the idea "Why not make it a library for everyone to use?", and here we are!
