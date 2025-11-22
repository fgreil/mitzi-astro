#include <furi.h> // Flipper Universal Registry Implementation = Core OS functionality
#include <gui/gui.h> // GUI system
#include <input/input.h> // Input handling (buttons)
#include <stdint.h> // Standard integer types
#include <stdlib.h> // Standard library functions
#include <gui/elements.h> // to access button drawing functions
#include <stdio.h>
#include <string.h>

#define TAG "Astro" // Tag for logging purposes
#define MAX_CITIES 200
#define MAX_LINE_LENGTH 256

extern const Icon I_splash;
extern const Icon I_icon_10x10;
extern const Icon I_capital_10x10;
extern const Icon I_ButtonDown_7x4;
extern const Icon I_ButtonUp_7x4;

typedef enum {
    ScreenSplash,
	ScreenCities
} AppScreen;

typedef enum {
    MenuCountry,
	MenuCity
} AppMenu;

struct EuropeanCountry {
    char code[3];       // enough for "AT" + null
    char name[32];
    char capital[32];
};

static const struct EuropeanCountry european_countries[] = {
    [0] = {
        .code = "AT",
        .name = "Austria",
        .capital = "Vienna"
    },
    [1] = {
        .code = "BE",
        .name = "Belgium",
        .capital = "Brussels"
    },
    [2] = {
        .code = "BG",
        .name = "Bulgaria",
        .capital = "Sofia"
    },
    [3] = {
        .code = "CH",
        .name = "Switzerland",
        .capital = "Bern"
    },
    [4] = {
        .code = "CY",
        .name = "Cyprus",
        .capital = "Nicosia"
    },
    [5] = {
        .code = "CZ",
        .name = "Czech Republic",
        .capital = "Prague"
    },
    [6] = {
        .code = "DE",
        .name = "Germany",
        .capital = "Berlin"
    },
    [7] = {
        .code = "DK",
        .name = "Denmark",
        .capital = "Copenhagen"
    },
    [8] = {
        .code = "EE",
        .name = "Estonia",
        .capital = "Tallinn"
    },
    [9] = {
        .code = "ES",
        .name = "Spain",
        .capital = "Madrid"
    },
    [10] = {
        .code = "FI",
        .name = "Finland",
        .capital = "Helsinki"
    },
    [11] = {
        .code = "FR",
        .name = "France",
        .capital = "Paris"
    },
    [12] = {
        .code = "GB",
        .name = "United Kingdom",
        .capital = "London"
    },
    [13] = {
        .code = "GR",
        .name = "Greece",
        .capital = "Athens"
    },
    [14] = {
        .code = "HR",
        .name = "Croatia",
        .capital = "Zagreb"
    },
    [15] = {
        .code = "HU",
        .name = "Hungary",
        .capital = "Budapest"
    },
    [16] = {
        .code = "IE",
        .name = "Ireland",
        .capital = "Dublin"
    },
    [17] = {
        .code = "IT",
        .name = "Italy",
        .capital = "Rome"
    },
    [18] = {
        .code = "LT",
        .name = "Lithuania",
        .capital = "Vilnius"
    },
    [19] = {
        .code = "LU",
        .name = "Luxembourg",
        .capital = "Luxembourg City"
    },
    [20] = {
        .code = "LV",
        .name = "Latvia",
        .capital = "Riga"
    },
    [21] = {
        .code = "MT",
        .name = "Malta",
        .capital = "Valletta"
    },
    [22] = {
        .code = "NL",
        .name = "Netherlands",
        .capital = "Amsterdam"
    },
    [23] = {
        .code = "PL",
        .name = "Poland",
        .capital = "Warsaw"
    },
    [24] = {
        .code = "PT",
        .name = "Portugal",
        .capital = "Lisbon"
    },
    [25] = {
        .code = "RO",
        .name = "Romania",
        .capital = "Bucharest"
    },
    [26] = {
        .code = "SE",
        .name = "Sweden",
        .capital = "Stockholm"
    },
    [27] = {
        .code = "SI",
        .name = "Slovenia",
        .capital = "Ljubljana"
    }
};

