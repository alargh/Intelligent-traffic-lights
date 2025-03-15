import json
import sys

def read_json(input_file):
    try:
        with open(input_file, 'r') as f:
            data = json.load(f)
        
        commands = []
        if "commands" in data:
            for cmd in data["commands"]:
                command_info = {}
                if "type" in cmd:
                    command_info["type"] = cmd["type"]

                    if cmd["type"] == "addVehicle":
                        if "vehicleId" in cmd:
                            command_info["vehicleId"] = cmd["vehicleId"]
                        if "startRoad" in cmd:
                            command_info["startRoad"] = cmd["startRoad"]
                        if "endRoad" in cmd:
                            command_info["endRoad"] = cmd["endRoad"]
                
                commands.append(command_info)

        print(json.dumps(commands))
        return 0
    except Exception as e:
        print(f"Error reading JSON: {str(e)}", file=sys.stderr)
        return 1

def write_json(output_file, commands_str):
    try:
        commands = json.loads(commands_str)

        output_data = {"stepStatuses": commands}

        with open(output_file, 'w') as f:
            json.dump(output_data, f, indent=2)
            
        return 0
    except Exception as e:
        print(f"Error writing JSON: {str(e)}", file=sys.stderr)
        return 1

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: script.py read|write [arguments]", file=sys.stderr)
        sys.exit(1)
    
    mode = sys.argv[1]
    
    if mode == "read" and len(sys.argv) == 3:
        input_file = sys.argv[2]
        sys.exit(read_json(input_file))
    elif mode == "write" and len(sys.argv) == 4:
        output_file = sys.argv[2]
        commands_str = sys.argv[3]
        sys.exit(write_json(output_file, commands_str))
    else:
        print("Invalid arguments.", file=sys.stderr)
        print("Usage: script.py read input.json", file=sys.stderr)
        print("       script.py write output.json '[...]'", file=sys.stderr)
        sys.exit(1)