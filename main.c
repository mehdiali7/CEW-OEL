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

// Function to generate and save a report with comments
static void generateReport(const char *city, const struct WeatherInfo *info, int isAnomaly) {
    FILE *reportFile = fopen("weather_report.txt", "a");

    if (reportFile != NULL) {
        time_t t;
        struct tm *tm_info;

        time(&t);
        tm_info = localtime(&t);

        fprintf(reportFile, "Weather Report for %s:\n", city);
        fprintf(reportFile, "Description: %s\n", info->description);
        fprintf(reportFile, "Temperature: %.2f degrees Celsius\n", info->temperature - 273.15);
        fprintf(reportFile, "Humidity: %.2f%%\n", info->humidity);
        const char *comments = generateComments(info);
        fprintf(reportFile, "\nAdditional Comments: %s\n", comments);
        fprintf(reportFile, "Date and Time: %s", asctime(tm_info));

        if (isAnomaly) {
            fprintf(reportFile, "\nAnomaly Detected: Temperature is outside the normal range.\n");
        } else {
            fprintf(reportFile, "\nNo anomaly detected.\n");
        }

        fclose(reportFile);
        printf("Weather report appended. See 'weather_report.txt' for details.\n");
    } else {
        printf("Error opening file for the report\n");
    }
}

// Function to generate and save processed data
static void generateProcessedData(const char *city, const struct WeatherInfo *info, int isAnomaly) {
    char processedFileName[256];
    snprintf(processedFileName, sizeof(processedFileName), "processed_data_%s.txt", city);

    FILE *processedFile = fopen(processedFileName, "a");

    if (processedFile != NULL) {
        time_t t;
        struct tm *tm_info;

        time(&t);
        tm_info = localtime(&t);

        fprintf(processedFile, "Processed Weather Information for %s:\n", city);
        fprintf(processedFile, "Weather Description: %s\n", info->description);
        fprintf(processedFile, "Temperature: %.2f degrees Celsius\n", info->temperature - 273.15);
        fprintf(processedFile, "Humidity: %.2f%%\n", info->humidity);
        fprintf(processedFile, "Date and Time: %s", asctime(tm_info));
        fprintf(processedFile, "Anomaly Detection: %s\n", isAnomaly ? "Temperature anomaly detected!" : "No temperature anomaly detected.");
        fclose(processedFile);
        printf("Processed data appended. See '%s' for details.\n", processedFileName);
    } else {
        printf("Error opening file for processed data\n");
    }
}

// Function to set up the alarm signal
static void setupAlarm(struct WeatherInfo *weatherInfo) {
    struct sigaction sa;
    sa.sa_sigaction = generateAlerts;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGALRM, &sa, NULL);
    alarm(3600); // Set the alarm for 1 hour
}

// Function to generate real-time alerts
static void generateAlerts(int signal, siginfo_t *info, void *context) {
    struct WeatherInfo *weatherInfo = (struct WeatherInfo *)info->si_value.sival_ptr;
    printf("\nChecking for real-time alerts...\n");

    int alertTriggered = 1;  // Set based on your conditions
    if (alertTriggered) {
        // Notify relevant personnel by sending an email
        sendEmail(weatherInfo);
        printf("ALERT: Critical environmental conditions detected! Email sent.\n");
    } else {
        printf("No critical environmental conditions detected.\n");
    }

    // Set up the alarm for the next hour
    alarm(3600);
}

// Function to send email using libcurl
void sendEmail(const struct WeatherInfo *info) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();

    if (curl) {
        const char *smtp_server = "smtps://smtp.gmail.com:465";
        const char *smtp_user = "binnib786@gmail.com";
        const char *smtp_password = "binnib123$";
        const char *from_address = "binnib786@gmail.com";
        const char *to_address = "konozrashid@gmail.com";  // Replace with the recipient's email

        const char *subject = "ALERT: Critical Environmental Conditions Detected!";
        char message[512];
        snprintf(message, sizeof(message),
                 "Temperature: %.2f degrees Celsius\nHumidity: %.2f%%\n\nAdditional Comments: %s",
                 info->temperature - 273.15, info->humidity, generateComments(info));

        curl_easy_setopt(curl, CURLOPT_URL, smtp_server);
        curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_user);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_password);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from_address);
        struct curl_slist *recipients = NULL;
        recipients = curl_slist_append(recipients, to_address);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        curl_easy_setopt(curl, CURLOPT_READDATA, message);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("Email sent successfully!\n");
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }
}
//main code

int main(void) {
    CURL *curl;
    CURLcode res;

    struct WeatherInfo weatherInfo;  // Declare weatherInfo here

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);
        chunk.size = 0;

        char city_name[] = "Karachi";

        char request_url[512];
        snprintf(request_url, sizeof(request_url), "%s%s&appid=%s", API_URL, city_name, API_KEY);

        curl_easy_setopt(curl, CURLOPT_URL, request_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            FILE *rawFile = fopen("raw_data.txt", "w");
            if (rawFile != NULL) {
                fprintf(rawFile, "%s", chunk.memory);
                fclose(rawFile);
                printf("Raw data saved to 'raw_data.txt'\n");
            } else {
                printf("Error opening file for raw data\n");
            }

            parseWeatherInfo(chunk.memory, &weatherInfo);

            int isAnomaly = detectTemperatureAnomaly(weatherInfo.temperature);

            printf("\nProcessed Weather Information:\n");
            printf("Weather in %s: %s\n", city_name, weatherInfo.description);
            printf("Temperature: %.2f degrees Celsius\n", weatherInfo.temperature - 273.15);
            printf("Humidity: %.2f%%\n", weatherInfo.humidity);

            if (isAnomaly) {
                printf("\nTemperature anomaly detected!\n");
            } else {
                printf("\nNo temperature anomaly detected.\n");
            }

            generateReport(city_name, &weatherInfo, isAnomaly);
            generateProcessedData(city_name, &weatherInfo, isAnomaly);

            // Set up the alarm for real-time alerts
            setupAlarm(&weatherInfo);
        }

        free(chunk.memory);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    // Keep the program running to handle real-time alerts
    while (1) {
        sleep(1);
    }

    return 0;
}
