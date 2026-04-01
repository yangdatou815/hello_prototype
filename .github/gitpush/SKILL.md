# 提交流程（仓库贡献指南）

此文档依据开源仓库常见实践整理，放在 `.github/gitpush/SKILL.md` 便于仓库协作者快速查阅。

目的
- 规范本仓库的构建、测试、提交与推送流程，提升代码质量与协作效率。

调用 Review Skill
- 提交与推送前必须先执行 Review Skill：`.github/review/SKILL.md`
- 调用方式：先执行 review（或使用 `/review`），确认无阻断问题后再继续 `git add`、`git commit`、`git push`。

调用 Coverage Skill
- 提交与推送前必须先执行 Coverage Skill：`.github/coverage/SKILL.md`
- 门禁要求：UT 覆盖率 `>= 80%` 且 SCT 覆盖率 `= 100%`，否则禁止 `git push`。

指令语义（避免歧义）
- 当用户说“提交代码”时，默认含义为完整提交流程：`git add` + `git commit` + `git push`。
- 仅当用户明确说明“只本地提交/不要推送”时，才跳过 `git push`。
- 若推送失败，需输出失败原因（如权限、网络、分支保护）并给出下一步处理建议，而不是静默结束。

快速工作流（提交前必做）
1. 代码评审（Review）
  - 执行：`.github/review/SKILL.md`
  - 结论要求：若存在阻断问题，先修复再提交。
2. 覆盖率门禁（Coverage）
  - 执行：`.github/coverage/SKILL.md`
  - 结论要求：UT `>=80%` 且 SCT `=100%` 才可继续。
3. 构建
   - mkdir -p build && cd build
   - cmake ..
   - make -j
4. 运行测试
   - 在项目根目录运行：ctest --output-on-failure -j
   - 若仓库没有测试用例，确保编译产物（例如 hello_app）已生成并能运行。
5. 代码格式与静态检查（可选但推荐）
   - 运行 clang-format、clang-tidy 或团队约定的工具。
6. 提交并推送
  - git add -A
  - git commit -m "<type>(scope): short summary"
  - git push origin <current-branch>

提交规范（采用 Conventional Commits 规范）
- 格式：<type>(scope): short summary
- 常用 type：feat, fix, docs, style, refactor, perf, test, chore
- 可选 body：详述改动背景、设计要点或迁移说明

示例：

    git add src/Client.cpp
    git commit -m "style(client): add comment for connect() method in Client.cpp" \
               -m "Added inline comment preceding Client::connect() to clarify its purpose. No functional changes."
    git push origin main

一键脚本示例（可放在 scripts/ci_build.sh 并加入执行许可）：

#!/bin/bash
set -euo pipefail
mkdir -p build && cd build
cmake ..
make -j
cd ..
ctest --output-on-failure -j

常见问题与解决
- git push 出现凭据/配额错误（例："fatal: unable to write credential store: Disk quota exceeded"）
  - 查看磁盘配额：df -h
  - 清理不必要文件或联系运维扩容
  - 临时改用 SSH：git remote set-url origin git@github.com:owner/repo.git
  - 在 CI 中使用 repository token / deploy key

使用 Docker 保证可复现环境
- 构建 & 启动：
  docker-compose -f docker-compose.dev.yml build
  docker-compose -f docker-compose.dev.yml up -d
- 进入容器：
  docker exec -it fish-speech /bin/bash
- 在容器内执行构建/测试（同本地流程）。

自动化与 CI 建议
- 添加 pre-commit 钩子，自动运行格式化与基础检查。
- 增加 GitHub Actions：自动化构建、测试、lint；为主分支设置分支保护策略。

附录
- 文档文件已放置：`.github/gitpush/SKILL.md`
