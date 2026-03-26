from os import listdir, makedirs, walk
from os.path import dirname, abspath, join, relpath, exists

from re import compile as regex
from re import DOTALL, search, sub

from sys import argv


ROOT      = dirname(dirname(abspath(__file__)))
GENERATED = ROOT
PLATFORM  = join(ROOT, "platform")

CMAKE     = regex(r"mx-generated.cmake")
CORE      = regex(r"Core")
LINKER    = regex(r".*\.ld$")

CMAKE_SKIP = [] # If any of these regexes match a line in the cmake file then don't edit them


def move_cmake(platform: str):

    files = [p for p in listdir(GENERATED) if CMAKE.match(p)]

    if len(files) == 0:
        return
    elif len(files) > 1:
        raise NameError(f"Mupltiple files found matching r'{CMAKE.pattern}' in '{GENERATED}'")

    src_path = join(GENERATED, files[0])
    dst_path = join(PLATFORM, platform, files[0])

    makedirs(dirname(dst_path), exist_ok=True)

    src_file = open(src_path, "r")
    dst_file = open(dst_path, "w")

    OLD_PREFIX = r"${CMAKE_CURRENT_SOURCE_DIR}/"
    NEW_PREFIX = r"${CMAKE_CURRENT_SOURCE_DIR}/platform/" + platform + "/"

    for line in src_file.readlines():

        line_leading_spaces = len(line) - len(line.lstrip(' '))
        line_stripped       = line.strip()

        # Line is not a path
        if not line_stripped.startswith(OLD_PREFIX):
            line = line.replace("Midllewares", "Middlewares") # Fix ST's typo
            dst_file.write(f"{line.rstrip()}\n")
            continue

        modify_prefix = not any(search(pattern, line) for pattern in CMAKE_SKIP)
        line_stripped = line_stripped[len(OLD_PREFIX):]

        # Count and remove leading ../
        ups = 0
        while line_stripped.startswith("../"):
            ups += 1
            line_stripped = line_stripped[3:]

        # Path relative to old Secure
        if ups == 0:
            line = (" " * line_leading_spaces) + f"{NEW_PREFIX if modify_prefix else OLD_PREFIX}{line_stripped}\n"
        elif ups > 1:
            raise ValueError(f"Too many '../' in path '{line_stripped}'")

        dst_file.write(line)

    src_file.close()
    dst_file.close()


def move_core(platform: str):

    USER_CODE = regex(
            r"/\* USER CODE BEGIN (?P<name>.+?)\s*\*/"  # Match the START tag strictly
            r"(?P<content>.*?)"                         # Capture ALL content (incl. whitespace/newlines)
            r"/\* USER CODE END (?P=name)\s*\*/",       # Match the END tag strictly
            DOTALL
        )

    def extract_user_code(text):

        blocks = {}
        for match in USER_CODE.finditer(text):
            name = match.group("name")
            content = match.group("content")
            blocks[name] = content

        return blocks

    def merge_user_code(generated_text: str, user_blocks: dict):

            to_remove = []
            for name, code in user_blocks.items():
                if code.strip() == "":
                    to_remove.append(name)
            for name in to_remove:
                user_blocks.pop(name)

            def replacer(match):
                name = match.group("name")
                if name in user_blocks:
                    code = user_blocks[name]

                    return (
                        f"/* USER CODE BEGIN {name} */"
                        f"{code}"
                        f"/* USER CODE END {name} */"
                    )
                return match.group(0) # leave empty if no user code

            return USER_CODE.sub(replacer, generated_text)


    folders = [p for p in listdir(GENERATED) if CORE.match(p)]

    if len(folders) == 0:
        return
    elif len(folders) > 1:
        raise NameError(f"Mupltiple files found matching r'{CORE.pattern}' in '{GENERATED}'")

    src_dir = join(GENERATED, folders[0])
    dst_dir = join(PLATFORM, platform, folders[0])

    for root, _, files in walk(src_dir):
        for name in files:

            src_path = join(root, name)
            rel_path = relpath(src_path, src_dir)
            dst_path = join(dst_dir, rel_path)
            # print(f"Copying '{CORE.pattern}/{rel_path}'")

            # Check if there is user code to merge
            overwriting = exists(dst_path)
            if not overwriting:
                makedirs(dirname(dst_path), exist_ok=True)

            # Get the newly generated files
            with open(src_path, "r") as src_file:
                contents = src_file.read()

            # Merge in the user code if required
            if overwriting:
                with open(dst_path, "r") as dst_file:
                    user_code = extract_user_code(dst_file.read())
                    contents  = merge_user_code(contents, user_code)

            # Write the new contents
            with open(dst_path, "w") as file:
                file.write(contents)


