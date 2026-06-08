import subprocess

subprocess.check_call([
    "cmake",
    "-S", ".",
    "-B", "build",
    "-DCMAKE_BUILD_TYPE=Release"
])

subprocess.check_call([
    "cmake",
    "--build",
    "build",
    "-j8"
])