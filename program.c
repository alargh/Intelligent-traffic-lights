#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>

#define MAX_BUFFER 16384
#define MAX_COMMANDS 100
#define MAX_FIELD_LENGTH 256

// Structure Vehicle, to put into queues on our intersection roads
typedef struct {
    char vehicleId[MAX_FIELD_LENGTH];
    char startRoad[MAX_FIELD_LENGTH];
    char endRoad[MAX_FIELD_LENGTH];
} Vehicle;

// Init Vehicle object
Vehicle* create_vehicle(const char* vehicleID, const char* startRoad, const char* endRoad) {
    Vehicle* v = (Vehicle*)malloc(sizeof(Vehicle));
    if (v) {
        strncpy(v->vehicleId, vehicleID, MAX_FIELD_LENGTH - 1);
        strncpy(v->startRoad, startRoad, MAX_FIELD_LENGTH - 1);
        strncpy(v->endRoad, endRoad, MAX_FIELD_LENGTH - 1);
        v->vehicleId[MAX_FIELD_LENGTH - 1] = '\0';
        v->startRoad[MAX_FIELD_LENGTH - 1] = '\0';
        v->endRoad[MAX_FIELD_LENGTH - 1] = '\0';
    }
    return v;
}

// Structure to hold our commands
typedef struct {
    char type[MAX_FIELD_LENGTH];
    char vehicleId[MAX_FIELD_LENGTH];
    char startRoad[MAX_FIELD_LENGTH];
    char endRoad[MAX_FIELD_LENGTH];
    int isVehicle;
} Command;

// Function to extract string value from JSON field
void extract_json_string(const char **json, char *output) {
    const char *p = *json;
    
    while (*p && *p != '"') p++;
    if (*p == '"') p++;
    
    int i = 0;
    while (*p && *p != '"' && i < MAX_FIELD_LENGTH - 2) {
        output[i++] = *p++;
    }
    output[i] = '\0';

    if (*p == '"') p++;
    
    *json = p;
}

// Funtion to parse a JSON object for a command
void parse_command_object(const char **json, Command *cmd) {
    const char *p = *json;
    
    memset(cmd, 0, sizeof(Command));
    cmd->isVehicle = 0;

    while (*p && *p != '{') p++;
    if (*p == '{') p++;

    while (*p && *p != '}') {

        if (*p == '"') {
            char field[MAX_FIELD_LENGTH];
            p++;
            
            int i = 0;
            while (*p && *p != '"' && i < MAX_FIELD_LENGTH - 1) {
                field[i++] = *p++;
            }
            field[i] = '\0';
            
            if (*p == '"') p++;

            while (*p && (*p == ':' || isspace(*p))) p++;

            if (strcmp(field, "type") == 0) {
                extract_json_string(&p, cmd->type);
                if (strcmp(cmd->type, "addVehicle") == 0) {
                    cmd->isVehicle = 1;
                }
            } else if (strcmp(field, "vehicleId") == 0) {
                extract_json_string(&p, cmd->vehicleId);
            } else if (strcmp(field, "startRoad") == 0) {
                extract_json_string(&p, cmd->startRoad);
            } else if (strcmp(field, "endRoad") == 0) {
                extract_json_string(&p, cmd->endRoad);
            } else {
                if (*p == '"') {
                    p++;
                    while (*p && *p != '"') p++;
                    if (*p == '"') p++;
                } else if (*p == '{') {
                    int depth = 1;
                    p++;
                    while (*p && depth > 0) {
                        if (*p == '{') depth++;
                        if (*p == '}') depth--;
                        p++;
                    }
                } else if (*p == '[') {
                    int depth = 1;
                    p++;
                    while (*p && depth > 0) {
                        if (*p == '[') depth++;
                        if (*p == ']') depth--;
                        p++;
                    }
                } else {
                    while (*p && *p != ',' && *p != '}') p++;
                }
            }
        }
        
        while (*p && *p != '"' && *p != '}') p++;
    }
    
    if (*p == '}') p++;
    
    *json = p;
}

