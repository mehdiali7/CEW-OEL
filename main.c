#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define API_KEY "b94486085331eff2b6fc6316d8951cf2"
#define API_URL "http://api.openweathermap.org/data/2.5/weather?q="

// Struct to hold weather information
struct WeatherInfo {
    char description[256];
    double temperature;
    double humidity;
};

// Struct to hold memory data
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Function prototypes
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
static void parseWeatherInfo(const char *response, struct WeatherInfo *info);
static int detectTemperatureAnomaly(double temperature);
static const char *generateComments(const struct WeatherInfo *info);
static void generateReport(const char *city, const struct WeatherInfo *info, int isAnomaly);
static void generateProcessedData(const char *city, const struct WeatherInfo *info, int isAnomaly);
static void setupAlarm(struct WeatherInfo *weatherInfo);
static void generateAlerts(int signal, siginfo_t *info, void *context);
void sendEmail(const struct WeatherInfo *info);

// Function to handle received data from libcurl
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Function to extract weather information from API response
static void parseWeatherInfo(const char *response, struct WeatherInfo *info) {
    const char *descriptionStart = strstr(response, "\"description\":\"");
    const char *temperatureStart = strstr(response, "\"temp\":");
    const char *humidityStart = strstr(response, "\"humidity\":");

    if (descriptionStart != NULL) {
        sscanf(descriptionStart, "\"description\":\"%255[^\"]", info->description);
    }

    if (temperatureStart != NULL) {
        sscanf(temperatureStart, "\"temp\":%lf", &(info->temperature));
    }

    if (humidityStart != NULL) {
        sscanf(humidityStart, "\"humidity\":%lf", &(info->humidity));
    }
}

// Function to detect anomalies in temperature
static int detectTemperatureAnomaly(double temperature) {
    const double upperLimitCelsius = 20.0;
    const double lowerLimitCelsius = 10.0;

    double temperatureCelsius = temperature - 273.15;

    if (temperatureCelsius > upperLimitCelsius || temperatureCelsius < lowerLimitCelsius) {
        return 1;
    }
    return 0;
}

// Function to generate comments based on weather conditions
static const char *generateComments(const struct WeatherInfo *info) {
    if (info->temperature > 25.0 && info->humidity > 80.0) {
        return "Expect warm and humid weather. Consider light clothing and staying hydrated.";
    } else if (info->temperature < 10.0) {
        return "It's quite cold. Make sure to wear warm clothing and stay indoors if possible.";
    } else {
        return "Weather conditions are generally normal. Enjoy your day!";
    }
}
