#include "suntimes.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

// ------------------------------------------------------------
// STRUCT: SunTimes
// ------------------------------------------------------------
// Contains all relevant solar times for a specific date and location.
// All times are expressed in hour + minute integer fields.
//
// Includes Civil, Nautical, and Astronomical dawn/dusk times,
// plus sunrise, sunset, and day length.
//
// If the sun does not rise or set (polar day/night), the times are
// set to -1 and 'comment' contains the explanation.
//
typedef struct {
    // Civil dawn/dusk (sun at -6 degrees)
    int civil_dawn_hour, civil_dawn_minute;
    int civil_dusk_hour, civil_dusk_minute;

    // Nautical dawn/dusk (sun at -12 degrees)
    int nautical_dawn_hour, nautical_dawn_minute;
    int nautical_dusk_hour, nautical_dusk_minute;

    // Astronomical dawn/dusk (sun at -18 degrees)
    int astronomical_dawn_hour, astronomical_dawn_minute;
    int astronomical_dusk_hour, astronomical_dusk_minute;

    // Sunrise & sunset times
    int sunrise_hour, sunrise_minute;
    int sunset_hour, sunset_minute;

    // Day length
    int daylength_hour, daylength_minute;

    // Comment for errors or special conditions
    char comment[256];

} SunTimes;


// ------------------------------------------------------------
// HELPER: Check Gregorian reform (1582)
// Invalid dates: October 5–14, 1582 (inclusive)
// ------------------------------------------------------------
int is_gregorian_reform_invalid(int y, int m, int d) {
    if (y == 1582 && m == 10 && d >= 5 && d <= 14)
        return 1;
    return 0;
}

// ------------------------------------------------------------
// HELPER: Check leap year (Gregorian rules, extended backward)
// ------------------------------------------------------------
int is_leap_year(int y) {
    if (y % 4 != 0) return 0;
    if (y % 100 == 0 && y % 400 != 0) return 0;
    return 1;
}

// ------------------------------------------------------------
// HELPER: Validate date; return 1 if valid, 0 if invalid
// ------------------------------------------------------------
int is_valid_date(int y, int m, int d) {
    if (y < 0 || y > 3000) return 0;
    if (m < 1 || m > 12) return 0;

    if (is_gregorian_reform_invalid(y, m, d)) return 0;

    int dim[] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
    if (m == 2 && is_leap_year(y)) dim[2] = 29;

    if (d < 1 || d > dim[m]) return 0;

    return 1;
}

// ------------------------------------------------------------
// HELPER: Convert hour float to (hour, minute)
// ------------------------------------------------------------
void split_time(double hours, int *h, int *m) {
    if (hours < 0) {
        *h = *m = -1;
        return;
    }
    *h = (int)hours;
    *m = (int)round((hours - *h) * 60.0);
    if (*m == 60) { *m = 0; (*h)++; }
}

// ------------------------------------------------------------
// HELPER: Compute solar event given depression angle
// Uses simplified NOAA algorithm
// ------------------------------------------------------------
double compute_event_time(int year, int month, int day,
                          double latitude_deg, double longitude_deg,
                          double tz_offset, double depression_deg,
                          int is_sunrise) {
    // Convert degrees to radians
    double lat = latitude_deg * PI / 180.0;

    // Days since J2000.0 (approx)
    int N1 = floor(275 * month / 9);
    int N2 = floor((month + 9) / 12);
    int N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
    int N = N1 - (N2 * N3) + day - 30;

    double lng_hour = longitude_deg / 15.0;

    double t = N + ((is_sunrise ? 6.0 : 18.0) - lng_hour) / 24;

    // Sun mean anomaly
    double M = (0.9856 * t) - 3.289;

    // Sun true longitude
    double L = M + 1.916 * sin(M * PI/180) +
                    0.020 * sin(2 * M * PI/180) + 282.634;
    if (L > 360) L -= 360;
    if (L < 0)   L += 360;

    // Sun right ascension
    double RA = atan(0.91764 * tan(L * PI/180)) * 180/PI;
    if (RA < 0) RA += 360;
    if (RA >= 360) RA -= 360;

    // Adjust RA to same quadrant as L
    int L_quadrant  = (int)(floor(L/90)) * 90;
    int RA_quadrant = (int)(floor(RA/90)) * 90;
    RA = RA + (L_quadrant - RA_quadrant);

    RA /= 15.0;

    // Sun declination
    double sinDec = 0.39782 * sin(L * PI/180);
    double cosDec = cos(asin(sinDec));

    // Sun local hour angle
    double cosH = (cos((90 + depression_deg) * PI/180) -
                   (sinDec * sin(lat))) / (cosDec * cos(lat));

    if (cosH > 1) return -1;   // Sun never rises (polar night)
    if (cosH < -1) return -1;  // Sun never sets   (midnight sun)

    double H = (is_sunrise ? 360 - acos(cosH) * 180/PI
                           : acos(cosH) * 180/PI) / 15.0;

    // Local mean time
    double T = H + RA - (0.06571 * t) - 6.622;

    // Convert to UTC, then add time zone
    double UT = T - lng_hour;
    if (UT < 0) UT += 24;
    if (UT >= 24) UT -= 24;

    double localT = UT + tz_offset;
    if (localT < 0) localT += 24;
    if (localT >= 24) localT -= 24;

    return localT;
}


