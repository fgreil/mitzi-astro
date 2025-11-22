#ifndef SUNTIMES_H
#define SUNTIMES_H

typedef struct {
    int civil_dawn_hour, civil_dawn_minute;
    int civil_dusk_hour, civil_dusk_minute;
    int nautical_dawn_hour, nautical_dawn_minute;
    int nautical_dusk_hour, nautical_dusk_minute;
    int astronomical_dawn_hour, astronomical_dawn_minute;
    int astronomical_dusk_hour, astronomical_dusk_minute;
    int sunrise_hour, sunrise_minute;
    int sunset_hour, sunset_minute;
    int daylength_hour, daylength_minute;
    char comment[256];
} SunTimes;

SunTimes sun(int year, int month, int day,
             int lat_degree, int lat_minute,
             int lon_degree, int lon_minute,
             int height_meters,
             float time_zone_offset_to_utc_in_hours);

#endif
