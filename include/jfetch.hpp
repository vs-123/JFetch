/**
 * @file jfetch.hpp
 * @brief JFetch - A header-only library for HTTP + JSON in C++
 * @author
 * @copyright Copyright 2025 vs-123
 * @license Apache License, Version 2.0
 */

#ifndef JFETCH_HPP
#define JFETCH_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <exception>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace jfetch {

/**
 * @brief Enum class representing HTTP request methods.
 */
enum class RequestMethod {
  GET,     ///< HTTP GET request
  POST,    ///< HTTP POST request
  PUT,     ///< HTTP PUT request
  DEL,     ///< HTTP DEL request
  PATCH,   ///< HTTP PATCH request
};

/**
 * @brief JFetch exception class
 */
class JFetchException : public std::exception {
public:
  /**
   * @brief Constructs the exception with a message.
   * @param message Error message.
   */
  explicit JFetchException(const std::string& message) : message_(message) {}
  /**
   * @brief Returns the exception string.
   * @return C-string error description.
   */
  const char* what() const noexcept override {
    return message_.c_str();
  }

private:
  std::string message_;
};

/**
 * @brief Exception thrown for HTTP request failures.
 */
class JFetchHTTPException : public JFetchException {
public:
  /**
   * @brief Constructs an HTTP exception.
   * @param status HTTP status code.
   * @param message Error message.
   */
  JFetchHTTPException(long status, const std::string& message)
    : JFetchException("HTTP Error: " + message + " (Status Code: " + std::to_string(status) + ")"),
      status_code_(status) {}

  /**
   * @brief Returns the HTTP status code.
   */
  long status_code() const {
    return status_code_;
  }

private:
  long status_code_;
};

/**
 * @brief Exception thrown when JSON parsing fails.
 */
class JFetchParsingException : public JFetchException {
public:
  /**
   * @brief Constructs a parsing exception.
   * @param message Error message.
   */
  explicit JFetchParsingException(const std::string& message)
    : JFetchException("Parsing Error: " + message) {}
};

/**
 * @brief Exception thrown when an endpoint is missing from the lookup table.
 */
class JFetchEndpointNotFoundException : public JFetchException {
public:
  /**
   * @brief Constructs an exception with the missing endpoint's name.
   * @param endpoint The requested but undefined endpoint.
   */
  explicit JFetchEndpointNotFoundException(const std::string& endpoint)
    : JFetchException("Endpoint \"" + endpoint + "\" not found in lookup table.") {}
};

/**
 * @brief Internal class to handle HTTP transactions via libcurl.
 */
class HttpClient {
public:
  /**
   * @brief Constructs a new HttpClient.
   * @param url Target URL for the HTTP request.
   * @param method HTTP method to use.
   * @param headers Optional HTTP headers.
   * @param body Optional request body.
   */
  HttpClient(const std::string& url,
         RequestMethod method,
         const std::vector<std::string>& headers = {},
         const std::string& body = "")
    : url_(url), method_(method), headers_(headers), body_(body) {}

  /**
   * @brief Performs the HTTP request and stores the response.
   * @param response String to receive the response body.
   * @return `true` on success, throws on failure.
   */
  bool perform_request(std::string& response) const {
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
    if (!curl) {
      throw JFetchException("Failed to initialize CURL");
    }

    curl_easy_setopt(curl.get(), CURLOPT_URL, url_.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, method_to_string().c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 10L);

    struct curl_slist* header_list = nullptr;
    for (const auto& header : headers_) {
      header_list = curl_slist_append(header_list, header.c_str());
    }
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, header_list);

    if (!body_.empty() &&
      (method_ == RequestMethod::POST ||
       method_ == RequestMethod::PUT ||
       method_ == RequestMethod::PATCH)) {
      curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body_.c_str());
    }

    CURLcode res = curl_easy_perform(curl.get());
    long http_status;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_status);
    curl_slist_free_all(header_list);

    if (res != CURLE_OK) {
      throw JFetchHTTPException(http_status, curl_easy_strerror(res));
    }

    if (http_status < 200 || http_status >= 300) {
      throw JFetchHTTPException(http_status, "Unexpected HTTP status code");
    }

    return true;
  }