def parse_size(size_str: str) -> int:
    """Parses sizes like '0x1000', '32K', '1M', '1024' into integer bytes."""
    size_str = size_str.strip().upper()
    multiplier = 1

    # Handle GNU ld size suffixes
    if size_str.endswith('K'):
        multiplier = 1024
        size_str = size_str[:-1]
    elif size_str.endswith('M'):
        multiplier = 1024 * 1024
        size_str = size_str[:-1]
    elif size_str.endswith('G'):
        multiplier = 1024 * 1024 * 1024
        size_str = size_str[:-1]

    # Handle hex vs decimal bases
    base = 16 if size_str.startswith('0X') else 10

    return int(size_str, base) * multiplier

def get_sections_from_linker(contents: str) -> dict:

    # 1. Strip C-style /* ... */ comments (re.DOTALL allows matching across newlines)
    clean_text = sub(r'/\*.*?\*/', '', contents, flags=DOTALL)

    # Optional: Strip C++ style // comments if the script was passed through a preprocessor
    clean_text = sub(r'//.*', '', clean_text)

    memory = {}

    # 2. Extract the block inside MEMORY { ... }
    # Using [^}]* ensures we stop at the first closing brace.
    mem_block_match = search(r'MEMORY\s*\{([^}]*)\}', clean_text)
    if not mem_block_match:
        return memory

    mem_block = mem_block_match.group(1)

    # 3. Parse individual memory regions using finditer (handles multi-line wraps safely)
    region_pattern = regex(
        r'(\w+)\s*'                    # Group 1: Name (e.g., FLASH)
        r'(?:\([^)]*\))?\s*:\s*'       # Optional attributes like (rx) -> not captured
        r'(?:ORIGIN|org|o)\s*=\s*'     # Origin keyword (handles abbreviations)
        r'([^,\s]+)\s*,\s*'            # Group 2: Origin value (stops at comma or space)
        r'(?:LENGTH|len|l)\s*=\s*'     # Length keyword
        r'([^\s;]+)'                   # Group 3: Length value (stops at space or ;)
    )

    for match in region_pattern.finditer(mem_block):
        name = match.group(1)
        origin_str = match.group(2)
        length_str = match.group(3)

        memory[name] = {
            "origin": parse_size(origin_str),
            "length": parse_size(length_str)
        }

    return memory

def move_linker(platform: str):

    files = [p for p in listdir(GENERATED) if LINKER.match(p)]

    if len(files) == 0:
        return
    elif len(files) > 1:
        raise NameError(f"Mupltiple files found matching r'{LINKER.pattern}' in '{GENERATED}'")

    src_path = join(GENERATED, files[0])
    dst_path = join(PLATFORM, platform, files[0])

    makedirs(dirname(dst_path), exist_ok=True)

    src_file = open(src_path, "r")
    src_file_contents = src_file.read()
    dst_file = open(dst_path, "w")

    for line in src_file_contents.split("\n"):

        # Make the backup section NOLOAD. This is done because the backup domain has to be enabled in firmware before being used
        if ".BACKUP :" in line:
            line = line.replace(".BACKUP :", ".BACKUP (NOLOAD):")

        dst_file.write(line + "\n")

    # Add a section to the end with symbols for the start and end of all sections
    sections = get_sections_from_linker(src_file_contents)
    for section, attributes in sections.items():
        dst_file.write("\n")
        dst_file.write(f"__{section}_START__ = ORIGIN({section});\n")
        dst_file.write(f"__{section}_END__   = ORIGIN({section}) + LENGTH({section});\n")
        dst_file.write(f"__{section}_SIZE__  = LENGTH({section});\n")
        dst_file.write("\n")

    src_file.close()
    dst_file.close()


if __name__ == "__main__":

    if len(argv) > 1:
        platform = argv[1]
        print(f"Platform selected: {platform}")
    else:
        raise ValueError("No platform argument provided.")

    move_cmake(platform)
    move_core(platform)
    move_linker(platform)

    exit(0)
