import re
import os
import sys
import hashlib
import json
from concurrent.futures import ThreadPoolExecutor

STAGE_MAPPINGS = {
    "vertex": {"stage_suffix": "vert", "entry": "vertexMain"},
    "fragment": {"stage_suffix": "frag", "entry": "fragmentMain"},
    "compute": {"stage_suffix": "comp", "entry": "computeMain"},
    "geometry": {"stage_suffix": "geom", "entry": "geometryMain"},
}

COMPILE_TARGETS = {
    "spirv": {"suffix": "spv"},
    "glsl": {"suffix": "glsl"},
}
COMPILE_PROFILE = "glsl_450"
COMPILE_OPTIONS = [
    "-fvk-use-scalar-layout",
    "-matrix-layout-column-major",
]

HASH_FILE = ".shader_cached.json"

def get_file_hash(filepath):
    hasher = hashlib.sha256()
    try:
        with open(filepath, 'rb') as file:
            buf = file.read()
            hasher.update(buf)
        return hasher.hexdigest()
    except FileNotFoundError:
        return None

def load_hashes():
    if os.path.exists(HASH_FILE):
        try:
            with open(HASH_FILE, 'r') as f:
                return json.load(f)
        except (json.JSONDecodeError, IOError):
            return {}
    return {}

def save_hashes(hashes):
    try:
        with open(HASH_FILE, 'w') as f:
            json.dump(hashes, f, indent=4)
    except IOError:
        pass

def parse_metadata_to_stages(file_path):
    try:
        with open(file_path, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        return []

    match = re.search(r'//\s*\[moe\s*\((.*?)\)\]', content)

    if not match:
        return []

    stages_str = match.group(1).replace('"', '').replace("'", '').strip()
    stage_names = [s.strip() for s in stages_str.split(',')]
    
    return [s for s in stage_names if s in STAGE_MAPPINGS]

def enumerate_files():
    files = []
    shader_dir = os.path.dirname(os.path.abspath(__file__)) if '__file__' in locals() or '__file__' in globals() else os.getcwd()
    try:
        for filename in os.listdir(shader_dir):
            if filename.endswith('.slang'):
                files.append(os.path.join(shader_dir, filename))
    except FileNotFoundError:
        pass
    return files

def generate_output_filename(filepath, stage, target):
    stage_suffix = STAGE_MAPPINGS[stage]["stage_suffix"]
    file_suffix = COMPILE_TARGETS[target]["suffix"]
    base_name = os.path.basename(filepath)
    return f"{os.path.splitext(base_name)[0]}.{stage_suffix}.{file_suffix}"

def generate_compile_command(filepath, stage, target, output_filename):
    stage_info = STAGE_MAPPINGS[stage]
    entry_point = stage_info["entry"]
    
    cmd_parts = [
        "slangc",
        filepath,
        f"-target {target}",
        f"-profile {COMPILE_PROFILE}",
        f"-stage {stage}",
        f"-entry {entry_point}",
    ] + COMPILE_OPTIONS + [
        f"-o {output_filename}",
    ]
    
    return " ".join(cmd_parts)

def execute_command_with_hash(filepath, stage, target, current_hashes, new_hashes):
    file_hash = get_file_hash(filepath)
    output_filename = generate_output_filename(filepath, stage, target)
    key = f"{filepath}::{stage}::{target}"

    if current_hashes.get(key) == file_hash and os.path.exists(output_filename):
        new_hashes[key] = file_hash
        return f"SKIP: {output_filename} (Hash Match)"

    command = generate_compile_command(filepath, stage, target, output_filename)
    status = os.system(command)
    
    if status != 0:
        return f"ERROR: Command failed with status {status}: {command}"

    new_hashes[key] = file_hash
    return f"BUILD: {output_filename}"

def compile_all_parallel():
    current_hashes = load_hashes()
    new_hashes = {}
    compile_tasks = []
    
    for filepath in enumerate_files():
        stages = parse_metadata_to_stages(filepath)
        
        for stage in stages:
            for target in COMPILE_TARGETS:
                task_args = (filepath, stage, target, current_hashes, new_hashes)
                compile_tasks.append(task_args)
                
    if not compile_tasks:
        print("No .slang files found or no stages defined.")
        return

    print(f"Executing {len(compile_tasks)} compilation tasks concurrently...")

    max_workers = os.cpu_count() or 4

    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        results = list(executor.map(
            lambda args: execute_command_with_hash(*args), 
            compile_tasks
        ))

    save_hashes(new_hashes)
    
    print("-" * 50)
    for result in results:
        print(result)

def clean_build():
    shader_dir = os.path.dirname(os.path.abspath(__file__)) if '__file__' in locals() or '__file__' in globals() else os.getcwd()
    cleaned_files = 0

    for filename in os.listdir(shader_dir):
        if any(filename.endswith(f".{info['suffix']}") for info in COMPILE_TARGETS.values()):
            os.remove(os.path.join(shader_dir, filename))
            cleaned_files += 1
     
    if os.path.exists(HASH_FILE):
        os.remove(HASH_FILE)
        cleaned_files += 1
        
    print(f"Clean successful. Removed {cleaned_files} files.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python script_name.py <command>")
        print("Commands: build, clean, rebuild")
        sys.exit(1)
        
    command = sys.argv[1].lower()
    
    if command == "build":
        compile_all_parallel()
    elif command == "clean":
        clean_build()
    elif command == "rebuild":
        clean_build()
        compile_all_parallel()
    else:
        print(f"Unknown command: {command}")
        print("Commands: build, clean, rebuild")
        sys.exit(1)