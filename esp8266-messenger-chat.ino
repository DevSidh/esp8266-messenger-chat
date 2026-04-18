#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid="ESP_CHAT";
const char* password="12345678";

ESP8266WebServer server(80);

String messages="";
int msgID=0;

String page = R"=====(
<!DOCTYPE html>
<html>

<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<link rel="stylesheet"
href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.2.1/css/all.min.css">

<title>ESP Messenger</title>

<style>

body{
margin:0;
font-family:Arial;
background:#f0f2f5;
display:flex;
justify-content:center;
align-items:center;
height:100vh;
}

#login{
width:90%;
max-width:400px;
background:white;
padding:30px;
border-radius:10px;
text-align:center;
box-shadow:0 0 10px rgba(0,0,0,0.2);
}

#login input{
width:100%;
padding:12px;
font-size:18px;
margin-top:10px;
}

#login button{
margin-top:15px;
padding:12px;
font-size:18px;
width:100%;
background:#0084ff;
color:white;
border:none;
border-radius:6px;
cursor:pointer;
}

#chatApp{
display:none;
width:95%;
max-width:700px;
height:90vh;
background:white;
border-radius:10px;
box-shadow:0 0 10px rgba(0,0,0,0.2);
display:flex;
flex-direction:column;
}

.header{
background:#0084ff;
color:white;
padding:15px;
text-align:center;
font-size:20px;
}

.chat-area{
flex:1;
overflow-y:auto;
padding:10px;
background:#e5ddd5;
}

.msg{
background:white;
padding:10px;
border-radius:8px;
margin:5px;
display:flex;
justify-content:space-between;
align-items:center;
}

.footer{
display:flex;
padding:10px;
background:white;
}

.footer textarea{
flex:1;
resize:none;
padding:10px;
font-size:16px;
border-radius:5px;
border:1px solid #ccc;
}

.send{
padding:10px;
font-size:22px;
cursor:pointer;
color:#0084ff;
}

.delete{
background:red;
color:white;
border:none;
border-radius:5px;
padding:5px;
margin-left:10px;
}

</style>

</head>

<body>

<div id="login">

<h2>Enter Your Name</h2>

<input id="username" placeholder="Your name">

<button onclick="enterChat()">Join Chat</button>

</div>

<div id="chatApp">

<div class="header">ESP Messenger</div>

<div class="chat-area" id="chat"></div>

<div class="footer">

<textarea id="msg" placeholder="Type message"></textarea>

<div class="send" onclick="sendMsg()">
<i class="fa-regular fa-paper-plane"></i>
</div>

</div>

</div>

<script>

let username="";

function enterChat(){

username=document.getElementById("username").value.trim();

if(username===""){
alert("Please enter name");
return;
}

document.getElementById("login").style.display="none";
document.getElementById("chatApp").style.display="flex";

}

function sendMsg(){

let m=document.getElementById("msg").value;

if(m==="") return;

fetch("/send?user="+username+"&msg="+encodeURIComponent(m));

document.getElementById("msg").value="";

}

function deleteMsg(id){

fetch("/delete?id="+id);

}

function update(){

fetch("/get")
.then(r=>r.text())
.then(data=>{
document.getElementById("chat").innerHTML=data;
});

}

setInterval(update,1000);

document.addEventListener("keydown",function(e){

if(e.key==="Enter"){
e.preventDefault();
sendMsg();
}

});

</script>

</body>

</html>
)=====";


void handleRoot(){
server.send(200,"text/html",page);
}

void handleSend(){

String user=server.arg("user");
String msg=server.arg("msg");

msgID++;

messages += "<div class='msg'><span><b>"+user+":</b> "+msg+
"</span><button class='delete' onclick='deleteMsg("+String(msgID)+")'>X</button></div>";

server.send(200,"text/plain","ok");

}

void handleGet(){
server.send(200,"text/html",messages);
}

void handleDelete(){

messages="";

server.send(200,"text/plain","deleted");

}

void setup(){

Serial.begin(115200);

WiFi.softAP(ssid,password);

server.on("/",handleRoot);
server.on("/send",handleSend);
server.on("/get",handleGet);
server.on("/delete",handleDelete);

server.begin();

}

void loop(){
server.handleClient();
}
