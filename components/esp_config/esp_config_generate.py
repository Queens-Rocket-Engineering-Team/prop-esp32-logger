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
    

parser = argparse.ArgumentParser(description="Convert JSON config to .h header")
parser.add_argument('config', type=str)
parser.add_argument('mapping', type=str)
parser.add_argument('outfile', type=str)
args = parser.parse_args()

config, config_str = read_json(args.config)
mapping, _ = read_json(args.mapping)

adc_map = mapping.get('ADC_map').values()
sensor_map = mapping.get('sensor_map').values()
control_map = mapping.get('control_map').values()

addr_to_drdy_cases = '\n'.join(
    f"    case {adc.get('addr')}:\n        return {adc.get('DRDY_pin')};"
    for adc in adc_map
)

header_content = f"""\
#pragma once

#include <stdint.h>

// Auto-generated header from esp_config.json and esp_mapping.json

static const char json_config_str[] = "{config_str}";

#define CONFIG_NUM_ADCS {len(mapping.get('ADC_map'))}

static inline uint8_t adc_addr_to_drdy(uint8_t addr) {{
    switch (addr) {{
{addr_to_drdy_cases}
    default:
        return 0xFF;
    }}
}}





"""

try:
    with open(args.outfile, 'w') as header:
        header.write(header_content)
        
except Exception as e:
    print(f"Failed to create header file: {e}")