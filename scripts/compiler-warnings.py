"""Active -Wall -Wextra -Werror sur notre code seulement (src/, lib/, test/), pas sur les tiers.

Script PlatformIO `extra_scripts` de type `pre:` (déclaré dans platformio.ini, section [env]) : il doit
tourner avant que PlatformIO collecte les sources. Un `-Werror` dans `build_flags` atteindrait aussi les
bibliothèques tierces (`.pio/libdeps/`) et le framework, que PlatformIO compile avec les mêmes flags ;
`build_src_flags` ne couvre que `src/`, alors que `lib/` est compilé comme des bibliothèques.

Le premier dossier du chemin du fichier source, relatif à la racine du projet, est comparé à la liste
blanche de nos dossiers ; tout le reste (`.pio/libdeps/`, framework hors du projet) est inchangé.
Le chemin est recalculé ici : `node.srcnode().get_path()`, sur lequel le motif de `AddBuildMiddleware`
est comparé, est absolu sous PlatformIO 6.2.0 (constaté en CI, run 46144333770).
"""
import os

Import("env")  # noqa: F821  (fourni par SCons)

OUR_FOLDERS = ("src", "lib", "test")
# Le framework Arduino exempte ces classes (-Wno-error=<classe>, -Wno-<classe>) dans CFLAGS/CXXFLAGS : on les
# force après lui (le dernier flag gagne en GCC ; SCons place $CCFLAGS après $CXXFLAGS et $CFLAGS).
CLASSES_EXEMPTED_BY_THE_FRAMEWORK = [
    "-Werror=unused-variable",
    "-Werror=unused-function",
    "-Werror=unused-but-set-variable",
    "-Werror=deprecated-declarations",
    "-Werror=uninitialized",
    "-Werror=maybe-uninitialized",
    "-Werror=array-bounds",
    "-Werror=extra",
    "-Wunused-parameter",
    "-Wsign-compare",
    "-Wmissing-field-initializers",
]
WARNING_FLAGS = ["-Wall", "-Wextra", "-Werror"] + CLASSES_EXEMPTED_BY_THE_FRAMEWORK


def is_our_source(env, node):
    try:
        relative_path = os.path.relpath(node.srcnode().abspath, env.subst("$PROJECT_DIR"))
    except ValueError:
        # Windows : fichier et projet sur deux lecteurs (framework sous %USERPROFILE%) : pas à nous.
        return False
    return relative_path.split(os.sep)[0] in OUR_FOLDERS


def with_warnings_as_errors(env, node):
    if not is_our_source(env, node):
        return node
    print("CW-PROBE CXXCOM", env.subst("$CXXCOM"), "| CCCOM", env.subst("$CCCOM"))  # TEMP
    return env.Object(node, CCFLAGS=env["CCFLAGS"] + WARNING_FLAGS)


env.AddBuildMiddleware(with_warnings_as_errors)  # noqa: F821
