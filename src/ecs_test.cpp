// pch.cpp: 与预编译标头对应的源文件；编译成功所必需的

#include <iostream>

#include "MeshManager.h"
#include "pch.h"
#include "RenderInfoManager.h"
#include "TransformManager.h"

// 一般情况下，忽略此文件，但如果你使用的是预编译标头，请保留它。
using namespace std;

void main()
{
    auto t1 = clock();

    Scene::instance().init(50000);
    auto t2 = clock();
    cout << "aaa: " << double(t2 - t1) << endl;
    getchar();
}
