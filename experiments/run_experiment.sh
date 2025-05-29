#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <basename>"
    echo "Example: $0 loop  (will use experiments/loop/loop.py, etc)"
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

    if ! command_exists "${cmd[0]}"; then
        RESULTS+=("$lang|$file|${cmd[*]}|COMMAND NOT FOUND|N/A")
        echo "$lang: command not found (${cmd[0]})"
        return
    fi

    if [ ! -f "$file" ]; then
        RESULTS+=("$lang|$file|${cmd[*]}|FILE NOT FOUND|N/A")
        echo "$lang: source file not found ($file)"
        return
    fi

    local output
    local elapsed

    if command_exists /usr/bin/time; then
        output=$({ /usr/bin/time -f "%e" "${cmd[@]}" "$file" 2> time.txt; } 2>&1)
        elapsed=$(cat time.txt)
        rm time.txt
    else
        output=$("${cmd[@]}" "$file")
        elapsed="N/A"
    fi

    RESULTS+=("$lang|$file|${cmd[*]}|$output|$elapsed")
    echo "$lang ($file) ran in $elapsed seconds."
}

run_c() {
    local file="$1"
    local base="$(basename "$file" .c)"
    local bin="$BUILD_DIR/$base"

    if ! command_exists gcc; then
        RESULTS+=("C|$file|gcc|COMMAND NOT FOUND|N/A")
        echo "gcc not found, skipping C."
        return
    fi

    if [ ! -f "$file" ]; then
        RESULTS+=("C|$file|gcc|FILE NOT FOUND|N/A")
        echo "C source file not found: $file"
        return
    fi

    mkdir -p "$BUILD_DIR"

    local compile_time
    /usr/bin/time -f "%e" gcc "$file" -o "$bin" 2> time_compile.txt
    compile_time=$(cat time_compile.txt)
    rm time_compile.txt

    if [ ! -x "$bin" ]; then
        RESULTS+=("C|$file|gcc|COMPILATION FAILED|N/A")
        echo "Compilation failed for $file"
        return
    fi

    local run_output
    local run_time
    run_output=$({ /usr/bin/time -f "%e" "$bin" 2> time_run.txt; } 2>&1)
    run_time=$(cat time_run.txt)
    rm time_run.txt

    RESULTS+=("C|$file|gcc|$run_output|${run_time} (compile: $compile_time)")
    echo "C ($file) compiled in $compile_time s and ran in $run_time s."
}

print_results() {
    while IFS='|' read -r lang file cmd result time; do
        local res_disp="$result"
        if [ ${#res_disp} -gt 10 ]; then
            res_disp="${res_disp:0:10}..."
        fi
        printf "%-10s | %-40s | %-30s | %-10s | %-20s\n" "$lang" "$file" "$cmd" "$res_disp" "$time"
    done
}

echo "Running tests for base: $BASE"
echo

run_and_time "Andy" "$SRC_DIR/$BASE.andy" andy
run_and_time "Python3" "$SRC_DIR/$BASE.py" python3
run_and_time "Ruby" "$SRC_DIR/$BASE.rb" ruby
run_c "$SRC_DIR/$BASE.c"

echo
echo "=== Results (sorted by descending time) ==="
echo
printf "%-10s | %-40s | %-30s | %-10s | %-20s\n" "Language" "File" "Command" "Result" "Time (s)"
echo "--------------------------------------------------------------------------------------------------------------"

for row in "${RESULTS[@]}"; do
    echo "$row"
done | sort -t'|' -k5,5nr | print_results
