import argparse
import orjson

def read_json(file_path: str) -> tuple[dict, str]:
    '''
    Docstring for read_json
    
    :param file_path: Filepath to JSON file
    :type file_path: str
    :return: Dictionary of JSON objects
    :rtype: dict
    '''
    try:
        with open(file_path, 'rb') as file:
            json_bytes = file.read()
            json_dict = orjson.loads(json_bytes)
            json_string = json_bytes.decode('utf-8')
            json_string = json_string.replace(r'"', r'\"')
            json_string = json_string.replace('\r', '')
            json_string = json_string.replace('\n', '\\n"\n"')
            return json_dict, json_string
    except Exception as e:
        print(f"Failed to read config file: {e}")
        raise
    
def nest_struct(members: dict, struct_name: str, structs: list, file) -> None:

    for key, value in members.items():
        if isinstance(value, dict):
            nest_struct(value, key, structs, file)

    define_struct(members, struct_name, structs, file)

def define_struct(members: dict, struct_name: str, structs: list, file) -> None:

    structs.append(struct_name)
    file.write('\ntypedef struct {\n')

    for key, value in members.items():
        if isinstance(value, bool):
            file.write(f'    bool {key};\n')
        elif isinstance(value, int):
            file.write(f'    int {key};\n')
        elif isinstance(value, float):
            file.write(f'    float {key};\n')
        elif isinstance(value, str):
            file.write(f'    char {key}[{len(value)+1}];\n')
        elif isinstance(value, dict):
            file.write(f'    {key}_t {key};\n')

    file.write(f'}} {struct_name}_t;\n')

#--------------------------------------------------------------------------#

parser = argparse.ArgumentParser(description="Convert JSON config to .h header")
parser.add_argument('config', type=str)
parser.add_argument('outfile', type=str)
args = parser.parse_args()

config, config_str = read_json(args.config)
structs = []

try:
    with open(args.outfile, 'w') as header:
        header.write('#ifndef ESP_CONFIG_H\n')
        header.write('#define ESP_CONFIG_H\n')
        header.write('\n')
        header.write('#include <stdbool.h>\n')
        header.write('\n')
        header.write('// Auto-generated header from ESPConfig.json\n')
        
        header.write(f'\nstatic const char json_config[] = "{config_str}";\n')

        #nest_struct(config, 'config', structs, header)

        header.write('\n#endif\n')

except Exception as e:
    print(f"Failed to create header file: {e}")