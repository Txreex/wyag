import argparse

# for learning purposes
parser = argparse.ArgumentParser()
sub_parser = parser.add_subparsers(dest= 'command')

#init
init_parser = sub_parser.add_parser('init')

#commit
commit_parser = sub_parser.add_parser('commit')
commit_parser.add_argument('-m','--message',required= True)

args = parser.parse_args()

if args.command == "commit"  :
    print(f"Commiting the files with message : {args.message}")
if args.command == "init" :
    print(f"Initializing repository...")
