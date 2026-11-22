#ifndef STEP_MESH_WEIGHTED_JOBS
#define STEP_MESH_WEIGHTED_JOBS 1
#endif
#if STEP_MESH_WEIGHTED_JOBS
#include <BRepAdaptor_Surface.hxx>
#endif
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <IMeshData_Status.hxx>
#include <OSD_Parallel.hxx>
#include <OSD_ThreadPool.hxx>
#include <Poly_Triangulation.hxx>
#include <Precision.hxx>
#include <STEPControl_Reader.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/task_arena.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
using Key = const TopoDS_TShape*;
using Count = std::uint64_t;

double seconds(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

class Components {
public:
    explicit Components(size_t n) : parent(n), sizes(n, 1) {
        std::iota(parent.begin(), parent.end(), size_t{0});
    }
    size_t root(size_t a) {
        while (a != parent[a]) {
            parent[a] = parent[parent[a]];
            a = parent[a];
        }
        return a;
    }
    void join(size_t a, size_t b) {
        a = root(a);
        b = root(b);
        if (a == b) return;
        if (sizes[a] < sizes[b]) std::swap(a, b);
        parent[b] = a;
        sizes[a] += sizes[b];
    }
private:
    std::vector<size_t> parent, sizes;
};

struct Job {
    TopoDS_Shape shape;
#if STEP_MESH_WEIGHTED_JOBS
    Count weight = 0, faces = 0;
#endif
};

// Descend through assembly containers. Keep the first placed occurrence of
// each leaf so transforms inside shells/solids and shared boundaries survive.
void collectLeaves(const TopoDS_Shape& shape, std::unordered_set<Key>& seen,
                   std::vector<Job>& leaves) {
    if (!seen.insert(shape.TShape().get()).second) return;
    if (shape.ShapeType() == TopAbs_COMPOUND || shape.ShapeType() == TopAbs_COMPSOLID) {
        for (TopoDS_Iterator it(shape); it.More(); it.Next())
            collectLeaves(it.Value(), seen, leaves);
    } else if (shape.ShapeType() != TopAbs_VERTEX) {
        leaves.push_back({shape});
    }
}

// Relative scheduling hints, in arbitrary units. No area integration or
// bounding-box computation in the planner. Each unique trimmed edge adds cost.
#if STEP_MESH_WEIGHTED_JOBS
Count faceWeight(const TopoDS_Face& face) {
    if (!BRep_Tool::IsGeometric(face)) return 1;
    switch (BRepAdaptor_Surface(face, false).GetType()) {
        case GeomAbs_Plane: return 2;
        case GeomAbs_BezierSurface:
        case GeomAbs_BSplineSurface:
        case GeomAbs_OffsetSurface: return 32;
        default: return 8;
    }
}
#endif

// Pointer identity deliberately ignores location/orientation. Even different
// solids can own the same writable edge. Visit each topology subtree once.
void claimTopology(const TopoDS_Shape& shape, size_t owner, Job& leaf,
                   std::unordered_map<Key, size_t>& owners, Components& groups) {
    const auto [it, fresh] = owners.emplace(shape.TShape().get(), owner);
    if (!fresh) {
        groups.join(owner, it->second);
        return;
    }
#if STEP_MESH_WEIGHTED_JOBS
    if (shape.ShapeType() == TopAbs_FACE) {
        ++leaf.faces;
        leaf.weight += faceWeight(TopoDS::Face(shape));
    } else if (shape.ShapeType() == TopAbs_EDGE) {
        ++leaf.weight;
    }
#endif
    for (TopoDS_Iterator child(shape, false, false); child.More(); child.Next())
        claimTopology(child.Value(), owner, leaf, owners, groups);
}

std::vector<Job> planJobs(const TopoDS_Shape& assembly, size_t& leafCount) {
    std::unordered_set<Key> seen;
    std::vector<Job> leaves;
    collectLeaves(assembly, seen, leaves);
    leafCount = leaves.size();
    Components groups(leaves.size());
    std::unordered_map<Key, size_t> owners;
    for (size_t i = 0; i < leaves.size(); ++i)
        claimTopology(leaves[i].shape, i, leaves[i], owners, groups);

    std::vector<std::vector<size_t>> members(leaves.size());
    for (size_t i = 0; i < leaves.size(); ++i) members[groups.root(i)].push_back(i);
    std::vector<Job> jobs;
    BRep_Builder builder;
    for (const auto& ids : members) {
        if (ids.empty()) continue;
        if (ids.size() == 1) {
            jobs.push_back(leaves[ids.front()]);
            continue;
        }
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        Job job;
        for (auto i : ids) {
            builder.Add(compound, leaves[i].shape);
#if STEP_MESH_WEIGHTED_JOBS
            job.weight += leaves[i].weight;
            job.faces += leaves[i].faces;
#endif
        }
        job.shape = compound;
        jobs.push_back(job);
    }
#if STEP_MESH_WEIGHTED_JOBS
    std::stable_sort(jobs.begin(), jobs.end(),
                     [](const Job& a, const Job& b) { return a.weight > b.weight; });
#endif
    return jobs;
}

enum class Mode { serial, parts, hybrid, whole };
Mode parseMode(const std::string& mode) {
    if (mode == "serial") return Mode::serial;
    if (mode == "parts") return Mode::parts;
    if (mode == "hybrid") return Mode::hybrid;
    if (mode == "whole") return Mode::whole;
    throw std::invalid_argument("mode must be serial, parts, hybrid or whole");
}

struct Result {
    int flags = 0;
    bool done = false;
    double elapsed = 0;
};

Result mesh(const TopoDS_Shape& shape, double linear, double angle, bool inside) {
    const auto start = Clock::now();
    BRepMesh_IncrementalMesh mesher(shape, linear, false, angle, inside);
    return {mesher.GetStatusFlags(), mesher.IsDone(), seconds(start)};
}

size_t runJobs(const std::vector<Job>& jobs, std::vector<Result>& results,
               Mode mode, int threads, double linear, double angle) {
    if (mode == Mode::serial || threads == 1) {
        for (size_t i = 0; i < jobs.size(); ++i)
            results[i] = mesh(jobs[i].shape, linear, angle, false);
        return 0;
    }
    oneapi::tbb::task_arena arena(threads);
    if (mode == Mode::whole) {
        arena.execute([&] { results[0] = mesh(jobs[0].shape, linear, angle, true); });
        return 1;
    }
#if !STEP_MESH_WEIGHTED_JOBS
    arena.execute([&] {
        oneapi::tbb::parallel_for(size_t{0}, jobs.size(), [&](size_t i) {
            results[i] = mesh(jobs[i].shape, linear, angle, false);
        });
    });
    return 0;
#else
    Count total = 0;
    for (const Job& job : jobs) total += job.weight;
    size_t innerJobs = 0;
    // A job well above one worker's average load can dominate the tail.
    // Run those one at a time so the native OCCT pool never overlaps outer TBB.
    std::vector<size_t> outer;
    arena.execute([&] {
        for (size_t i = 0; i < jobs.size(); ++i) {
            if (mode == Mode::hybrid && jobs[i].faces > 1 &&
                jobs[i].weight > 1.5 * static_cast<double>(total) / threads) {
                results[i] = mesh(jobs[i].shape, linear, angle, true);
                ++innerJobs;
            } else {
                outer.push_back(i);
            }
        }
        if (outer.empty()) return;
        // Sorting parallel_for iteration indices alone does not guarantee
        // heavy-first starts. Workers claim the next heavy job as they finish.
        std::atomic<size_t> next{0};
        const auto workers = std::min(outer.size(), static_cast<size_t>(threads));
        oneapi::tbb::parallel_for(size_t{0}, workers, [&](size_t) {
            for (;;) {
                const auto pos = next.fetch_add(1, std::memory_order_relaxed);
                if (pos >= outer.size()) break;
                const auto i = outer[pos];
                results[i] = mesh(jobs[i].shape, linear, angle, false);
            }
        });
    });
    return innerJobs;
#endif
}

struct Counts {
    Count occurrences = 0, uniqueFaces = 0, triangles = 0, missing = 0;
    std::vector<int> trianglesPerFace; // first-occurrence order in the input assembly
};
Counts checkMesh(const TopoDS_Shape& root) {
    Counts counts;
    std::unordered_set<Key> seen;
    for (TopExp_Explorer it(root, TopAbs_FACE); it.More(); it.Next()) {
        ++counts.occurrences;
        const auto& face = TopoDS::Face(it.Current());
        if (!seen.insert(face.TShape().get()).second) continue;
        ++counts.uniqueFaces;
        TopLoc_Location location;
        const auto& tri = BRep_Tool::Triangulation(face, location);
        const int n = tri.IsNull() ? 0 : tri->NbTriangles();
        counts.trianglesPerFace.push_back(n);
        if (n == 0) ++counts.missing;
        counts.triangles += n;
    }
    return counts;
}

double positiveNumber(const char* arg) {
    size_t end = 0;
    const double x = std::stod(arg, &end);
    if (arg[end] != '\0' || !std::isfinite(x) || x <= 0)
        throw std::invalid_argument("expected a positive finite number");
    return x;
}

TopoDS_Shape readShape(const char* path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    TopoDS_Shape assembly;
    if (ext == ".brep") {
        BRep_Builder builder;
        if (!BRepTools::Read(assembly, path, builder)) throw std::runtime_error("BREP read failed");
    } else {
        STEPControl_Reader reader;
        if (reader.ReadFile(path) != IFSelect_RetDone || reader.TransferRoots() == 0)
            throw std::runtime_error("STEP read/transfer failed");
        assembly = reader.OneShape();
    }
    if (assembly.IsNull()) throw std::runtime_error("empty import result");
    return assembly;
}

struct Trial {
    std::string name;
    Mode mode;
    int threads;
};

Trial trial(const std::string& name, const std::string& count) {
    Mode mode = parseMode(name);
    size_t end = 0;
    int threads = std::stoi(count, &end);
    if (end != count.size() || threads < 1 || (mode == Mode::serial && threads != 1))
        throw std::invalid_argument("threads must be positive, and 1 for serial");
    return {name, mode, threads};
}

int benchmark(const TopoDS_Shape& assembly, const Trial& t, double linear, double angle,
              std::vector<int>& reference) {
    const auto mode = t.mode;
    const int threads = t.threads;
    // CLI owns the process. Resize only between trials, with both pools idle.
    if (OSD_Parallel::ToUseOcctThreads()) {
        auto pool = OSD_ThreadPool::DefaultPool(threads);
        pool->Init(threads);
        pool->SetNbDefaultThreadsToLaunch(threads);
    }
    // Force a cold mesh even if the source also carries polygonal data.
    // OCCT keeps triangulations that are the only representation of a face.
    BRepTools::Clean(assembly);
    const auto prep = Clock::now();
    size_t leaves = 0;
    std::vector<Job> jobs = mode == Mode::whole ? std::vector<Job>{{assembly}}
                                                : planJobs(assembly, leaves);
    const auto planTime = seconds(prep);
    std::vector<Result> results(jobs.size());
    const auto meshStart = Clock::now();
    const auto inner = runJobs(jobs, results, mode, threads, linear, angle);
    const auto meshTime = seconds(meshStart);
    auto counts = checkMesh(assembly);
    int flags = 0;
    size_t failedJobs = 0;
    double longest = 0;
    for (const Result& r : results) {
        flags |= r.flags;
        if (!r.done || (r.flags & (IMeshData_Failure | IMeshData_UserBreak))) ++failedJobs;
        longest = std::max(longest, r.elapsed);
    }
    std::cout << "mode=" << t.name << " threads=" << threads
              << " weighted_jobs=" << STEP_MESH_WEIGHTED_JOBS
              << " occt_backend=" << (OSD_Parallel::ToUseOcctThreads() ? "native" : "tbb")
              << " linear_mm=" << linear << " angle_rad=" << angle << '\n'
              << "face_occurrences=" << counts.occurrences << " unique_faces=" << counts.uniqueFaces
              << " unique_triangles=" << counts.triangles << " missing_faces=" << counts.missing << '\n';
    if (mode != Mode::whole) std::cout << "unique_leaves=" << leaves << ' ';
    std::cout << "jobs=" << jobs.size() << " inner_jobs=" << inner << " failed_jobs=" << failedJobs
              << " status=0x" << std::hex << flags << std::dec << '\n'
              << "plan_s=" << planTime << " mesh_s=" << meshTime
              << " plan_mesh_s=" << planTime + meshTime << " longest_job_s=" << longest << std::endl;
    if (reference.empty()) {
        reference = std::move(counts.trianglesPerFace);
    } else {
        if (reference.size() != counts.trianglesPerFace.size())
            throw std::runtime_error("face count changed during sweep");
        size_t different = 0, coverage = 0;
        for (size_t i = 0; i < reference.size(); ++i) {
            different += reference[i] != counts.trianglesPerFace[i];
            coverage += (reference[i] == 0) != (counts.trianglesPerFace[i] == 0);
        }
        std::cout << "vs_first_changed_face_counts=" << different
                  << " vs_first_changed_face_coverage=" << coverage << std::endl;
    }
    return counts.uniqueFaces == 0 || counts.missing || failedJobs ? 2 : 0;
}

int run(int argc, char** argv) {
    if (argc < 4 || argc > 7) {
        std::cerr << "Usage: step_mesh_bench model.step|model.brep MODE THREADS"
                     " [linear_mm [angle_rad [output.brep]]]\n"
                     "MODE: serial | parts | hybrid | whole\n"
                     "Or replace MODE THREADS with: sweep serial/1,parts/8,hybrid/8,whole/8\n";
        return 1;
    }
    std::vector<Trial> trials;
    if (std::string(argv[2]) == "sweep") {
        std::istringstream items(argv[3]);
        for (std::string item; std::getline(items, item, ',');) {
            const auto slash = item.find('/');
            if (slash == std::string::npos) throw std::invalid_argument("expected mode/threads");
            trials.push_back(trial(item.substr(0, slash), item.substr(slash + 1)));
        }
        if (trials.empty()) throw std::invalid_argument("empty sweep");
    } else {
        trials.push_back(trial(argv[2], argv[3]));
    }
    const double linear = argc > 4 ? positiveNumber(argv[4]) : 0.1;
    const double angle = argc > 5 ? positiveNumber(argv[5]) : 0.35;
    if (linear < Precision::Confusion() || angle < Precision::Angular())
        throw std::invalid_argument("deflection below OCCT precision");
    if (argc == 7 && std::filesystem::exists(argv[6]) && std::filesystem::equivalent(argv[1], argv[6]))
        throw std::invalid_argument("input and output refer to the same file");

    const auto start = Clock::now();
    const auto assembly = readShape(argv[1]);
    std::cout << "import_s=" << seconds(start) << std::endl;
    int status = 0;
    std::vector<int> reference;
    for (const auto& t : trials)
        status = std::max(status, benchmark(assembly, t, linear, angle, reference));
    if (argc == 7) {
        if (!BRepTools::Write(assembly, argv[6])) throw std::runtime_error("BREP output failed");
        std::cout << "wrote last trial to " << argv[6] << std::endl;
    }
    return status;
}
} // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const Standard_Failure& e) {
        std::cerr << "OCCT: " << e.GetMessageString() << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
    return 1;
}
