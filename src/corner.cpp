// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <imgui_internal.h>
#include <utils/opengl_include_code.h>
#include <cstdio>
#include <set>
namespace {
struct Part {std::string text;ImVec4 color;bool italic=false;};
const ImVec4 countryColor{.63f,.89f,.66f,1},callColor{.66f,.82f,1,1},distanceColor{1,.76f,.55f,1},neutral{.88f,.90f,.93f,1},gold{1,.80f,.32f,1};
std::vector<Part> rowParts(const dx::Decode& d,bool record,bool miles){char number[64];std::string distance=std::string("-- ")+dx::distanceUnit(miles);if(d.km){std::snprintf(number,sizeof(number),"%s%s",d.gridSource=="cached"?"* ":"",dx::distanceSummary(*d.km,miles).c_str());distance=number;}std::snprintf(number,sizeof(number),"%+d dB",d.snr);std::vector<Part> parts;if(record)parts.push_back({"Farthest",gold});parts.push_back({d.country.empty()?"Unknown":d.country,countryColor});parts.push_back({d.identity.transmitter.empty()?"--":d.identity.transmitter,callColor});parts.push_back({distance,distanceColor});parts.push_back({number,neutral});parts.push_back({"\""+d.message+"\"",neutral,true});return parts;}
unsigned int upload(const std::vector<unsigned char>& pixels){if(pixels.size()!=64*64*4)return 0;GLint old=0,row=0,alignment=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&old);glGetIntegerv(GL_UNPACK_ROW_LENGTH,&row);glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_ALIGNMENT,1);GLuint texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());glBindTexture(GL_TEXTURE_2D,GLuint(old));glPixelStorei(GL_UNPACK_ROW_LENGTH,row);glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);return texture;}
}
unsigned int DXNotebook::countryFlagIcon(const std::string& country){std::string code;auto found=flagCodes_.find(dx::countryOutlineKey(country));if(found!=flagCodes_.end())code=found->second;auto key="flag:"+code;auto icon=cornerIcons_.find(key);if(icon==cornerIcons_.end())icon=cornerIcons_.emplace(key,upload(dx::flagIconPixels(code))).first;return icon->second;}
unsigned int DXNotebook::trophyTexture(){auto icon=cornerIcons_.find("trophy");if(icon==cornerIcons_.end())icon=cornerIcons_.emplace("trophy",upload(dx::trophyIconPixels())).first;return icon->second;}
void DXNotebook::countryFlag(const std::string& country){float size=ImGui::GetTextLineHeight();auto texture=countryFlagIcon(country);if(texture)ImGui::Image(reinterpret_cast<ImTextureID>(uintptr_t(texture)),{size,size});else ImGui::Dummy({size,size});ImGui::SameLine(0,5);}
void DXNotebook::cornerNotifications(CornerBounds args){
 std::lock_guard<std::mutex> lock(uiMutex_);cornerVisible_=false;if(!enabled_||!cornerEnabled_||!settingsLoaded_)return;auto s=engine_->snapshot();std::vector<const dx::CornerReception*> receptions;auto now=dx::monotonicMs();for(auto it=s->cornerReceptions.rbegin();it!=s->cornerReceptions.rend();++it)if(now-it->observedMs<int64_t(cornerSeconds_)*1000)receptions.push_back(&*it);if(receptions.empty()&&!s->stats.farthestReception){cornerChanges_.clear();return;}
 float availableWidth=args.wfMax.x-args.wfMin.x-20,availableHeight=args.wfMax.y-args.wfMin.y-20;if(availableWidth<180||availableHeight<65)return;
 auto* font=ImGui::GetFont();float fontSize=std::clamp(ImGui::GetFontSize()*.82f,11.f,16.f),rowHeight=fontSize+6,iconSize=fontSize+5,gap=9;
 auto width=[&](const std::vector<Part>& parts){float size=(parts.size()==6?2:1)*(iconSize+gap);for(const auto& part:parts)size+=font->CalcTextSizeA(fontSize,FLT_MAX,0,part.text.c_str()).x+gap+(part.italic?fontSize*.2f:0);return size;};
 float countryWidth=0,recordWidth=0;for(const auto* reception:receptions)countryWidth=std::max(countryWidth,width(rowParts(reception->decode,false,distanceMiles_)));if(s->stats.farthestReception)recordWidth=width(rowParts(*s->stats.farthestReception,true,distanceMiles_));
 float panelWidth=std::min({std::max(countryWidth,recordWidth)+18,availableWidth,900.f});float bar=ImGui::GetStyle().ScrollbarSize;float footerHeight=s->stats.farthestReception?rowHeight+(recordWidth>panelWidth-16?bar:0)+4:0;
 float countryHeight=float(receptions.size())*rowHeight+(countryWidth>panelWidth-16?bar:0);float height=std::min(availableHeight,countryHeight+footerHeight+12);
 auto cursor=ImGui::GetCursorScreenPos();
#if DX_HOST_BROWN
 ImGui::SetCursorScreenPos({args.wfMax.x-10-panelWidth,args.wfMax.y-10-height});
#else
 ImGui::SetNextWindowPos({args.wfMax.x-10-panelWidth,args.wfMax.y-10-height});ImGui::SetNextWindowSize({panelWidth,height});
#endif
 ImGui::SetNextWindowBgAlpha(.76f);
 ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{8,6});ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{0,0});ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize,{1,1});ImGui::PushStyleColor(ImGuiCol_ChildBg,{.045f,.06f,.07f,1});
 const auto flags=ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_AlwaysUseWindowPadding;
