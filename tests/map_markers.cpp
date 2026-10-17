// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "map_markers.h"
#include <imgui_internal.h>
#include <cstring>
#include <iostream>
#if DX_HOST_BROWN
// Headless geometry tests have no host widget inspector to register with.
namespace ImGui {void sdrcppRegisterWidget(ImGuiID,int,const ImRect&) {}}
#endif
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
bool same(const ImDrawList& a,const ImDrawList& b){
 if(a.VtxBuffer.Size!=b.VtxBuffer.Size||a.IdxBuffer.Size!=b.IdxBuffer.Size)return false;
 for(int i=0;i<a.VtxBuffer.Size;i++)if(std::memcmp(&a.VtxBuffer[i],&b.VtxBuffer[i],sizeof(ImDrawVert)))return false;
 for(int i=0;i<a.IdxBuffer.Size;i++)if(a.IdxBuffer[i]!=b.IdxBuffer[i])return false;return true;
}
int main(){
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={256,128};io.DeltaTime=1.f/60;
 unsigned char* pixels;int width,height;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);ImGui::NewFrame();
 auto prepare=[](ImDrawList& list){list._ResetForNewFrame();list.PushClipRectFullScreen();};
 for(const auto* mode:{"FT8","FT4"})for(float radius:{4.f,5.5f,6.f,7.5f,8.f}){
  ImDrawList previous(ImGui::GetDrawListSharedData()),next(ImGui::GetDrawListSharedData());prepare(previous);prepare(next);
  previous.AddCircleFilled({64,64},radius,IM_COL32(98,207,178,255));dx::drawModeMarker(&next,mode,{64,64},radius,IM_COL32(98,207,178,255));CHECK(same(previous,next));
 }
 for(const auto* mode:{"FT8","FT4"}){
  ImDrawList previous(ImGui::GetDrawListSharedData()),next(ImGui::GetDrawListSharedData());prepare(previous);prepare(next);
  previous.AddCircle({64,64},8,IM_COL32(242,196,114,255),16,1.8f);dx::drawModeMarker(&next,mode,{64,64},8,IM_COL32(242,196,114,255),false);CHECK(same(previous,next));
 }
 for(const auto* mode:{"WSPR","FST4","FST4W","Q65","JS8"}){
  ImDrawList marker(ImGui::GetDrawListSharedData());prepare(marker);dx::drawModeMarker(&marker,mode,{64,64},5,IM_COL32_WHITE);CHECK(marker.VtxBuffer.Size>0&&marker.IdxBuffer.Size>0);
 }
 ImGui::EndFrame();ImGui::DestroyContext();std::cout<<"FT8/FT4 marker geometry unchanged; new mode markers render in native ImGui\n";return 0;
}