// Number of countries in the array
const int country_count = sizeof(european_countries) / sizeof(european_countries[0]);

double lat = 51.5074; 
double lon = -0.1278;

// Main application structure
typedef struct {
    FuriMessageQueue* input_queue;  // Queue for handling input events
    ViewPort* view_port;            // ViewPort for rendering UI
    Gui* gui;                       // GUI instance
    uint8_t current_screen;         // 0 = first screen, 1 = second screen 
	int current_menu;	
	int selected_country;
	int selected_city;
} AppState;

// =============================================================================
// MAIN CALLBACK - called whenever the screen needs to be redrawn
// =============================================================================
void draw_callback(Canvas* canvas, void* context) {
    AppState* state = context;  // Get app context to check current screen
	char buffer[64]; // buffer for string concatination

    // Clear the canvas and set drawing color to black
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    switch (state -> current_screen) {
		case ScreenSplash: // Splash screen ----------------------------
			canvas_draw_icon(canvas, 1, 1, &I_splash); // 51 is a pixel above the buttons
			canvas_set_color(canvas, ColorBlack);
			canvas_set_font(canvas, FontPrimary);
			canvas_draw_str_aligned(canvas, 1, 1, AlignLeft, AlignTop, "Sun data");
			canvas_draw_str_aligned(canvas, 1, 10, AlignLeft, AlignTop, "for your");
			canvas_draw_str_aligned(canvas, 1, 20, AlignLeft, AlignTop, "city");
			canvas_draw_str_aligned(canvas, 88, 56, AlignLeft, AlignTop, "f418.eu");
			
			canvas_set_font(canvas, FontSecondary);
			canvas_draw_str_aligned(canvas, 1, 49, AlignLeft, AlignTop, "Hold 'back'");
			canvas_draw_str_aligned(canvas, 1, 57, AlignLeft, AlignTop, "to exit.");
			

			canvas_draw_str_aligned(canvas, 110, 1, AlignLeft, AlignTop, "v0.1");
			
			// Draw button hints at bottom using elements library
			elements_button_center(canvas, "OK"); // for the OK button
			break;	
		case ScreenCities: // Screen to choose city ----------------------------
			canvas_draw_icon(canvas, 1, -1, &I_icon_10x10);
			canvas_set_font(canvas, FontPrimary);
			canvas_draw_str_aligned(canvas, 13, 1, AlignLeft, AlignTop, "City data");
	
			canvas_set_font(canvas, FontSecondary);
			// Country chooser
			canvas_draw_frame(canvas, 1, 11, 24, 12);
			canvas_draw_str_aligned(canvas, 4, 13, AlignLeft, AlignTop, 
				european_countries[state -> selected_country].code); 
			// City chooser
			canvas_draw_frame(canvas, 26, 11, 102, 12);
			canvas_draw_str_aligned(canvas, 28, 13, AlignLeft, AlignTop, 
				european_countries[state -> selected_country].capital); 
			// Indicator whether the city is the capital	
			canvas_draw_icon(canvas, 118, 1, &I_capital_10x10);	
			// Show time zone info
			snprintf(buffer, sizeof(buffer), "UTC %sh",
				european_countries[state->selected_country].name);
			canvas_draw_str_aligned(canvas, 1, 24, AlignLeft, AlignTop, buffer);
			// Place holder for latitude and longitude
			snprintf(buffer, sizeof(buffer), "Lat: %.3f %c  Lon: %.3f %c",
				fabs(lat), (lat >= 0 ? 'N' : 'S'), fabs(lon), (lon >= 0 ? 'E' : 'W'));
			canvas_draw_str_aligned(canvas, 1, 33, AlignLeft, AlignTop, buffer);
			
			switch (state -> current_menu) {
				case MenuCountry: 
					if (state -> selected_country > 0){
						canvas_draw_icon(canvas, 16, 12, &I_ButtonUp_7x4); 
					}
					canvas_draw_icon(canvas, 16, 17, &I_ButtonDown_7x4);
					break;
				case MenuCity: 
					canvas_draw_icon(canvas, 119, 12, &I_ButtonUp_7x4);
					if (state -> selected_country < country_count - 2){
						canvas_draw_icon(canvas, 119, 17, &I_ButtonDown_7x4);
					}
					break;
			}		
			
			
			// Draw button hints at bottom using elements library
			// canvas_draw_str_aligned(canvas, 1, 57, AlignLeft, AlignTop, "to exit.");
			canvas_draw_str_aligned(canvas, 1, 49, AlignLeft, AlignTop, "Hold 'back'");
			canvas_draw_str_aligned(canvas, 1, 57, AlignLeft, AlignTop, "to exit.");
			break;	
    }
}