#if DX_HOST_BROWN
 bool visible=ImGui::BeginChild("Corner notifications on waterfall###dx_corner",{panelWidth,height},false,flags);
#else
 bool visible=ImGui::Begin("Corner notifications on waterfall###dx_corner",nullptr,flags|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
#endif
 if(visible){
  cornerVisible_=true;cornerMin_=ImGui::GetWindowPos();auto size=ImGui::GetWindowSize();cornerMax_={cornerMin_.x+size.x,cornerMin_.y+size.y};std::set<std::string> seen;
  auto row=[&](const dx::Decode& d,bool record,int64_t id,int64_t observedMs){auto parts=rowParts(d,record,distanceMiles_);auto at=ImGui::GetCursorScreenPos();auto* draw=ImGui::GetWindowDrawList();std::string key=record?"record":std::to_string(id);seen.insert(key);auto& change=cornerChanges_[key];auto reception=d.key();if(change.first!=reception)change={reception,record?now:observedMs};float pulse=dx::cornerFlashOpacity((dx::monotonicMs()-change.second)/1000.);
   if(pulse>0)draw->AddRectFilled(at,{at.x+std::max(width(parts),ImGui::GetContentRegionAvail().x),at.y+rowHeight},ImGui::ColorConvertFloat4ToU32(ImVec4{.45f,.82f,.55f,pulse*.20f}),2);
   std::string code;auto found=flagCodes_.find(dx::countryOutlineKey(d.country));if(found!=flagCodes_.end())code=found->second;float x=at.x,y=at.y+(rowHeight-fontSize)/2;auto icon=[&](const std::string& key,bool trophy){auto found=cornerIcons_.find(key);if(found==cornerIcons_.end())found=cornerIcons_.emplace(key,upload(trophy?dx::trophyIconPixels():dx::flagIconPixels(code))).first;if(found->second)draw->AddImage(reinterpret_cast<ImTextureID>(uintptr_t(found->second)),{x,at.y+(rowHeight-iconSize)/2},{x+iconSize,at.y+(rowHeight+iconSize)/2});x+=iconSize+gap;};if(record)icon("trophy",true);icon("flag:"+code,false);
   for(const auto& part:parts){int start=draw->VtxBuffer.Size;draw->AddText(font,fontSize,{x,y},ImGui::ColorConvertFloat4ToU32(part.color),part.text.c_str());if(part.italic)for(int i=start;i<draw->VtxBuffer.Size;i++)draw->VtxBuffer[i].pos.x+=.18f*(y+fontSize-draw->VtxBuffer[i].pos.y);x+=font->CalcTextSizeA(fontSize,FLT_MAX,0,part.text.c_str()).x+gap+(part.italic?fontSize*.2f:0);}
   ImGui::Dummy({width(parts),rowHeight});if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::TextUnformatted(d.country.c_str());ImGui::Text("%s | %s | %+d dB SNR",d.identity.transmitter.c_str(),d.mode.c_str(),d.snr);ImGui::PushTextWrapPos(600);ImGui::TextUnformatted(d.message.c_str());ImGui::PopTextWrapPos();ImGui::EndTooltip();}
  };
  if(!receptions.empty()){ImGui::BeginChild("recent_receptions",{0,std::max(1.f,height-footerHeight-12)},false,ImGuiWindowFlags_HorizontalScrollbar);ImGuiListClipper clipper;clipper.Begin(int(receptions.size()),rowHeight);while(clipper.Step())for(int i=clipper.DisplayStart;i<clipper.DisplayEnd;i++){const auto* reception=receptions[size_t(i)];row(reception->decode,false,reception->id,reception->observedMs);}ImGui::EndChild();}
  if(s->stats.farthestReception){ImGui::BeginChild("farthest",{0,footerHeight},false,ImGuiWindowFlags_HorizontalScrollbar);row(*s->stats.farthestReception,true,0,0);ImGui::EndChild();}
  for(auto it=cornerChanges_.begin();it!=cornerChanges_.end();)if(!seen.count(it->first))it=cornerChanges_.erase(it);else ++it;
 }
#if DX_HOST_BROWN
 ImGui::EndChild();ImGui::SetCursorScreenPos(cursor);
#else
 ImGui::End();
#endif
 ImGui::PopStyleColor();ImGui::PopStyleVar(3);
}
