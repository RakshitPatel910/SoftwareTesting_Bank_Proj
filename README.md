Step 1: Update System & Install Dependencies
sudo apt-get update
sudo apt-get install -y build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev

Step 2: Install AFL++
sudo apt-get install -y afl++

Step 3: Install Coverage Tools (GCOV/LCOV)
sudo apt-get install -y lcov

Step 4. Execution Guide
Phase 1: Test Harness Compilation
Compile for Coverage Analysis (GCC):
gcc -fprofile-arcs -ftest-coverage master_harness.c -o coverage_runner

Compile for Fuzzing (AFL-Clang-Fast):
afl-clang-fast master_harness.c -o fuzz_target

Phase 2: Coverage Verification
1. Reset Counters:
lcov --zerocounters --directory .

2. Run All Seeds:
for seed in input_seeds/*.txt; do
    ./coverage_runner < "$seed" > /dev/null 2>&1
done

3. Generate HTML Report:
lcov --capture --directory . --output-file coverage_final.info
genhtml coverage_final.info --output-directory coverage_report_final

Phase 3: Fuzz Testing
1. Configure System for Fuzzing: (Fixes the core_pattern error to allow AFL to handle crashes)
sudo bash -c 'echo core > /proc/sys/kernel/core_pattern'

2. Start the Fuzzer:
afl-fuzz -i input_seeds -o output_results -- ./fuzz_target

Step 5. Results & Analysis
hexdump -C output_results/default/crashes/id:000000*

