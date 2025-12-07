# moe-graphics shaders slang compile command generator
# python gen_commands.py [output_filename]

import re, os, sys

STAGE_MAPPINGS = {
    "vertex":   {"stage_suffix": "vert", "entry": "vertexMain"},
    "fragment": {"stage_suffix": "frag", "entry": "fragmentMain"},
    "compute":  {"stage_suffix": "comp", "entry": "computeMain"},
    "geometry": {"stage_suffix": "geom", "entry": "geometryMain"},
}

COMPILE_TARGETS = {
    "spirv": {"suffix": "spv"},
    "glsl":  {"suffix": "glsl"},
}
COMPILE_PROFILE = "glsl_450"
COMPILE_OPTIONS = [
    "-fvk-use-scalar-layout"
]

def parse_metadata_to_stages(file_path):
    with open(file_path, 'r') as f:
        content = f.read()

    match = re.search(r'//\s*\[\[moe\s*\((.*?)\)\]\]', content)

    if not match:
        print(f"Warning: No metadata found in {file_path}")
        return []

    stages_str = match.group(1).replace('"', '').replace("'", '').strip()
    stage_names = [s.strip() for s in stages_str.split(',')]
    
    for stage in stage_names:
        if stage not in STAGE_MAPPINGS:
            print(f"Warning: Unknown shader stage '{stage}' in {file_path}")
            stage_names.remove(stage)
            
    return stage_names

def enumerate_files():
    files = []
    shader_dir = os.path.dirname(os.path.abspath(__file__))
    for filename in os.listdir(shader_dir):
        if filename.endswith('.slang'):
            files.append(filename)
            
    return files
            
def generate_commands():
    commands = []
    
    for filename in enumerate_files():
        if not filename.endswith('.slang'):
            continue
        
        stages = parse_metadata_to_stages(filename)
        
        for stage in stages:
            stage_info = STAGE_MAPPINGS[stage]
            stage_suffix = stage_info["stage_suffix"]
            entry_point = stage_info["entry"]
            
            for target in COMPILE_TARGETS:
                file_suffix = COMPILE_TARGETS[target]["suffix"]
                output_filename = f"{os.path.splitext(filename)[0]}.{stage_suffix}.{file_suffix}"
                
                cmd_parts = [
                    "slangc",
                    filename,
                    f"-target {target}",
                    f"-profile {COMPILE_PROFILE}",
                    f"-stage {stage}",
                    f"-entry {entry_point}",
                ] + COMPILE_OPTIONS + [
                    f"-o {output_filename}",
                ]
                
                command = " ".join(cmd_parts)
                commands.append(command)
    
    return commands

def select_os():
    if sys.platform.startswith('win'):
        return 'windows'
    elif sys.platform.startswith('linux'):
        return 'linux'
    elif sys.platform.startswith('darwin'):
        return 'macos'
    else:
        return 'unknown'
    
def select_shell_script_extension():
    os_name = select_os()
    if os_name == 'windows':
        return "bat"
    else:
        return "sh"
    
def shell_script_header():
    os_name = select_os()
    if os_name == 'windows':
        return "@echo off\nsetlocal enabledelayedexpansion\n"
    else:
        return "#!/bin/bash\n"

if __name__ == "__main__":
    commands = "\n".join(generate_commands())
    out_filename = ""
    if len(sys.argv) == 2:
        out_filename = sys.argv[1]
    else:
        out_filename = f"compile.{select_shell_script_extension()}"
        
    with open(out_filename, 'w') as f:
        f.write(shell_script_header())
        f.write(commands)
        f.write("\n")
            
    print(commands)