private:
  std::string url_;
  RequestMethod method_;
  std::vector<std::string> headers_;
  std::string body_;

  /**
   * @brief Converts the enum method to a string.
   */
  std::string method_to_string() const {
    switch (method_) {
      case RequestMethod::GET: return "GET";
      case RequestMethod::POST: return "POST";
      case RequestMethod::PUT: return "PUT";
      case RequestMethod::DEL: return "DELETE";
      case RequestMethod::PATCH: return "PATCH";
      default: return "GET";
    }
  }

  /**
   * @brief Callback for libcurl to write response data.
   */
  static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* user_data) {
    user_data->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
  }
};

/**
 * @brief Main template class for interfacing with JSON HTTP endpoints.
 * @tparam T The return type expected after JSON processing.
 */
template <typename T>
class JFetch {
public:
  /**
   * @brief Virtual Destructor
   */
  virtual ~JFetch() = default;

  /**
   * @brief Sends/Fetches a request to a specified endpoint and parses the JSON response into `T`.
   *
   * @param endpoint Name of the registered endpoint.
   * @param query_params Optional URL query parameters.
   * @param custom_headers Optional headers specific to this request.
   * @param custom_body Optional body payload.
   * @return Parsed object of type `T`.
   *
   * @throws JFetchException On initialization or CURL failure.
   * @throws JFetchHTTPException On HTTP error response.
   * @throws JFetchParsingException On JSON parse failure.
   * @throws JFetchEndpointNotFoundException If endpoint isn't registered.
   */
  T fetch(const std::string& endpoint,
      const std::unordered_map<std::string, std::string>& query_params = {},
      const std::vector<std::string>& custom_headers = {},
      const std::string& custom_body = "") {
    std::string raw_json;

    // construct the full URL with query parameters
    std::string full_url = get_base() + endpoint;
    if (!query_params.empty()) {
      full_url += "?";
      for (const auto& [key, value] : query_params) {
        full_url += key + "=" + value + "&";
      }
      full_url.pop_back();  // remove the trailing '&'
    }

    // combine global and custom headers
    std::vector<std::string> headers = global_headers;
    headers.insert(headers.end(), custom_headers.begin(), custom_headers.end());

    // use the provided body, otherwise fallback to the default body
    std::string body = custom_body.empty() ? get_body() : custom_body;

    // get request method from the endpoint lookup table
    if (endpoint_lookup.find(endpoint) == endpoint_lookup.end()) {
      throw JFetchEndpointNotFoundException(endpoint);
    }

    const auto& [method, lambda] = endpoint_lookup.at(endpoint);

    HttpClient client(full_url, method, headers, body);
    client.perform_request(raw_json);

    auto json_data = nlohmann::json::parse(raw_json);
    return lambda(json_data);
  }

  /**
   * @brief Sets global headers applied to all requests.
   * @param headers List of HTTP headers.
   */
  void set_global_headers(const std::vector<std::string>& headers) {
    global_headers = headers;
  }

protected:
  /**
   * @brief Returns the base URL for all requests.
   * @return Base URL as a string.
   */
  virtual std::string get_base() const = 0;
  /**
   * @brief Returns a default request body.
   * @return Body string (default is empty).
   */
  virtual std::string get_body() const { return ""; }

  /**
   * @brief User-defined mapping of endpoints to their HTTP method and parsing logic.
   */
  std::unordered_map<std::string, std::pair<RequestMethod, std::function<T(const nlohmann::json&)>>> endpoint_lookup;
  /**
   * @brief List of global HTTP headers applied to all requests.
   */
  std::vector<std::string> global_headers;
};

}  // namespace jfetch

#endif  // JFETCH_HPP
