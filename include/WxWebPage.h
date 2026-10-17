#pragma once
#include <pgmspace.h>

// ESPressWeather32-Dashboard (Stil wie ESPressMiner32/ESPressDuino). Holt alle 5 s /api/stats.
static const char WX_WEB_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESPressWeather32</title>
<script>try{if((localStorage.getItem('cmTheme')||'dark')==='light')document.documentElement.classList.add('light')}catch(e){}</script>
<style>
:root{--bg:#161821;--card:#222530;--line:#333644;--txt:#e8eaf0;--dim:#9aa0b0;
--accent:#2fa8ff;--accent2:#168ee6;--green:#3fb950;--red:#e5534b;--orange:#f0a020;
--modal:#232733;--field:#191c26;--ibd:#3a3f50;--shadow:rgba(0,0,0,.5)}
:root.light{--bg:#eef1f6;--card:#ffffff;--line:#e2e5ee;--txt:#1a1d26;--dim:#5c6472;
--modal:#ffffff;--field:#f3f5fa;--ibd:#cdd3e0;--shadow:rgba(30,40,80,.15)}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--txt);
font:15px/1.45 system-ui,Segoe UI,Roboto,sans-serif;padding:18px;transition:background .2s,color .2s}
.top{max-width:1200px;margin:0 auto 20px;display:flex;align-items:center;justify-content:center;position:relative}
h1{font-weight:600;text-align:center;margin:6px 0;font-size:26px;letter-spacing:.02em}
.gear{position:absolute;right:0;top:50%;transform:translateY(-50%);background:var(--card);color:var(--txt);
border:1px solid var(--line);border-radius:9px;padding:8px 13px;font-size:14px;cursor:pointer;transition:.15s}
.gear:hover{border-color:var(--accent);color:var(--accent)}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;max-width:1200px;margin:0 auto}
.card{background:var(--card);border-radius:12px;padding:18px;text-align:center;border-top:4px solid var(--accent);
box-shadow:0 1px 3px var(--shadow)}
.lbl{color:var(--dim);letter-spacing:.09em;font-size:11.5px;text-transform:uppercase}
.val{font-size:30px;font-weight:700;margin:7px 0 3px;font-variant-numeric:tabular-nums;overflow:hidden;white-space:nowrap}
.sub{color:var(--dim);font-size:12px}
.wide{max-width:1200px;margin:16px auto 0;background:var(--card);border-radius:12px;padding:16px;box-shadow:0 1px 3px var(--shadow)}
table{width:100%;border-collapse:collapse}
td,th{padding:8px 4px;border-bottom:1px solid var(--line)}
tr:last-child td{border-bottom:0}
#info td:first-child{color:var(--dim)}
#info td:last-child{text-align:right;font-variant-numeric:tabular-nums;word-break:break-word}
.dot{display:inline-block;width:9px;height:9px;border-radius:50%;margin-right:7px;background:var(--red)}
.dot.on{background:var(--green)}
.days{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;margin-top:10px}
.day{border:1px solid var(--line);border-radius:10px;padding:12px;text-align:center}
.day .ic{font-size:34px;line-height:1.2}.day b{display:block;font-size:17px;font-variant-numeric:tabular-nums}
.day span{color:var(--dim);font-size:12px}
.mini{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:10px;margin:12px 0 6px}
.mini div{border:1px solid var(--line);border-radius:10px;padding:10px;text-align:center}
.mini b{display:block;font-size:19px;font-variant-numeric:tabular-nums}.mini span{color:var(--dim);font-size:11px;text-transform:uppercase;letter-spacing:.08em}
.swbtn{background:var(--card);color:var(--accent);border:1px solid var(--line);border-radius:7px;
padding:3px 9px;font-size:12px;font-weight:600;cursor:pointer;margin-left:8px;transition:.12s}
.swbtn:hover{border-color:var(--accent)}.swbtn:disabled{opacity:.6;cursor:default}
.ov{position:fixed;inset:0;background:rgba(10,12,20,.55);backdrop-filter:blur(2px);display:none;
align-items:flex-start;justify-content:center;overflow:auto;padding:26px 16px;z-index:9}
.ov.open{display:flex}
.modal{background:var(--modal);border-radius:16px;width:100%;max-width:520px;padding:0;overflow:hidden;
box-shadow:0 20px 60px var(--shadow);animation:pop .18s ease-out}
@keyframes pop{from{transform:translateY(10px);opacity:0}to{transform:none;opacity:1}}
.mhead{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:18px 22px;border-bottom:1px solid var(--line)}
.mhead h2{margin:0;font-weight:600;font-size:19px}
.mbody{padding:18px 22px;max-height:70vh;overflow:auto}
.seg{display:inline-flex;flex-wrap:wrap;background:var(--field);border:1px solid var(--ibd);border-radius:9px;padding:3px}
.seg button{border:0;background:none;color:var(--dim);border-radius:7px;padding:6px 12px;font-size:13px;cursor:pointer;font-weight:600}
.seg button.act{background:var(--accent);color:#fff}
.intro{color:var(--dim);font-size:13.5px;margin:0 0 14px;line-height:1.5}
.sec{border:1px solid var(--line);border-radius:12px;padding:6px 14px 14px;margin:0 0 16px}
.sec-h{color:var(--accent);font-weight:700;font-size:13px;letter-spacing:.04em;text-transform:uppercase;padding:12px 2px 4px}
.fld{margin:10px 0}
.fld label{display:block;color:var(--dim);font-size:12.5px;margin:0 0 5px;font-weight:500}
.fld input,.fld select{width:100%;background:var(--field);color:var(--txt);border:1px solid var(--ibd);border-radius:8px;
padding:10px 12px;font-size:14px;transition:.12s}
.fld input:focus,.fld select:focus{outline:none;border-color:var(--accent);box-shadow:0 0 0 3px rgba(47,168,255,.2)}
.two{display:flex;gap:10px}.two .fld{flex:1}
.scanrow{display:flex;gap:8px}.scanrow input{flex:1}
.scanbtn{background:var(--field);color:var(--accent);border:1px solid var(--ibd);border-radius:8px;
padding:0 14px;cursor:pointer;font-size:13px;font-weight:600;white-space:nowrap;transition:.12s}
.scanbtn:hover{border-color:var(--accent)}.scanbtn:disabled{opacity:.6;cursor:default}
.ssidsel{width:100%;margin-top:8px;background:var(--field);color:var(--txt);border:1px solid var(--ibd);
border-radius:8px;padding:9px 10px;font-size:14px}
.mfoot{display:flex;gap:10px;justify-content:flex-end;align-items:center;padding:16px 22px;border-top:1px solid var(--line)}
.btn{border:0;border-radius:9px;padding:10px 18px;font-size:14px;font-weight:600;cursor:pointer;transition:.12s}
.btn.save{background:var(--accent);color:#fff}.btn.save:hover{background:var(--accent2)}
.btn.save:disabled{opacity:.6;cursor:default}
.btn.cancel{background:var(--field);color:var(--txt);border:1px solid var(--ibd)}
.msg{flex:1;font-size:13.5px;min-height:1.2em}.msg.err{color:var(--red)}.msg.ok{color:var(--green)}
.rng{display:flex;align-items:center;gap:10px}.rng input{flex:1;accent-color:var(--accent)}
.rng b{min-width:44px;text-align:right;font-variant-numeric:tabular-nums}
.fld label.tog{display:flex;align-items:center;justify-content:space-between;color:var(--txt);font-size:14px;cursor:pointer;margin:0}
.fld .tog input{position:absolute;opacity:0;width:0;height:0}
.sw2{position:relative;width:44px;height:24px;border-radius:12px;background:var(--ibd);transition:.15s;flex:none}
.sw2::after{content:"";position:absolute;top:3px;left:3px;width:18px;height:18px;border-radius:50%;background:#fff;transition:.15s}
.tog input:checked+.sw2{background:var(--accent)}.tog input:checked+.sw2::after{left:23px}
.dstat{color:var(--dim);font-size:12.5px;margin-top:8px}
.hint{color:var(--dim);font-size:12.5px;margin:6px 0 0}
.hint code{color:var(--accent)}
.foot{max-width:1200px;margin:22px auto 4px;text-align:center;color:var(--dim);font-size:13px}.foot a{color:var(--dim)}.foot a:hover{color:var(--accent)}
.xbtn{background:none;border:0;color:var(--dim);font-size:26px;line-height:1;cursor:pointer;padding:0 2px}.xbtn:hover{color:var(--accent)}
</style></head><body>
<div class="top">
 <h1 id="mn">ESPressWeather32</h1>
 <button class="gear" onclick="openCfg()">&#9881; Settings</button>
</div>
<div class="grid">
 <div class="card"><div class="lbl">Now</div><div class="val"><span id="wic"></span> <span id="tmp">-</span></div><div class="sub" id="desc">-</div></div>
 <div class="card"><div class="lbl">Humidity</div><div class="val" id="hum">-</div><div class="sub">relative</div></div>
 <div class="card"><div class="lbl">Pressure</div><div class="val" id="prs">-</div><div class="sub" id="prsub">hPa</div></div>
 <div class="card" id="pwc"><div class="lbl">Power</div><div class="val" id="pw">-</div><div class="sub" id="pwsub">-</div></div>
</div>
<div class="wide"><div class="lbl">3-day forecast</div><div class="days" id="days"></div></div>
<div class="wide" id="enw">
 <div class="lbl" style="display:flex;justify-content:space-between;gap:10px">Energy (Tasmota via MQTT) <span id="ename" style="text-transform:none;letter-spacing:0"></span></div>
 <div class="mini"><div><b id="e_pw">-</b><span>Power</span></div><div><b id="e_td">-</b><span>Today</span></div><div><b id="e_yd">-</b><span>Yesterday</span></div><div><b id="e_tt">-</b><span>Total</span></div><div><b id="e_ui">-</b><span>Voltage / current</span></div></div>
 <div class="hint" id="ehint"></div>
</div>
<div class="wide"><table id="info">
 <tr><td>Location</td><td id="loc">-</td></tr>
 <tr><td>Weather data</td><td><span class="dot" id="wd"></span><span id="wage">-</span> <button class="swbtn" id="rfb" onclick="refreshWx()">&#8635; Refresh now</button></td></tr>
 <tr><td>Display</td><td id="dview">-</td></tr>
 <tr><td>Wi-Fi</td><td id="wifi">-</td></tr>
 <tr><td>Uptime</td><td id="up">-</td></tr>
 <tr><td>Free memory</td><td id="heap">-</td></tr>
 <tr><td>Firmware</td><td id="ver">-</td></tr>
 <tr><td>Project</td><td><a href="https://github.com/brenner23/ESPressWeather32" target="_blank" rel="noopener" style="color:inherit">GitHub</a></td></tr>
</table></div>

<div class="foot"><a href="https://github.com/brenner23/ESPressWeather32" target="_blank" rel="noopener">&#9729; ESPressWeather32 on GitHub</a> &middot; Weather data: <a href="https://openweathermap.org" target="_blank" rel="noopener">OpenWeather</a></div>

<div class="ov" id="ov" onclick="if(event.target===this)closeCfg()"><div class="modal">
 <div class="mhead">
  <h2 id="ctitle">Settings</h2>
  <div style="display:flex;align-items:center;gap:10px"><div class="seg"><button type="button" id="thDark" onclick="setTheme('dark')">Dark</button><button type="button" id="thLight" onclick="setTheme('light')">Light</button></div><button type="button" class="xbtn" id="cx" onclick="closeCfg()" title="Close">&times;</button></div>
 </div>
 <div class="mbody">
  <p class="intro" id="cintro" style="display:none"></p>
  <div class="sec">
   <div class="sec-h">General</div>
   <div class="fld"><label for="c_name">Device name</label><input id="c_name" maxlength="24" placeholder="ESPressWeather32"></div>
   <div class="fld"><label for="c_host">Network name (hostname)</label><input id="c_host" maxlength="31"></div>
   <div class="fld"><label for="c_ssid">Wi-Fi name (SSID)</label>
    <div class="scanrow"><input id="c_ssid" maxlength="32"><button type="button" class="scanbtn" id="scanbtn" onclick="scanWifi()">&#128246; Scan</button></div>
    <select id="ssidsel" class="ssidsel" style="display:none" onchange="if(this.value){$('c_ssid').value=this.value;cfgBtn()}"></select>
   </div>
   <div class="fld"><label for="c_wpass">Wi-Fi password</label><input id="c_wpass" type="password" maxlength="64"></div>
  </div>
  <div class="sec">
   <div class="sec-h">Weather (OpenWeather)</div>
   <div class="two"><div class="fld"><label for="c_owkey">API key (free at openweathermap.org)</label><input id="c_owkey" type="password" maxlength="47" autocapitalize="off"></div>
    <div class="fld" style="flex:0 0 96px"><label for="d_wxm" title="Fetch weather every N minutes (2-60), applies immediately. Forecast every 2 h.">Every (min)</label><input id="d_wxm" type="number" min="2" max="60" value="5" title="2-60 minutes, applies immediately" onchange="wxmSave()"></div></div>
   <div class="two"><div class="fld"><label for="c_city">City</label><input id="c_city" maxlength="39"></div>
    <div class="fld" style="flex:0 0 90px"><label for="c_ctry">Country</label><input id="c_ctry" maxlength="7" placeholder="DE"></div></div>
   <div class="two"><div class="fld"><label for="c_lang">Language (display)</label><select id="c_lang"><option value="de">Deutsch</option><option value="en">English</option></select></div>
    <div class="fld"><label for="c_units">Units</label><select id="c_units"><option value="metric">&deg;C (metric)</option><option value="imperial">&deg;F (imperial)</option></select></div></div>
   <div class="fld"><label for="c_tz">Time zone (POSIX)</label><input id="c_tz" maxlength="63" placeholder="CET-1CEST,M3.5.0,M10.5.0/3"></div>
  </div>
  <div class="sec">
   <div class="sec-h">Energy (Tasmota via MQTT)</div>
   <div class="fld"><label class="tog">Energy view on (this display is the MQTT broker)<input type="checkbox" id="c_mqon"><span class="sw2"></span></label></div>
   <p class="hint" id="mqhint"></p>
   <div class="two"><div class="fld" style="flex:0 0 110px"><label for="c_mqport">Port</label><input id="c_mqport" type="number" min="1" max="65535"></div>
    <div class="fld"><label for="c_mqname">Name on the display (empty = from Tasmota)</label><input id="c_mqname" maxlength="23"></div></div>
   <div class="two"><div class="fld"><label for="c_mquser">User (empty = no login)</label><input id="c_mquser" maxlength="31" autocapitalize="off"></div>
    <div class="fld"><label for="c_mqpass">Password</label><input id="c_mqpass" type="password" maxlength="32"></div></div>
  </div>
  <div class="sec" id="ssec" style="display:none">
   <div class="sec-h">Display preview <span style="color:var(--dim);font-weight:500;text-transform:none">· debug, rendered on the device</span></div>
   <div style="text-align:center;margin:8px 0"><img id="shot" alt="display" style="max-width:100%;image-rendering:pixelated;border:1px solid var(--line);border-radius:6px;background:#000"></div>
   <div style="display:flex;gap:10px;align-items:center;justify-content:space-between"><button type="button" class="scanbtn" style="padding:8px 14px" onclick="shot()">&#8635; Reload</button>
    <label class="tog" style="display:flex;gap:10px;align-items:center;color:var(--txt);font-size:13.5px;cursor:pointer">Auto every 5 s<input type="checkbox" id="shotauto" style="position:absolute;opacity:0;width:0;height:0" onchange="shotAuto()"><span class="sw2"></span></label></div>
  </div>  <div class="sec" id="dsec" style="display:none">
   <div class="sec-h">Display <span id="dboard" style="color:var(--dim);font-weight:500;text-transform:none"></span></div>
   <p class="dstat" style="margin:2px 0 6px">Changes here apply immediately (no restart).</p>
   <div class="fld"><label>View</label><div class="seg" id="d_view"></div></div>
   <div class="fld" id="d_cycf"><label for="d_cyc">Change view every N seconds (auto)</label><input id="d_cyc" type="number" min="5" max="3600" onchange="dSave({cycleSec:parseInt(this.value)||15})"></div>
   <div class="fld"><label>Sky preview (60 s; normally automatic by sunrise / sunset)</label><div class="seg" id="d_prev"></div></div>
   <div class="fld" id="d_rotf" style="display:none"><label>Rotation</label><div class="seg" id="d_rot"></div></div>
   <div class="fld"><label for="d_day">Brightness</label>
    <div class="rng"><input id="d_day" type="range" min="1" max="100"><b id="d_dayv"></b></div></div>
   <div class="fld" id="d_ldrf" style="display:none"><label class="tog">Auto light (light sensor)<input type="checkbox" id="d_ldr" onchange="dSave({autoLight:this.checked})"><span class="sw2"></span></label></div>
   <div class="dstat" id="dstat"></div>
  </div>
 </div>
 <div class="mfoot" id="cfoot">
  <span class="msg" id="cmsg"></span>
  <button class="btn cancel" id="ccancel" onclick="openCfg()">Undo</button>
  <button class="btn save" id="csave" onclick="saveCfg()">Save &amp; restart</button>
 </div>
</div></div>

<script>
const $=id=>document.getElementById(id);
let portal=false,owKeySet=false,mqPassSet=false,cfgSnap='',fromSecrets=false,mqIp='';
const CFG_IDS=['c_name','c_host','c_ssid','c_wpass','c_owkey','c_city','c_ctry','c_lang','c_units','c_tz','c_mqon','c_mqport','c_mqname','c_mquser','c_mqpass'];
const VIEWS=['Weather 2','3 days','Energy','Weather 3 (moon)','Auto','Weather 1'];const VORDER=[5,0,3,1,2,4];const PREV=['Live','Day','Dusk','Night'];
function fv(i){const e=$(i);return e.type==='checkbox'?e.checked:e.value}
function cfgNow(){return JSON.stringify(CFG_IDS.map(fv))}
function cfgBtn(){const ch=portal||cfgNow()!==cfgSnap;$('csave').textContent=ch?'Save & restart':'Restart';$('ccancel').style.display=ch&&!portal?'':'none';$('csave').disabled=fromSecrets&&ch}
CFG_IDS.forEach(i=>{$(i).addEventListener('input',cfgBtn);$(i).addEventListener('change',cfgBtn)});
function setTheme(t){document.documentElement.classList.toggle('light',t==='light');
 try{localStorage.setItem('cmTheme',t)}catch(e){}
 $('thDark').className=t==='light'?'':'act';$('thLight').className=t==='light'?'act':''}
(function(){let t='dark';try{t=localStorage.getItem('cmTheme')||'dark'}catch(e){}setTheme(t)})();
function mqHint(){const p=$('c_mqport').value||1883;$('mqhint').innerHTML='In Tasmota set <b>MQTT &rarr; Host</b> = <code>'+esc(mqIp||'IP of this display')+'</code>, Port <code>'+esc(p)+'</code>, and the same user/password.'}
$('c_mqport').addEventListener('input',mqHint);
async function openCfg(){const m=$('cmsg');m.textContent='';m.className='msg';
 $('cx').style.display=portal?'none':'';
 $('ctitle').textContent=portal?'Set up ESPressWeather32':'Settings';
 try{const c=await(await fetch('/api/config',{cache:'no-store'})).json();
 $('c_name').value=c.deviceName||'';$('c_host').value=c.hostname||'';$('c_ssid').value=c.wifiSsid||'';$('c_wpass').value='';
 $('c_wpass').placeholder=c.wifiPassSet?'•••••• (unchanged)':'';
 $('c_owkey').value='';owKeySet=!!c.owKeySet;$('c_owkey').placeholder=c.owKeySet?'•••••• (unchanged)':'';
 $('c_city').value=c.owCity||'';$('c_ctry').value=c.owCountry||'';$('c_lang').value=(c.owLang||'de').startsWith('de')?'de':'en';
 $('c_units').value=c.owUnits==='imperial'?'imperial':'metric';$('c_tz').value=c.tz||'';
 $('c_mqon').checked=!!c.mqttOn;$('c_mqport').value=c.mqttPort||1883;$('c_mqname').value=c.mqttName||'';$('c_mquser').value=c.mqttUser||'';
 $('c_mqpass').value='';mqPassSet=!!c.mqttPassSet;$('c_mqpass').placeholder=c.mqttPassSet?'•••••• (unchanged)':'';
 fromSecrets=!!c.fromSecrets;const ci=$('cintro');
 if(fromSecrets){ci.style.display='';ci.textContent='These settings come from secrets.ini (use_secrets = 1) and can only be changed there. Display settings below work as usual.'}
 else if(portal){ci.style.display='';ci.textContent='Welcome! Enter your Wi-Fi and your OpenWeather API key and city. The display then restarts and connects to your Wi-Fi.'}
 else ci.style.display='none';
 mqHint();cfgSnap=cfgNow();cfgBtn();dLoad();shot();$('ov').classList.add('open')}catch(e){alert('Could not load settings')}}
function closeCfg(){if(!portal)$('ov').classList.remove('open')}
let dPrevBusy=false,dPrevNext=null,dPrevT=0;
function dPrev(v){if(dPrevBusy){dPrevNext=v;return}dPrevBusy=true;
 fetch('/api/display/preview?value='+v).catch(()=>{}).finally(()=>{dPrevBusy=false;if(dPrevNext!==null){const n=dPrevNext;dPrevNext=null;dPrev(n)}})}
async function dSave(o){const st=$('dstat');
 try{const r=await fetch('/api/display',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(o)});
 const t=await r.text();if(!r.ok){st.textContent=t;st.style.color='var(--red)';return}st.style.color='';dLoad();setTimeout(shot,700)}catch(e){st.textContent='Saving failed'}}
{const el=$('d_day'),v=$('d_dayv');el.oninput=()=>{v.textContent=el.value+' %';clearTimeout(dPrevT);dPrevT=setTimeout(()=>dPrev(el.value),120)};
 el.onchange=()=>dSave({brightness:parseInt(el.value)})}
async function wxmSave(){const e=$('d_wxm');let v=Math.min(60,Math.max(2,parseInt(e.value)||5));e.value=v;await dSave({wxMin:v})}
let shotT=0;function shot(){const i=$('shot');i.onload=()=>{$('ssec').style.display=''};i.onerror=()=>{$('ssec').style.display='none'};i.src='/api/screen?t='+Date.now()}
function shotAuto(){clearInterval(shotT);if($('shotauto').checked)shotT=setInterval(()=>{if($('ov').classList.contains('open'))shot()},5000)}
async function dLoad(){let d;try{const r=await fetch('/api/display',{cache:'no-store'});if(!r.ok)return;d=await r.json()}catch(e){return}
 $('dsec').style.display='';$('dboard').textContent='· '+d.board;$('dstat').textContent=d.status||'';
 $('d_day').value=d.brightness;$('d_dayv').textContent=d.brightness+' %';
 $('d_ldrf').style.display=d.hasLdr?'':'none';$('d_ldr').checked=!!d.autoLight;
 $('d_rotf').style.display=d.hasRotation?'':'none';
 $('d_rot').innerHTML=[0,90,180,270].map(r=>'<button type="button" class="'+(r===d.rotation?'act':'')+'" onclick="dSave({rotation:'+r+'})">'+r+'&deg;</button>').join('');
 $('d_view').innerHTML=VORDER.map(i=>'<button type="button" class="'+(i===d.view?'act':'')+'" onclick="dSave({view:'+i+'})">'+VIEWS[i]+'</button>').join('');
$('d_prev').innerHTML=PREV.map((n,i)=>'<button type="button" class="'+(i===d.skyPreview?'act':'')+'" onclick="dSave({skyPreview:'+i+'})">'+n+'</button>').join('');
 $('d_cycf').style.display=d.view===4?'':'none';if(document.activeElement!==$('d_cyc'))$('d_cyc').value=d.cycleSec;if(document.activeElement!==$('d_wxm'))$('d_wxm').value=d.wxMin}
async function saveCfg(){const m=$('cmsg'),b=$('csave');
 if(!portal&&cfgNow()===cfgSnap){b.disabled=true;m.className='msg';m.textContent='Restarting (about 15 s) ...';
  try{await fetch('/restart',{method:'POST'})}catch(e){}waitBack();return}  // nichts geaendert: nur Neustart
 const k=$('c_owkey').value,mp=$('c_mqpass').value;
 const body={deviceName:$('c_name').value,hostname:$('c_host').value,wifiSsid:$('c_ssid').value,wifiPass:$('c_wpass').value,
 owKey:k,owKeyKeep:!k&&owKeySet,owCity:$('c_city').value,owCountry:$('c_ctry').value,owLang:$('c_lang').value,owUnits:$('c_units').value,tz:$('c_tz').value,
 mqttOn:$('c_mqon').checked,mqttPort:parseInt($('c_mqport').value)||1883,mqttName:$('c_mqname').value,mqttUser:$('c_mquser').value,mqttPass:mp,mqttPassKeep:!mp&&mqPassSet};
 b.disabled=true;m.className='msg';m.textContent='Saving ...';
 try{const r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
 const t=await r.text();if(!r.ok){m.className='msg err';m.textContent=t;b.disabled=false;return}
 m.className='msg ok';
 if(portal){m.textContent='Saved! Restarting and connecting to "'+body.wifiSsid+'". Then open: http://'+body.hostname+'.local/';return}
 m.textContent='Saved. Display is restarting (about 15 s) ...';waitBack()}
 catch(e){m.className='msg err';m.textContent='Saving failed';b.disabled=false}}
function waitBack(){setTimeout(async function p(){try{await fetch('/api/stats',{cache:'no-store'});location.reload()}catch(e){setTimeout(p,2000)}},8000)}
async function refreshWx(){const b=$('rfb');b.disabled=true;try{const r=await fetch('/api/refresh',{method:'POST'});if(!r.ok)alert(await r.text())}catch(e){}setTimeout(()=>{b.disabled=false;tick()},4000)}
async function scanWifi(){const b=$('scanbtn'),sel=$('ssidsel');b.disabled=true;const old=b.innerHTML;b.textContent='Scanning ...';
 try{const j=await(await fetch('/api/scan',{cache:'no-store'})).json();const ns=j.nets||[];ns.sort((a,c)=>c.rssi-a.rssi);
 sel.innerHTML='<option value="">'+(ns.length?('-- '+ns.length+' networks found --'):'-- no networks found --')+'</option>';
 ns.forEach(n=>{const o=document.createElement('option');o.value=n.ssid;
  const q=n.rssi>-60?'++++':n.rssi>-70?'+++':n.rssi>-80?'++':'+';o.textContent=n.ssid+'   ['+q+']'+(n.lock?' 🔒':'');sel.appendChild(o)});
 sel.style.display=''}catch(e){alert('Wi-Fi scan failed')}
 b.disabled=false;b.innerHTML=old}
function fu(s){const d=Math.floor(s/86400);s%=86400;const p=n=>String(n).padStart(2,'0');
 return(d?d+'d ':'')+p(Math.floor(s/3600))+':'+p(Math.floor(s/60)%60)+':'+p(s%60)}
function ago(s){if(s<0)return'-';if(s<90)return s+' s ago';if(s<5400)return Math.round(s/60)+' min ago';return(s/3600).toFixed(1)+' h ago'}
function esc(t){return String(t).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
function emo(id,n){if(id>=200&&id<300)return'⛈️';if(id>=300&&id<600)return'🌧️';if(id>=600&&id<700)return'❄️';if(id>=700&&id<800)return'🌫️';
 if(id===800)return n?'🌙':'☀️';if(id===801||id===802)return n?'☁️':'⛅';return'☁️'}
let BUILD='';
async function tick(){try{const s=await(await fetch('/api/stats',{cache:'no-store'})).json();
 if(s.build){if(BUILD&&s.build!==BUILD){location.reload();return}BUILD=s.build}
 if(s.name){$('mn').textContent=s.name;document.title=s.name}
 const u=s.units==='imperial'?'°F':'°C',w=s.weather||{};
 if(w.valid){$('wic').textContent=emo(w.cond,w.night);$('tmp').textContent=w.temp.toFixed(1)+' '+u;
  $('desc').textContent=(w.desc||'')+' · feels like '+w.feels.toFixed(0)+' '+u;$('hum').textContent=w.hum+' %';$('prs').textContent=w.press;
  $('prsub').textContent='hPa · '+(w.press*0.750062).toFixed(0)+' mmHg'}
 else{$('wic').textContent='';$('tmp').textContent='-';$('desc').textContent=s.wxError||'waiting for data';$('hum').textContent='-';$('prs').textContent='-'}
 const f=s.forecast||[];$('days').innerHTML=f.length?f.map(d=>'<div class="day"><span>'+esc(d.day)+'</span><div class="ic">'+emo(d.cond,false)+'</div><b>'+d.max.toFixed(0)+' / '+d.min.toFixed(0)+' '+u+'</b><span>rain '+d.pop+' %</span></div>').join(''):'<div class="sub">no forecast yet</div>';
 const e=s.energy||{};$('enw').style.display=e.on?'':'none';$('pwc').style.display=e.on?'':'none';
 if(e.on){$('ename').textContent=e.name+(e.state?' · '+e.state:'');
  $('pw').textContent=e.valid?e.power.toFixed(0)+' W':'-';$('pwsub').textContent=e.valid?e.name+' · today '+e.today.toFixed(3)+' kWh':'waiting for Tasmota';
  $('e_pw').textContent=e.valid?e.power.toFixed(0)+' W':'-';$('e_td').textContent=e.valid?e.today.toFixed(3)+' kWh':'-';
  $('e_yd').textContent=e.valid?e.yesterday.toFixed(3)+' kWh':'-';$('e_tt').textContent=e.valid?e.total.toFixed(1)+' kWh':'-';
  $('e_ui').textContent=e.valid?e.voltage.toFixed(0)+' V / '+e.current.toFixed(2)+' A':'-';
  $('ehint').innerHTML=(e.age>=0?'Last message '+ago(e.age)+'. ':'')+'Tasmota: MQTT host <code>'+esc(s.ip)+'</code>, port <code>'+e.port+'</code>.'}
 mqIp=s.ip;
 $('loc').textContent=s.city?(s.city+(s.country?', '+s.country:'')):'-';
 $('wd').className='dot'+(w.valid&&w.age<3600?' on':'');$('wage').textContent=(w.valid?ago(w.age):(s.wxError||'-'))+' · '+s.apiCalls+' / '+s.apiMax+' API calls today';
 $('dview').textContent=s.viewName+(s.lightOn?'':' · light off');
 $('wifi').textContent=s.ip+' · '+s.rssi+' dBm'+(s.wifiDrops?' · '+s.wifiDrops+' drops / '+s.wifiReconnects+' reconnects':'');
 $('up').textContent=fu(s.uptime);$('heap').textContent=(s.heap/1024).toFixed(1)+' KB';$('ver').textContent=s.version;
 if(s.portal&&!portal){portal=true;openCfg()}}catch(e){$('wd').className='dot'}}
tick();setInterval(tick,5000);if(location.hash==='#settings')openCfg();
</script></body></html>)rawliteral";
