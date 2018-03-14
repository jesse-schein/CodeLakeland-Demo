import 'jQuery/dist/jquery.min'
window.$ = jQuery;

import 'bootstrap/dist/js/bootstrap.min'
import 'bootstrap/dist/css/bootstrap.min.css'

import spectrum from 'spectrum-colorpicker'
import styles from 'spectrum-colorpicker/spectrum.css'



window.ip = "esp32.local";
const h = window.location.hostname;
if(h!="192.168.1.17" | h!="esp32.local" ){

}else if(h="localhost"){

}

var clientId;
window.ws = new WebSocket(`ws://${ip}/ws`);
window.clientId = clientId;
window.colorQueue = [];

ws.onopen = function() {
  ws.send('Connected'); // Send the message 'Ping' to the server
  $("#messageInput").prop("disabled", false);
  $("#messageButton").prop("disabled", false);
};

// Log errorss
ws.onerror = function(error) {
  console.log('WebSocket Error ' + error);
}; 

// Log messages from the server
ws.onmessage = function(e) {
  let j;
  try {
    j = JSON.parse(e.data);
    console.log(j);
  } catch (e) {

  }

  switch (j.action) {
    case "connected":
      clientId = parseInt(j.id);
      break;
    case "message":
      if (j.id != clientId) {
        addFromMessage(j);
      }
      break;
    case "color":
      addColor(j);
      break;
    case "processed":
      processColor(j);
      break;
  }
};




window.sendMessage = ()=> {

  let msg = $("#messageInput").val();
  let name = sessionStorage.getItem("name");

  let payload = {
    user: name,
    msg: msg,
    action: "message"
  };

  ws.send(JSON.stringify(payload));
  addSentMessage();


  $("#messageInput").val('');
}

window.addSentMessage = () => {
  let msg = $("#messageInput").val();

  let name = sessionStorage.getItem("name");
  let date = new Date();
  let hour = date.getHours();
  let min = date.getMinutes();
  $("#chat").append(
    `
    <div class="row no-gutters">
      <div class="col-lg-2 col-sm-4 col-4 col-md-2">
        ${name} - ${hour}:${min}
      </div>
      <div class="col-lg-10 col-sm-8 col-8 col-md-8">
        ${msg}
      </div>
    </div>
    `
  );
}

window.addFromMessage = (msg) => {
  if (msg.msg != "") {
    let name = msg.name;
    let date = new Date();
    let hour = date.getHours();
    let min = date.getMinutes();
    $("#chat").append(
      `
        <div class="row no-gutters">
          <div class="col-lg-2 col-sm-4 col-4 col-md-2">
            ${name} - ${hour}:${min}
          </div>
          <div class="col-lg-10 col-sm-8 col-md-8 col-8">
            ${msg.msg}
          </div>
        </div>
        `
    );


  }

  $("#chatDiv").scrollTop($("#chatDiv").height());
}
$(document).ready(() => {

  if (sessionStorage.getItem("name") == null) {
    $('#ModalCenter').modal();
  }

  $("#messageInput").keydown((e) => {
    if (e.key == "Enter") {
      sendMessage();
    }
  });

  $("#nameInput").keydown((e) => {
    if (e.key == "Enter") {
      setName();
    }
  });


})

window.setName = () => {
  let name = $("#nameInput").val();
  sessionStorage.setItem("name", name);
  $('#btnCloseModal').click();
}

window.hexToRgb = (hex) => {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
}

window.sendColor = () =>{
  let name = sessionStorage.getItem("name");
  let hexColor = $("#colorInput").val();
  let rgbColor = hexToRgb(hexColor);
  let duration = parseInt($("#durationInput").val());

  let config = {
    color: rgbColor,
    duration: duration
  };

  let payload = {
    user: name,
    data: config,
    action: "color"
  };

  ws.send(JSON.stringify(payload));

}

window.addColor = (payload) => {
  let r = payload.data.color.r;
  let g = payload.data.color.g;
  let b = payload.data.color.b;
  let name = payload.user;
  let d = payload.data.duration;
  let mid = payload.messageid;

  let color = `rgba(${r},${g},${b},.5)`;
  //console.log(color);
  $("#ColorQueue").append(`
    <div id="msgid${mid}" class="col-lg-4 col-4">
      <span  class="badge" style="background-color:${color}">${name} - ${d}s</span>
    </div>
    `);

}

window.processColor = (payload)=>{
  let id = payload.messageid;
  $(`#msgid${id}`).fadeOut('slow',()=>$(this).remove());
}
