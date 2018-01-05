/* Location: https://developer.mbed.org/users/mimil/code/PicoJSONSample/ */
#include "mbed.h"
#include "picojson.h"


void parse() {
    picojson::value v;
    const char *json = "{\"string\": \"it works\", \"number\": 3.14, \"integer\":5}";

    string err = picojson::parse(v, json, json + strlen(json));
    printf("res error? %s\r\n", err.c_str());
    printf("string =%s\r\n" ,  v.get("string").get<string>().c_str());
    printf("number =%f\r\n" ,  v.get("number").get<double>());
    printf("integer =%d\r\n" ,  (int)v.get("integer").get<double>());
}

void serialize() {
    picojson::object v;
    picojson::object inner;
    string val = "tt";

    v["aa"] = picojson::value(val);
    v["bb"] = picojson::value(1.66);
    inner["test"] =  picojson::value(true);
    inner["integer"] =  picojson::value(1.0);
    v["inner"] =  picojson::value(inner);

    string str = picojson::value(v).serialize();
    printf("serialized content = %s\r\n" ,  str.c_str());
}

void advanced() {
    picojson::value v;
    const char *jsonsoure =
        "{\"menu\": {"
        "\"id\": \"f\","
        "\"popup\": {"
        "  \"menuitem\": ["
        "    {\"v\": \"0\"},"
        "    {\"v\": \"1\"},"
        "    {\"v\": \"2\"}"
        "   ]"
        "  }"
        "}"
        "}";
    char * json = (char*) malloc(strlen(jsonsoure)+1);
    strcpy(json, jsonsoure);
    string err = picojson::parse(v, json, json + strlen(json));
    printf("res error? %s\r\n", err.c_str());

    printf("id =%s\r\n", v.get("menu").get("id").get<string>().c_str());

    picojson::array list = v.get("menu").get("popup").get("menuitem").get<picojson::array>();

    for (picojson::array::iterator iter = list.begin(); iter != list.end(); ++iter) {
        printf("menu item value =%s\r\n", (*iter).get("v").get<string>().c_str());
    }
}

int main() {
    printf("\r\nStarting PicoJSONSample "__TIME__"\r\n");
    printf(">>> parsing \r\n");
    parse();
    printf(">>> serializing \r\n");
    serialize();
    printf(">>> advanced parsing \r\n");
    advanced();
    printf("Ending PicoJSONSample " __TIME__ "\r\n");
}
