// Isolated UI QA only; never write synthetic receptions to a real data root.
#include "store.h"
#include <ctime>
#include <iostream>
int main(int argc,char** argv){if(argc!=2)return 2;std::filesystem::path root=std::filesystem::absolute(argv[1]).lexically_normal();if(root.string().rfind("/tmp/advanced-dx-",0)!=0)return 2;dx::Store store(root/"history.sqlite3");
 std::vector<std::pair<std::string,std::string>> messages={{"CQ G1TEST IO91","United Kingdom"},{"CQ K1TEST FN42","United States"},{"CQ VE1TEST FN03","Canada"},{"CQ K6TEST CM87","United States"},{"CQ LU1TEST GF05","Argentina"}};
 auto now=time(nullptr);int offset=0;
 auto insert=[&](const std::string& message,const std::string& country,int age){dx::Decode d;d.utc=now-age;d.slotMs=d.utc*1000;d.dial=7074000;d.mode="FT8";d.audio=1000;d.snr=-12;d.dt=.2;d.message=message;d.source="fixture";store.insert(d,{39.5,33},[&](const std::string&){return std::make_pair(country,"Test");});};
 for(auto& entry:messages)insert(entry.first,entry.second,90-offset++);
 for(int i=0;i<6;i++)insert("K1TEST G1TEST -"+std::to_string(10+i),"United Kingdom",60-i*8);
 insert("G1TEST K1TEST R-12","United States",5);insert("K1TEST G1TEST -15","United Kingdom",2);
 std::cout<<"Isolated visual fixtures inserted\n";return 0;
}
