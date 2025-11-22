#include <furi.h> // Flipper Universal Registry Implementation = Core OS functionality
#include <gui/gui.h> // GUI system
#include <gui/icon.h>
#include <input/input.h> // Input handling (buttons)
#include <stdint.h> // Standard integer types
#include <stdlib.h> // Standard library functions
#include <gui/elements.h> // to access button drawing functions

#define TAG "Astro" // Tag for logging purposes
extern const Icon I_splash;

typedef enum {
    ScreenSplash,
} AppScreen;

// Main application structure
typedef struct {
    FuriMessageQueue* input_queue;  // Queue for handling input events
    ViewPort* view_port;            // ViewPort for rendering UI
    Gui* gui;                       // GUI instance
    uint8_t current_screen;         // 0 = first screen, 1 = second screen 
} AppState;

// =============================================================================
// MAIN CALLBACK - called whenever the screen needs to be redrawn
// =============================================================================
void draw_callback(Canvas* canvas, void* context) {
    AppState* state = context;  // Get app context to check current screen

    // Clear the canvas and set drawing color to black
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    switch (state -> current_screen) {
		case ScreenSplash: // First screen ----------------------------
			canvas_draw_icon(canvas, 1, 1, &I_splash); // 51 is a pixel above the buttons
			
			canvas_set_font(canvas, FontPrimary);
			canvas_draw_str_aligned(canvas, 1, 1, AlignLeft, AlignTop, "Sun data");
			canvas_draw_str_aligned(canvas, 1, 10, AlignLeft, AlignTop, "for your");
			canvas_draw_str_aligned(canvas, 1, 20, AlignLeft, AlignTop, "city");
			canvas_draw_str_aligned(canvas, 88, 56, AlignLeft, AlignTop, "f418.eu");
			
			canvas_set_color(canvas, ColorBlack);
			canvas_set_font(canvas, FontSecondary);
						
			canvas_draw_str_aligned(canvas, 1, 49, AlignLeft, AlignTop, "Hold 'back'");
			canvas_draw_str_aligned(canvas, 1, 57, AlignLeft, AlignTop, "to exit.");
			

			canvas_draw_str_aligned(canvas, 110, 1, AlignLeft, AlignTop, "v0.1");
			
			// Draw button hints at bottom using elements library
			elements_button_center(canvas, "OK"); // for the OK button
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
	app.current_screen = ScreenSplash;  // Start on first screen (0)
	
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
		case InputKeyDown:
		case InputKeyLeft:
		case InputKeyRight:
		case InputKeyOk:
		if(input.type == InputTypePress) {
				exit_loop = 1;  
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
