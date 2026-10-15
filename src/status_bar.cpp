// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <core.h>
#include <ctime>
void DXNotebook::statusBar(const dx::Snapshot& s){
 const double now=ImGui::GetTime();
 if(s.noticeRevision!=heardNotice_){heardNotice_=s.noticeRevision;statusNotice_.observe("notice:"+std::to_string(s.noticeRevision),s.notice,now);}
 if(s.alertUtc!=statusAlertUtc_){statusAlertUtc_=s.alertUtc;if(time(nullptr)-s.alertUtc<120)statusNotice_.observe("alert:"+std::to_string(s.alertUtc),s.alert,now);}
 std::string error=s.error;if(!s.modeError.empty()){if(!error.empty())error+=" | ";error+=s.modeError;}if(!s.csvError.empty()){if(!error.empty())error+=" | ";error+="CSV log: "+s.csvError;}
 if(error!=statusError_){statusError_=error;statusErrorStarted_=now;}
 std::string activity=paused_?"Paused":!gui::mainWindow.isPlaying()?"Radio stopped":channels_.empty()?"No preset in RF capture":"Receiving";
 if(s.decoding||s.modeDecoding)activity="Decoding";
 const auto queued=s.queued+s.modeQueued+s.pendingWrites;if(queued)activity+=" | "+std::to_string(queued)+" queued";
 if(historyAll_!=s.historyAll||(statsHistory_||historyAll_)!=s.statsHistory)activity="Loading history...";
 const bool showing=!statusNotice_.text.empty()&&now-statusNotice_.started<8;float opacity=showing?statusNotice_.opacity(now):.65f;std::string message=showing?statusNotice_.text:"Ready";
 if(!error.empty()){message=error;opacity=dx::statusOpacity(std::min(2.,now-statusErrorStarted_));}
 if(dx::drawStatusBar(message,opacity,activity,!error.empty(),!s.error.empty()))engine_->retry();
}
