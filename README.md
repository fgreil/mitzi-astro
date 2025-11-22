# mitzi-astro
A demo app for Flipper Zero to calculate sunset and sunrise (not yet).

![Splash screen](data/splash.png)

The user first has to choose country and then a city from a list in two menus. Once the city is determinted, the user sees more information about that city.

## The user flow
The **Splash Screen** displays app title and version information, you can proceed with `OK` or exit.
The **City Data Screen** has two menu boxes, the country selector (with 2-letter ISO country codes) and the city selector. Based on the choices, the user currently sees the following data:
* Capital Indicator icon appears for capital cities
* Latitude, longitude, elevation, and UTC offset

## Key Functions
* `load_cities_from_csv()` loads city data from external CSV file
* `filter_cities_by_country()` filters city list based on selected country
* `draw_callback()` renders UI based on current screen and state
* `input_callback()`h andles button input events

## Further notes.
The **data file** is located at `/ext/apps_data/mitzi-astro/european_cities.txt` (note the postfix!). It is in CSV with fields like `country_code`, `utc_shift`, `city_name`, `longitude`, `latitude`, `elevation_m`. It supports up to 200 cities (specified in the code).

## Sun maths
The function `SunTimes sun(int year, int month, int day, int lat_degree, int lat_minute, int lon_degree, int lon_minute, int height_meters, float time_zone_offset_to_utc_in_hours)` located in [suntimes.c](suntimes.c) computes for any date between year 0 and 3000, using formulas from https://gml.noaa.gov/grad/solcalc/calcdetails.html
* astronomical dawn and astronomical dusk,
* nautical dawn and nautical dusk,
* civil dawn and civil dusk,
* sunrise, sunset,
* day length,

The function musts return a `SunTimes`, including a field `comment` reflecting special conditions (polar night or day!) or errors (non-existent dates during the 1582 Gregorian calendar reform).

# Links and further reading
* [Mitzi (C. C. Odontoceti)](https://www.floridamemory.com/items/show/82844) was a movie star, born in 1958 and died in 1972.
* https://developer.flipper.net/flipperzero/doxygen/applications.html is the official (too) short introduction to writing own apps.
* The real app **tuning fork** seems to be well documented, see https://github.com/besya/flipperzero-tuning-fork/tree/main 
* Under https://github.com/flipperdevices/flipperzero-firmware/tree/dev/assets/icons/Common we find nice icons, e.g. `ButtonLeft_4x7.png`.
* [uFBT](https://github.com/flipperdevices/flipperzero-ufbt) is the *micro Flipper Build Tool*, a cross-platform tool for building applications. It requires python to be installed. 