// ------------------------------------------------------------
// MAIN FUNCTION: sun()
// ------------------------------------------------------------
SunTimes sun(int year, int month, int day,
             int lat_degree, int lat_minute,
             int lon_degree, int lon_minute,
             int height_meters,
             float time_zone_offset_to_utc_in_hours)
{
    SunTimes result;
    memset(&result, -1, sizeof(result));
    strcpy(result.comment, "OK");

    // Validate date
    if (!is_valid_date(year, month, day)) {
        strcpy(result.comment,
               "Error: Invalid date (consider Gregorian reform 1582-10-5..14)");
        return result;
    }

    // Convert lat/lon to decimal degrees
    double lat = lat_degree + lat_minute / 60.0;
    double lon = lon_degree + lon_minute / 60.0;

    // Twilight angles
    double civil_angle        = -6.0;
    double nautical_angle     = -12.0;
    double astronomical_angle = -18.0;
    double sunrise_angle      = -0.833;   // includes refraction + solar radius

    // Compute all events
    double civil_dawn  = compute_event_time(year,month,day,lat,lon,time_zone_offset_to_utc_in_hours,civil_angle,1);
    double civil_dusk  = compute_event_time(year,month,day,lat,lon,time_zone_offset_to_utc_in_hours,civil_angle,0);
    double nautical_dawn  = compute_event_time(year,month,day,lat,lon,time_zone_offset_to_utc_in_hours,nautical_angle,1);
    double nautical_dusk  = compute_event_time(year,month,day,lat,lon,time_zone_offset_to_utc_in_hours,nautical_angle,0);
    double astronomical_dawn  = compute_event_time(year,month,day,lat,lon,time_zone_offset_to_utc_in_hours,astronomical_angle,1);
    double astronomical_dusk  = compute_event_time(year,month,day,lat,lon,time_zone_offset_to_utc_in_hours,astronomical_angle,0);

    double sunrise = compute_event_time(year,month,day,lat,lon,time_zone_offset_to_utc_in_hours,sunrise_angle,1);
    double sunset  = compute_event_time(year,month,day,lat,lon,time_zone_offset_to_utc_in_hours,sunrise_angle,0);

    // Polar cases
    if (sunrise < 0 && sunset < 0) {
        strcpy(result.comment, "Polar day or polar night: sun does not rise or set.");
        return result;
    }

    // Split times into hour/minute
    split_time(civil_dawn, &result.civil_dawn_hour, &result.civil_dawn_minute);
    split_time(civil_dusk, &result.civil_dusk_hour, &result.civil_dusk_minute);

    split_time(nautical_dawn, &result.nautical_dawn_hour, &result.nautical_dawn_minute);
    split_time(nautical_dusk, &result.nautical_dusk_hour, &result.nautical_dusk_minute);

    split_time(astronomical_dawn, &result.astronomical_dawn_hour, &result.astronomical_dawn_minute);
    split_time(astronomical_dusk, &result.astronomical_dusk_hour, &result.astronomical_dusk_minute);

    split_time(sunrise, &result.sunrise_hour, &result.sunrise_minute);
    split_time(sunset,  &result.sunset_hour,  &result.sunset_minute);

    // Compute daylength
    if (sunrise >= 0 && sunset >= 0) {
        double dl = sunset - sunrise;
        if (dl < 0) dl += 24;
        split_time(dl, &result.daylength_hour, &result.daylength_minute);
    }

    return result;
}

