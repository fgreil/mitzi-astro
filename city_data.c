#include "city_data.h"
#include <storage/storage.h>
#include <toolbox/stream/file_stream.h>

static bool parse_csv_line(const char* line, CityData* city) {
    // Simple CSV parser - you may want to use a more robust implementation
    char* line_copy = strdup(line);
    char* token;
    int field = 0;
    
    token = strtok(line_copy, ",");
    while(token != NULL && field < 10) {
        // Remove quotes if present
        if(token[0] == '"' && token[strlen(token)-1] == '"') {
            token[strlen(token)-1] = '\0';
            token++;
        }
        
        switch(field) {
            case 0: strncpy(city->country_code, token, sizeof(city->country_code)-1); break;
            case 1: strncpy(city->country_name, token, sizeof(city->country_name)-1); break;
            case 2: strncpy(city->province_state, token, sizeof(city->province_state)-1); break;
            case 3: strncpy(city->city_name, token, sizeof(city->city_name)-1); break;
            case 4: strncpy(city->city_description, token, sizeof(city->city_description)-1); break;
            case 5: city->utc_offset = atof(token); break;
            case 6: city->longitude_degrees = atoi(token); break;
            case 7: city->longitude_minutes = atoi(token); break;
            case 8: city->latitude_degrees = atoi(token); break;
            case 9: city->latitude_minutes = atoi(token); break;
        }
        token = strtok(NULL, ",");
        field++;
    }
    
    free(line_copy);
    return field >= 10;
}

bool city_data_load_from_csv(CityDatabase* db, const char* file_path) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);
    
    if(!file_stream_open(stream, file_path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        stream_free(stream);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    
    db->total_cities = 0;
    db->total_countries = 0;
    
    FuriString* line = furi_string_alloc();
    
    // Skip header line
    stream_read_line(stream, line);
    
    while(stream_read_line(stream, line) && db->total_cities < MAX_CITIES) {
        if(furi_string_size(line) == 0) continue;
        
        CityData* city = &db->cities[db->total_cities];
        if(parse_csv_line(furi_string_get_cstr(line), city)) {
            // Find or create country entry
            int country_index = -1;
            for(int i = 0; i < db->total_countries; i++) {
                if(strcmp(db->countries[i].name, city->country_name) == 0) {
                    country_index = i;
                    break;
                }
            }
            
            if(country_index == -1 && db->total_countries < MAX_COUNTRIES) {
                country_index = db->total_countries;
                strncpy(db->countries[country_index].name, city->country_name, 
                       sizeof(db->countries[country_index].name)-1);
                db->countries[country_index].city_count = 0;
                db->total_countries++;
            }
            
            if(country_index != -1) {
                db->countries[country_index].city_indices[db->countries[country_index].city_count] = 
                    db->total_cities;
                db->countries[country_index].city_count++;
                db->total_cities++;
            }
        }
    }
    
    furi_string_free(line);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
    
    return db->total_cities > 0;
}
