// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "status_bar.h"
#include "settings_navigation.h"
#include "table_defaults.h"
#include <imgui_internal.h>
#include <iostream>
#include <array>
#if DX_HOST_BROWN
namespace ImGui {void sdrcppRegisterWidget(ImGuiID,int,const ImRect&) {}}
#endif
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 CHECK(dx::statusOpacity(0)==0&&dx::statusOpacity(1)==.5f&&dx::statusOpacity(2)==1);
 CHECK(dx::statusOpacity(6)==1&&dx::statusOpacity(7)==.5f&&dx::statusOpacity(8)==0);
 dx::StatusNotification notice;notice.observe("1","Export saved",10);notice.observe("1","Export saved",11);CHECK(notice.started==10);notice.observe("2","Export saved",20);CHECK(notice.started==20&&notice.opacity(22)==1);
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={900,600};io.DeltaTime=.05f;unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
 auto frame=[&](float width,bool error,bool details,const std::string& text){
  ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({width,550});ImGui::Begin("Decoder",nullptr,ImGuiWindowFlags_NoTitleBar);
  auto origin=ImGui::GetCursorScreenPos();auto available=ImGui::GetContentRegionAvail();float reserved=dx::statusBarHeight()+ImGui::GetStyle().ItemSpacing.y;
  const float height=available.y-reserved;ImGui::BeginChild("content",{0,details?height*.6f:height});ImGui::TextUnformatted("Map and tables");ImGui::EndChild();
  if(details){ImGui::BeginChild("details",{0,height*.4f-ImGui::GetStyle().ItemSpacing.y});ImGui::TextUnformatted("Details");ImGui::EndChild();}
  ImGui::SetCursorScreenPos({origin.x,origin.y+available.y-dx::statusBarHeight()});auto before=ImGui::GetCursorScreenPos();dx::drawStatusBar(text,.5f,error?"Decoding | 32 queued":"Receiving",error,error);auto after=ImGui::GetCursorScreenPos();float scroll=ImGui::GetScrollMaxY();ImGui::End();ImGui::Render();return std::array<float,4>{before.y,after.y,height,scroll};
 };
 for(float width:{380.f,880.f})for(bool details:{false,true}){frame(width,false,details,"Ready");auto normal=frame(width,false,details,"Ready");auto longError=frame(width,true,details,std::string(2000,'X'));if(normal!=longError){for(auto x:normal)std::cerr<<x<<' ';std::cerr<<" vs ";for(auto x:longError)std::cerr<<x<<' ';std::cerr<<'\n';}CHECK(normal==longError);CHECK(normal[3]==0);}
 // The database tools are a child of History, above Integrations, not a
 // separate last item. Exercise the real settings navigation with clicks.
 int page=0;ImVec2 history,child;
 auto nav=[&]{ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({240,360});ImGui::Begin("Navigation",nullptr,ImGuiWindowFlags_NoTitleBar);auto origin=ImGui::GetCursorScreenPos();const float row=ImGui::GetTextLineHeightWithSpacing();history={origin.x+70,origin.y+row*5+row/2};child={origin.x+70,origin.y+row*6+row/2};page=dx::settingsNavigation(page);ImGui::End();ImGui::Render();};
 nav();nav();io.MousePos=child;io.MouseDown[0]=true;nav();io.MouseDown[0]=false;nav();CHECK(page==7);
 io.MousePos=history;io.MouseDown[0]=true;nav();io.MouseDown[0]=false;nav();CHECK(page==5);
 auto layout=[&](bool customize,bool reset){ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({600,360});ImGui::Begin("Layout reset",nullptr,ImGuiWindowFlags_NoTitleBar);float width=0;unsigned int pending=reset?1:0;dx::resetTableLayout("test_columns",pending,1);if(ImGui::BeginTable("test_columns",2,ImGuiTableFlags_Resizable|ImGuiTableFlags_Reorderable|ImGuiTableFlags_SizingFixedFit)){auto* table=GImGui->CurrentTable;ImGui::TableSetupColumn("Sender",ImGuiTableColumnFlags_WidthFixed,80);ImGui::TableSetupColumn("Country",ImGuiTableColumnFlags_WidthFixed,120);ImGui::TableHeadersRow();if(customize){table->Columns[0].WidthRequest=230;table->IsSettingsDirty=true;}width=table->Columns[0].WidthRequest;ImGui::EndTable();}ImGui::End();ImGui::Render();return width;};
 layout(false,false);CHECK(layout(true,false)==230);CHECK(layout(false,true)==80);
 // A tab never opened after restart has settings but no live table object.
 layout(true,false);const std::string saved=ImGui::SaveIniSettingsToMemory();ImGui::DestroyContext();ImGui::CreateContext();auto& restored=ImGui::GetIO();restored.IniFilename=nullptr;restored.LogFilename=nullptr;restored.DisplaySize={900,600};restored.DeltaTime=.05f;restored.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);ImGui::LoadIniSettingsFromMemory(saved.c_str(),saved.size());CHECK(layout(false,true)==80);
 ImGui::DestroyContext();std::cout<<"Two-second fades, fixed status geometry and nested History DB navigation\n";
}
