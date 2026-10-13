// Test adapter around the unchanged pinned Brown WAV wrapper.
#include <functional>
#include <vector>
#include <string>
#include <cstdio>
extern void doDecode(const char*,const char*,int,std::function<void(int,std::vector<std::string>)>);
int main(int argc,char** argv){if(argc!=3)return 2;doDecode(argv[1],argv[2],1,[](int,std::vector<std::string> result){for(auto& field:result)printf("%s\t",field.c_str());puts("");});return 0;}
