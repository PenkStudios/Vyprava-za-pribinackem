#!/bin/python

version = ""
with open("README.md", "r") as readme:
	line = readme.read().split("\n")[0]
	version = line[31:len(line)]

import os
os.system("make release")
os.chdir("timer mod")
os.system("make")
os.chdir("..")
os.system("git add -A")
os.system(f"git commit -m \"Update {version}\"")
os.system("git push origin main")
