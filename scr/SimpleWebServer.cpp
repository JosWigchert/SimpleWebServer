#include "SimpleWebServer.h"

#include "visitors/InteractableElementCallbackVisitor.h"

#include <ArduinoJson.h>
#include <SPIFFS.h>

SimpleWebServer::SimpleWebServer()
{
    server = AsyncWebServer(80);
    events = AsyncEventSource("api/events");
}

SimpleWebServer::~SimpleWebServer()
{
}

void SimpleWebServer::begin()
{
    events.onConnect([this](AsyncEventSourceClient *client)
                     { handleEvent(client); });
    server.addHandler(&events);

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
              { handleRoot(request); });
    server.on("/api/values", HTTP_GET, [this](AsyncWebServerRequest *request)
              { handleValues(request); });
    server.on("/api/callback", HTTP_GET, [this](AsyncWebServerRequest *request)
              { handleCallback(request); });
    server.onNotFound([this](AsyncWebServerRequest *request)
                      { handleFile(request); });
    server.begin();
}

void SimpleWebServer::stop()
{
    server.end();
}

void SimpleWebServer::addHTMLElement(Position position, BaseElement *element)
{
    elements.emplace(position, element);
}

void SimpleWebServer::removeHTMLElement(Position position)
{
    if (elements.find(position) != elements.end())
    {
        delete elements[position];
        elements.erase(position);
    }
}

void SimpleWebServer::addValueWatch(const char *id, void *valuePtr, ValueType valueType)
{
    ValueInfo valueInfo = {valuePtr, valueType};
    addValueWatch(id, valueInfo);
}

void SimpleWebServer::addValueWatch(const char *id, ValueInfo valueInfo)
{
    values.emplace(id, valueInfo);
}

void SimpleWebServer::removeValueWatch(const char *id)
{
    values.erase(id);
}

void SimpleWebServer::handleRoot(AsyncWebServerRequest *request)
{
    String response = generateHTML();
    request->send_P(200, "text/html", response.c_str());
}

void SimpleWebServer::handleValues(AsyncWebServerRequest *request)
{
    StaticJsonDocument<1024> jsonBuffer;
    JsonObject json = jsonBuffer.to<JsonObject>();

    for (const auto &pair : values)
    {
        switch (pair.second.valueType)
        {
        case INT_TYPE:
            json[pair.first] = *(int *)(pair.second.valuePtr);
            break;
        case FLOAT_TYPE:
            json[pair.first] = *(float *)(pair.second.valuePtr);
            break;
        case DOUBLE_TYPE:
            json[pair.first] = *(double *)(pair.second.valuePtr);
            break;
        case BOOL_TYPE:
            json[pair.first] = *(bool *)(pair.second.valuePtr);
            break;
        case STRING_TYPE:
            json[pair.first] = *(String *)(pair.second.valuePtr);
            break;
        case STD_STRING_TYPE:
            json[pair.first] = *(std::string *)(pair.second.valuePtr);
            break;
        }
    }

    String jsonString;
    serializeJson(json, jsonString);

    request->send_P(200, "application/json", jsonString.c_str());
}

void SimpleWebServer::handleCallback(AsyncWebServerRequest *request)
{
    // Check if it's a POST request
    if (request->method() != HTTP_POST)
    {
        request->send(400, "text/plain", "Bad Request");
        return;
    }

    // Check if there is any data available in the request
    if (request->args() == 0)
    {
        request->send_P(400, "text/plain", "No JSON data");
        return;
    }

    // Get the JSON data from the request
    String jsonString = request->arg("plain");

    // Parse the JSON data
    DynamicJsonDocument jsonDocument(1024); // Adjust the size according to your JSON data
    DeserializationError error = deserializeJson(jsonDocument, jsonString);

    // Check for parsing errors
    if (error)
    {
        Serial.print("Parsing failed: ");
        Serial.println(error.c_str());
        request->send_P(400, "text/plain", "JSON Parsing Error");
        return;
    }

    // Access the JSON data fields
    const char *identifier = jsonDocument["identifier"];
    auto value = jsonDocument["value"];

    InteractableElementCallbackVisitor visitor(value);

    for (const auto &pair : elements)
    {
        if (pair.second->hasIdentifier(identifier))
        {
            pair.second->accept(visitor);
        }
    }

    request->send_P(200, "text/plain", "Success");
}

