#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "ESP_CHAT";
const char* password = "12345678";

ESP8266WebServer server(80);

#define MAX_ROOMS 3

struct Room {
  String name;
  String pass;
  String messages;
};

Room rooms[MAX_ROOMS];

String currentRoom = "";

/* ================= HTML ================= */
String page = R"=====(

<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Pro Chat V2</title>

<style>

body{
margin:0;
font-family:Arial;
background:#e5ddd5;
display:flex;
justify-content:center;
align-items:center;
height:100vh;
}

.box{
width:95%;
max-width:500px;
background:white;
border-radius:12px;
overflow:hidden;
box-shadow:0 0 12px rgba(0,0,0,0.2);
}

/* LOGIN */
#login{
padding:20px;
text-align:center;
}

input{
width:90%;
padding:12px;
margin:8px;
font-size:16px;
border:1px solid #ccc;
border-radius:6px;
}

/* BUTTON FIXED */
button{
width:90%;
padding:12px;
margin-top:10px;
font-size:16px;
background:#25D366;
color:white;
border:none;
border-radius:6px;
cursor:pointer;
}

/* CHAT */
#chatApp{
display:none;
flex-direction:column;
height:80vh;
}

.header{
background:#25D366;
color:white;
padding:12px;
text-align:center;
font-weight:bold;
}

.chat{
flex:1;
padding:10px;
overflow-y:auto;
background:#e5ddd5;
}

.msg{
background:white;
padding:8px;
margin:5px;
border-radius:8px;
font-size:14px;
}

/* INPUT AREA FIX */
.footer{
display:flex;
padding:8px;
background:white;
align-items:center;
}

textarea{
flex:1;
height:40px;
resize:none;
padding:8px;
font-size:14px;
border-radius:6px;
border:1px solid #ccc;
}

/* SMALL SEND BUTTON FIXED */
.send{
width:60px;
height:40px;
margin-left:8px;
background:#25D366;
color:white;
display:flex;
justify-content:center;
align-items:center;
border-radius:6px;
cursor:pointer;
font-weight:bold;
}

</style>

</head>

<body>

<div class="box">

<!-- LOGIN -->
<div id="login">

<h3>Pro Chat V2</h3>

<input id="room" placeholder="Room Name">
<input id="pass" placeholder="Room Password">
<input id="user" placeholder="Your Name">

<button onclick="joinRoom()">Join / Create Room</button>

</div>

<!-- CHAT -->
<div id="chatApp">

<div class="header" id="title">Chat</div>

<div class="chat" id="chat"></div>

<div class="footer">

<textarea id="msg" placeholder="Message..."></textarea>

<div class="send" onclick="sendMsg()">Send</div>

</div>

</div>

</div>

<script>

let room="", user="";

function joinRoom(){

room=document.getElementById("room").value;
user=document.getElementById("user").value;
let pass=document.getElementById("pass").value;

if(room=="" || user=="") return;

fetch(`/join?room=${room}&pass=${pass}`)
.then(r=>r.text())
.then(res=>{

if(res=="ok"){
document.getElementById("login").style.display="none";
document.getElementById("chatApp").style.display="flex";
document.getElementById("title").innerText=room;
}
else alert("Wrong room/password");
});

}

function sendMsg(){

let m=document.getElementById("msg").value;
if(m=="") return;

fetch(`/send?room=${room}&user=${user}&msg=${encodeURIComponent(m)}`);

document.getElementById("msg").value="";
}

function update(){

fetch(`/get?room=${room}`)
.then(r=>r.text())
.then(data=>{
document.getElementById("chat").innerHTML=data;
});

}

setInterval(update,1000);

</script>

</body>
</html>

)=====";

/* ================= ROOM LOGIC ================= */

int findRoom(String r){
for(int i=0;i<MAX_ROOMS;i++){
if(rooms[i].name==r) return i;
}
return -1;
}

int createRoom(String r, String p){
for(int i=0;i<MAX_ROOMS;i++){
if(rooms[i].name==""){
rooms[i].name=r;
rooms[i].pass=p;
rooms[i].messages="";
return i;
}
}
return -1;
}

/* ================= HANDLERS ================= */

void handleRoot(){
server.send(200,"text/html",page);
}

void handleJoin(){

String r=server.arg("room");
String p=server.arg("pass");

int idx=findRoom(r);

if(idx==-1){
idx=createRoom(r,p);
server.send(200,"text/plain","ok");
return;
}

if(rooms[idx].pass==p){
server.send(200,"text/plain","ok");
}
else server.send(200,"text/plain","fail");
}

void handleSend(){

String r=server.arg("room");
String u=server.arg("user");
String m=server.arg("msg");

int idx=findRoom(r);
if(idx==-1) return;

rooms[idx].messages += "<div class='msg'><b>"+u+":</b> "+m+"</div>";

server.send(200,"text/plain","ok");
}

void handleGet(){

String r=server.arg("room");
int idx=findRoom(r);

if(idx==-1){
server.send(200,"text/html","");
return;
}

server.send(200,"text/html",rooms[idx].messages);
}

/* ================= SETUP ================= */

void setup(){

Serial.begin(115200);

WiFi.softAP(ssid,password);

server.on("/",handleRoot);
server.on("/join",handleJoin);
server.on("/send",handleSend);
server.on("/get",handleGet);

server.begin();

}

/* ================= LOOP ================= */

void loop(){
server.handleClient();
}
