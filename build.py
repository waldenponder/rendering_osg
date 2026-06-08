import subprocess
import os
import shutil

print(shutil.which("cmake"))

script_directory = os.path.dirname(os.path.abspath(__file__))

source_dir = script_directory  # CMakeLists.txt所在目录
build_dir = os.path.join(script_directory,  "build")

os.makedirs(build_dir, exist_ok=True)

# 配置
subprocess.check_call([
    "cmake",
    "-S", source_dir,
    "-B", build_dir
])

# 编译
subprocess.check_call([
    "cmake",
    "--build", build_dir,
    "--config", "Debug"
])

print("Build Success Debug")


subprocess.check_call([
    "cmake",
    "--build", build_dir,
    "--config", "Release"
])

print("Build Success Release")