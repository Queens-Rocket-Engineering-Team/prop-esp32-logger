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
    

#--------------------------------------------------------------------------#

parser = argparse.ArgumentParser(description="Convert JSON config to .h header")
parser.add_argument('config', type=str)
parser.add_argument('mapping', type=str)
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
        
        header.write(f'\nstatic char json_config_str[] = "{config_str}";\n')

        

        header.write('\n#endif\n')

except Exception as e:
    print(f"Failed to create header file: {e}")