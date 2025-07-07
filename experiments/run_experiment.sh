#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <basename>"
    exit 1
fi

BASE="$1"
SRC_DIR="experiments/$BASE"
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

declare -a RESULTS

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

run_and_time() {
    local lang="$1"
    local file="$2"
    shift 2
    local cmd=("$@")

    if ! command_exists "${cmd[0]}" || [ ! -f "$file" ]; then
        RESULTS+=("$lang|${cmd[*]} $file|N/A|N/A")
        return
    fi

    local output elapsed
    output=$({ /usr/bin/time -f "%e" "${cmd[@]}" "$file" 2> time.txt; } 2>&1)
    elapsed=$(cat time.txt)
    rm -f time.txt

    RESULTS+=("$lang|${cmd[*]} $file|$output|$elapsed")
}

run_c() {
    local file="$1"
    local base="$(basename "$file" .c)"
    local bin="$BUILD_DIR/$base"

    if ! command_exists gcc || [ ! -f "$file" ]; then
        RESULTS+=("C|gcc $file && $bin|N/A|N/A")
        return
    fi

    mkdir -p "$BUILD_DIR"

    local compile_time run_time run_output

    /usr/bin/time -f "%e" gcc "$file" -o "$bin" 2> time_compile.txt
    compile_time=$(cat time_compile.txt)
    rm -f time_compile.txt

    run_output=$({ /usr/bin/time -f "%e" "$bin" 2> time_run.txt; } 2>&1)
    run_time=$(cat time_run.txt)
    rm -f time_run.txt

    local time_combined="${run_time} (compiled in ${compile_time})"
    RESULTS+=("C|gcc $file && $bin|$run_output|$time_combined")
}

print_results() {
    while IFS='|' read -r lang cmd result time; do
        local res_disp="$result"
        if [ ${#res_disp} -gt 10 ]; then
            res_disp="${res_disp:0:10}..."
        fi
        printf "%-10s | %-70s | %-10s | %-20s\n" "$lang" "$cmd" "$res_disp" "$time"
    done
}

echo "Running $BASE experiment"

run_and_time "Andy" "$SRC_DIR/$BASE.andy" andy
run_and_time "Python3" "$SRC_DIR/$BASE.py" python3
run_and_time "Ruby" "$SRC_DIR/$BASE.rb" ruby
run_c "$SRC_DIR/$BASE.c"

echo
echo "=== Results (sorted by ascending time) ==="
echo
printf "%-10s | %-70s | %-10s | %-20s\n" "Language" "Command" "Result" "Time (seconds)"
echo "-----------------------------------------------------------------------------------------------------------------------------"

for row in "${RESULTS[@]}"; do
    echo "$row"
done | sort -t'|' -k4,4n | print_results
