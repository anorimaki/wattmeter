from os.path import basename, join
from SCons.Script import Builder
import re

Import("env")


def file_to_obj(target, source, env):
    env.Command(target[0], source[0], 
            "a --input-target binary --output-target elf32-xtensa-le "
            "--binary-architecture xtensa --rename-section .data=.rodata.embedded $SOURCE $TARGET")
    return None


def calculate_redefine_symbols_parameter(source):
    suffixes = ["start", "end", "size"]

    file_to_symbol = lambda file: re.sub('[^0-9a-zA-Z]', '_', str(file))

    to_remove = file_to_symbol(source.dir)
    to_keep = file_to_symbol(source.name)
    return map( lambda suffix: "--redefine-sym _binary_%s_%s_%s=_binary_%s_%s" %
            (to_remove, to_keep, suffix, to_keep, suffix), suffixes )


def txt_to_bin_actions(source, target, env, for_signature):
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
        TxtToBin=Builder(
            generator=txt_to_bin_actions,
            suffix=".txt.o"
        )
    )
)


def embed_files(files):
    for f in files:
        input_file = join("$PROJECTLIBDEPS_DIR", "$PIOENV", "WiFiManager/src", f)
        output_file = join("$BUILD_DIR", basename(f) + ".txt.o")
        file_target = env.TxtToBin(output_file, input_file)
        env.Depends("$PIOMAINPROG", file_target)
        env.Append(PIOBUILDFILES=[file_target])

files = ["style.css", "jquery.gz", "code.js", "index.html"] 
embed_files(files)