#pragma once
#include <furi.h>

typedef struct {
    const char* country_code;
    const char* country_name;
    const char* city_name;
    const char* city_description;
    float utc_offset;
    int deg_lon;
    int deg_lat;
    int min_lon;
    int min_lat;
} CityInfo;

typedef struct {
    const char* country_name;
    const CityInfo* cities;
    size_t city_count;
} CountryInfo;

extern const CountryInfo countries[];
extern const size_t countries_count;

const CountryInfo* find_country_by_name(const char* name);
