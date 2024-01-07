#ifndef RTMREQUEST_H_INCLUDE
#define RTMREQUEST_H_INCLUDE

#include <Arduino.h>
#include <map>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <MD5.h>
#include <UrlEncode.h>
#include "NJScanner.h"

using namespace std;

// RTMRequest class handles HTTPS request for Remember the Milk API

class RTMRequest
{
public:
    RTMRequest(String method, boolean auth);
    void setKeyValue(String key, String value);
    int getRequest();

    String getDateString();
    String getXmlString();
    int getErrorCode();
    std::map<String, String> getNextTagProperties(String tag);
    String getNextTagContent(String tag);

    static void setRTM_rootCA(String newRootCA);
    static void setRTM_apiKey(String newApiKey);
    static void setRTM_sharedSecret(String newSharedSecret);
    static void setRTM_authToken(String newAuthToken);
    static String getRTM_sharedSecret();
    static String getRTM_authToken();
    static String getRTM_frob();
    static String externalAuthURLString();
private:
    std::map<String, String> parameterMap;
    String urlString;
    String dateString;
    int errorCode;
    String xmlString;
    NJScanner xmlScanner;

    static String RTM_rootCA;
    static String RTM_apiKey;
    static String RTM_sharedSecret;
    static String RTM_frob;
    static String RTM_authToken;
};

#endif