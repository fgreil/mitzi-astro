# mitzi-astro
A demo app for Flipper Zero to calculate sunset and sunrise (not yet).

![Splash screen](data/splash.png)

The user first has to choose country and then a city from a list in two menus. Once the city is determinted, the user sees more information about that city.

## The user flow

The base App consists of 4 screens *and can not yet do anything related to astronomy*:
1. A Splash-Screen should welcome the user once the app is opened. A simple PNG-file is displayed. Pressing the OK-button or any of the arrow keys moves to the next screen, the "back"-button terminates the app. The next screen is the main menu, if there is already a user-preference persisted. After the first startup and whenever the local preferences-file is missing or empty, the next screen after the splash screen is the "Choose"-screen.
2. The screen "main menu" consists of a title "City info" in bold, and four menu entries below. A right-pointing arrow-cursor allows to choose between the options "Choose City", "City Info" and "Exit" with the "up" and "down" button. "City Info" is only shown as menu entry, if the user-preference-file exists and has a valid city entry. The screen also shows a right-aligned PNG-image of 64x64 pixels with a logo. Pressing the "back"-button terminates the app. 
3. The screen entitled "Choose City" comes with an instruction "Please choose a city". Here, a CSV-file like the attached file is read in. There are two selector-fields, "Country" and "City". At start, the "City"-selector is on focus. The "left" and the "right" button moves the focus to the other one. "OK" saves the chosen city into the preference file. The back-button does not save the changes and moves back to the "main menu".
4. The screen "City Info" shows more details about the saved preference. The bold-printed title is the city name with the country code in brackets. The next line shows the longitude in degrees and minutes, the line after that shows the city description from the file, and the last one shows the current time in hours and minutes, adjusted for the hours obtained from the file.

## Details on the architecture

* The CSV file is parsed at runtime.
* The UI is scene-based, i.e. there are different screens. Widget for details display.

Please note that the CSV file goes in /ext/apps_data/mitzi-astro/cities.csv

## Fun facts
* [Mitzi (C. C. Odontoceti)](https://www.floridamemory.com/items/show/82844) was a movie star, born in 1958 and died in 1972.

# Links and further reading
* https://developer.flipper.net/flipperzero/doxygen/applications.html is the official (too) short introduction to writing own apps.
* The real app **tuning fork** seems to be well documented, see https://github.com/besya/flipperzero-tuning-fork/tree/main 
* Under https://github.com/flipperdevices/flipperzero-firmware/tree/dev/assets/icons/Common we find nice icons, e.g. `ButtonLeft_4x7.png`.
* [uFBT](https://github.com/flipperdevices/flipperzero-ufbt) is the *micro Flipper Build Tool*, a cross-platform tool for building applications. It requires python to be installed. 
