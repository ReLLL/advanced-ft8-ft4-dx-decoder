#import <AppKit/AppKit.h>
#include <vector>
#include <iostream>
namespace dx {std::vector<unsigned char> homeIconPixels();std::vector<unsigned char> trophyIconPixels();std::vector<unsigned char> flagIconPixels(const std::string&);}
int main(){@autoreleasepool{
 [NSApplication sharedApplication];auto pixels=dx::homeIconPixels();if(pixels.size()!=64*64*4)return 1;size_t visible=0;
 for(size_t i=3;i<pixels.size();i+=4)if(pixels[i])++visible;if(visible<512)return 1;
 for(const auto& icon:std::vector<std::vector<unsigned char>>{dx::trophyIconPixels(),dx::flagIconPixels("TR"),dx::flagIconPixels("US"),dx::flagIconPixels("")}){if(icon.size()!=64*64*4)return 1;size_t visible=0;for(size_t i=3;i<icon.size();i+=4)if(icon[i])++visible;if(visible<128)return 1;}if(!dx::flagIconPixels("bad").empty())return 1;
 std::cout<<"Native house, trophy and flag emoji have visible RGBA pixels\n";return 0;
}}
