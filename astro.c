#include <furi.h> // Flipper Universal Registry Implementation = Core OS functionality
#include <gui/gui.h> // GUI system
#include <input/input.h> // Input handling (buttons)
#include <stdint.h> // Standard integer types
#include <stdlib.h> // Standard library functions
#include <gui/elements.h> // to access button drawing functions
#include <stdio.h>
#include <string.h>
#include <storage/storage.h>

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
    [0] = { .code = "AD", .name = "Andorra", .capital = "Andorra la Vella" },
    [1] = { .code = "AL", .name = "Albania", .capital = "Tirana" },
    [2] = { .code = "AM", .name = "Armenia", .capital = "Yerevan" },
    [3] = { .code = "AT", .name = "Austria", .capital = "Vienna" },
    [4] = { .code = "AZ", .name = "Azerbaijan", .capital = "Baku" },
    [5] = { .code = "BA", .name = "Bosnia and Herzegovina", .capital = "Sarajevo" },
    [6] = { .code = "BE", .name = "Belgium", .capital = "Brussels" },
    [7] = { .code = "BG", .name = "Bulgaria", .capital = "Sofia" },
    [8] = { .code = "BY", .name = "Belarus", .capital = "Minsk" },
    [9] = { .code = "CH", .name = "Switzerland", .capital = "Bern" },
    [10] = { .code = "CY", .name = "Cyprus", .capital = "Nicosia" },
    [11] = { .code = "CZ", .name = "Czech Republic", .capital = "Prague" },
    [12] = { .code = "DE", .name = "Germany", .capital = "Berlin" },
    [13] = { .code = "DK", .name = "Denmark", .capital = "Copenhagen" },
    [14] = { .code = "EE", .name = "Estonia", .capital = "Tallinn" },
    [15] = { .code = "ES", .name = "Spain", .capital = "Madrid" },
    [16] = { .code = "FI", .name = "Finland", .capital = "Helsinki" },
    [17] = { .code = "FR", .name = "France", .capital = "Paris" },
    [18] = { .code = "GB", .name = "United Kingdom", .capital = "London" },
    [19] = { .code = "GE", .name = "Georgia", .capital = "Tbilisi" },
    [20] = { .code = "GR", .name = "Greece", .capital = "Athens" },
    [21] = { .code = "HR", .name = "Croatia", .capital = "Zagreb" },
    [22] = { .code = "HU", .name = "Hungary", .capital = "Budapest" },
    [23] = { .code = "IE", .name = "Ireland", .capital = "Dublin" },
    [24] = { .code = "IS", .name = "Iceland", .capital = "Reykjavik" },
    [25] = { .code = "IT", .name = "Italy", .capital = "Rome" },
    [26] = { .code = "LI", .name = "Liechtenstein", .capital = "Vaduz" },
    [27] = { .code = "LT", .name = "Lithuania", .capital = "Vilnius" },
    [28] = { .code = "LU", .name = "Luxembourg", .capital = "Luxembourg City" },
    [29] = { .code = "LV", .name = "Latvia", .capital = "Riga" },
    [30] = { .code = "MC", .name = "Monaco", .capital = "Monaco" },
    [31] = { .code = "MD", .name = "Moldova", .capital = "Chisinau" },
    [32] = { .code = "ME", .name = "Montenegro", .capital = "Podgorica" },
    [33] = { .code = "MK", .name = "North Macedonia", .capital = "Skopje" },
    [34] = { .code = "MT", .name = "Malta", .capital = "Valletta" },
    [35] = { .code = "NL", .name = "Netherlands", .capital = "Amsterdam" },
    [36] = { .code = "NO", .name = "Norway", .capital = "Oslo" },
    [37] = { .code = "PL", .name = "Poland", .capital = "Warsaw" },
    [38] = { .code = "PT", .name = "Portugal", .capital = "Lisbon" },
    [39] = { .code = "RO", .name = "Romania", .capital = "Bucharest" },
    [40] = { .code = "RS", .name = "Serbia", .capital = "Belgrade" },
    [41] = { .code = "SE", .name = "Sweden", .capital = "Stockholm" },
    [42] = { .code = "SI", .name = "Slovenia", .capital = "Ljubljana" },
    [43] = { .code = "SK", .name = "Slovakia", .capital = "Bratislava" },
    [44] = { .code = "SM", .name = "San Marino", .capital = "San Marino" },
    [45] = { .code = "UA", .name = "Ukraine", .capital = "Kyiv" },
    [46] = { .code = "VA", .name = "Vatican City", .capital = "Vatican City" }
};

// Number of countries in the array
const int country_count = sizeof(european_countries) / sizeof(european_countries[0]);

// City structure
typedef struct {
    char country_code[3];
    double utc_shift;
    char name[32];
    double longitude;
    double latitude;
    int elevation_m;
    uint8_t is_capital;
} City;

