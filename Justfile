# Local build outputs stay here; downloaded dependencies live in a separate workspace.
root := justfile_directory()
workspace := env_var_or_default("ZMK_WORKSPACE", root / ".build/west")
west := workspace / ".venv/bin/west"
export PATH := workspace / ".venv/bin" + ":" + env_var("PATH")
export ZEPHYR_BASE := workspace / "zephyr"
# GNU Arm is installed on this machine. Override these for a Zephyr SDK toolchain.
export ZEPHYR_TOOLCHAIN_VARIANT := env_var_or_default("ZEPHYR_TOOLCHAIN_VARIANT", "gnuarmemb")
export GNUARMEMB_TOOLCHAIN_PATH := env_var_or_default("GNUARMEMB_TOOLCHAIN_PATH", "/usr")

# List available recipes.
default:
    @just --list

# Download pinned ZMK/Zephyr dependencies and install Python build tools (requires uv).
setup:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p "{{workspace}}"
    if [[ ! -x "{{west}}" ]]; then
        uv venv "{{workspace}}/.venv"
    fi
    uv pip install --python "{{workspace}}/.venv/bin/python" west
    mkdir -p "{{workspace}}/config"
    cp "{{root}}/config/west.yml" "{{workspace}}/config/west.yml"
    if [[ ! -d "{{workspace}}/.west" ]]; then
        "{{west}}" init -l "{{workspace}}/config"
    fi
    cd "{{workspace}}"
    "{{west}}" update --narrow -o=--depth=1
    uv pip install --python "{{workspace}}/.venv/bin/python" \
        -r zephyr/scripts/requirements-base.txt \
        -r zmk/app/scripts/requirements.txt \
        'setuptools<81' protobuf grpcio-tools

# Build both halves and copy named UF2 files into build/.
build: build-left build-right

# Build the left half with ZMK Studio enabled.
build-left:
    cd "{{workspace}}" && "{{west}}" build -s zmk/app -d "{{root}}/build/left" -b nice_nano_v2 -S studio-rpc-usb-uart -- -DZephyr_DIR="{{workspace}}/zephyr/share/zephyr-package/cmake" -DSHIELD="temper_left nice_view_adapter nice_view" -DZMK_CONFIG="{{root}}/config" -DZMK_EXTRA_MODULES="{{root}}" -DCONFIG_ZMK_STUDIO=y -DCONFIG_ZMK_STUDIO_LOCKING=n
    cp "{{root}}/build/left/zephyr/zmk.uf2" "{{root}}/build/temper-left.uf2"

# Build the right half.
build-right:
    cd "{{workspace}}" && "{{west}}" build -s zmk/app -d "{{root}}/build/right" -b nice_nano_v2 -- -DZephyr_DIR="{{workspace}}/zephyr/share/zephyr-package/cmake" -DSHIELD="temper_right nice_view_adapter nice_view" -DZMK_CONFIG="{{root}}/config" -DZMK_EXTRA_MODULES="{{root}}"
    cp "{{root}}/build/right/zephyr/zmk.uf2" "{{root}}/build/temper-right.uf2"

# Run the dependency-free display tests.
test:
    #!/usr/bin/env bash
    set -euo pipefail
    executable=$(mktemp)
    trap 'rm -f "$executable"' EXIT
    cc -std=c99 -Wall -Wextra -Werror "{{root}}/tests/display_test.c" -o "$executable"
    "$executable"

# Regenerate the keymap YAML and SVG (requires uv; no firmware workspace needed).
draw:
    uvx --from keymap-drawer keymap -c "{{root}}/keymap_img/keymap_drawer.config.yaml" parse -z "{{root}}/boards/shields/temper/temper.keymap" -c 10 > "{{root}}/keymap_img/temper.yaml"
    uvx --from keymap-drawer keymap -c "{{root}}/keymap_img/keymap_drawer.config.yaml" draw "{{root}}/keymap_img/temper.yaml" -k chocofi -s Base Extra System Mouse > "{{root}}/keymap_img/temper.svg"

# Remove generated firmware/build files, retaining downloaded dependencies.
clean:
    rm -rf "{{root}}/build"