void input_callback(InputEvent* event, void* context) {
    AppState* app = context;
	// Put the input event into the message queue for processing
    // Timeout of 0 means non-blocking
    furi_message_queue_put(app->input_queue, event, 0);
}

// =============================================================================
// MAIN APPLICATION
// =============================================================================
int32_t astro_main(void* p) {
    UNUSED(p);

    AppState app; // Application state struct
	app.current_screen = ScreenSplash;  // Start on splash screen
	app.current_menu = MenuCountry; // Start on menu chooser
	app.selected_country = 0;
	app.selected_city = 0;
	
	// Allocate resources for rendering and 
    app.view_port = view_port_alloc(); // for rendering
    app.input_queue = furi_message_queue_alloc(1, sizeof(InputEvent)); // Only one element in queue
    // Callbacks
    view_port_draw_callback_set(app.view_port, draw_callback, &app);
    view_port_input_callback_set(app.view_port, input_callback, &app);
    // Initialize GUI
    app.gui = furi_record_open("gui");
    gui_add_view_port(app.gui, app.view_port, GuiLayerFullscreen);
    // Input handling
    InputEvent input;
    uint8_t exit_loop = 0; // Flag to exit main loop

    FURI_LOG_I(TAG, "Start the main loop.");
    while(1) {
        furi_check(
            furi_message_queue_get(app.input_queue, &input, FuriWaitForever) == FuriStatusOk);
			
		// Handle button presses based on current screen
        switch(input.key) {
		case InputKeyUp:
			if ((input.type == InputTypePress) && (app.current_screen == ScreenCities) &&
					(app.current_menu == MenuCountry)){
				if (app.selected_country > 0 ){ app.selected_country--; }
			}
			break;
		case InputKeyDown:
			if ((input.type == InputTypePress) && (app.current_screen == ScreenCities) &&
				(app.current_menu == MenuCountry)){
				if (app.selected_country < country_count - 1 ){ app.selected_country++; }
			}
			break;
		case InputKeyLeft:
			if ((input.type == InputTypePress) && (app.current_screen == ScreenCities)){
				switch (app.current_menu) {
					case MenuCity: app.current_menu = MenuCountry; break;
					case MenuCountry: app.current_menu = MenuCity; break;
				}
			}
			break;
		case InputKeyRight:
			if ((input.type == InputTypePress) && (app.current_screen == ScreenCities)) {
				switch (app.current_menu) {
					case MenuCity: app.current_menu = MenuCountry; break;
					case MenuCountry: app.current_menu = MenuCity; break;
				}
			}
			break;
		case InputKeyOk:
			if (input.type == InputTypePress){
				switch (app.current_screen) {
					case ScreenSplash:
						app.current_screen = ScreenCities;   
					break;
				}
				break;
			}
			break;
        case InputKeyBack:
		default:
			if(input.type == InputTypeLong) {
				exit_loop = 1;  // Exit app after long press
			}
            break;
		}
		// Exit main app loop if exit flag is set
        if(exit_loop) break; 
		// Trigger screen redraw
		view_port_update(app.view_port);
    }

    // Cleanup: Free all allocated resources
    view_port_enabled_set(app.view_port, false);
    gui_remove_view_port(app.gui, app.view_port);
    furi_record_close("gui");
    view_port_free(app.view_port);

    return 0;
}
