import argparse
import orjson

def read_json(file_path: str) -> dict:
    '''
    Docstring for read_json
    
    :param file_path: Filepath to JSON file
    :type file_path: str
    :return: Dictionary of JSON objects
    :rtype: dict
    '''
    try:
        with open(file_path, 'rb') as file:
            return orjson.loads(file.read())
    except Exception as e:
        print(f"Failed to read config file: {e}")
        return {}
    
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

def fill_struct(members: dict, struct_name: str, structs: list, file) -> None: #not finished
    for key, value in members.items():

        if isinstance(value, dict) and key in structs:
            fill_struct(value, key, structs, file)
        else:
            print(f'{key} not in {structs}')

    instance_struct(members, struct_name, structs, file)

def instance_struct(members: dict, struct_name: str, structs: list, file) -> None: #not finished
    file.write(f'\n{struct_name}_t {struct_name} {{\n')

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

    file.write('};\n')

#--------------------------------------------------------------------------#

parser = argparse.ArgumentParser(description="Convert JSON config to .h header")
parser.add_argument('template', type=str)
parser.add_argument('config', type=str)
parser.add_argument('outfile', type=str)
args = parser.parse_args()

template = read_json(args.template)
config = read_json(args.config)
structs = []

try:
    with open(args.outfile, 'w') as header:
        header.write('#ifndef ESPCONFIG_H\n')
        header.write('#define ESPCONFIG_H\n')
        header.write('\n')
        header.write('#include <stdbool.h>\n')
        header.write('\n')
        header.write('// Auto-generated header from ESPConfig.json\n')

        nest_struct(template, 'config', structs, header)
        fill_struct(config, 'test', structs, header)

        header.write('\n#endif\n')

except Exception as e:
    print(f"Failed to create header file: {e}")