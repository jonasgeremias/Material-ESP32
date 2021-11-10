
function send_form() {
  const ID = +document.getElementById("id_device").value;
  const data = {
    mode: +document.getElementById("mode").value,
    description: document.getElementById("description").value,
    timer_on: +document.getElementById("timer_on").value,
    timer_off: +document.getElementById("timer_off").value,
    timer_rising: +document.getElementById("timer_rising").value,
    timer_falling: +document.getElementById("timer_falling").value,
    colors: [
      document.getElementById("color1").value.replace('#', "0x"),
      document.getElementById("color2").value.replace('#', "0x"),
      document.getElementById("color3").value.replace('#', "0x"),
      document.getElementById("color4").value.replace('#', "0x"),
      document.getElementById("color5").value.replace('#', "0x"),
      document.getElementById("color6").value.replace('#', "0x"),
      document.getElementById("color7").value.replace('#', "0x"),
      document.getElementById("color8").value.replace('#', "0x")]
  }

  console.log(ID, data);

  fetch('/pubconfig', {
    method: 'post',
    body: JSON.stringify({ id: ID, msg: data }),
    headers: {
      'Content-Type': 'application/json'
    },
  })
    .then(res => res.json())
    .then(res => {
      console.log(res)
      if (('pub' in res) && (res.pub)) alert('Enviado!');
      else alert('Erro ao enviar!');
    })
}


