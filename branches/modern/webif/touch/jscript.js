function XML2jsobj(e){function s(e,t){i[e]?(i[e].constructor!=Array&&(i[e]=[i[e]]),i[e][i[e].length]=t):i[e]=t}var t='attributes',n,r,i={};if(e[t])for(n=0;r=e[t][n];n++)s(r.name,r.value);for(n=0;r=e.childNodes[n];n++)r.nodeType==3&&s('TEXT',r.nodeValue),r.nodeType==1&&s(r.nodeName,XML2jsobj(r));return i}function hms(e){e%=86400,time=[0,0,e];for(var t=2;t>0;t--)time[t-1]=Math.floor(time[t]/60),time[t]=time[t]%60,time[t]<10&&(time[t]='0'+time[t]);return time.join(':')}function fillLog(){var e='length',t=$.map($('#logText').text().split('\n'),$.trim).reverse(),n=$('#logList');n.empty();for(i=0;i<t[e];i++){var r=t[i].split(' ');if(r[e]==1)continue;var s,o=0;for(s=0;s<r[e];s++)if(r[s]){o++;if(o==5)break}var u=r.slice(0,s).join(' '),a=r.slice(s).join(' ');n.append("<li style='padding-bottom:0'><p>"+u+'</p>'+a+'</li>')}n.listview('refresh')}function appendTitle(e,t,n,r,i,s){var o="<li data-role='list-divider' data-theme='b' style='background:gray;"+r+"'>&nbsp;"+n+(i?"<span class='ui-li-count'>"+i+'</span>':'')+(t?"<a data-role='button' data-ajax='false' href='"+t+"'></a>":'')+'</li>';s?e.prepend(o):e.append(o),e.listview('refresh')}function appendRecord(e,t){var n='request',r='name',i='</p>',s='',o=t[n],u='<p><strong>',a=' </strong>';return t[r]&&(s+='<h3>'+t[r]+'</h3>'),t.type&&(s+=u+'Type:'+a+(t.type=='p'?'Proxy':'Local')+i),t.protocol&&(s+=u+'Protocol:'+a+t.protocol+i),t[n]&&(s+=u+'CAID/SRVID:'+a+t[n].caid+' / '+t[n].srvid+i+(t[n].TEXT?u+'Channel:'+a+t[n].TEXT+i:'')+u+'Online/Idle:'+a+hms(t.times.online)+' / '+hms(t.times.idle)+i),e&&(e.append("<li data-icon='"+(o?'stop':'play')+"'><a id='r_"+t[r]+"'>"+s+'</a></li>'),buttonUrlCallback('#r_'+t[r],'readers.html?label='+t[r]+'&action='+(o?'disable':'enable'),1)),s}function findClients(e,t,n){var r=e.oscam.status.client,i=[];for(j=0;j<r.length;j++)(t==null||r[j].name==t)&&n.indexOf(r[j].type)>=0&&(i[i.length]=r[j]);return i}function fillReaders(e){var t=null,n='length',r='getAttribute',s=$('#rdrList').empty();$.get(apiURL('readerlist'),function(o){var u=findClients(e,t,'rp');appendTitle(s,'readers.html','Online Readers','',u[n]);for(j=0;j<u[n];j++)appendRecord(s,u[j]);u=o.getElementsByTagName('reader');if(u[0]){appendTitle(s,t,'Disabled Readers','margin-top:10px',t);for(i=0;i<u[n];i++){if(u[i][r]('enabled')==1)continue;appendRecord(s,{name:u[i][r]('label'),type:u[i][r]('type'),protocol:u[i][r]('protocol')})}}s.listview('refresh')})}function fillUsers(e){var t=null,r='connection',s='substr',o=$('#usrList').empty();$.get(apiURL('userstats'),function(u){var a=0,f=[].concat(XML2jsobj(u).oscam.users.user);appendTitle(o,t,'Offline Users','margin-top:10px',t);for(i=0;i<f.length;i++){var l=f[i].name,c=findClients(e,l,'c')[0],h=f[i].status.indexOf('disabled')<0,p="<li data-icon='"+(h?'stop':'play')+"'><a id='u_"+l+"'><p><strong  style='font-size:16px'> "+l+'</strong>';if(c!=t){p+=' ('+c[r].ip+', port:  '+c[r].port+')</p>',c.name=t,c.type=t,p+=appendRecord(t,c)+"<table style='font-size:10px;font-weight:normal' cellspacing=0><tr>";var d='<tr>';for(var v in f[i].stats){n=v;if(v=='TEXT')continue;n=='cwlastresptime'&&(n='cwltime'),n=='cwignore'&&(n='ign'),n=='cwtimeout'&&(n='tout'),n=='timeonchannel'&&(n='tOnCh'),n=='expectsleep'&&(n='eSlp'),n[s](0,2)=='cw'&&(n=n[s](2)),n[s](0,3)=='emm'&&(n='e'+n[s](3));var m=f[i].stats[v].TEXT;p+="<th style='border:1px solid'>"+n,d+="<td style='border:1px solid'>"+(m=='undefined'?'':m)}p+=d+'</table></a></li>',a++,o.prepend(p)}else p+='</p></a></li>',o.append(p);$('#u_'+l).live('tap',{name:l,enabled:h},function(e){$.get('userconfig.html?user='+e.data.name+'&action='+(e.data.enabled?'disable':'enable'),reloadPage)})}appendTitle(o,'userconfig.html','Online Users','',a,!0),o.listview('refresh')})}function buttonUrlCallback(e,t,n){$(e).live('tap',function(e){$.mobile.showPageLoadingMsg(),$.get(t,typeof n=='undefined'?$.mobile.hidePageLoadingMsg:function(){window.setTimeout(reloadPage,n)})})}function reloadPage(){window.location.search='?touchOpen='+touchOpen}function goTab(e){$.each(['rdr','usr','log','sys'],function(e,t){$('#'+t+'Div').hide()}),$('#'+e+'Div').toggle(),$('#'+e+'Button').toggleClass('ui-btn-active'),touchOpen=e}function setDebugLevel(){$.mobile.showPageLoadingMsg(),$.get('status.html?debug='+dbgLevel(),$.mobile.hidePageLoadingMsg)}function dbgLevel(e){var t='#dbg',n='checked';if(typeof e=='undefined'){e=0;for(i=1;i<=maxDebugLevel;i*=2)$(t+i).attr(n)&&(e+=i)}else for(i=1;i<=maxDebugLevel;i*=2)$(t+i).attr(n,e&i?n:null).checkboxradio('refresh');return $('#dbgLevel').html(e),e}function fillSys(){var e='Stats',t='Clear',n='create';buttonUrlCallback('#restart','shutdown.html?action=Restart',5e3),buttonUrlCallback('#shutdown','shutdown.html?action=Shutdown',3e4);var r='',s=[['Load',e],['Save',e],[t,e],[t,'Timeouts'],[t,'Not','Founds']];$.each(s,function(e,t){if(e==0||e==3)r+="<div data-role='controlgroup' data-type='horizontal'>";r+="<a data-theme='e' id='lb"+e+"' data-role='button'>"+t[0]+'<br>'+t[1]+(t[2]?' '+t[2]:'')+'&nbsp;</a>';if(e==2||e==4)r+='</div>'}),$('#lbButs').append(r).trigger(n),$.each(s,function(e,t){buttonUrlCallback('#lb'+e,'config.html?part=loadbalancer&button='+t[0]+'%20'+t[1]+(t[2]?'%20'+t[2]:''))});if(actDebugLevel.length==0){$('#dbgBlk').hide();return}var o=0,u=['',''],a=0,f=['Detailed error','ATR/ECM/CW','Reader traffic','Clients traffic','Reader IFD','Reader I/O','EMM','DVBAPI','Load Balancer','CACHEEX','Client ECM'];for(i=1;i<=maxDebugLevel;i*=2)u[a]+="<input type='checkbox' id='dbg"+i+"'/><label for='dbg"+i+"'>"+f[o]+'</label>',o++,a=1-a;for(i=0;i<2;i++)u[i]="<div class='ui-block-"+['a','b'][i]+"' style='text-align:"+['right','left'][i]+"'><fieldset data-role='controlgroup' data-mini='true'>"+u[i]+'</fieldset></div>';$('#dbgButs').append("<fieldset class='ui-grid-a'>"+u[0]+u[1]+'</fieldset>').trigger(n);for(i=1;i<=maxDebugLevel;i*=2)$('#dbg'+i).bind('change',setDebugLevel);$('#dbgOff').live('tap',function(){dbgLevel(0),setDebugLevel()}),$('#dbgAll').live('tap',function(){dbgLevel(65535),setDebugLevel()}),dbgLevel(actDebugLevel)}function apiURL(e){return'oscamapi.html?part='+e}touchOpen='sys',maxDebugLevel=1024,$(document).ready(function(){var e="<div data-role='navbar' data-iconpos='bottom'><ul>";$.each([['rdr','card','Readers'],['usr','user','Users'],['log','info','Log'],['sys','gear','System']],function(t,n){e+="<li><a data-theme='b' id='"+n[0]+"Button' data-icon='"+n[1]+"' data-role='button' onclick='goTab(\""+n[0]+'")\'>'+n[2]+'</a>'}),$('#footer').append(e+'</ul></div>').trigger('create'),fillSys();var t=window.location.search;fillLog(),$.get(apiURL('status'),function(e){var t=XML2jsobj(e);fillReaders(t),fillUsers(t)});if(t){var n=t.indexOf('touchOpen');n>0&&(touchOpen=t.substr(n+10))}goTab(touchOpen)});