// Global city storage
static City cities[MAX_CITIES];
static int city_count = 0;
static int filtered_city_count = 0;
static int filtered_city_indices[MAX_CITIES];

// ChatGPT test coordinate
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
	int selected_city; // city ID from CSV file
	int city_idx; // filtered city index
	bool csv_loaded;  // Status indicator
} AppState;

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================
static float parse_float(const char* str) { // Manual string to float conversion
    float result = 0.0f;
    float sign = 1.0f;
    float decimal = 0.0f;
    int decimal_places = 0;
    bool past_decimal = false;
    
    if(*str == '-') {
        sign = -1.0f;
        str++;
    } else if(*str == '+') {
        str++;
    }
    
    while(*str) {
        if(*str >= '0' && *str <= '9') {
            if(past_decimal) {
                decimal = decimal * 10.0f + (*str - '0');
                decimal_places++;
            } else {
                result = result * 10.0f + (*str - '0');
            }
        } else if(*str == '.') {
            past_decimal = true;
        }
        str++;
    }
    
    while(decimal_places > 0) {
        decimal /= 10.0f;
        decimal_places--;
    }
    
    return sign * (result + decimal);
}

// Manual string to int conversion
static int parse_int(const char* str) {
    int result = 0;
    int sign = 1;
    
    if(*str == '-') {
        sign = -1;
        str++;
    } else if(*str == '+') {
        str++;
    }
    
    while(*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

// Manual CSV field extraction
static char* get_next_field(char** str_ptr) {
    if(!*str_ptr || **str_ptr == '\0' || **str_ptr == '\n' || **str_ptr == '\r') {
        return NULL;
    }
    
    char* start = *str_ptr;
    char* end = start;
    
    // Find next comma or end of line
    while(*end && *end != ',' && *end != '\n' && *end != '\r') {
        end++;
    }
    
    // Null-terminate this field
    if(*end == ',') {
        *end = '\0';
        *str_ptr = end + 1;
    } else {
        *end = '\0';
        *str_ptr = end;
    }
    
    return start;
}

// Load cities from file
bool load_cities_from_csv(const char* filepath) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    
    if(!storage_file_open(file, filepath, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FURI_LOG_E(TAG, "Failed to open file");
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    
    char line[MAX_LINE_LENGTH];
    int line_pos = 0;
    city_count = 0;
    bool first_line = true;
    char byte;
    
    while(storage_file_read(file, &byte, 1) == 1 && city_count < MAX_CITIES) {
        if(byte == '\n' || byte == '\r') {
            if(line_pos == 0) continue; // Skip empty lines
            
            line[line_pos] = '\0'; // Null terminate
            
            // Skip header line
            if(first_line) {
                first_line = false;
                line_pos = 0;
                continue;
            }
            
            // Parse this line
            char* line_ptr = line;
            char* field;
            int field_num = 0;
            
            while((field = get_next_field(&line_ptr)) != NULL && field_num < 9) {
                switch(field_num) {
                    case 0: // Country code
                        strncpy(cities[city_count].country_code, field, 2);
                        cities[city_count].country_code[2] = '\0';
                        break;
                    case 1: // UTC shift
                        cities[city_count].utc_shift = parse_float(field);
                        break;
                    case 2: // City name
                        strncpy(cities[city_count].name, field, 31);
                        cities[city_count].name[31] = '\0';
                        break;
                    case 3: // Longitude
                        cities[city_count].longitude = parse_float(field);
                        break;
                    case 4: // Latitude
                        cities[city_count].latitude = parse_float(field);
                        break;
                    case 5: // Elevation
                        cities[city_count].elevation_m = parse_int(field);
                        break;
                    case 8: // Capital flag
                        cities[city_count].is_capital = (field[0] == 'Y' || field[0] == 'y');
                        break;
                }
                field_num++;
            }
            
            if(field_num > 0) {
                city_count++;
            }
            
            line_pos = 0; // Reset for next line
        } else if(line_pos < MAX_LINE_LENGTH - 1) {
            line[line_pos++] = byte;
        }
    }
    
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    FURI_LOG_I(TAG, "Loaded %d cities", city_count);
    return city_count > 0;
}

// Filter cities by selected country
void filter_cities_by_country(AppState* state) {
    filtered_city_count = 0;
    const char* country_code = european_countries[state->selected_country].code;
    
    for(int i = 0; i < city_count; i++) {
        if(strcmp(cities[i].country_code, country_code) == 0) {
            filtered_city_indices[filtered_city_count++] = i;
        }
    }
    
    // Reset selected city if out of range
    if(state->selected_city >= filtered_city_count) {
        state->selected_city = 0;
    }
}

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

			canvas_draw_str_aligned(canvas, 110, 1, AlignLeft, AlignTop, "v0.2");
			
			// Draw button hints at bottom using elements library
			elements_button_center(canvas, "OK"); // for the OK button
			break;	
		case ScreenCities: // Screen to choose city ----------------------------
			canvas_draw_icon(canvas, 1, -1, &I_icon_10x10);
			// Title
			canvas_set_font(canvas, FontPrimary);
			canvas_draw_str_aligned(canvas, 13, 1, AlignLeft, AlignTop, "City data"); 
			// CSV load status indicator
		    if(!state->csv_loaded) {
				canvas_draw_str_aligned(canvas, 126, 55, AlignRight, AlignTop, "Error: No CSV!");
			}
			canvas_set_font(canvas, FontSecondary);
			// Country chooser
			canvas_draw_frame(canvas, 1, 11, 26, 12);
			canvas_draw_str_aligned(canvas, 4, 13, AlignLeft, AlignTop, 
				european_countries[state -> selected_country].code); 
			// City chooser
			canvas_draw_frame(canvas, 28, 11, 100, 12);
			if(filtered_city_count > 0) {
				state->city_idx = filtered_city_indices[state->selected_city];
				canvas_draw_str_aligned(canvas, 30, 13, AlignLeft, AlignTop, cities[state->city_idx].name);
				if(cities[state->city_idx].is_capital) { // Draw capital indicator if applicable
					canvas_draw_icon(canvas, 118, 1, &I_capital_10x10);
				}
				// Display latitude and longitude
				snprintf(buffer, sizeof(buffer), "Lat:%.2f%c Lon:%.2f%c",
					fabs(cities[state->city_idx].latitude), (cities[state->city_idx].latitude >= 0 ? 'N' : 'S'),
					fabs(cities[state->city_idx].longitude), (cities[state->city_idx].longitude >= 0 ? 'E' : 'W'));
				canvas_draw_str_aligned(canvas, 1, 24, AlignLeft, AlignTop, buffer);
				// Display elevation and time zone
				snprintf(buffer, sizeof(buffer), "Elev: %dm UTC %+.1fh", 
					cities[state->city_idx].elevation_m, cities[state->city_idx].utc_shift);
				canvas_draw_str_aligned(canvas, 1, 33, AlignLeft, AlignTop, buffer);
			}
			// Navigation arrows
			switch(state->current_menu) {
				case MenuCountry:
					if(state->selected_country > 0) {
						canvas_draw_icon(canvas, 18, 12, &I_ButtonUp_7x4);
					}
					if(state->selected_country < country_count - 1) {
						canvas_draw_icon(canvas, 18, 17, &I_ButtonDown_7x4);
					}
					break;
				case MenuCity:
					canvas_draw_icon(canvas, 119, 12, &I_ButtonUp_7x4);
					canvas_draw_icon(canvas, 119, 17, &I_ButtonDown_7x4);
					// if(state->selected_city > 0) {
						// canvas_draw_icon(canvas, 119, 12, &I_ButtonUp_7x4);
					// }
					// if(state->selected_city < filtered_city_count - 1) {
						// canvas_draw_icon(canvas, 119, 17, &I_ButtonDown_7x4);
					// }
					break;
			}
			// Verbose area
			snprintf(buffer, sizeof(buffer), "Selected city %i / %i", state -> selected_city, state -> city_idx);
			canvas_draw_str_aligned(canvas, 1, 49, AlignLeft, AlignTop, buffer);
			// Navigation hint
			canvas_draw_str_aligned(canvas, 1, 57, AlignLeft, AlignTop, "Hold 'back' to exit.");
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
	// Load cities from CSV
	app.csv_loaded = load_cities_from_csv(APP_DATA_PATH("european_cities.txt"));
	FURI_LOG_I(TAG, "CSV loaded: %d, City count: %d", app.csv_loaded, city_count);
	if(city_count > 0) {
		FURI_LOG_I(TAG, "First city: code='%s' name='%s'", 
				   cities[0].country_code, cities[0].name);
	}
	filter_cities_by_country(&app);
	FURI_LOG_I(TAG, "After filter: %d cities", filtered_city_count);

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
			if((input.type == InputTypePress) && (app.current_screen == ScreenCities)) {
				if(app.current_menu == MenuCountry && app.selected_country > 0) {
					app.selected_country--;
					filter_cities_by_country(&app);
				} else if(app.current_menu == MenuCity && app.selected_city > 0) {
					app.selected_city--;
				}
			}
			break;
		case InputKeyDown:
			if((input.type == InputTypePress) && (app.current_screen == ScreenCities)) {
				if(app.current_menu == MenuCountry && app.selected_country < country_count - 1) {
					app.selected_country++;
					filter_cities_by_country(&app);
				} else if(app.current_menu == MenuCity && app.selected_city < filtered_city_count - 1) {
					app.selected_city++;
				}
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
