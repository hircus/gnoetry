# This is -*- Python -*-

import os, sys
from xxx_gnoetics import *

class Library:

    def __init__(self, dir=None):
        self.__texts = []
        if dir:
            self.add_from_directory(dir)

    def add(self, filename):
        txt = Text(filename)
        sys.stderr.write("Adding '%s'\n" % txt.get_title())
        self.__texts.append(txt)

    def add_from_directory(self, dirname):
        for fn in os.listdir(dirname):
            if os.path.splitext(fn)[1] == ".ts":
                self.add(os.path.join(dirname, fn))

    def get_by_title(self, title):
        for txt in self.__texts:
            if txt.get_title().lower() == title.lower().strip():
                return txt
        return None

    def __iter__(self):
        return iter(self.__texts)
