import os
import sys
import subprocess

test_path = os.path.dirname(os.path.realpath(__file__))
test_runner = f"{test_path}/check_clang_tidy.py"
test_name = "bugprone-reference-returned-from-temporary"
test_file_name_prefix = f"{test_path}/checkers/{test_name}"

def extra_args(test_case):
    if test_case == 2:
        return ["-extra-arg=-std=c++17"]
    if test_case == 1:
        options = {'CheckOptions': [
        {
        'key': 'bugprone-reference-returned-from-temporary.TempWhiteListRE',
        'value': '.*iterator.*|.*proxy.*|ns::match_exact_name_in_ns'
        },
        {
        'key': 'bugprone-reference-returned-from-temporary.CastFunctionsWhiteList',
        'value': 'cast_func2'
        }]}
        return [f"-config={options}"]
    return []

all_test_cases = range(1, 3)
if len(sys.argv) > 1:
    test_case_to_run = int(sys.argv[1])
    if test_case_to_run in all_test_cases:
        all_test_cases = [test_case_to_run]

for test_case in all_test_cases:
    print(f'Running test case {test_case}')
    cmd = ["python", test_runner, f"{test_file_name_prefix}{test_case}.cpp", test_name, "temp~"]
    cmd.extend(extra_args(test_case))
    print(f'Running test case {test_case}: {cmd}')
    ret = subprocess.run(cmd)
    ret.check_returncode()
