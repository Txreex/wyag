#include<iostream>
#include<filesystem>
#include<fstream>
#include<unordered_map>
#include<vector>
#include<functional>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <chrono>

using namespace std;
namespace fs = std::filesystem;


// ---------------------------FORWARD DECLARATIONS----------------------------------
class Object;
class Blob;
class TreeEntry;
class Tree;
class GitRepository;

// Git object and tree related
Tree* build_tree(fs::path dir_path);
Tree* build_tree_with_root(fs::path dir_path);
Tree* build_blobs_only(fs::path p);
TreeEntry* make_blob(fs::path p);
void write_object(TreeEntry* entry);
Blob* write_blob_only(TreeEntry* entry);

// Git repository utilities
void create_structure(GitRepository *repo);
string make_blob_content(fs::path p);
string make_tree_content(fs::path dir_path);
void stage_file(fs::path file_path);
void create_file(fs::path p, string content = "");
fs::path get_relative_path(const string& filepath);
static vector<string> split_path(const string& s);

// Commit related
string get_parent_commit_sha();
void commit(string message, string author);

// ---------------------------SHA1 IMPLIMENTATION-----------------------------------
class SHA1_CTX {
public:
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
};

inline uint32_t rol(uint32_t value, size_t bits) {
    return (value << bits) | (value >> (32 - bits));
}

// Fix: blk now returns uint32_t
inline uint32_t blk(uint32_t block[16], int i) {
    block[i & 15] = rol(block[(i+13)&15] ^ block[(i+8)&15] ^ block[(i+2)&15] ^ block[i&15], 1);
    return block[i & 15];
}

#define R0(v,w,x,y,z,i) do { z += ((w&(x^y))^y) + block[i] + 0x5A827999 + rol(v,5); w=rol(w,30); } while(0)
#define R1(v,w,x,y,z,i) do { z += ((w&(x^y))^y) + blk(block,i) + 0x5A827999 + rol(v,5); w=rol(w,30); } while(0)
#define R2(v,w,x,y,z,i) do { z += (w^x^y) + blk(block,i) + 0x6ED9EBA1 + rol(v,5); w=rol(w,30); } while(0)
#define R3(v,w,x,y,z,i) do { z += (((w|x)&y)|(w&x)) + blk(block,i) + 0x8F1BBCDC + rol(v,5); w=rol(w,30); } while(0)
#define R4(v,w,x,y,z,i) do { z += (w^x^y) + blk(block,i) + 0xCA62C1D6 + rol(v,5); w=rol(w,30); } while(0)

