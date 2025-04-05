import argparse
import configparser
from datetime import datetime
import grp, pwd
from fnmatch import fnmatch
import hashlib
from math import ceil
import os
import re
import sys
import zlib

argparser = argparse.ArgumentParser(description="Something Atleast")    # This is like git 

argsubparsers = argparser.add_subparsers(title="Commands", dest="command")  # This is a way to make the commands -> kind of like a folder named commands and then we create commands using commit_parser = argsubparsers.add_parser("commit") and then add arguments to those parsers like commit_parser.add_argument("-m",'--message',required = True)
argsubparsers.required = True       # you pass in <git COMMAND> and not just <git>


def main(argv = sys.argv[1:]) :
    args = argparser.parse_args(argv)
    match args.command :
        case "add"          : cmd_add(args)
        case "cat-file"     : cmd_cat_file(args)
        case "check_ignore" : cmd_check_ignore(args)
        case "checkout"     : cmd_checkout(args)
        case "commit"       : cmd_commit(args)
        case "hash-object"  : cmd_hash_object(args)
        case "init"         : cmd_init(args)
        case "log"          : cmd_log(args)
        case "ls-files"     : cmd_ls_files(args)
        case "ls-tree"      : cmd_ls_tree(args)
        case "rev-parse"    : cmd_rev_parse(args)
        case "rm"           : cmd_rm(args)
        case "show-ref"     : cmd_show_ref(args)
        case "status"       : cmd_status(args)
        case "tag"          : cmd_tag(args)
        case _              : print("Bad command.")

