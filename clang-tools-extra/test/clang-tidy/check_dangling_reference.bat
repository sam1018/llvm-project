set test_path=%~dp0
set test_name=bugprone-reference-returned-from-temporary
python %test_path%\check_clang_tidy.py %test_path%\checkers\%test_name%.cpp %test_name% temp
python %test_path%\check_clang_tidy.py %test_path%\checkers\%test_name%2.cpp %test_name% temp -extra-arg=-std=c++17
python %test_path%\check_clang_tidy.py %test_path%\checkers\%test_name%3.cpp %test_name% temp -config="{CheckOptions: [{key: bugprone-reference-returned-from-temporary.TempWhiteListRE, value: '.*iterator.*|.*proxy.*'}]}"
