#pragma once
#include <furi.h>
#include <gui/gui.h>

#define MAX_CITIES 200
#define MAX_COUNTRIES 50
#define MAX_STRING_LENGTH 64

typedef struct {
    char country_code[4];
    char country_name[MAX_STRING_LENGTH];
    char province_state[MAX_STRING_LENGTH];
    char city_name[MAX_STRING_LENGTH];
    char city_description[MAX_STRING_LENGTH];
    float utc_offset;
    int longitude_degrees;
    int longitude_minutes;
    int latitude_degrees;
    int latitude_minutes;
} CityData;

typedef struct {
    char name[MAX_STRING_LENGTH];
    int city_indices[MAX_CITIES];
    int city_count;
} CountryData;

typedef struct {
    CityData cities[MAX_CITIES];
    CountryData countries[MAX_COUNTRIES];
    int total_cities;
    int total_countries;
} CityDatabase;

bool city_data_load_from_csv(CityDatabase* db, const char* file_path);
void city_data_free(CityDatabase* db);
