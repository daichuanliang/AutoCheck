
#ifndef CURL_POST_H
#define CURL_POST_H

#include <string>
using std::string;


//static const string url_base="218.98.48.92/api/products";
static const string url_base="192.168.0.100/api/products";
static const string url_view_control = url_base+"/view";
static const string url_lost_control = url_base+"/lost";
//static const long server_port = 8101L;
static const long server_port = 80L;

int curlPostJson(string jsonStr, string url, long port, string& output);

#endif
