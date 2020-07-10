import os
import subprocess

test_path = os.path.dirname(os.path.realpath(__file__))
test_runner = f"{test_path}/check_clang_tidy.py"
test_name = "bugprone-reference-returned-from-temporary"
test_file_name_prefix = f"{test_path}/checkers/{test_name}"

def extra_args(test_case):
    if test_case == 2:
        return ["-extra-arg=-std=c++17"]
    if test_case == 3:
        test3_check_options = "{CheckOptions: [{key: bugprone-reference-returned-from-temporary.TempWhiteListRE, value: '.*iterator.*|.*proxy.*'}]}"
        return [f"-config={test3_check_options}"]
    return []

for test_case in range(1, 4):
    print(f'Running test case {test_case}')
    cmd = ["python", test_runner, f"{test_file_name_prefix}{test_case}.cpp", test_name, "temp~"]
    cmd.extend(extra_args(test_case))
    print(f'Running test case {test_case}: {cmd}')
    ret = subprocess.run(cmd)
    ret.check_returncode()