int main(int argc, char *argv[]) {
    char input_path[1024] = "";
    char output_path[1024] = "";
    char read_cmd[2048];
    char write_cmd[MAX_BUFFER * 2];
    char buffer[MAX_BUFFER];
    Command commands[MAX_COMMANDS];
    int command_count = 0;
    
    // Parse command line arguments to handle ./program INPUT=input.json OUTPUT=output.json logic
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "INPUT=", 6) == 0) {
            strcpy(input_path, argv[i] + 6);
        } else if (strncmp(argv[i], "OUTPUT=", 7) == 0) {
            strcpy(output_path, argv[i] + 7);
        }
    }
    
    // Check if input and output paths are provided
    if (strlen(input_path) == 0 || strlen(output_path) == 0) {
        fprintf(stderr, "Usage: %s INPUT=input.json OUTPUT=output.json\n", argv[0]);
        return 1;
    }
    
    // Step 1: Read JSON using Python script and capture its output
    sprintf(read_cmd, "python3 script.py read %s", input_path);
    FILE *pipe = popen(read_cmd, "r");
    
    if (!pipe) {
        fprintf(stderr, "Error executing Python read script\n");
        return 1;
    }
    
    // Read Python script's output
    if (fgets(buffer, MAX_BUFFER, pipe) == NULL) {
        fprintf(stderr, "Error reading from Python script\n");
        pclose(pipe);
        return 1;
    }
    
    pclose(pipe);
    
    // Step 2: Parse the JSON array from Python output
    const char *p = buffer;
    
    while (*p && *p != '[') p++;
    if (*p == '[') p++;

    while (*p && *p != ']' && command_count < MAX_COMMANDS) {
        while (*p && isspace(*p)) p++;
        
        if (*p == '{') {
            parse_command_object(&p, &commands[command_count]);
            command_count++;
        }
        
        while (*p && *p != '{' && *p != ']') p++;
    }
    
    // Step 3: Main algorithm deciding what cars to move in which order

    // queue containing which cars to move on single step
    GQueue* cars = g_queue_new();

    int iter = 0;

    // queues containing cars for every world direction
    GQueue *north = g_queue_new();
    GQueue *east = g_queue_new();
    GQueue *south = g_queue_new();
    GQueue *west = g_queue_new();

    // overall variable - how much cars we have now in one direction
    // out variable - how much cars left this direction in a row
    int overall_north = 0, overall_east = 0, overall_south = 0, overall_west = 0;
    int out_north = 0, out_east = 0, out_south = 0, out_west = 0;

    // helper variable for multiple cars on interseciton handling
    int turn_left = 0;
    int forward = 0;

    // time of waiting for each direction without moving (if it goes >=10, we let one car passthrough)
    int waiting_south = 0;
    int waiting_north = 0;
    int waiting_west = 0;
    int waiting_east = 0;

    strcpy(buffer, "[");
    while(iter < command_count) {
        char cmd_json[MAX_BUFFER * 2] = {0};
        
        // Process all addVehicle commands
        while(iter < command_count && strcmp(commands[iter].type, "addVehicle") == 0) {
            // Make a copy of the vehicle ID
            char* vehicle_copy = strdup(commands[iter].vehicleId);
            if (vehicle_copy) {
                // g_queue_push_tail(cars, vehicle_copy);
                if (strcmp(commands[iter].startRoad, "north") == 0) {
                    g_queue_push_tail(north, create_vehicle(commands[iter].vehicleId, commands[iter].startRoad, commands[iter].endRoad));
                } else if (strcmp(commands[iter].startRoad, "east") == 0) {
                    g_queue_push_tail(east, create_vehicle(commands[iter].vehicleId, commands[iter].startRoad, commands[iter].endRoad));
                } else if (strcmp(commands[iter].startRoad, "south") == 0) {
                    g_queue_push_tail(south, create_vehicle(commands[iter].vehicleId, commands[iter].startRoad, commands[iter].endRoad));
                } else if (strcmp(commands[iter].startRoad, "west") == 0) {
                    g_queue_push_tail(west, create_vehicle(commands[iter].vehicleId, commands[iter].startRoad, commands[iter].endRoad));
                }
            }
            iter++;
        }
        
        // Process step command
        if (iter < command_count && strcmp(commands[iter].type, "step") == 0) {
            // Update overall counters to reflect current queue lengths
            overall_north = g_queue_get_length(north);
            overall_east = g_queue_get_length(east);
            overall_south = g_queue_get_length(south);
            overall_west = g_queue_get_length(west);

            // Calculate ratios using (out + 1) / (overall + 1)
            double north_ratio = (double)(out_north + 1) / (overall_north + 1 + out_north);
            double east_ratio = (double)(out_east + 1) / (overall_east + 1 + out_east);
            double south_ratio = (double)(out_south + 1) / (overall_south + 1 + out_south);
            double west_ratio = (double)(out_west + 1) / (overall_west + 1 + out_west);

            // Choose the direction with the lowest ratio
            char* direction = "north";
            double min_ratio = north_ratio;

            if (east_ratio < min_ratio) {
                direction = "east";
                min_ratio = east_ratio;
            }
            if (south_ratio < min_ratio) {
                direction = "south";
                min_ratio = south_ratio;
            }
            if (west_ratio < min_ratio) {
                direction = "west";
                min_ratio = west_ratio;
            }


            //add waiting time for waiting directions
            if (strcmp(direction, "south") != 0) {
                waiting_south++;
            }
            if (strcmp(direction, "west") != 0) {
                waiting_west++;
            }
            if (strcmp(direction, "north") != 0) {
                waiting_north++;
            }
            if (strcmp(direction, "east") != 0) {
                waiting_east++;
            }
            
            //choose direction that was waiting too long (>=10 steps)
            if (waiting_south >= 10 && g_queue_get_length(south) > 0) {
                waiting_south = 0;
                direction = "south";
            } else if (waiting_west >= 10 && g_queue_get_length(west) > 0) {
                waiting_west = 0;
                direction = "west";
            } else if (waiting_north >= 10 && g_queue_get_length(north) > 0) {
                waiting_north = 0;
                direction = "north";
            } else if (waiting_east >= 10 && g_queue_get_length(east) > 0) {
                waiting_east = 0;
                direction = "east";
            }

            // Move a car from the chosen direction
            // North implementation
            if (strcmp(direction, "north") == 0 && g_queue_get_length(north) > 0) {
                Vehicle* v = (Vehicle*)g_queue_pop_head(north);
                out_north++;
                waiting_north = 0;
                
                //1 v and v1 going opposite dirs
                if(strcmp(v->endRoad, "south") == 0 && g_queue_get_length(south) > 0)
                {
                    Vehicle* v1 = (Vehicle*)g_queue_peek_head(south);
                    if(strcmp(v1->endRoad, "north") == 0)
                    {
                        g_queue_pop_head(south);
                        g_queue_push_tail(cars, v1->vehicleId);
                    }
                }

                //2 v turning left
                else if(strcmp(v->endRoad, "east") == 0)
                {
                    //v1 turning right
                    if(g_queue_get_length(east) > 0){
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(east);
                        if(strcmp(v1->endRoad, "north") == 0)
                        {
                            g_queue_pop_head(east);
                            g_queue_push_tail(cars, v1->vehicleId);
                        }
                    }
                }

                //3 v Turning right
                else if(strcmp(v->endRoad, "west") == 0)
                {
                    //v1 moving forward or turning left or right
                    if(g_queue_get_length(south) > 0)
                    {
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(south);
                        if(strcmp(v1->endRoad, "north") == 0)
                        {
                            g_queue_pop_head(south);
                            g_queue_push_tail(cars, v1->vehicleId);
                            forward = 1;
                        }
                    }
                    else if(g_queue_get_length(west) > 0)
                    {
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(west);
                        if(strcmp(v1->endRoad, "north") == 0)
                        {
                            g_queue_pop_head(west);
                            g_queue_push_tail(cars, v1->vehicleId);
                            turn_left = 1;
                        }
                        else if(strcmp(v1->endRoad, "south") == 0)
                        {
                            g_queue_pop_head(west);
                            g_queue_push_tail(cars, v1->vehicleId);
                        }
                    }
                    if(turn_left == 0 && forward == 0)
                    {
                        if(g_queue_get_length(south) > 0){
                            Vehicle* v2 = (Vehicle*)g_queue_peek_head(south);
                            if(strcmp(v2->endRoad, "east") == 0)
                            {
                                g_queue_pop_head(south);
                                g_queue_push_tail(cars, v2->vehicleId);
                            }
                        }
                        if(g_queue_get_length(east) > 0){
                            Vehicle* v3 = (Vehicle*)g_queue_peek_head(east);
                            if(strcmp(v3->endRoad, "north") == 0)
                            {
                                g_queue_pop_head(east);
                                g_queue_push_tail(cars, v3->vehicleId);
                            }
                        }
                    }
                    turn_left = 0;
                    forward = 0;
                    
                }
                
                out_east = out_south = out_west = 0; // Reset other counters to 1
                g_queue_push_tail(cars, v->vehicleId);
                // free(v); // Free the Vehicle struct
            } 

            // East implementation
            else if (strcmp(direction, "east") == 0 && g_queue_get_length(east) > 0) 
            {
                Vehicle* v = (Vehicle*)g_queue_pop_head(east);
                out_east++;
                waiting_east = 0;
                
                //1 v and v1 going opposite dirs
                if(strcmp(v->endRoad, "west") == 0 && g_queue_get_length(west) > 0)
                {
                    Vehicle* v1 = (Vehicle*)g_queue_peek_head(west);
                    if(strcmp(v1->endRoad, "east") == 0)
                    {
                        g_queue_pop_head(west);
                        g_queue_push_tail(cars, v1->vehicleId);
                    }
                }

                //2 v turning left
                else if(strcmp(v->endRoad, "south") == 0)
                {
                    //v1 turning right
                    if(g_queue_get_length(south) > 0){
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(south);
                        if(strcmp(v1->endRoad, "east") == 0)
                        {
                            g_queue_pop_head(south);
                            g_queue_push_tail(cars, v1->vehicleId);
                        }
                    }
                }

                //3 v Turning right
                else if(strcmp(v->endRoad, "north") == 0)
                {
                    //v1 moving forward or turning left or right
                    if(g_queue_get_length(west) > 0)
                    {
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(west);
                        if(strcmp(v1->endRoad, "east") == 0)
                        {
                            g_queue_pop_head(west);
                            g_queue_push_tail(cars, v1->vehicleId);
                            forward = 1;
                        }
                    }
                    else if(g_queue_get_length(north) > 0)
                    {
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(north);
                        if(strcmp(v1->endRoad, "east") == 0)
                        {
                            g_queue_pop_head(north);
                            g_queue_push_tail(cars, v1->vehicleId);
                            turn_left = 1;
                        }
                        else if(strcmp(v1->endRoad, "west") == 0)
                        {
                            g_queue_pop_head(north);
                            g_queue_push_tail(cars, v1->vehicleId);
                        }
                    }
                    if(turn_left == 0 && forward == 0)
                    {
                        if(g_queue_get_length(west) > 0){
                            Vehicle* v2 = (Vehicle*)g_queue_peek_head(west);
                            if(strcmp(v2->endRoad, "south") == 0)
                            {
                                g_queue_pop_head(west);
                                g_queue_push_tail(cars, v2->vehicleId);
                            }
                        }
                        if(g_queue_get_length(south) > 0){
                            Vehicle* v3 = (Vehicle*)g_queue_peek_head(south);
                            if(strcmp(v3->endRoad, "east") == 0)
                            {
                                g_queue_pop_head(south);
                                g_queue_push_tail(cars, v3->vehicleId);
                            }
                        }
                    }
                    turn_left = 0;
                    forward = 0;
                }
                
                out_north = out_south = out_west = 0;
                g_queue_push_tail(cars, v->vehicleId);
            } 

            // South implementation
            else if (strcmp(direction, "south") == 0 && g_queue_get_length(south) > 0) 
            {
                Vehicle* v = (Vehicle*)g_queue_pop_head(south);
                out_south++;
                waiting_south = 0;
                
                //1 v and v1 going opposite dirs
                if(strcmp(v->endRoad, "north") == 0 && g_queue_get_length(north) > 0)
                {
                    Vehicle* v1 = (Vehicle*)g_queue_peek_head(north);
                    if(strcmp(v1->endRoad, "south") == 0)
                    {
                        g_queue_pop_head(north);
                        g_queue_push_tail(cars, v1->vehicleId);
                    }
                }

                //2 v turning left
                else if(strcmp(v->endRoad, "west") == 0)
                {
                    //v1 turning right
                    if(g_queue_get_length(west) > 0){
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(west);
                        if(strcmp(v1->endRoad, "south") == 0)
                        {
                            g_queue_pop_head(west);
                            g_queue_push_tail(cars, v1->vehicleId);
                        }
                    }
                }

                //3 v Turning right
                else if(strcmp(v->endRoad, "east") == 0)
                {
                    //v1 moving forward or turning left or right
                    if(g_queue_get_length(north) > 0)
                    {
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(north);
                        if(strcmp(v1->endRoad, "south") == 0)
                        {
                            g_queue_pop_head(north);
                            g_queue_push_tail(cars, v1->vehicleId);
                            forward = 1;
                        }
                    }
                    else if(g_queue_get_length(east) > 0)
                    {
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(east);
                        if(strcmp(v1->endRoad, "south") == 0)
                        {
                            g_queue_pop_head(east);
                            g_queue_push_tail(cars, v1->vehicleId);
                            turn_left = 1;
                        }
                        else if(strcmp(v1->endRoad, "north") == 0)
                        {
                            g_queue_pop_head(east);
                            g_queue_push_tail(cars, v1->vehicleId);
                        }
                    }
                    if(turn_left == 0 && forward == 0)
                    {
                        if(g_queue_get_length(north) > 0){
                            Vehicle* v2 = (Vehicle*)g_queue_peek_head(north);
                            if(strcmp(v2->endRoad, "west") == 0)
                            {
                                g_queue_pop_head(north);
                                g_queue_push_tail(cars, v2->vehicleId);
                            }
                        }
                        if(g_queue_get_length(west) > 0){
                            Vehicle* v3 = (Vehicle*)g_queue_peek_head(west);
                            if(strcmp(v3->endRoad, "south") == 0)
                            {
                                g_queue_pop_head(west);
                                g_queue_push_tail(cars, v3->vehicleId);
                            }
                        }
                    }
                    turn_left = 0;
                    forward = 0;
                }
                
                out_north = out_east = out_west = 0;
                g_queue_push_tail(cars, v->vehicleId);
            } 

            // West implementation
            else if (strcmp(direction, "west") == 0 && g_queue_get_length(west) > 0) 
            {
                Vehicle* v = (Vehicle*)g_queue_pop_head(west);
                out_west++;
                waiting_west = 0;
                
                //1 v and v1 going opposite dirs
                if(strcmp(v->endRoad, "east") == 0 && g_queue_get_length(east) > 0)
                {
                    Vehicle* v1 = (Vehicle*)g_queue_peek_head(east);
                    if(strcmp(v1->endRoad, "west") == 0)
                    {
                        g_queue_pop_head(east);
                        g_queue_push_tail(cars, v1->vehicleId);
                    }
                }

                //2 v turning left
                else if(strcmp(v->endRoad, "north") == 0)
                {
                    //v1 turning right
                    if(g_queue_get_length(north) > 0){
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(north);
                        if(strcmp(v1->endRoad, "west") == 0)
                        {
                            g_queue_pop_head(north);
                            g_queue_push_tail(cars, v1->vehicleId);
                        }
                    }
                }

                //3 v Turning right
                else if(strcmp(v->endRoad, "south") == 0)
                {
                    //v1 moving forward or turning left or right
                    if(g_queue_get_length(east) > 0)
                    {
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(east);
                        if(strcmp(v1->endRoad, "west") == 0)
                        {
                            g_queue_pop_head(east);
                            g_queue_push_tail(cars, v1->vehicleId);
                            forward = 1;
                        }
                    }
                    else if(g_queue_get_length(south) > 0)
                    {
                        Vehicle* v1 = (Vehicle*)g_queue_peek_head(south);
                        if(strcmp(v1->endRoad, "west") == 0)
                        {
                            g_queue_pop_head(south);
                            g_queue_push_tail(cars, v1->vehicleId);
                            turn_left = 1;
                        }
                        else if(strcmp(v1->endRoad, "east") == 0)
                        {
                            g_queue_pop_head(south);
                            g_queue_push_tail(cars, v1->vehicleId);
                        }
                    }
                    if(turn_left == 0 && forward == 0)
                    {
                        if(g_queue_get_length(east) > 0){
                            Vehicle* v2 = (Vehicle*)g_queue_peek_head(east);
                            if(strcmp(v2->endRoad, "north") == 0)
                            {
                                g_queue_pop_head(east);
                                g_queue_push_tail(cars, v2->vehicleId);
                            }
                        }
                        if(g_queue_get_length(north) > 0){
                            Vehicle* v3 = (Vehicle*)g_queue_peek_head(north);
                            if(strcmp(v3->endRoad, "west") == 0)
                            {
                                g_queue_pop_head(north);
                                g_queue_push_tail(cars, v3->vehicleId);
                            }
                        }
                    }
                    turn_left = 0;
                    forward = 0;
                }
                
                out_north = out_east = out_south = 0;
                g_queue_push_tail(cars, v->vehicleId);
            }

            // Create a JSON array for leftVehicles
            char left_vehicles_json[MAX_BUFFER] = "[";
            int first_vehicle = 1;
            
            // Dequeue vehicles and add them to the leftVehicles array
            while (g_queue_get_length(cars) > 0) {
                char *vehicle_id = (char *)g_queue_pop_head(cars);
                
                if (!first_vehicle) {
                    strcat(left_vehicles_json, ",");
                }
                strcat(left_vehicles_json, "\"");
                strcat(left_vehicles_json, vehicle_id);
                strcat(left_vehicles_json, "\"");

                first_vehicle = 0;
            }
            
            strcat(left_vehicles_json, "]");
        
            // Construct the step command JSON
            snprintf(cmd_json, sizeof(cmd_json),
                "{\"leftVehicles\":%s}",
                left_vehicles_json
            );
            
            iter++;
        } 
        
        // Only append if we have content in cmd_json
        if (cmd_json[0] != '\0') {
            strcat(buffer, cmd_json);
            if (iter < command_count) {
                strcat(buffer, ",");
            }
        }
    }
    strcat(buffer, "]");

    // Free the queues
    g_queue_free(north);
    g_queue_free(east);
    g_queue_free(south);
    g_queue_free(west);
    
    // Step 4: Writing output to JSON using Python script
    sprintf(write_cmd, "python3 script.py write %s '%s'", output_path, buffer);
    int result = system(write_cmd);
    
    if (result != 0) {
        fprintf(stderr, "Error executing Python write script\n");
        return 1;
    }
    
    printf("Successfully processed %s to %s\n", input_path, output_path);
    printf("Modified %d commands by adding dots at the end of each field\n", command_count);
    
    return 0;
}