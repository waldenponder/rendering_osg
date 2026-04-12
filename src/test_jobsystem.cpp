#include "test_jobsystem.h"
#include "utils/JobSystem.h"
#include <iostream>
#include <thread>
#include <vector>

using namespace utils;

void test4()
{
    JobSystem js;
    js.adopt();

    struct User
    {
        int c;
        int i, j;

        void func(JobSystem& js, JobSystem::Job* job)
        {
            if (c < 43)
            {
                User u{c + 1};
                JobSystem::Job* p = js.createJob<User, &User::func>(job, &u);
                js.runAndWait(p);

                i = u.i + u.j;
                j = u.i;
            }
            else
            {
                i = 0;
                j = 1;
            }
        }
    };

    User u{0};

    utils::JobSystem::Job* root = js.createJob<User, &User::func>(nullptr, &u);
    js.runAndWait(root);

    // 43rd fibonacci number
    assert(433494437 == u.i);

    js.emancipate();
}

//void tt5() {
//  utils::JobSystem *js_p = new utils::JobSystem;
//  auto &js = *js_p;
//
//  js.adopt();
//  auto *parent =
//      utils::jobs::createJob(js, nullptr, [&js](JobSystem::Job *self) {
//        // 只有 parent 跑起来以后，才产生 c1 和 c2
//        auto *c1 = utils::jobs::createJob(js, self, [] { /* 子任务1 */ });
//        auto *c2 = utils::jobs::createJob(js, self, [] { /* 子任务2 */ });
//
//        js.run(c1);
//        js.run(c2);
//
//        // Parent 自身的逻辑...
//        std::cout << "Parent working...\n";
//      });
//
//  // 主线程只需要管好 parent 即可
//  js.runAndWait(parent);
//}

void test_jobsystem()
{
    // tt5();
    //return;
    utils::JobSystem* js_p = new utils::JobSystem;
    auto& js = *js_p;

    js.adopt();

    auto* parent = utils::jobs::createJob(js, nullptr, []
    {
        //
        std::cout << "\nchild p\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "child p job done\n";
    });

    auto c1 = utils::jobs::createJob(js, parent, []
    {
        //
        std::cout << "child 1\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "child 1 job done\n";
    });

    auto c2 = utils::jobs::createJob(js, parent, []
    {
        //
        std::cout << "child 2\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "child 2 job done\n";
    });
    js.run(c1);
    js.run(c2);
    //js.run(parent);
    js.runAndWait(parent);
    std::cout << "\n";
}
