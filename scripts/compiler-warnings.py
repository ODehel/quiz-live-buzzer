"""Active -Wall -Wextra -Werror sur notre code seulement (src/, lib/, test/), pas sur les tiers.

Script PlatformIO `extra_scripts` de type `pre:` (déclaré dans platformio.ini, section [env]) : il doit
tourner avant que PlatformIO collecte les sources. Un `-Werror` dans `build_flags` atteindrait aussi les
bibliothèques tierces (`.pio/libdeps/`) et le framework, que PlatformIO compile avec les mêmes flags ;
`build_src_flags` ne couvre que `src/`, alors que `lib/` est compilé comme des bibliothèques.

Le motif est comparé au chemin du fichier source relatif à la racine du projet (`src/main.cpp`,
`lib/game/X.cpp`, `test/test_x/y.cpp`) : liste blanche de nos dossiers, tout le reste est inchangé.
"""
Import("env")  # noqa: F821  (fourni par SCons)

OUR_FOLDERS = ("src", "lib", "test")
WARNING_FLAGS = ["-Wall", "-Wextra", "-Werror"]


def probe(env, node):
    print("DEBUG-CW seen", node.srcnode().get_path(), node.srcnode().abspath)  # TEMP
    return node


def with_warnings_as_errors(env, node):
    print("DEBUG-CW applied", node.srcnode().get_path())  # TEMP
    return env.Object(node, CCFLAGS=env["CCFLAGS"] + WARNING_FLAGS)


print("DEBUG-CW loaded")  # TEMP
env.AddBuildMiddleware(probe)  # TEMP
for folder in OUR_FOLDERS:
    env.AddBuildMiddleware(with_warnings_as_errors, f"{folder}/*")  # noqa: F821
