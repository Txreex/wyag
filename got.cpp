#include<iostream>
#include<filesystem>
#include<fstream>
#include<unordered_map>
using namespace std;
namespace fs = std::filesystem;

void create_file(fs::path p , string content = ""){
    ofstream out(p);
    if(!out){
        throw runtime_error("Failed to create file " + p.string());
    }else{
        out<<content;
        out.close();
    }
}

void create_structure(class GitRepository *repo);

class GitRepository {
public:
    fs::path worktree;
    fs::path gitdir;

    void init() {
        this->worktree = fs::current_path();
        this->gitdir   = this->worktree / ".git";

        if(fs::exists(gitdir) && fs::is_directory(gitdir)){
            throw runtime_error("Already a git repository");
        }else{
            create_structure(this);
        }
    }
};

int main(int argc , char* argv[]){
    GitRepository repo;
    unordered_map<string , function<void()>> commands = {
        {"init" , [&](){repo.init();}}
    };

    if(argc > 1){
        string command = argv[1];
        if(commands.find(command) != commands.end()){
            commands[command]();
        }else{
            cout<<command<<" not found";
        }
    }else{
        cout<<"No command given";
    }
}

void create_structure(GitRepository *repo){
    fs::path gitdir = repo->gitdir;
    fs::create_directories(gitdir);
    create_file(gitdir/"HEAD", "ref: refs/heads/master\n");
    create_file(gitdir/"config", "[core]\n\trepositoryformatversion = 0\n\tfilemode = true\n\tbare = false\n\tlogallrefupdates = true");
    create_file(gitdir/"description","unnamed repository; edit this file 'description' to name the repository.");
    fs::create_directories(gitdir/"hooks");
    fs::create_directories(gitdir/"info");
    create_file(gitdir/"info/"/"exclude");
    fs::create_directories(gitdir/"objects"/"info");
    fs::create_directories(gitdir/"objects"/"pack");
    fs::create_directories(gitdir/"refs"/"heads");
    fs::create_directories(gitdir/"refs"/"tags");
}