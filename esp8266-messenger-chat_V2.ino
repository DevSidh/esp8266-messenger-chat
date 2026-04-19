/*
 * ╔══════════════════════════════════════════════╗
 * ║         ESP Chat Pro  –  v4.0               ║
 * ║  WhatsApp-style • Rooms • Timestamps         ║
 * ║  Mobile + Desktop responsive UI              ║
 * ╚══════════════════════════════════════════════╝
 *
 * HOW TO USE:
 *   1. Flash to any ESP8266 board
 *   2. Connect to Wi-Fi "ESP_CHAT" / password "12345678"
 *   3. Open browser → 192.168.4.1
 *
 * FEATURES ADDED vs original:
 *   ✓ WhatsApp-style bubble UI (own msgs on right / green)
 *   ✓ Room list screen with lock icons
 *   ✓ Create / join modals with slide-up animation
 *   ✓ Message timestamps (HH:MM uptime-based)
 *   ✓ Incremental message polling (since=N, saves bandwidth)
 *   ✓ Memory-safe: old messages auto-trimmed
 *   ✓ XSS-safe: JSON-escaped server-side, HTML-escaped client-side
 *   ✓ Duplicate room detection
 *   ✓ Password-protected rooms (badge shown in room list)
 *   ✓ Open rooms (no password) joinable with 1 tap
 *   ✓ Enter-key to send / submit forms
 *   ✓ Auto-scroll only when already at bottom
 *   ✓ Back button (rooms → chat)
 *   ✓ Responsive: full-screen mobile, centered card on desktop
 *   ✓ Smooth fade-in animation on new messages
 *   ✓ Double-tick (✓✓) on own messages
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

// ─── Wi-Fi Config ────────────────────────────────────────────
const char* SSID     = "ESP_CHAT";
const char* PASSWORD = "12345678";

// ─── Limits ──────────────────────────────────────────────────
#define MAX_ROOMS    5       // Max simultaneous rooms
#define MAX_MSG_BUF  5500   // Bytes per room message buffer
#define MAX_MSG_LEN  280    // Max chars per message
#define MAX_NAME_LEN 20     // Max username / room name chars

// ─── Data Model ──────────────────────────────────────────────
struct Room {
  String name;
  String pass;
  String buf;     // newline-delimited JSON message records
  int    count;   // absolute total messages ever pushed
  int    offset;  // messages trimmed from front (for since= calc)
};

Room rooms[MAX_ROOMS];
int  roomCount = 0;

// ─── Helpers ─────────────────────────────────────────────────

// Escape for embedding in a JSON string value (not HTML – client handles that)
String jsonEsc(String s) {
  if ((int)s.length() > MAX_MSG_LEN) s = s.substring(0, MAX_MSG_LEN);
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\r", "");
  s.replace("\n", " ");
  return s;
}

// HH:MM based on uptime
String getTime() {
  unsigned long sec = millis() / 1000UL;
  char buf[6];
  sprintf(buf, "%02lu:%02lu", (sec / 3600UL) % 24UL, (sec % 3600UL) / 60UL);
  return String(buf);
}

int findRoom(const String& name) {
  for (int i = 0; i < roomCount; i++)
    if (rooms[i].name == name) return i;
  return -1;
}

// Append a message; trim oldest if buffer is full
void pushMsg(int id, const String& u, const String& m) {
  String rec = "{\"u\":\"" + u + "\",\"m\":\"" + m + "\",\"t\":\"" + getTime() + "\"}\n";
  // Drop oldest records until there is room
  while ((int)(rooms[id].buf.length() + rec.length()) > MAX_MSG_BUF) {
    int nl = rooms[id].buf.indexOf('\n');
    if (nl < 0) { rooms[id].buf = ""; break; }
    rooms[id].buf = rooms[id].buf.substring(nl + 1);
    rooms[id].offset++;
  }
  rooms[id].buf += rec;
  rooms[id].count++;
}

// ─── HTML / CSS / JS (stored in Flash) ───────────────────────
const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>ESP Chat</title>
<style>
/* ── Reset & tokens ── */
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
:root{
  --g:#25D366;--gd:#075E54;--gt:#128C7E;--gl:#DCF8C6;
  --bg:#E5DDD5;--wh:#FFFFFF;
  --sh:0 1px 3px rgba(0,0,0,.18);
  --r:14px;
}
body{
  font-family:-apple-system,BlinkMacSystemFont,'SF Pro Text','Segoe UI',
              Roboto,'Helvetica Neue',Arial,sans-serif;
  background:var(--bg);height:100dvh;overflow:hidden;
  display:flex;justify-content:center;align-items:center;
}

