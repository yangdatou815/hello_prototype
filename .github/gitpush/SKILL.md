# 提交流程（仓库贡献指南）

此文档依据开源仓库常见实践整理，放在 `.github/skill.md` 便于仓库协作者快速查阅。

目的
- 规范本仓库的构建、测试、提交与推送流程，提升代码质量与协作效率。

快速工作流（提交前必做）
1. 构建
   - mkdir -p build && cd build
   - cmake ..
   - make -j
2. 运行测试
   - 在项目根目录运行：ctest --output-on-failure -j
   - 若仓库没有测试用例，确保编译产物（例如 hello_app）已生成并能运行。
3. 代码格式与静态检查（可选但推荐）
   - 运行 clang-format、clang-tidy 或团队约定的工具。

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
- 文档文件已放置：`.github/skill.md`
- 若需要将此内容作为 CONTRIBUTING 或其他文件，请告知，我将同时生成标准的 `CONTRIBUTING.md`。