void SimpleWebServer::handleFile(AsyncWebServerRequest *request)
{
    String path = request->urlDecode(request->url());
    Serial.printf("path: \"%s\"\n", path);

    // Check if the file exists in SPIFFS
    if (SPIFFS.exists(path))
    {
        // Determine the content type based on the file extension
        String contentType;
        if (path.endsWith(".html"))
        {
            contentType = "text/html";
        }
        else if (path.endsWith(".css"))
        {
            contentType = "text/css";
        }
        else if (path.endsWith(".js"))
        {
            contentType = "application/javascript";
        }
        else
        {
            contentType = "text/plain";
        }

        Serial.printf("SPIFF FOUND\n\n");
        // request.send(SPIFFS, path, contentType, true);
        // Serve the file
        File file = SPIFFS.open(path, "r");
        if (file)
        {
            request->send_P(200, contentType.c_str(), file.readString().c_str());
            file.close();
            return;
        }
    }

    // If the file does not exist, return a 404 Not Found error
    request->send_P(404, "text/plain", "Not found");
}

String SimpleWebServer::generateHTML()
{
    String htmlContent = "<!DOCTYPE html>\n";
    htmlContent += "<html lang=\"en\">\n";
    htmlContent += "<head>\n";
    htmlContent += "\t<meta charset=\"utf-8\">\n";
    htmlContent += "\t<title>Arduino Webserver</title>\n";
    htmlContent += "\t<meta name=\"description\" content=\"Arduino Webserver\">\n";
    htmlContent += "\t<meta name=\"author\" content=\"\">\n";
    htmlContent += "\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
    htmlContent += "\t<link rel=\"stylesheet\" href=\"/css/font.min.css\">\n";
    htmlContent += "\t<link rel=\"stylesheet\" href=\"/css/normalize.min.css\">\n";
    htmlContent += "\t<link rel=\"stylesheet\" href=\"/css/skeleton.min.css\">\n";
    htmlContent += "\t<link rel=\"stylesheet\" href=\"/css/custom.min.css\">\n";
    htmlContent += "\t<link rel=\"icon\" href=\"favicon.gif\" type=\"image/gif\">\n";
    htmlContent += "</head>\n";
    htmlContent += "<body>\n";

    // Initialize the container where all the rows will sit
    htmlContent += "<div class=\"container\">";

    // Add the title to the HTML content (centered and at the top)
    htmlContent += "\t<div class=\"row\">\n";
    htmlContent += "\t\t<div class=\"column\" style=\"margin-top: 25%\">\n";
    htmlContent += "\t\t<h1>Arduino Web Server</h1>\n";
    htmlContent += "\t</div>\n";

    // Add buttons and text inputs to the HTML content
    int currentRow = 0;
    int currentColumn = 0;

    htmlContent += "\t<div class=\"row\" style=\"margin-top: 8px\">\n";
    for (const auto &pair : elements)
    {
        while (pair.first.getRow() > currentRow)
        {
            htmlContent += "\t</div>\n";
            htmlContent += "\t<div class=\"row\" style=\"margin-top: 8px\">\n";

            currentRow++;
            currentColumn = 0;
        }

        htmlContent += "\t\t<div class=\"four columns\">\n";
        while (pair.first.getColumn() > currentColumn)
        {
            htmlContent += "\t\t</div>\n";
            htmlContent += "\t\t<div class=\"four columns\">\n";
            currentColumn++;
        }

        htmlContent += "\t\t\t";
        htmlContent += pair.second->toString();
        htmlContent += "\n\t\t</div>\n"; // Close the .four .columns div
        currentColumn++;
    }

    htmlContent += "</div>\n"; // Close the .container div

    // Add a script tag to link the script.js file
    htmlContent += "<script src='/script.js'></script>\n";

    htmlContent += "</body></html>\n";

    return htmlContent;
}

void SimpleWebServer::handleEvent(AsyncEventSourceClient *client)
{
    Serial.printf("handleEvent\n");
    if (client->lastId())
    {
        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
}

void SimpleWebServer::pushUpdate()
{
    StaticJsonDocument<1024> jsonBuffer;
    JsonObject json = jsonBuffer.to<JsonObject>();

    for (const auto &pair : values)
    {
        switch (pair.second.valueType)
        {
        case INT_TYPE:
            json[pair.first] = *(int *)(pair.second.valuePtr);
            break;
        case FLOAT_TYPE:
            json[pair.first] = *(float *)(pair.second.valuePtr);
            break;
        case DOUBLE_TYPE:
            json[pair.first] = *(double *)(pair.second.valuePtr);
            break;
        case BOOL_TYPE:
            json[pair.first] = *(bool *)(pair.second.valuePtr);
            break;
        case STRING_TYPE:
            json[pair.first] = *(String *)(pair.second.valuePtr);
            break;
        case STD_STRING_TYPE:
            json[pair.first] = *(std::string *)(pair.second.valuePtr);
            break;
        }
    }

    String jsonString;
    serializeJson(json, jsonString);

    Serial.println("Sending content");
    events.send(jsonString.c_str(), NULL, millis());
}