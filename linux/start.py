#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys

def process(cname, pname):
  bazel_build = "bazel build //" + cname + ":"+ pname
  os.system(bazel_build)
  print("\n~~~ bin start ~~~")
  bazel_bin = "./bazel-bin/" + cname + "/" + pname
  os.system(bazel_bin)
  print("~~~ bin end ~~~\n")
  os.system("bazel clean")


def p02():
  r = 100.0 * (85 - 72) / 85
  print(f"hello, the percentage is {r:.1f}")

script, cname, pname = sys.argv

if int(cname) < 10:
  cname = "0" + cname
if int(pname) < 10:
  pname = "0" + pname

cname = "c" + cname
pname = "p" + pname
process(cname, pname)
