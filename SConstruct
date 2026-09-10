#!/usr/bin/env python
import os
import sys
from SCons.Script import *

# Import godot-cpp build environment
env = SConscript("godot-cpp/SConstruct")

env["strip"] = False
env.Append(CPPPATH=["src"])

# ---------------------------------------
# Debug flags (optional)
# ---------------------------------------
env.Append(CCFLAGS=["-g", "-O0"])
env.Append(CXXFLAGS=["-g", "-O0"])

# ---------------------------------------
# Platform specific sources
# ---------------------------------------
platform = env["platform"]

if platform == "linux":
    platform_sources = Glob("src/Linux/*.cpp")
    env.Append(CXXFLAGS=["-fexceptions"])
    target_name = "linuxhost"
elif platform == "windows":
    platform_sources = Glob("src/Windows/*.cpp")
    target_name = "windowshost"
else:
    print("Unsupported platform:", platform)
    Exit(1)

sources = platform_sources

# ---------------------------------------
# Windows specific fixes
# ---------------------------------------
if platform == "windows":
    env.Append(CPPDEFINES=["WIN32", "_WINDOWS", "UNICODE"])
    env.Append(LIBS=["user32", "kernel32"])
    env.Append(LINKFLAGS=["/SUBSYSTEM:WINDOWS"])

# ---------------------------------------
# Build library
# ---------------------------------------
library = env.SharedLibrary(
    "bin/{}{}{}".format(target_name, env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)