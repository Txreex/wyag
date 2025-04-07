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

argparser = argparse.ArgumentParser(description="Something Atleast")  # This is like git

argsubparsers = argparser.add_subparsers(title="Commands", dest="command")  # Create subcommands
argsubparsers.required = True  # You must pass a command like <git COMMAND>


def main(argv=sys.argv[1:]):
    args = argparser.parse_args(argv)
    match args.command:
        case "add": cmd_add(args)
        case "cat-file": cmd_cat_file(args)
        case "check_ignore": cmd_check_ignore(args)
        case "checkout": cmd_checkout(args)
        case "commit": cmd_commit(args)
        case "hash-object": cmd_hash_object(args)
        case "init": cmd_init(args)
        case "log": cmd_log(args)
        case "ls-files": cmd_ls_files(args)
        case "ls-tree": cmd_ls_tree(args)
        case "rev-parse": cmd_rev_parse(args)
        case "rm": cmd_rm(args)
        case "show-ref": cmd_show_ref(args)
        case "status": cmd_status(args)
        case "tag": cmd_tag(args)
        case _: print("Bad command.")


# ------------------------ Git Repo Object --------------------------------------#

class GitRepository(object):
    worktree = None
    gitdir = None
    conf = None

    def __init__(self, path, force=False):
        self.worktree = path
        self.gitdir = os.path.join(path, '.git')

        if not (force or os.path.isdir(self.gitdir)):
            raise Exception(f"Not a git repository {path}")

        # ---- Read config file in .git/config ----
        self.conf = configparser.ConfigParser()
        cf = repo_file(self, "config")

        if cf and os.path.exists(cf):
            self.conf.read([cf])
        elif not force:
            raise Exception("Configuration file missing")

        if not force:
            vers = int(self.conf.get("core", "repositoryformatversion"))
            if vers != 0:
                raise Exception(f"Unsupported repositoryformatversion: {vers}")
        # -----------------------------------------


# ------------------------ Repo Path functions -----------------------------------#

def repo_path(repo, *path):
    """Computes the path under the repo"""
    return os.path.join(repo.gitdir, *path)


def repo_dir(repo, *path, mkdir=False):
    """Like repo_path, but ensures the directory exists (creates if needed)"""
    path = repo_path(repo, *path)

    if os.path.exists(path):
        if os.path.isdir(path):
            return path
        else:
            raise Exception(f"Not a directory: {path}")

    if mkdir:
        os.makedirs(path)
        return path
    else:
        return None


def repo_file(repo, *path, mkdir=False):
    """Same as repo_path, but ensures the directory leading to the file exists."""
    if repo_dir(repo, *path[:-1], mkdir=mkdir):
        return repo_path(repo, *path)


# ------------------------ Creating Repositories ---------------------------------#
def repo_create(path):
    repo = GitRepository(path, force=True)

    if os.path.exists(repo.worktree):
        if not os.path.isdir(repo.worktree):
            raise Exception(f"Not a directory {path}")
        if os.path.exists(repo.gitdir) and os.listdir(repo.gitdir):
            raise Exception(f"Cannot initialize repository: '{path}' is not empty!")
    else:
        os.makedirs(repo.worktree)

    assert repo_dir(repo, "branches", mkdir=True)
    assert repo_dir(repo, "objects", mkdir=True)
    assert repo_dir(repo, "refs", "tags", mkdir=True)
    assert repo_dir(repo, "refs", "heads", mkdir=True)

    # .git/description
    with open(repo_file(repo, "description", mkdir=True), "w") as f:
        f.write("Unnamed repository; edit this file 'description' to name the repository.\n")

    # .git/HEAD
    with open(repo_file(repo, "HEAD", mkdir=True), "w") as f:
        f.write("ref: refs/heads/master\n")

    with open(repo_file(repo, "config", mkdir=True), "w") as f:
        config = repo_default_config()
        config.write(f)

    return repo


# --------------------------------------------------------------------------------#

def repo_default_config():
    ret = configparser.ConfigParser()

    ret.add_section("core")
    ret.set("core", "repositoryformatversion", "0")
    ret.set("core", "filemode", "false")
    ret.set("core", "bare", "false")

    return ret


# ------------------------ Init command parser -----------------------------------#

init_parser = argsubparsers.add_parser('init', help="Initialize a new, empty repository.")
init_parser.add_argument("path",
                         metavar="directory",
                         nargs="?",
                         default=".",
                         help="Where to create the repository.")


def cmd_init(args):
    repo_create(args.path)
