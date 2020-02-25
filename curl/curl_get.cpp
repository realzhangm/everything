#include <cstdio>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>

#include <iostream>
#include <fstream>
#include <iterator>

#include "curl/curl.h"

static size_t WriteCallBack(void *data, size_t size, size_t nmemb, void *ptr);

class CurlClient
{
public:
    CurlClient();
    ~CurlClient();

    int Get(const std::string& url);
    int Head(const std::string& url, int64_t* size);

    void SetWriteCallBack(const std::function<size_t(char *data, size_t size)>& cb);

    friend size_t WriteCallBack(void *data, size_t size, size_t nmemb, void *ptr);

private:
    CURL* curl_;
    std::function<size_t(char* data, size_t size)> cb_;
    CURLcode res_;
    std::string msg_;
};


CurlClient::CurlClient() : curl_(nullptr), cb_(nullptr) {
    curl_ = curl_easy_init();
}

CurlClient::~CurlClient() {
    if (nullptr != curl_) {
        curl_easy_cleanup(curl_);
    }
}

void CurlClient::SetWriteCallBack(const std::function<size_t(char* data, size_t size)>& cb) {
    cb_ = cb;
}

static size_t WriteCallBack(void *data, size_t size, size_t nmemb, void *ptr) {
  if (size * nmemb == 0) return 0;
  CurlClient* cli = reinterpret_cast<CurlClient*>(ptr);
  char* ptr_char = reinterpret_cast<char*>(data);
  return cli->cb_(ptr_char, size * nmemb);
}

int CurlClient::Get(const std::string& url) {
    curl_easy_reset(curl_);
    curl_easy_setopt(curl_, CURLOPT_URL, url.data());

    curl_easy_setopt(curl_, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl_, CURLOPT_HTTP_CONTENT_DECODING, 1L);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallBack);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);

    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);

    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);

    res_ = curl_easy_perform(curl_);
    if (CURLE_OK != res_) {
      long response_code;
      curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);
      char buf[128];
      snprintf(buf, sizeof(buf), "error:[%d][%s], http response_code[%ld] path[%s]", 
                              res_, 
                              curl_easy_strerror(res_), 
                              response_code,
                              url.c_str());
      msg_ = std::string(buf);
      return -1;
    }

    return 0;
}

int CurlClient::Head(const std::string& url, int64_t* file_size) {
    curl_easy_reset(curl_);
    curl_easy_setopt(curl_, CURLOPT_URL, url.data());
    curl_easy_setopt(curl_, CURLOPT_NOBODY, 1);
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10);

    res_ = curl_easy_perform(curl_);
    if (!res_) {
        double cl;
        res_ = curl_easy_getinfo(curl_, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl);
        if (!res_) {
            *file_size = static_cast<int64_t>(cl);
        } else {
            *file_size = -1;
        }
    }
    return 0;
}


std::vector<char> V;
size_t W(char *data, size_t size) {
    std::copy(data, data + size, std::back_inserter(V));
    return size;
}

int main()
{   
    std::string url = "https://aliyun-nls.oss-cn-hangzhou.aliyuncs.com/asr/fileASR/examples/nls-sample-16k.wav";
    CurlClient curl;
    int64_t size;
    curl.Head(url, &size);
    printf("size = %lld \n", size);


    curl.SetWriteCallBack(&W);

    printf("V size = %ld \n", V.size());
    curl.Get(url);
    printf("V size = %ld \n", V.size());

    std::ofstream output_file("16k.wav", std::ios::out | std::ios::binary); 
    std::ostream_iterator<char> out_it (output_file);
    std::copy(V.begin(), V.end(), out_it);
}


