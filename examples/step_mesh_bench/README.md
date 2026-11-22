# STEP 装配的 oneTBB 并行三角化

独立 C++17 命令行示例，入口为 `main.cpp`，依赖 Open CASCADE 的 STEP、BRep 和 TKMesh 模块以及 oneTBB。输出仍是保留装配位置的 BRep，可在面上用 `BRep_Tool::Triangulation(face, loc)` 读取三角形，在渲染时应用返回的位置和面的朝向。该程序生成边界面网格。

## 复现

测试模型来自 Voron Design 官方整机装配。压缩包 Git blob 为 `142a355c22f6e488deb0aac4104cc2073bc31321`，解压后的 `Voron_2.4r2_Assembly.step` 为 241,522,213 字节，SHA-256 为 `06b372e40217caa1a7d1f516cb74887247f809e41e1e389f740f290e98441b55`。[官方模型](https://github.com/VoronDesign/Voron-2/blob/Voron2.4/CAD/Voron_2.4r2_Assembly_STEP.zip)

在仓库根目录运行。下面使用本机的 x64 VS 开发者命令环境、Ninja 和 Release 版 SDK。其他安装可替换两个包路径。

```powershell
cmake -S examples/step_mesh_bench -B out/step-mesh -G Ninja `
  "-DCMAKE_MAKE_PROGRAM=C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" `
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release `
  -DOpenCASCADE_DIR=E:/OCCT/build-vs2026-x64 `
  -DCMAKE_PREFIX_PATH=D:/rendering_osg/out/mesh-deps/tbb-install
cmake --build out/step-mesh

$model = 'out/mesh-data/voron/Voron_2.4r2_Assembly.step'
.\out\step-mesh\step_mesh_bench.exe $model parts 8 0.1 0.35 out/voron-meshed.brep
.\out\step-mesh\step_mesh_bench.exe $model sweep 'serial/1,parts/4,parts/8,parts/12,parts/16,parts/28,hybrid/16,whole/16' 0.1 0.35
```

两个精度参数分别是绝对线偏差（本 STEP 导入后的单位为 mm）和角偏差（弧度）。输入也可以是 OCCT 的文本 BREP，此时线偏差取该文件的模型单位。`sweep` 仅导入一次，每轮先调用 `BRepTools::Clean` 清除可重算的离散数据。传入输出路径时保存最后一轮的模型，包括仍然缺少网格的面。返回码 0 表示有面且逐面检查和 mesher 错误检查均通过，2 表示存在缺面、失败任务或输入没有面，1 表示参数、读写或异常错误。

## 四种模式

| 模式 | 并行单位 |
|---|---|
| `serial` | 按独立拓扑组逐个处理，基准使用 1 线程。 |
| `parts` | oneTBB 并行处理独立组，每次 `BRepMesh_IncrementalMesh` 均关闭内部并行。编译开关选择工作分配方式。 |
| `whole` | 将原始装配交给一个 mesher，线程数大于 1 时启用 OCCT 内部并行。 |
| `hybrid` | 具有多个面且估算成本大于 `1.5 × 总成本 / 线程数` 的组先逐个使用内部并行，剩余组执行 `parts`。系数用于试验，可按实测调整。 |

CMake 选项 `STEP_MESH_WEIGHTED_JOBS` 同时设置同名 C++ 宏。默认 `ON` 对应宏值 1，`parts` 估算成本、降序排列，工作线程用原子下标领取下一个任务。`OFF` 对应宏值 0，`Job` 只保存形状，规划阶段跳过计数和排序，直接以 `oneapi::tbb::parallel_for(0, jobs.size(), ...)` 的索引重载遍历分组。关闭时 `hybrid` 也走这个循环。两种构建都执行共享拓扑分组。运行输出的 `weighted_jobs` 显示实际编入的宏值。

比较时，用前面的 CMake 命令分别配置两个 Release 构建目录，指定 `-B out/step-mesh -DSTEP_MESH_WEIGHTED_JOBS=ON` 和 `-B out/step-mesh-plain -DSTEP_MESH_WEIGHTED_JOBS=OFF`，分别构建。两个版本可对同一文件运行相同命令。

```powershell
.\out\step-mesh\step_mesh_bench.exe       $model parts 8 0.1 0.35
.\out\step-mesh-plain\step_mesh_bench.exe $model parts 8 0.1 0.35
```

调度前按底层 `TShape` 去重。对共享子拓扑的叶形状用并查集合组，同一条边的多边形表示和相邻面的三角化都由同一次 mesher 调用处理。规划只读原模型，共享同一原型的实例直接得到写回的三角化。曲面类型与边数给出初始工作量估计，排序后由空闲线程领取任务。对只有一个昂贵面的零件，面级并行能分配的工作有限。[OCCT 网格算法](https://occt3d.com/dev/doc/overview/html/occt_user_guides__mesh.html) [oneTBB task_arena](https://uxlfoundation.github.io/oneTBB/main/reference/source/task_scheduler/task_arena/task_arena_cls.html)

`plan_mesh_s` 是选择策略的主要指标，包含去重、分组、排序和三角化。导入、清理旧网格、逐面检查及可选写盘单独处理。`face_occurrences` 随装配实例展开，`unique_faces` 与 `unique_triangles` 按共享面原型统计。每轮还检查缺面数、失败任务、OCCT 状态位。第 2 轮起额外逐面对比首轮的三角形数量及缺面位置。三角形计数相同仅表示计数一致。

在查看器中复用 `planJobs` 和 `runJobs` 时，可以保留已有网格，让 incremental mesher 自行判断能否复用。调度期间，渲染侧应继续使用上一批顶点缓冲，等任务结束后再从 BRep 读取新网格。示例在独立命令行进程中配置 OCCT 默认线程池，嵌入已有应用时应在应用启动处统一设置这个进程级配置。

## Voron 整机实测

Windows x64，Core i7-14700F，28 个逻辑处理器，MSVC Release，OCCT 8.1.0-dev1-53706f78a8（内部并行使用原生线程），oneTBB 2022.2.0。以 0.1 mm 和 0.35 rad 三角化，9 种配置各跑 3 次，交错顺序，未绑核。下表单位为秒，取各列中位数。原始输出位于 `out/mesh-data/voron-benchmark.log`。

| 模式 | 线程数 | 三角化 | 分组及三角化 | 相对串行加速 |
|---|---:|---:|---:|---:|
| serial | 1 | 29.15 | 29.42 | 1.00× |
| parts | 4 | 12.09 | 12.40 | 2.37× |
| parts | 8 | 9.07 | 9.34 | 3.15× |
| parts | 12 | 10.39 | 10.68 | 2.76× |
| parts | 16 | 10.13 | 10.40 | 2.83× |
| parts | 20 | 10.04 | 10.30 | 2.86× |
| parts | 28 | 10.29 | 10.65 | 2.76× |
| hybrid | 16 | 10.08 | 10.35 | 2.84× |
| whole | 16 | 14.53 | 14.53 | 2.02× |

该次 STEP 导入用时 85.16 秒。装配展开后有 107,855 个面实例，按底层拓扑去重后有 100,800 个面，规划产生 1,563 个独立任务。hybrid/16 在此模型上没有触发重件阈值，实际采用与 parts/16 相同的调度。全部 27 轮生成 2,998,189 个原型三角形。每个面的三角形数量及 104 个缺面的位置均与首轮串行相同。OCCT 报告 `IMeshData_Failure`，这些面分布在 46 个任务中。全装配输出保留这些面及其原始 BRep 几何。

重新打开 `out/mesh-data/Voron_2.4r2_triangulated.brep` 后也得到同样的计数。文件 `out/mesh-data/Voron_2.4r2_subassembly.brep` 取自该装配根节点的第 7 个子形状（按 `TopoDS_Iterator` 的 1 起始顺序）。这套子装配有 221 个独立任务和 9,604 个面。重新清理后分别执行 serial、parts、hybrid 和 whole，四轮均得到 312,570 个三角形、零缺面、状态码 0。原始结果位于 `out/mesh-data/voron-subassembly.log`。

## 简单 parallel_for 与加权调度的配对测试

在同一台机器上，将上述两个宏值分别编译成 Release 版本，使用同一进程里一次 STEP 导入得到的整机拓扑。每次都用 `parts` 模式处理同样的 1,563 个任务，精度为 0.1 mm 和 0.35 rad。每个线程数先给两个版本各预热一次，随后成对交替先后顺序，每次重新清理网格。下表为各版本的 `plan_mesh_s` 中位数，百分比以简单版中位数为基准。原始逐轮结果位于 `out/mesh-data/voron-weight-ab.log`，其中 `pair=0` 为预热。

| 线程数 | 简单 parallel_for | 加权排序与动态领取 | 加权版耗时变化 | 加权版用时更短的配对数 |
|---:|---:|---:|---:|---:|
| 8 | 7.57 s | 7.93 s | +4.72% | 0/6 |
| 16 | 8.11 s | 8.36 s | +3.09% | 1/4 |
| 28 | 11.47 s | 10.49 s | −8.55% | 3/4 |

8 线程下，简单版与加权版的规划耗时中位数分别为 0.257 s 和 0.277 s，三角化阶段分别为 7.317 s 和 7.646 s。开关同时改变成本估算、排序和 TBB 任务分配方式。含预热的 34 次运行逐面三角形计数一致，均生成 2,998,189 个三角形，缺面的 104 个位置也一致。
