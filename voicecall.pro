TEMPLATE = subdirs
SUBDIRS += src lib plugins

plugins.depends = lib
src.depends = lib
