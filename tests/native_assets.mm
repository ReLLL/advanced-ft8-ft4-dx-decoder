#import <AppKit/AppKit.h>
#include <vector>
#include <iostream>
namespace dx {std::vector<unsigned char> homeIconPixels();}
int main(){@autoreleasepool{
 [NSApplication sharedApplication];auto pixels=dx::homeIconPixels();if(pixels.size()!=64*64*4)return 1;size_t visible=0;
 for(size_t i=3;i<pixels.size();i+=4)if(pixels[i])++visible;if(visible<512)return 1;
 std::cout<<"Native house emoji has visible RGBA pixels\n";return 0;
}}
