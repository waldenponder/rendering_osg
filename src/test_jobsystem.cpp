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

void test_jobsystem()
{
    test4();
    return;
    utils::JobSystem* js_p = new utils::JobSystem;
    auto& js = *js_p;

    js.adopt();

    auto* parent = js.createJob();

    utils::jobs::createJob(js, parent, []
    {
        //
        std::cout << "child 1\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "child 1 job done\n";
    });

    utils::jobs::createJob(js, parent, []
    {
        //
        std::cout << "child 2\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "child 2 job done\n";
    });

    js.runAndWait(parent);
    std::cout << "\n";
}
