from os.path import basename, join
from SCons.Script import Builder
import re

Import("env")


def calculate_redefine_symbols_parameter(source):
    file_to_symbol = lambda file: re.sub('[^0-9a-zA-Z]', '_', str(file))

    to_remove = file_to_symbol(source.dir)
    to_keep = file_to_symbol(source.name)
    return map( lambda suffix: "--redefine-sym _binary_%s_%s_%s=_binary_%s_%s" %
            (to_remove, to_keep, suffix, to_keep, suffix), ["start", "end", "size"] )


def raw_to_obj_actions(source, target, env, for_signature):
    redefine_symbols_parameters = calculate_redefine_symbols_parameter(source[0])
    
    txt_to_bin = env.VerboseAction(" ".join([
                "xtensa-esp32-elf-objcopy",
                "--input-target", "binary",
                "--output-target", "elf32-xtensa-le",
                "--binary-architecture", "xtensa",
                "--rename-section", ".data=.rodata.embedded",
                "$SOURCE", "$TARGET"
            ]), "Converting $TARGET")

    redefine_symbols = env.VerboseAction(" ".join([
                "xtensa-esp32-elf-objcopy",
                " ".join(redefine_symbols_parameters),
                "$TARGET"
            ]), "Redefining symbols of $TARGET")

    return [txt_to_bin, redefine_symbols]


env.Append(
    BUILDERS=dict(
        RawToObj=Builder(
            generator=raw_to_obj_actions,
            suffix=".txt.o"
        )
    )
)


def embed_files(files):
    for f in files:
        input_file = join("$PROJECT_LIBDEPS_DIR", "$PIOENV", "WiFiManager/src", f)
        output_file = join("$BUILD_DIR", basename(f) + ".raw.o")
        file_target = env.RawToObj(output_file, input_file)
        env.Depends("$PIOMAINPROG", file_target)
        env.Append(PIOBUILDFILES=[file_target])

files = ["style.css", "jquery.gz", "code.js", "index.html"] 
embed_files(files)