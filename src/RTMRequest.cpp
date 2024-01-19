#include "RTMRequest.h"
#define RTM_baseURL "https://api.rememberthemilk.com/services/rest/"

// Declare internal functions

String hashStringFromString(String inputString);
String signedParameters(std::map<String, String> parametersMap);

// Initialize static member varialbles
String RTMRequest::RTM_rootCA = "";
String RTMRequest::RTM_apiKey = "";
String RTMRequest::RTM_sharedSecret = "";
String RTMRequest::RTM_frob = "";
String RTMRequest::RTM_authToken = "";

// Constructor
RTMRequest::RTMRequest(String method, boolean auth)
{
    String urlString = RTM_baseURL;
    parameterMap.emplace("api_key", RTM_apiKey);
    parameterMap.emplace("method", method);
    if (auth)
        parameterMap.emplace("auth_token", RTM_authToken);
}

// Set key-value pair on request URL
void RTMRequest::setKeyValue(String key, String value)
{
    parameterMap.emplace(key, value);
}

// Send request and return HTTP result
int RTMRequest::getRequest()
{
    urlString = RTM_baseURL + signedParameters(parameterMap);
    HTTPClient httpClient;
    httpClient.begin(urlString, RTM_rootCA.c_str());
    dateString = "";

    const char *headerKeys[] = {"Date"};
    httpClient.collectHeaders(headerKeys, 1);

    int result = httpClient.GET();
    if (result == HTTP_CODE_OK)
    {
        dateString = httpClient.header("Date");
        WiFiClient *stream = httpClient.getStreamPtr();
        if (httpClient.connected()) {
            String body = "";
            while(stream->available()) {
                body += stream->readStringUntil('\n');
            }
            xmlString = body;
            xmlScanner = NJScanner(xmlString);
        }
    }
    httpClient.end();
    return result;
}

// HTTP date string on response header
String RTMRequest::getDateString()
{
    return dateString;
}

// XML string on response body
String RTMRequest::getXmlString()
{
    return xmlString;
}

// parse XML and return error code (0 = no error)
int RTMRequest::getErrorCode()
{
    NJScanner scanner = NJScanner(xmlString);
    scanner.scanUpToString("<rsp stat=\"", true);
    String resultString = scanner.scanUpToString("\"", false);
    if (resultString.equals("ok")) {
        return 0;
    }
    scanner.scanUpToString("<err code=\"", true);
    resultString = scanner.scanUpToString("\"", false);
    return resultString.toInt();
}

// get property map from specified tag
std::map<String, String> RTMRequest::getNextTagProperties(String tag) {
    std::map<String, String> properties;
    if (xmlScanner.isAtEnd()) return properties;
    // Serial.print("<" + tag + "> ");
    xmlScanner.scanUpToString("<" + tag, true);
    String propertyName, propertyValue;
    do
    {
        int scanLocation = xmlScanner.scanLocation();
        propertyName = xmlScanner.scanUpToString("=", false); // scan property name
        propertyName.trim();
        if (propertyName.indexOf(">") > -1) {
            xmlScanner.setScanLocation(scanLocation); // revert scan location
            break;
        }
        xmlScanner.scanUpToString("\"", true);
        if (xmlScanner.scanString("\"") != -1) {
            propertyValue = "";
        } else {
            propertyValue = xmlScanner.scanUpToString("\"", false); // scan property value
            propertyValue.trim();
            xmlScanner.scanString("\"");
        }
        properties.emplace(propertyName, propertyValue);

        // Serial.print(propertyName + "=" + propertyValue + " ");

    } while (!xmlScanner.isAtEnd());
    // Serial.print("\n");
    return properties;
}

// get content from specified tag
String RTMRequest::getNextTagContent(String tag)
{
    if (xmlScanner.isAtEnd()) return "";
    xmlScanner.scanUpToString("<" + tag, false);
    xmlScanner.scanUpToString(">", true);
    String content = xmlScanner.scanUpToString("</" + tag, false);
    return content;
}

// Static methods
void RTMRequest::setRTM_rootCA(String newRootCA)
{
    RTM_rootCA = newRootCA;
}
void RTMRequest::setRTM_apiKey(String newApiKey)
{
    RTM_apiKey = newApiKey;
}
void RTMRequest::setRTM_sharedSecret(String newSharedSecret)
{
    RTM_sharedSecret = newSharedSecret;
}
void RTMRequest::setRTM_authToken(String newAuthToken)
{
    RTM_authToken = newAuthToken;
}
String RTMRequest::getRTM_sharedSecret() {
    return RTM_sharedSecret;
}
String RTMRequest::getRTM_authToken() {
    return RTM_authToken;
}
String RTMRequest::getRTM_frob() {
    return RTM_frob;
}

// request frob and return authorization URL using the frob
String RTMRequest::externalAuthURLString()
{
    String authURL = "https://www.rememberthemilk.com/services/auth/";

    // Get frob
    RTMRequest request = RTMRequest("rtm.auth.getFrob", false);
    int result = request.getRequest();
    String frob = request.getNextTagContent("frob");
    Serial.println("frob: " + frob);
    RTM_frob = frob;

    // Parameters
    std::map<String, String> parametersMap;
    parametersMap.emplace("api_key", RTM_apiKey);
    parametersMap.emplace("perms", "read");
    parametersMap.emplace("frob", frob);
    authURL += signedParameters(parametersMap);

    return authURL;
}

// md5 hash string (using MD5 library)
String hashStringFromString(String inputString)
{
    unsigned int length = inputString.length();
    const char *inputCStr = inputString.c_str();
    char cStr[length+1];
    memcpy(cStr, inputCStr, length+1);
    unsigned char *hash = MD5::make_hash(cStr);
    char *md5str = MD5::make_digest(hash, 16);
    String result = String(md5str);
    free(hash);
    free(md5str);
    return result;
}

// return request URL using parameter map and api key, shared secret
String signedParameters(std::map<String, String> parametersMap)
{
    String result = "";
    String concatenatedParameters = RTMRequest::getRTM_sharedSecret();
    for (auto itr = parametersMap.begin(); itr != parametersMap.end(); ++itr)
    {
        String key = itr->first;
        String value = itr->second;

        concatenatedParameters += key;
        concatenatedParameters += value;
        if (result.isEmpty())
        {
            result += ("?" + key);
        }
        else
        {
            result += ("&" + key);
        }
        result += ("=" + urlEncode(value));
    }
    String hash = hashStringFromString(concatenatedParameters);
    result += ("&api_sig=" + hash);
    return result;
}