std::string sha1(const std::string& input) {
    SHA1_CTX ctx;
    ctx.state[0]=0x67452301;
    ctx.state[1]=0xEFCDAB89;
    ctx.state[2]=0x98BADCFE;
    ctx.state[3]=0x10325476;
    ctx.state[4]=0xC3D2E1F0;
    ctx.count[0]=ctx.count[1]=0;

    uint64_t totalBits = input.size() * 8;
    std::vector<unsigned char> data(input.begin(), input.end());
    data.push_back(0x80);

    while ((data.size() % 64) != 56) data.push_back(0x00);

    for (int i = 7; i >= 0; --i) {
        data.push_back((totalBits >> (i*8)) & 0xFF);
    }

    for (size_t chunk=0; chunk<data.size(); chunk+=64) {
        uint32_t block[16];
        for(int i=0;i<16;i++) {
            block[i] = (data[chunk + i*4]<<24) | (data[chunk + i*4 + 1]<<16) |
                       (data[chunk + i*4 + 2]<<8) | (data[chunk + i*4 + 3]);
        }

        uint32_t a=ctx.state[0];
        uint32_t b=ctx.state[1];
        uint32_t c=ctx.state[2];
        uint32_t d=ctx.state[3];
        uint32_t e=ctx.state[4];

        R0(a,b,c,d,e,0); R0(e,a,b,c,d,1); R0(d,e,a,b,c,2); R0(c,d,e,a,b,3);
        R0(b,c,d,e,a,4); R0(a,b,c,d,e,5); R0(e,a,b,c,d,6); R0(d,e,a,b,c,7);
        R0(c,d,e,a,b,8); R0(b,c,d,e,a,9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
        R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
        for(int i=16;i<20;i++) R1(a,b,c,d,e,i);

        for(int i=20;i<40;i++) R2(a,b,c,d,e,i);
        for(int i=40;i<60;i++) R3(a,b,c,d,e,i);
        for(int i=60;i<80;i++) R4(a,b,c,d,e,i);

        ctx.state[0]+=a; ctx.state[1]+=b; ctx.state[2]+=c; ctx.state[3]+=d; ctx.state[4]+=e;
    }

    std::ostringstream result;
    for(int i=0;i<5;i++)
        result << std::hex << std::setw(8) << std::setfill('0') << ctx.state[i];
    return result.str();
}
// ---------------------------------------------------------------------------------

// ------------------------------------TREE-----------------------------------------

class Object{
public :
    virtual ~Object(){
        //
    }
};

class Blob : public Object{
public :
    string content;
};

class TreeEntry{
public :
    int mode;
    string name;
    string sha;
    Object* hash;

    TreeEntry(int mode , string name , Object* hash , string sha = ""){
        this->sha = sha;
        this->mode = mode;
        this->name = name;
        this->hash = hash;
    }
};

class Tree : public Object{
public :
    vector<TreeEntry*> entries;
    string content;

    void add_entries(int mode , string name , Object* hash , string sha){
        TreeEntry* new_entry = new TreeEntry(mode, name, hash , sha);
        entries.push_back(new_entry);
    }
};

void print_tree(Tree* tree, string indent = ""){
    for(TreeEntry* entry : tree->entries){
        if(entry->name[0] == '.'){
            continue;
        }
        Blob* blob = dynamic_cast<Blob*>(entry->hash);
        if(blob){   // The hash is blob*
            cout<<indent<<entry->name<<" : "<<entry->sha<<endl;
        }else{
            Tree* t = dynamic_cast<Tree*>(entry->hash);
            if(t){  // the has is a tree
                cout<<indent<<entry->name<<"/"<<" : "<<entry->sha<<endl;
                print_tree(t , indent + "       ");
            }else{
                cout<<"Unknown type";
            }
        }
    }
}

// ---------------------------------------------------------------------------------

// ---------------------------STAGING AREA------------------------------------------
unordered_map<string, TreeEntry*> staging_area;

void load_index() {
    staging_area.clear();
    ifstream indexFile(".mgit/index");
    string line;

    while (getline(indexFile, line)) {
        istringstream iss(line);
        int mode;
        string path, sha;

        iss >> mode >> path >> sha;

        Blob* blob = new Blob(); 
        blob->content = ""; 
        staging_area[path] = new TreeEntry(mode, path, blob, sha);
    }
}

void save_index() {
    std::ofstream indexFile(".mgit/index", std::ios::trunc);
    for (auto& [path, entry] : staging_area) {
        indexFile << entry->mode << " " << path << " " << entry->sha << "\n";
    }
    indexFile.close();
}

// ---------------------------------------------------------------------------------


// ------------------------------------GitRepository--------------------------------

class GitRepository {
public:
    fs::path worktree;
    fs::path gitdir;
    int is_repo = false;

    void init() {
        this->worktree = fs::current_path();
        this->gitdir   = this->worktree / ".mgit";

        if(fs::exists(gitdir) && fs::is_directory(gitdir)){
            throw runtime_error("Already a mgit repository");
        }else{
            create_structure(this);
            is_repo = true;

            // Pretty printing
            cout << "Initialized empty Git++ repository in " << fs::absolute(gitdir) << endl;
            cout << "Created structure:" << endl;
            cout << "  HEAD, config, description" << endl;
            cout << "  hooks/, info/, objects/, refs/heads/, refs/tags/" << endl;
        }
    }

    void add(fs::path p = fs::current_path()) {
        if(p == "."){
            for (const auto& entry : fs::recursive_directory_iterator(fs::current_path())) {
                if(entry.path().filename().string()[0] == '.') continue;
                if (entry.is_regular_file()){
                    stage_file(entry.path());
                }     
            }
        }else if (fs::is_regular_file(p)) {
            stage_file(p);
        } else if (fs::is_directory(p)) {
            for (const auto& entry : fs::recursive_directory_iterator(p)) {
                if(entry.path().filename().string()[0] == '.') continue;
                if (entry.is_regular_file()) stage_file(entry.path());
            }
        } else {
            throw std::runtime_error("Invalid path for add: " + p.string());
        }

        // Pretty printing
        cout << "Staging " << staging_area.size() << " file(s):" << endl;
        for (const auto& [path, entry] : staging_area) {
            cout << "  " << path << endl;
        }
        cout << "Files added to staging area.\n";
    }
};

// ---------------------------------------------------------------------------------


// ------------------------------------MAIN-----------------------------------------

int main(int argc , char** argv){
    if(fs::exists(".mgit/index")){
        load_index();
    }

    for (auto& [path, entry] : staging_area) {
        cout << "Path (from key): " << path << "\n";
        cout << "Path (from entry): " << entry->name << "\n";
        cout << "SHA: " << entry->sha << "\n";
    }

    GitRepository repo;
    unordered_map<string , function<void()>> commands = {
        {"init" , [&](){repo.init();}},
        {"add" , [&](){
                if(argc < 3) {
                    repo.add(fs::current_path()); 
                } else {
                    repo.add(argv[2]);
                };
            }
        },
        {"commit" , [&](){
            commit("test" , "Tanay");
        }}
    };

    if(argc > 1){
        string command = argv[1];
        if(commands.find(command) != commands.end()){
            commands[command]();
        }else{
            cout<<command<<" not found";
        }
    }else{
        cout<<"No command given"<<endl;
    }

    return 0;
}
// ---------------------------------------------------------------------------------


// ---------------------------------UTILITIES---------------------------------------

void create_structure(GitRepository *repo){
    fs::path gitdir = repo->gitdir;
    fs::create_directories(gitdir);
    create_file(gitdir / "HEAD", "ref: refs/heads/master\n");
    create_file(gitdir / "config", "[core]\n\trepositoryformatversion = 0\n\tfilemode = true\n\tbare = false\n\tlogallrefupdates = true");
    create_file(gitdir / "description","unnamed repository; edit this file 'description' to name the repository.");
    fs::create_directories(gitdir / "hooks");
    fs::create_directories(gitdir / "info");
    create_file(gitdir / "info" / "exclude");
    fs::create_directories(gitdir / "objects" / "info");
    fs::create_directories(gitdir / "objects" / "pack");
    fs::create_directories(gitdir / "refs/heads");
    fs::create_directories(gitdir / "refs/tags");
}


string make_blob_content(fs::path p){
    if(!fs::is_regular_file(p)){
        throw runtime_error("NOT A VALID FILE PATH ");
    }
    ifstream in(p , ios::binary);

    string file_content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    string blob_content = "blob " + to_string(file_content.size()) + "\\0" + file_content;  

    return blob_content;
}

string make_tree_content(fs::path dir_path) {
    if(!fs::is_directory(dir_path)) {
        throw runtime_error(dir_path.string() + " is not a directory");
    }

    string tree_content;

    // Iterate over immediate children only
    for(const auto& entry : fs::directory_iterator(dir_path)) {
        string name = entry.path().filename().string();
        string entry_content;
        int mode;

        if(entry.is_regular_file()) {
            mode = 100644;
            entry_content = make_blob_content(entry.path());
        } else if(entry.is_directory()) {
            mode = 40000;
            entry_content = make_tree_content(entry.path()); // recursive call
        } else {
            continue; // skip symlinks, etc.
        }

        if(entry_content.size()){
            string hash = sha1(entry_content);
            // Git tree format: "<mode> <name>\0<hash as raw bytes>"
            tree_content += to_string(mode) + " " + name + "\0" + hash +"\n";
        }
    }

    return tree_content;
}


Tree* build_tree(fs::path dir_path) {
    // build a tree for the children inside dir_path
    Tree* result = new Tree();
    
    if(fs::is_directory(dir_path)){
        for (const auto& entry : fs::directory_iterator(dir_path)) {
            if (entry.path().filename() == ".mgit") {
                continue;
            }

            if (entry.is_regular_file()) {
                fs::path rel_path = get_relative_path(entry.path());
                if (staging_area.find(rel_path.string()) != staging_area.end()) {
                    Blob* blob = new Blob();
                    string content = make_blob_content(entry.path());
                    blob->content = content;
                    string hash = sha1(content);
                    result->add_entries(100644, entry.path().filename().string(), blob, hash);
                }
            }
             else if (entry.is_directory()) {
                Tree* sub_tree = build_tree(entry.path());
                string tree_content = make_tree_content(entry.path());
                sub_tree->content = tree_content;
                string hash = sha1(tree_content);
                result->add_entries(40000, entry.path().filename().string(), sub_tree, hash);
            }
        }
    }else if(fs::is_regular_file(dir_path)){
        fs::path rel_path = get_relative_path(dir_path);

        if (staging_area.find(rel_path.string()) != staging_area.end()) {
            Blob* blob = new Blob();
            string content = make_blob_content(dir_path);
            blob->content = content;
            string hash = sha1(content);
            result->add_entries(100644, dir_path.filename().string(), blob, hash);
        }
    }

    return result;
}

void stage_file(fs::path file_path) {
    if (file_path.string().find(".mgit") != string::npos) return;

    TreeEntry* entry = make_blob(file_path);
    write_blob_only(entry);

    fs::path p = get_relative_path(file_path);
    staging_area[p.string()] = entry; // add or overwrite in-memory
    
    //append to .mgit/index immediately
    save_index();
}


TreeEntry* make_blob(fs::path p){
    Blob* blob = new Blob();
    blob->content = make_blob_content(p);
    string hash = sha1(blob->content);

    TreeEntry* entry = new TreeEntry(100644 , p.filename().string(),blob,hash);
    return entry;
}

Tree* build_tree_with_root(fs::path dir_path) {
    Tree* content_tree = build_tree(dir_path);
    Tree* root_tree = new Tree();

    string tree_content = make_tree_content(dir_path);
    content_tree->content = tree_content;
    string hash = sha1(tree_content);

    root_tree->add_entries(40000, dir_path.filename().string(), content_tree, hash);

    return root_tree;
}

Tree* build_blobs_only(fs::path p){
    Tree* result = build_tree(p);
    for(auto & entry : result->entries){
        write_blob_only(entry);
    }
    return result;
}

Blob* write_blob_only(TreeEntry* entry){
    Blob* blob = dynamic_cast<Blob*>(entry->hash);
    if(blob){
        string hash = entry->sha;
        string content = blob->content;
        fs::path folder_path = ".mgit/objects/" + hash.substr(0,2);
        fs::path file_path = folder_path/hash.substr(2);

        fs::create_directories(folder_path);
        create_file(file_path , content);
    }else{
        Tree* tree = dynamic_cast<Tree*>(entry->hash);
        if(tree){
            for (auto* subentry : tree->entries) {
               write_blob_only(subentry);
            }
        }else{  
            throw runtime_error("Not a valid type ");
        }
    }
    return blob;
}

void write_object(TreeEntry* entry){
    Blob* blob = dynamic_cast<Blob*>(entry->hash);
    if(blob){
        string hash = entry->sha;
        string content = blob->content;
        fs::path folder_path = ".mgit/objects/" + hash.substr(0,2);
        fs::path file_path = folder_path/hash.substr(2);

        fs::create_directories(folder_path);
        create_file(file_path , content);
    }else{
        Tree* tree = dynamic_cast<Tree*>(entry->hash);
        if(tree){
            string hash = entry->sha;
            string content = tree->content;
            fs::path folder_path = ".mgit/objects/" + hash.substr(0,2);
            fs::path file_path = folder_path/hash.substr(2);

            fs::create_directories(folder_path);
            create_file(file_path , content);

            for (auto* subentry : tree->entries) {
               write_object(subentry);
            }

        }else{  
            throw runtime_error("Not a valid type ");
        }
    }
}

void create_file(fs::path p , string content){
    ofstream out(p);
    if(!out){
        throw runtime_error("Failed to create file " + p.string());
    }else{
        out<<content;
        out.close();
    }
}

static vector<string> split_path(const string& s) {
    vector<string> parts;
    string cur;
    for (char c : s) {
        if (c == '/') { 
            if (!cur.empty()) {
                parts.push_back(cur); 
                cur.clear(); 
            } 
        }
        else cur.push_back(c);
    }
    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

fs::path get_relative_path(const string& filepath) {
    fs::path full = fs::absolute(filepath);
    fs::path repoRoot = fs::current_path(); 
    return fs::relative(full, repoRoot);
}

string get_parent_commit_sha() {
    string parent_sha;
    
    // 1️⃣ Read HEAD to find current branch
    ifstream head_file(".mgit/HEAD");
    if (!head_file) return ""; // no HEAD, no parent

    string head_content;
    getline(head_file, head_content);
    head_file.close();

    // HEAD usually contains: "ref: refs/heads/master"
    if (head_content.rfind("ref: ", 0) == 0) {
        fs::path branch_ref = head_content.substr(5); // skip "ref: "
        fs::path branch_file = ".mgit" / branch_ref;
        
        if (fs::exists(branch_file)) {
            ifstream branch_in(branch_file);
            getline(branch_in, parent_sha);
            branch_in.close();
        }
    } 
    return parent_sha; // empty string if no previous commit
}

void commit(string message, string author) {
    // 1️⃣ Build the root tree from staged files
    Tree* root_tree = build_tree_with_root(fs::current_path());
    
    for(auto entry : root_tree->entries){
        write_object(entry);
    }

    string tree_sha = root_tree->entries[0]->sha; // root tree SHA

    // 2️⃣ Get parent commit SHA
    string parent_sha = get_parent_commit_sha();

    // 3️⃣ Prepare commit content
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    string timestamp = to_string(t) + " +0000";

    string commit_content = "tree " + tree_sha + "\n";
    if(!parent_sha.empty()) commit_content += "parent " + parent_sha + "\n";
    commit_content += "author " + author + " " + timestamp + "\n";
    commit_content += "committer " + author + " " + timestamp + "\n\n";
    commit_content += message + "\n";

    // 4️⃣ Compute commit SHA
    string full_content = "commit " + to_string(commit_content.size()) + "\0" + commit_content;
    string commit_sha = sha1(full_content);

    // 5️⃣ Write commit object to .mgit/objects
    fs::path folder = ".mgit/objects/" + commit_sha.substr(0,2);
    fs::path file   = folder / commit_sha.substr(2);
    fs::create_directories(folder);
    create_file(file, full_content);

    // 6️⃣ Update current branch reference
    ifstream head_file(".mgit/HEAD");
    string head_content;
    getline(head_file, head_content);
    head_file.close();

    if (head_content.rfind("ref: ", 0) == 0) {
        fs::path branch_ref = head_content.substr(5);
        fs::path branch_file = ".mgit" / branch_ref;
        create_file(branch_file, commit_sha); // update branch with new commit SHA
    } else {
        // detached HEAD → update HEAD directly
        create_file(".mgit/HEAD", commit_sha);
    }

    staging_area.clear();
    save_index();

    cout << "Committed: " << commit_sha << endl;
}
// ---------------------------------------------------------------------------------