/* ── App container ── */
#app{
  width:100%;max-width:480px;
  height:100dvh;
  position:relative;overflow:hidden;
  display:flex;flex-direction:column;
  background:var(--bg);
}

/* ── Screens ── */
.scr{
  display:none;position:absolute;inset:0;
  flex-direction:column;background:var(--bg);
}
.scr.on{display:flex}

/* ══ LOGIN SCREEN ══ */
#sLogin{
  background:linear-gradient(160deg,var(--gd) 0%,var(--gt) 55%,#1a9e6e 100%);
  align-items:center;justify-content:center;
}
.card{
  background:var(--wh);border-radius:22px;
  padding:36px 28px 32px;width:88%;max-width:360px;
  box-shadow:0 24px 64px rgba(0,0,0,.28);text-align:center;
}
.wicon{
  width:76px;height:76px;background:var(--g);border-radius:50%;
  margin:0 auto 18px;display:flex;align-items:center;justify-content:center;
  box-shadow:0 8px 24px rgba(37,211,102,.45);
}
.wicon svg{width:42px;height:42px;fill:#fff}
.card h2{color:var(--gd);font-size:26px;font-weight:700;letter-spacing:-.3px;margin-bottom:6px}
.card .sub{color:#aaa;font-size:13.5px;margin-bottom:28px}
.inp{
  width:100%;padding:13px 16px;
  border:2px solid #eee;border-radius:12px;
  font-size:16px;outline:none;
  transition:border-color .18s,box-shadow .18s;
  margin-bottom:11px;background:var(--wh);color:#111;
}
.inp:focus{border-color:var(--g);box-shadow:0 0 0 3px rgba(37,211,102,.15)}
.inp::placeholder{color:#bbb}
.btn{
  display:block;width:100%;padding:14px;border:none;border-radius:12px;
  font-size:16px;font-weight:700;cursor:pointer;
  transition:transform .1s,opacity .15s;letter-spacing:.1px;
}
.btn:active{transform:scale(.97);opacity:.88}
.bp{background:var(--g);color:#fff;box-shadow:0 4px 14px rgba(37,211,102,.35)}
.bs{background:#f2f2f2;color:#555}
.bss{background:var(--gt);color:#fff}

/* ══ ROOMS SCREEN ══ */
#sRooms{background:#f5f5f5}
.rhead{
  background:var(--gd);color:#fff;
  padding:14px 18px;display:flex;align-items:center;
  flex-shrink:0;min-height:58px;
  box-shadow:0 2px 8px rgba(0,0,0,.2);
}
.rhead h3{flex:1;font-size:19px;font-weight:700;letter-spacing:-.2px}
.rlist{flex:1;overflow-y:auto;-webkit-overflow-scrolling:touch}
.ri{
  background:var(--wh);padding:13px 18px;
  border-bottom:1px solid #f0f0f0;
  display:flex;align-items:center;gap:14px;
  cursor:pointer;transition:background .1s;user-select:none;
}
.ri:active{background:#f7f7f7}
.rav{
  width:50px;height:50px;border-radius:50%;
  background:var(--gt);
  display:flex;align-items:center;justify-content:center;
  color:#fff;font-weight:700;font-size:19px;flex-shrink:0;
  box-shadow:var(--sh);
}
.ri-info{flex:1;min-width:0}
.ri-name{font-weight:600;font-size:16px;color:#111;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.ri-sub{font-size:13px;color:#999;margin-top:3px}
.norooms{
  flex:1;display:flex;flex-direction:column;
  align-items:center;justify-content:center;color:#ccc;gap:10px;padding:40px;
}
.norooms .ni{font-size:58px}
.norooms p{font-size:15px;text-align:center;line-height:1.5}
.fab{
  position:absolute;bottom:22px;right:20px;
  width:58px;height:58px;background:var(--g);border:none;border-radius:50%;
  color:#fff;font-size:30px;cursor:pointer;
  box-shadow:0 6px 18px rgba(37,211,102,.45);
  display:flex;align-items:center;justify-content:center;
  z-index:10;transition:transform .15s,box-shadow .15s;
}
.fab:active{transform:scale(.91);box-shadow:0 3px 10px rgba(37,211,102,.35)}

/* ══ MODALS ══ */
.mover{
  display:none;position:fixed;inset:0;
  background:rgba(0,0,0,.48);z-index:50;
  align-items:flex-end;justify-content:center;
}
.mover.on{display:flex}
.modal{
  background:var(--wh);
  border-radius:22px 22px 0 0;
  padding:26px 22px 34px;
  width:100%;max-width:480px;
  animation:su .22s cubic-bezier(.22,.61,.36,1);
}
@keyframes su{from{transform:translateY(100%)}to{transform:translateY(0)}}
.modal h4{color:var(--gd);font-size:18px;font-weight:700;margin-bottom:18px}
.mbtns{display:flex;gap:10px;margin-top:16px}
.mbtns .btn{flex:1;padding:13px}
.err-shake{animation:shake .35s ease}
@keyframes shake{0%,100%{transform:translateX(0)}25%{transform:translateX(-6px)}75%{transform:translateX(6px)}}

/* ══ CHAT SCREEN ══ */
#sChat{background:var(--bg)}

/* Subtle tile-pattern background (CSS only, no network) */
#sChat::before{
  content:'';position:absolute;inset:0;
  background-image:
    radial-gradient(circle,rgba(0,0,0,.04) 1px,transparent 1px);
  background-size:22px 22px;
  pointer-events:none;z-index:0;
}

.chead{
  background:var(--gd);padding:8px 12px;
  display:flex;align-items:center;gap:10px;
  color:#fff;flex-shrink:0;min-height:58px;
  box-shadow:0 2px 8px rgba(0,0,0,.2);
  position:relative;z-index:1;
}
.cback{
  background:none;border:none;color:#fff;
  font-size:26px;padding:4px 8px 4px 2px;cursor:pointer;
  border-radius:50%;line-height:1;
  display:flex;align-items:center;justify-content:center;
  transition:background .15s;
}
.cback:active{background:rgba(255,255,255,.18)}
.cav2{
  width:42px;height:42px;border-radius:50%;
  background:var(--gt);display:flex;align-items:center;justify-content:center;
  font-weight:700;font-size:17px;flex-shrink:0;
}
.cinfo{flex:1}
.cname{font-size:17px;font-weight:700;line-height:1.2;letter-spacing:-.2px}
.cstatus{font-size:12px;opacity:.75;margin-top:2px}

/* Messages area */
.cmsgs{
  flex:1;overflow-y:auto;-webkit-overflow-scrolling:touch;
  padding:8px 8px 6px;
  display:flex;flex-direction:column;gap:2px;
  position:relative;z-index:1;
}

/* Bubble wrapper */
.mw{display:flex;animation:fi .18s ease both}
@keyframes fi{from{opacity:0;transform:translateY(7px)}to{opacity:1;transform:translateY(0)}}
.mw.me{justify-content:flex-end}
.mw.them{justify-content:flex-start}

.bub{
  max-width:76%;padding:8px 12px 5px;
  border-radius:14px;
  box-shadow:var(--sh);
  word-break:break-word;
  position:relative;
}
.me .bub{background:var(--gl);border-bottom-right-radius:3px}
.them .bub{background:var(--wh);border-bottom-left-radius:3px}
.bn{font-size:12px;font-weight:700;color:var(--gt);margin-bottom:3px;line-height:1}
.bm{font-size:15px;line-height:1.45;color:#111}
.bt{
  font-size:11px;color:#9fa8a0;
  text-align:right;margin-top:4px;
  display:flex;justify-content:flex-end;align-items:center;gap:4px;
}
.tick{font-size:13px;color:#53bdeb}

/* Date / system messages */
.sysmsg{
  text-align:center;margin:6px 0;
  display:flex;justify-content:center;
}
.sysmsg span{
  background:rgba(0,0,0,.13);color:#6b7c8a;
  padding:4px 14px;border-radius:10px;font-size:11.5px;
}

/* Input area */
.cinput{
  padding:7px 8px;background:transparent;
  display:flex;align-items:center;gap:7px;
  flex-shrink:0;position:relative;z-index:1;
}
.cinput .inp{
  flex:1;padding:11px 16px;border:none;
  border-radius:26px;background:var(--wh);
  font-size:16px;box-shadow:var(--sh);margin-bottom:0;
}
.sbtn{
  width:50px;height:50px;background:var(--g);border:none;
  border-radius:50%;color:#fff;font-size:22px;cursor:pointer;
  display:flex;align-items:center;justify-content:center;
  box-shadow:0 3px 10px rgba(37,211,102,.4);flex-shrink:0;
  transition:background .15s,transform .1s;
}
.sbtn:active{background:var(--gt);transform:scale(.92)}

/* ── Desktop ── */
@media(min-width:600px){
  body{background:#1a1a1a}
  #app{
    border-radius:12px;
    height:min(95dvh,820px);
    box-shadow:0 32px 80px rgba(0,0,0,.5);
    overflow:hidden;
  }
  .mover{position:absolute;border-radius:12px 12px 0 0}
  .modal{border-radius:18px 18px 0 0}
  .fab{bottom:28px;right:22px}
  /* Wider bubbles on desktop */
  .bub{max-width:68%}
}

/* ── Extra-large screens ── */
@media(min-width:1024px){
  #app{max-width:420px}
}
</style>
</head>
<body>
<div id="app">

<!-- ══ LOGIN ══ -->
<div id="sLogin" class="scr on">
  <div class="card">
    <div class="wicon">
      <svg viewBox="0 0 24 24">
        <path d="M17.472 14.382c-.297-.149-1.758-.867-2.03-.967-.273-.099-.471-.148-.67.15-.197.297-.767.966-.94 1.164-.173.199-.347.223-.644.075-.297-.15-1.255-.463-2.39-1.475-.883-.788-1.48-1.761-1.653-2.059-.173-.297-.018-.458.13-.606.134-.133.298-.347.446-.52.149-.174.198-.298.298-.497.099-.198.05-.371-.025-.52-.075-.149-.669-1.612-.916-2.207-.242-.579-.487-.5-.669-.51-.173-.008-.371-.01-.57-.01-.198 0-.52.074-.792.372-.272.297-1.04 1.016-1.04 2.479 0 1.462 1.065 2.875 1.213 3.074.149.198 2.096 3.2 5.077 4.487.709.306 1.262.489 1.694.625.712.227 1.36.195 1.871.118.571-.085 1.758-.719 2.006-1.413.248-.694.248-1.289.173-1.413-.074-.124-.272-.198-.57-.347zm-5.421 7.403h-.004a9.87 9.87 0 01-5.031-1.378l-.361-.214-3.741.982.998-3.648-.235-.374a9.86 9.86 0 01-1.51-5.26c.001-5.45 4.436-9.884 9.888-9.884 2.64 0 5.122 1.03 6.988 2.898a9.825 9.825 0 012.893 6.994c-.003 5.45-4.437 9.884-9.885 9.884z"/>
      </svg>
    </div>
    <h2>ESP Chat</h2>
    <p class="sub">Local Network • Private • Instant</p>
    <input class="inp" id="uname" type="text" placeholder="Your name…"
           maxlength="20" autocomplete="off" autocorrect="off"
           autocapitalize="words" spellcheck="false">
    <button class="btn bp" onclick="goRooms()">Get Started &rarr;</button>
  </div>
</div>

<!-- ══ ROOMS ══ -->
<div id="sRooms" class="scr">
  <div class="rhead">
    <h3>💬 Chat Rooms</h3>
  </div>
  <div id="rList" class="rlist"></div>
  <button class="fab" onclick="showMod('mCreate')" title="Create room">+</button>
</div>

<!-- ══ CREATE MODAL ══ -->
<div id="mCreate" class="mover" onclick="bgClose(event,'mCreate')">
  <div class="modal">
    <h4>✏️ New Room</h4>
    <input class="inp" id="cRN" placeholder="Room name" maxlength="20"
           autocomplete="off" autocorrect="off" spellcheck="false">
    <input class="inp" id="cRP" type="password" placeholder="Password (optional)"
           maxlength="20" autocomplete="off">
    <div class="mbtns">
      <button class="btn bs" onclick="hideMod('mCreate')">Cancel</button>
      <button class="btn bp" onclick="doCreate()">Create</button>
    </div>
  </div>
</div>

<!-- ══ JOIN MODAL ══ -->
<div id="mJoin" class="mover" onclick="bgClose(event,'mJoin')">
  <div class="modal">
    <h4 id="jTitle">🔐 Join Room</h4>
    <input class="inp" id="jPass" type="password" placeholder="Enter password"
           maxlength="20" autocomplete="off">
    <div class="mbtns">
      <button class="btn bs" onclick="hideMod('mJoin')">Cancel</button>
      <button class="btn bp" onclick="doJoin()">Join &rarr;</button>
    </div>
  </div>
</div>

<!-- ══ CHAT ══ -->
<div id="sChat" class="scr">
  <div class="chead">
    <button class="cback" onclick="goBack()">&#8592;</button>
    <div class="cav2" id="cAvL">R</div>
    <div class="cinfo">
      <div class="cname" id="cRName">Room</div>
      <div class="cstatus">🔒 Local Network · ESP8266</div>
    </div>
  </div>
  <div id="cMsgs" class="cmsgs"></div>
  <div class="cinput">
    <input class="inp" id="mIn" type="text" placeholder="Message…"
           maxlength="280" autocomplete="off" autocorrect="off" spellcheck="false">
    <button class="sbtn" id="sBtnEl">&#10148;</button>
  </div>
</div>

</div><!-- #app -->

<script>
// ── State ───────────────────────────────────────────────────
var U='', R='', P='', jRoom='', cnt=0;

// ── Screen helpers ──────────────────────────────────────────
function show(id){
  document.querySelectorAll('.scr').forEach(function(s){s.classList.remove('on')});
  document.getElementById(id).classList.add('on');
}
function showMod(id){
  document.getElementById(id).classList.add('on');
}
function hideMod(id){
  document.getElementById(id).classList.remove('on');
}
function bgClose(e,id){
  if(e.target===e.currentTarget) hideMod(id);
}

// Shake + red border on invalid input
function err(id){
  var el=document.getElementById(id);
  el.style.borderColor='#ff4757';
  el.classList.add('err-shake');
  el.focus();
  setTimeout(function(){
    el.style.borderColor='#eee';
    el.classList.remove('err-shake');
  },700);
}

// HTML-escape for safe innerHTML insertion
function esc(s){
  return String(s)
    .replace(/&/g,'&amp;')
    .replace(/</g,'&lt;')
    .replace(/>/g,'&gt;')
    .replace(/"/g,'&quot;');
}

// ── Login ────────────────────────────────────────────────────
function goRooms(){
  var n=document.getElementById('uname').value.trim();
  if(!n){err('uname');return;}
  U=n;
  show('sRooms');
  loadRooms();
}

// ── Rooms ────────────────────────────────────────────────────
function loadRooms(){
  fetch('/rooms')
    .then(function(r){return r.json();})
    .then(function(data){
      var el=document.getElementById('rList');
      if(!data||data.length===0){
        el.innerHTML=
          '<div class="norooms">'
          +'<div class="ni">🏠</div>'
          +'<p>No rooms yet.<br>Tap <b>+</b> to create one!</p>'
          +'</div>';
        return;
      }
      el.innerHTML='';
      data.forEach(function(r){
        var div=document.createElement('div');
        div.className='ri';
        div.dataset.n=r.n;
        div.dataset.p=r.p?'1':'0';
        div.innerHTML=
          '<div class="rav">'+esc(r.n[0].toUpperCase())+'</div>'
          +'<div class="ri-info">'
          +'<div class="ri-name">'+esc(r.n)+'</div>'
          +'<div class="ri-sub">'+(r.p?'🔒 Password protected':'🔓 Open room')+'</div>'
          +'</div>';
        div.addEventListener('click',function(){
          askJoin(this.dataset.n, this.dataset.p==='1');
        });
        el.appendChild(div);
      });
    })
    .catch(function(){});
}

// ── Create room ──────────────────────────────────────────────
function doCreate(){
  var r=document.getElementById('cRN').value.trim();
  var p=document.getElementById('cRP').value;
  if(!r){err('cRN');return;}
  fetch('/create?room='+encodeURIComponent(r)+'&pass='+encodeURIComponent(p))
    .then(function(res){return res.text();})
    .then(function(d){
      if(d==='ok'){
        hideMod('mCreate');
        document.getElementById('cRN').value='';
        document.getElementById('cRP').value='';
        enterChat(r,p);
      } else if(d==='exists'){
        alert('A room called "'+r+'" already exists.');
      } else if(d==='full'){
        alert('Max rooms reached ('+5+'). Delete a room first.');
      } else {
        alert('Could not create room.');
      }
    });
}

// ── Join room ────────────────────────────────────────────────
function askJoin(rName, hasPw){
  jRoom=rName;
  if(!hasPw){
    // No password — join instantly
    joinWith(rName,'');
    return;
  }
  document.getElementById('jTitle').textContent='🔐 Join: '+rName;
  document.getElementById('jPass').value='';
  showMod('mJoin');
  setTimeout(function(){document.getElementById('jPass').focus();},220);
}

function doJoin(){
  joinWith(jRoom, document.getElementById('jPass').value);
}

function joinWith(r,p){
  fetch('/join?room='+encodeURIComponent(r)+'&pass='+encodeURIComponent(p))
    .then(function(res){return res.text();})
    .then(function(d){
      if(d==='ok'){
        hideMod('mJoin');
        enterChat(r,p);
      } else {
        err('jPass');
      }
    });
}

// ── Chat ─────────────────────────────────────────────────────
function enterChat(r,p){
  R=r; P=p; cnt=0;
  document.getElementById('cRName').textContent=r;
  document.getElementById('cAvL').textContent=r[0].toUpperCase();
  document.getElementById('cMsgs').innerHTML=
    '<div class="sysmsg"><span>🔒 Messages are local — no internet</span></div>';
  show('sChat');
  setTimeout(function(){document.getElementById('mIn').focus();},200);
}

function goBack(){
  R=''; cnt=0;
  show('sRooms');
  loadRooms();
}

function sendMsg(){
  var m=document.getElementById('mIn').value.trim();
  if(!m||!R) return;
  document.getElementById('mIn').value='';
  fetch('/send?room='+encodeURIComponent(R)
       +'&user='+encodeURIComponent(U)
       +'&msg='+encodeURIComponent(m));
}

// ── Polling ──────────────────────────────────────────────────
function pollChat(){
  if(!R) return;
  fetch('/get?room='+encodeURIComponent(R)+'&since='+cnt)
    .then(function(r){return r.json();})
    .then(function(data){
      if(!data||!data.msgs||!data.msgs.length) return;

      var box=document.getElementById('cMsgs');
      // Auto-scroll only if already near bottom
      var atBottom=(box.scrollHeight-box.clientHeight<=box.scrollTop+60);

      data.msgs.forEach(function(msg){
        var mine=(msg.u===U);
        var div=document.createElement('div');
        div.className='mw '+(mine?'me':'them');
        div.innerHTML=
          '<div class="bub">'
          +(mine?'':'<div class="bn">'+esc(msg.u)+'</div>')
          +'<div class="bm">'+esc(msg.m)+'</div>'
          +'<div class="bt">'
          +esc(msg.t)
          +(mine?'<span class="tick">✓✓</span>':'')
          +'</div>'
          +'</div>';
        box.appendChild(div);
      });

      cnt=data.count;
      if(atBottom) box.scrollTop=box.scrollHeight;
    })
    .catch(function(){});
}

// ── Key bindings ─────────────────────────────────────────────
document.getElementById('uname').addEventListener('keydown',function(e){if(e.key==='Enter')goRooms();});
document.getElementById('mIn').addEventListener('keydown',function(e){if(e.key==='Enter')sendMsg();});
document.getElementById('sBtnEl').addEventListener('click',sendMsg);
document.getElementById('cRN').addEventListener('keydown',function(e){if(e.key==='Enter')doCreate();});
document.getElementById('jPass').addEventListener('keydown',function(e){if(e.key==='Enter')doJoin();});

// ── Intervals ────────────────────────────────────────────────
setInterval(pollChat,1000);     // Refresh chat messages
setInterval(function(){         // Refresh room list when visible
  if(document.getElementById('sRooms').classList.contains('on')) loadRooms();
},5000);
</script>
</body>
</html>
)rawliteral";

// ─── HTTP Handlers ───────────────────────────────────────────

void hRoot() {
  server.send_P(200, "text/html", PAGE);
}

// GET /rooms → JSON array of {n, p}
void hRooms() {
  String j = "[";
  for (int i = 0; i < roomCount; i++) {
    if (i) j += ",";
    j += "{\"n\":\"" + rooms[i].name + "\",\"p\":"
       + (rooms[i].pass.length() ? "true" : "false") + "}";
  }
  j += "]";
  server.send(200, "application/json", j);
}

// GET /create?room=X&pass=Y
void hCreate() {
  String r = server.arg("room");
  String p = server.arg("pass");

  // Validate name
  if (r.length() == 0 || (int)r.length() > MAX_NAME_LEN) {
    server.send(200, "text/plain", "fail"); return;
  }
  if (findRoom(r) >= 0) {
    server.send(200, "text/plain", "exists"); return;
  }
  if (roomCount >= MAX_ROOMS) {
    server.send(200, "text/plain", "full"); return;
  }

  rooms[roomCount].name   = r;
  rooms[roomCount].pass   = p;
  rooms[roomCount].buf    = "";
  rooms[roomCount].count  = 0;
  rooms[roomCount].offset = 0;
  roomCount++;

  server.send(200, "text/plain", "ok");
}

// GET /join?room=X&pass=Y
void hJoin() {
  String r = server.arg("room");
  String p = server.arg("pass");
  int id = findRoom(r);
  if (id < 0 || rooms[id].pass != p)
    server.send(200, "text/plain", "fail");
  else
    server.send(200, "text/plain", "ok");
}

// GET /send?room=X&user=Y&msg=Z
void hSend() {
  int id = findRoom(server.arg("room"));
  if (id < 0) { server.send(200, "text/plain", "fail"); return; }

  String m = server.arg("msg");
  if (m.length() == 0) { server.send(200, "text/plain", "ok"); return; }

  pushMsg(id,
    jsonEsc(server.arg("user")),
    jsonEsc(m));

  server.send(200, "text/plain", "ok");
}

// GET /get?room=X&since=N
// Returns: { "count": <total>, "msgs": [ {u,m,t}, ... ] }
void hGet() {
  int id = findRoom(server.arg("room"));
  if (id < 0) {
    server.send(200, "application/json", "{\"count\":0,\"msgs\":[]}");
    return;
  }

  int since = server.arg("since").toInt();
  int start = since - rooms[id].offset;   // line index in current buf
  if (start < 0) start = 0;

  String& all = rooms[id].buf;
  String res = "{\"count\":" + String(rooms[id].count) + ",\"msgs\":[";

  int line = 0, pos = 0;
  bool first = true;

  while (pos < (int)all.length()) {
    int nl = all.indexOf('\n', pos);
    if (nl < 0) nl = all.length();

    if (line >= start && nl > pos) {
      if (!first) res += ',';
      res += all.substring(pos, nl);
      first = false;
    }

    line++;
    pos = nl + 1;
  }

  res += "]}";
  server.send(200, "application/json", res);
}

// ─── Setup / Loop ────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASSWORD);

  Serial.println();
  Serial.println("╔══════════════════════════════╗");
  Serial.println("║     ESP Chat Pro  v4.0       ║");
  Serial.println("╠══════════════════════════════╣");
  Serial.print  ("║  SSID     : "); Serial.print(SSID);
  Serial.println("         ║");
  Serial.print  ("║  Password : "); Serial.print(PASSWORD);
  Serial.println("     ║");
  Serial.print  ("║  IP       : "); Serial.print(WiFi.softAPIP());
  Serial.println("  ║");
  Serial.println("╚══════════════════════════════╝");

  server.on("/",       hRoot);
  server.on("/rooms",  hRooms);
  server.on("/create", hCreate);
  server.on("/join",   hJoin);
  server.on("/send",   hSend);
  server.on("/get",    hGet);

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
}
