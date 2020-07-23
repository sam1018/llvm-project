import os
import sys
import subprocess
import check_clang_tidy

test_path = os.path.dirname(os.path.realpath(__file__))
test_runner = f"{test_path}/check_clang_tidy.py"
test_name = "bugprone-reference-returned-from-temporary"

def extra_args(test_case):
    if test_case == 2:
        options = {'CheckOptions': [
        {
        'key': 'bugprone-reference-returned-from-temporary.CastFunctionsWhiteList',
        'value': 'static_pointer_cast'
        }]}
        return ["-extra-arg=-std=c++17", f"-config={options}"]
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

def expand_std(std):
  if std == 'c++98-or-later':
    return ['c++98', 'c++11', 'c++14', 'c++17', 'c++20']
  if std == 'c++11-or-later':
    return ['c++11', 'c++14', 'c++17', 'c++20']
  if std == 'c++14-or-later':
    return ['c++14', 'c++17', 'c++20']
  if std == 'c++17-or-later':
    return ['c++17', 'c++20']
  if std == 'c++20-or-later':
    return ['c++20']
  return [std]

class CTCheckArgs:
    def __init__(self, test_case):
        self.assume_filename = None
        self.check_name = 'bugprone-reference-returned-from-temporary'
        self.check_suffix = ['']
        self.expect_clang_tidy_error = False
        test_file_name_prefix = f'{test_path}/checkers/{test_name}'
        self.input_file_name = f'{test_file_name_prefix}{test_case}.cpp'
        self.resource_dir = None
        self.std = ['c++11-or-later']
        self.temp_file_name = 'temp~'

for test_case in [1, 2]:
    args = CTCheckArgs(test_case)
    abbreviated_stds = args.std
    for abbreviated_std in abbreviated_stds:
        for std in expand_std(abbreviated_std):
            args.std = std
            check_clang_tidy.run_test_once(args, extra_args(test_case))
