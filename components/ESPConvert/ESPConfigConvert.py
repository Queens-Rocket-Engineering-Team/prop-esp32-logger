import argparse
import orjson

def read_config(file_path: str) -> dict:
    try:
        with open(file_path, 'rb') as file:
            config = orjson.loads(file.read())
            return config
    except Exception as e:
        print(f"Failed to read config file: {e}")
        return {}
    
def define_struct(members: dict, struct_name):

    for key, value in members.items():
        if isinstance(value, dict):
                header.write('\ntypedef struct {\n')
                
                define_struct(value)

                header.write(f'}} {key};\n')

        elif isinstance(value, int):
            header.write(f'int {key};\n')
        elif isinstance(value, float):
            header.write(f'float {key};\n')
        elif isinstance(value, str):
            header.write(f'char {key}[len(value)];\n')

parser = argparse.ArgumentParser(description="Convert JSON config to .h header")
parser.add_argument('infile', type=str)
parser.add_argument('outfile', type=str)
args = parser.parse_args()

config = read_config(args.infile)
print(config)

try:
    with open(args.outfile, 'w') as header:
        header.write('#ifndef ESPCONFIG_H\n')
        header.write('#define ESPCONFIG_H\n')
        header.write('// Auto-generated header from ESPConfig.json\n')
        header.write('\n')

        lines = define_struct(config)

        header.write('\n#endif\n')

except Exception as e:
    print(f"Failed to create header file: {e}")

