---
name: coverage
description: "提交前覆盖率检查技能。用于生成覆盖率证据并判断 UT 是否>=80%、SCT 是否=100%。"
---

# Coverage Skill

目的
- 在提交前给出可复现的覆盖率证据，并执行门禁判断。

门禁规则
- UT 行覆盖率（`src/*`，gcovr）必须 `>= 80%`。
- SCT 覆盖率按用例通过率计算，必须 `= 100%`（即全部 SCT 用例通过）。

执行步骤
1. 构建覆盖率版本
   - `cmake -S . -B build-coverage -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='--coverage -O0' -DCMAKE_CXX_FLAGS='--coverage -O0'`
   - `cmake --build build-coverage -j`
2. 运行 UT 并生成 gcovr 报告
   - `ctest --test-dir build-coverage --output-on-failure > .github/coverage/artifacts/ut-ctest.txt`
   - `python3 -m gcovr -r . --object-directory build-coverage --filter 'src/' --exclude 'test/' --exclude-directories 'build-coverage/CMakeFiles|build-coverage/_deps' --gcov-ignore-errors source_not_found --gcov-ignore-errors no_working_dir_found --txt > .github/coverage/artifacts/ut-gcovr.txt`
3. 运行 SCT 并生成结果证据
   - `cd test/sct && python3 -m tox > ../../.github/coverage/artifacts/sct-tox.txt`
4. 输出结论报告
   - 生成 `.github/coverage/REPORT.md`，至少包含 UT 覆盖率值、SCT 覆盖率值、门禁通过/失败结论。

输出要求
- 必须明确写出：
  - `UT: <value>% (threshold >= 80%) => pass/fail`
  - `SCT: <value>% (threshold = 100%) => pass/fail`
- 若任一项失败，则禁止进入 `git push`。
