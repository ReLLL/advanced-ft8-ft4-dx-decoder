// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "station_selection.h"
#include <imgui_internal.h>
#include <iostream>
#if DX_HOST_BROWN
namespace ImGui {void sdrcppRegisterWidget(ImGuiID,int,const ImRect&) {}}
#endif
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={400,300};io.DeltaTime=.05f;
 unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);int selected=-1;bool details=false;ImVec2 positions[3];
 auto frame=[&]{ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({380,280});ImGui::Begin("Stations",nullptr,ImGuiWindowFlags_NoTitleBar);if(ImGui::BeginTable("rows",2,ImGuiTableFlags_ScrollY,{350,240})){
   selected=dx::keyboardStationRow(selected,3);
   for(int i=0;i<3;i++){ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::PushID(i);auto result=dx::stationRowSelection("K1ABC",selected==i);auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();positions[i]={(a.x+b.x)/2,(a.y+b.y)/2};if(result.select)selected=i;if(result.openDetails)details=true;ImGui::PopID();ImGui::TableNextColumn();ImGui::TextUnformatted("Country");}ImGui::EndTable();}ImGui::End();ImGui::Render();};
 frame();frame();io.MousePos=positions[0];io.MouseDown[0]=true;frame();io.MouseDown[0]=false;frame();CHECK(selected==0&&!details);
 io.AddKeyEvent(ImGuiKey_DownArrow,true);frame();io.AddKeyEvent(ImGuiKey_DownArrow,false);frame();CHECK(selected==1&&!details);
 io.AddKeyEvent(ImGuiKey_UpArrow,true);frame();io.AddKeyEvent(ImGuiKey_UpArrow,false);frame();CHECK(selected==0&&!details);
 for(int i=0;i<8;i++)frame();io.MousePos=positions[2];io.MouseDown[0]=true;frame();io.MouseDown[0]=false;frame();CHECK(selected==2&&!details);io.MouseDown[0]=true;frame();io.MouseDown[0]=false;frame();CHECK(selected==2&&details);
 ImGui::DestroyContext();std::cout<<"Mouse and keyboard selection leave details closed; double click opens details\n";return 0;
}
